#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

#include "lib-support.h"

#define MAX_PHNUM 32

#define ELFW_R_TYPE(x) ELFW(R_TYPE)(x)
#define ELFW_R_SYM(x) ELFW(R_SYM)(x)
#define ELFW_ST_BIND(x) ((unsigned char) (x) >> 4)

struct __DLoader_Internal {
    uintptr_t load_bias;
    void *entry;
    ElfW(Dyn) * pt_dynamic;

    void **dt_pltgot;
    ElfW_Reloc *dt_jmprel;
    size_t plt_entries;

    plt_resolver_t user_plt_resolver;
    void *user_plt_resolver_handle;

    /* Symbol resolution */
    ElfW(Sym) * dt_symtab;
    const char *dt_strtab;
    size_t dt_strsz;
    size_t sym_count;

    /* ELF (SYSV) hash table */
    const uint32_t *hash_buckets;
    const uint32_t *hash_chains;
    uint32_t hash_nbuckets;
    uint32_t hash_nchains;

    /* GNU hash table */
    const uint32_t *gnu_buckets;
    const uint32_t *gnu_chains;
    uint32_t gnu_nbuckets;
    uint32_t gnu_symndx;

    /* For deferred relocation */
    ElfW_Reloc *dt_relocs;
    size_t dt_relocs_count;
    const ElfW(Phdr) * phdr;
    size_t phnum;
    size_t pagesize;
};

/*
 * In recent glibc, even simple functions like memset and strlen can
 * depend on complex startup code, because they are defined using
 * STT_GNU_IFUNC.
 */
static inline void my_bzero(void *buf, size_t n)
{
    char *p = buf;
    while (n-- > 0)
        *p++ = 0;
}

static inline size_t my_strlen(const char *s)
{
    size_t n = 0;
    while (*s++ != '\0')
        ++n;
    return n;
}

static inline int my_strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return (unsigned char) *a - (unsigned char) *b;
}

static unsigned long elf_hash(const char *name)
{
    unsigned long h = 0, g;
    while (*name) {
        h = (h << 4) + (unsigned char) *name++;
        g = h & 0xf0000000;
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

static uint32_t gnu_hash_func(const char *name)
{
    uint32_t h = 5381;
    while (*name)
        h = (h << 5) + h + (unsigned char) *name++;
    return h;
}

/*
 * We're avoiding libc, so no printf.  The only nontrivial thing we need
 * is rendering numbers, which is, in fact, pretty trivial.
 * bufsz of course must be enough to hold INT_MIN in decimal.
 */
static void iov_int_string(int value,
                           struct iovec *iov,
                           char *buf,
                           size_t bufsz)
{
    static const char *const lookup = "9876543210123456789" + 9;
    char *p = &buf[bufsz];
    int negative = value < 0;
    do {
        --p;
        *p = lookup[value % 10];
        value /= 10;
    } while (value != 0);
    if (negative)
        *--p = '-';
    iov->iov_base = p;
    iov->iov_len = &buf[bufsz] - p;
}

#define STRING_IOV(string_constant, cond) \
    {(void *) string_constant, cond ? (sizeof(string_constant) - 1) : 0}

__attribute__((noreturn)) static void fail(const char *filename,
                                           const char *message,
                                           const char *item,
                                           int value)
{
    char valbuf[32];
    struct iovec iov[] = {
        STRING_IOV("[loader] ", 1),
        {(void *) filename, my_strlen(filename)},
        STRING_IOV(": ", 1),
        {(void *) message, my_strlen(message)},
        {(void *) item, !item ? 0 : my_strlen(item)},
        STRING_IOV("=", !item),
        {NULL, 0},
        {"\n", 1},
    };
    const int niov = sizeof(iov) / sizeof(iov[0]);
    if (item != NULL)
        iov_int_string(value, &iov[6], valbuf, sizeof(valbuf));

    writev(2, iov, niov);
    exit(2);
}

static int add_overflows_uintptr(uintptr_t a, uintptr_t b)
{
    return a > UINTPTR_MAX - b;
}

static uintptr_t add_uintptr_or_fail(uintptr_t a,
                                     uintptr_t b,
                                     const char *filename,
                                     const char *message)
{
    if (add_overflows_uintptr(a, b))
        fail(filename, message, NULL, 0);
    return a + b;
}

static int prot_from_phdr(const ElfW(Phdr) * phdr)
{
    int prot = 0;
    if (phdr->p_flags & PF_R)
        prot |= PROT_READ;
    if (phdr->p_flags & PF_W)
        prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X)
        prot |= PROT_EXEC;
    return prot;
}

static inline uintptr_t round_up(uintptr_t value, uintptr_t size)
{
    if (add_overflows_uintptr(value, size - 1))
        return UINTPTR_MAX;
    return (value + size - 1) & -size;
}

static inline uintptr_t round_down(uintptr_t value, uintptr_t size)
{
    return value & -size;
}

/*
 * Handle the "bss" portion of a segment, where the memory size
 * exceeds the file size and we zero-fill the difference.
 * For any whole pages in this region, we over-map anonymous pages.
 * For the sub-page remainder, we zero-fill bytes directly.
 */
static void handle_bss(const ElfW(Phdr) * ph,
                       const char *filename,
                       ElfW(Addr) load_bias,
                       size_t pagesize)
{
    if (ph->p_memsz > ph->p_filesz) {
        ElfW(Addr) file_end = add_uintptr_or_fail(
            add_uintptr_or_fail(ph->p_vaddr, load_bias, filename,
                                "BSS range overflows!"),
            ph->p_filesz, filename, "BSS range overflows!");
        ElfW(Addr) file_page_end = round_up(file_end, pagesize);
        if (file_page_end == UINTPTR_MAX)
            fail(filename, "BSS range overflows!", NULL, 0);
        ElfW(Addr) page_end =
            round_up(add_uintptr_or_fail(
                         add_uintptr_or_fail(ph->p_vaddr, load_bias, filename,
                                             "BSS range overflows!"),
                         ph->p_memsz, filename, "BSS range overflows!"),
                     pagesize);
        if (page_end == UINTPTR_MAX)
            fail(filename, "BSS range overflows!", NULL, 0);
        if (page_end > file_page_end &&
            mmap((void *) file_page_end, page_end - file_page_end,
                 prot_from_phdr(ph), MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1,
                 0) == MAP_FAILED)
            fail(filename, "Failed to map BSS pages!", NULL, 0);
        if (file_page_end > file_end && (ph->p_flags & PF_W))
            my_bzero((void *) file_end, file_page_end - file_end);
    }
}

static uintptr_t get_dynamic_entry(ElfW(Dyn) * dynamic, int field)
{
    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        if (dynamic->d_tag == field)
            return dynamic->d_un.d_ptr;
    }
    return 0;
}

static void pread_exact(int fd,
                        void *buf,
                        size_t count,
                        off_t offset,
                        const char *filename,
                        const char *what)
{
    ssize_t ret = pread(fd, buf, count, offset);
    if (ret < 0)
        fail(filename, "pread failed!", "errno", errno);
    if ((size_t) ret != count)
        fail(filename, "Short read while loading ELF!", what, (int) ret);
}

static const ElfW(Phdr) *
    find_load_segment(const ElfW(Phdr) * phdr, size_t phnum, ElfW(Addr) vaddr)
{
    for (size_t i = 0; i < phnum; ++i) {
        const ElfW(Phdr) *ph = &phdr[i];
        if (ph->p_type != PT_LOAD)
            continue;
        if (add_overflows_uintptr(ph->p_vaddr, ph->p_memsz))
            continue;
        if (vaddr >= ph->p_vaddr && vaddr < ph->p_vaddr + ph->p_memsz)
            return ph;
    }
    return NULL;
}

/*
 * Use pre-defined macro in arch/ to support different
 * system-specific assembly, see lib-support.h.
 */
void plt_trampoline();
asm(".pushsection .text,\"ax\",\"progbits\""  "\n"
    ".globl plt_trampoline"                   "\n"
    "plt_trampoline:"                         "\n"
    POP_S(REG_ARG_1) /* Argument 1 */         "\n"
    POP_S(REG_ARG_2) /* Argument 2 */         "\n"
    PUSH_STACK_STATE                          "\n"
    CALL(system_plt_resolver)                 "\n"
    POP_STACK_STATE                           "\n"
    JMP_REG(REG_RET)                          "\n"
    ".popsection"                             "\n");

static size_t get_plt_count(struct program_header *ph)
{
    if (ph->pltgot_end < ph->pltgot)
        fail("<runtime>", "Corrupt PLT header: end < start!", NULL, 0);
    return (size_t) (ph->pltgot_end - ph->pltgot);
}

void *system_plt_resolver(dloader_p o, int import_id)
{
    if (o->user_plt_resolver == NULL)
        fail("<runtime>", "PLT resolver used before initialization!", NULL, 0);
    struct program_header *ph = (struct program_header *) o->entry;
    size_t plt_count = get_plt_count(ph);
    if (import_id < 0 || (size_t) import_id >= plt_count)
        fail("<runtime>", "PLT import_id out of bounds!", "import_id",
             import_id);
    return o->user_plt_resolver(o->user_plt_resolver_handle, import_id);
}

dloader_p api_load(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        fail(filename, "Failed to open ELF!", "errno", errno);

    long pagesize_long = sysconf(_SC_PAGESIZE);
    if (pagesize_long <= 0)
        fail(filename, "Failed to determine page size!", NULL, 0);
    size_t pagesize = (size_t) pagesize_long;

    ElfW(Ehdr) ehdr;
    pread_exact(fd, &ehdr, sizeof(ehdr), 0, filename, "ELF header");

    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr.e_ident[EI_DATA] != ELFDATA2LSB || ehdr.e_version != EV_CURRENT ||
        ehdr.e_ehsize != sizeof(ehdr) || ehdr.e_phentsize != sizeof(ElfW(Phdr)))
        fail(filename, "File has no valid ELF header!", NULL, 0);

#if defined(__x86_64__)
    const int expected_machine = EM_X86_64;
    const unsigned char expected_class = ELFCLASS64;
#elif defined(__arm__)
    const int expected_machine = EM_ARM;
    const unsigned char expected_class = ELFCLASS32;
#elif defined(__aarch64__)
    const int expected_machine = EM_AARCH64;
    const unsigned char expected_class = ELFCLASS64;
#else
#error "Unsupported architecture"
#endif

    if (ehdr.e_machine != expected_machine)
        fail(filename, "ELF file has wrong architecture!  ", "e_machine",
             ehdr.e_machine);
    if (ehdr.e_ident[EI_CLASS] != expected_class)
        fail(filename, "ELF file has wrong class!", "EI_CLASS",
             ehdr.e_ident[EI_CLASS]);

    ElfW(Phdr) phdr[MAX_PHNUM];
    if (ehdr.e_phnum > sizeof(phdr) / sizeof(phdr[0]) || ehdr.e_phnum < 1)
        fail(filename, "ELF file has unreasonable ", "e_phnum", ehdr.e_phnum);

    if (ehdr.e_type != ET_DYN)
        fail(filename, "ELF file not ET_DYN!  ", "e_type", ehdr.e_type);

    pread_exact(fd, phdr, sizeof(phdr[0]) * ehdr.e_phnum, ehdr.e_phoff,
                filename, "program headers");

    size_t i = 0;
    while (i < ehdr.e_phnum && phdr[i].p_type != PT_LOAD)
        ++i;
    if (i == ehdr.e_phnum)
        fail(filename, "ELF file has no PT_LOAD header!", NULL, 0);

    /*
     * ELF requires that PT_LOAD segments be in ascending order of p_vaddr.
     * Find the last one to calculate the whole address span of the image.
     */
    const ElfW(Phdr) *first_load = &phdr[i];
    const ElfW(Phdr) *last_load = &phdr[ehdr.e_phnum - 1];
    while (last_load > first_load && last_load->p_type != PT_LOAD)
        --last_load;

    /*
     * Total memory size of phdr between first and last PT_LOAD.
     */
    if (add_overflows_uintptr(last_load->p_vaddr, last_load->p_memsz) ||
        last_load->p_vaddr + last_load->p_memsz < first_load->p_vaddr)
        fail(filename, "ELF PT_LOAD span overflows!", NULL, 0);
    size_t span = last_load->p_vaddr + last_load->p_memsz - first_load->p_vaddr;

    /*
     * Map the first segment and reserve the space used for the rest and
     * for holes between segments.
     */
    const uintptr_t mapping =
        (uintptr_t) mmap((void *) round_down(first_load->p_vaddr, pagesize),
                         span, prot_from_phdr(first_load), MAP_PRIVATE, fd,
                         round_down(first_load->p_offset, pagesize));
    if (mapping == (uintptr_t) MAP_FAILED)
        fail(filename, "Failed to map ELF into memory!", NULL, 0);

    /*
     * Mapping will not always equal to round_down(first_load->p_vaddr,
     * pagesize).
     */
    const ElfW(Addr) load_bias =
        mapping - round_down(first_load->p_vaddr, pagesize);

    if (first_load->p_offset > ehdr.e_phoff ||
        first_load->p_filesz <
            ehdr.e_phoff + (ehdr.e_phnum * sizeof(ElfW(Phdr))))
        fail(filename, "First load segment of ELF does not contain phdrs!",
             NULL, 0);

    handle_bss(first_load, filename, load_bias, pagesize);

    ElfW(Addr) last_end = add_uintptr_or_fail(
        add_uintptr_or_fail(first_load->p_vaddr, load_bias, filename,
                            "PT_LOAD range overflows!"),
        first_load->p_memsz, filename, "PT_LOAD range overflows!");

    /* Map the remaining segments, and protect any holes between them. */
    for (const ElfW(Phdr) *ph = first_load + 1; ph <= last_load; ++ph) {
        if (ph->p_type == PT_LOAD) {
            ElfW(Addr) last_page_end = round_up(last_end, pagesize);

            last_end = add_uintptr_or_fail(
                add_uintptr_or_fail(ph->p_vaddr, load_bias, filename,
                                    "PT_LOAD range overflows!"),
                ph->p_memsz, filename, "PT_LOAD range overflows!");
            ElfW(Addr) start = round_down(ph->p_vaddr + load_bias, pagesize);
            ElfW(Addr) end = round_up(last_end, pagesize);

            if (start > last_page_end &&
                mprotect((void *) last_page_end, start - last_page_end,
                         PROT_NONE))
                fail(filename, "mprotect failed on hole!", NULL, 0);

            if (mmap((void *) start, end - start, prot_from_phdr(ph),
                     MAP_PRIVATE | MAP_FIXED, fd,
                     round_down(ph->p_offset, pagesize)) == MAP_FAILED)
                fail(filename, "Failed to map PT_LOAD segment!", NULL, 0);

            handle_bss(ph, filename, load_bias, pagesize);
        }
    }

    /* Find PT_DYNAMIC header. */
    ElfW(Dyn) *dynamic = NULL;
    for (i = 0; i < ehdr.e_phnum; ++i) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            assert(dynamic == NULL);
            dynamic = (ElfW(Dyn) *) (load_bias + phdr[i].p_vaddr);
        }
    }
    if (dynamic == NULL)
        fail(filename, "ELF file has no PT_DYNAMIC header!", NULL, 0);

    ElfW_Reloc *relocs =
        (ElfW_Reloc *) (load_bias + get_dynamic_entry(dynamic, ELFW_DT_RELW));
    size_t relocs_size = get_dynamic_entry(dynamic, ELFW_DT_RELWSZ);
    if (relocs_size % sizeof(ElfW_Reloc) != 0)
        fail(filename, "Malformed relocation table size!", NULL, 0);
    for (i = 0; i < relocs_size / sizeof(ElfW_Reloc); i++) {
        ElfW_Reloc *reloc = &relocs[i];
        int reloc_type = ELFW_R_TYPE(reloc->r_info);
        switch (reloc_type) {
        case R_X86_64_RELATIVE:
        case R_ARM_RELATIVE:
        case R_AARCH64_RELATIVE: {
            ElfW(Addr) *addr = (ElfW(Addr) *) (load_bias + reloc->r_offset);
            uintptr_t page = round_down((uintptr_t) addr, pagesize);
            const ElfW(Phdr) *target_load =
                find_load_segment(phdr, ehdr.e_phnum, reloc->r_offset);
            if (target_load == NULL)
                fail(filename, "Relocation target is outside PT_LOAD!",
                     "r_offset", (int) reloc->r_offset);
            int need_mprotect = !(target_load->p_flags & PF_W);
            if (need_mprotect &&
                mprotect((void *) page, pagesize,
                         prot_from_phdr(target_load) | PROT_WRITE))
                fail(filename, "mprotect failed before relocation!", NULL, 0);
#if ELFW_DT_RELW == DT_RELA
            *addr = load_bias + reloc->r_addend;
#else
            *addr += load_bias;
#endif
            if (need_mprotect &&
                mprotect((void *) page, pagesize, prot_from_phdr(target_load)))
                fail(filename, "mprotect failed after relocation!", NULL, 0);
            break;
        }
        case R_X86_64_GLOB_DAT:
        case R_ARM_GLOB_DAT:
        case R_AARCH64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
        case R_ARM_JUMP_SLOT:
        case R_AARCH64_JUMP_SLOT:
            /* Deferred: resolved by api_resolve_symbols */
            break;
        default:
            fail(filename, "Unsupported relocation type!", "r_info",
                 reloc_type);
        }
    }

    dloader_p o = malloc(sizeof(struct __DLoader_Internal));
    assert(o != NULL);
    my_bzero(o, sizeof(*o));

    o->load_bias = load_bias;
    o->entry = (void *) (ehdr.e_entry + load_bias);
    o->pt_dynamic = dynamic;

    uintptr_t pltgot = get_dynamic_entry(dynamic, DT_PLTGOT);
    uintptr_t pltrelsz = get_dynamic_entry(dynamic, DT_PLTRELSZ);
    uintptr_t jmprel = get_dynamic_entry(dynamic, DT_JMPREL);
    if (pltrelsz % sizeof(ElfW_Reloc) != 0)
        fail(filename, "Malformed PLT relocation size!", NULL, 0);
    if (pltgot != 0)
        o->dt_pltgot = (void **) (pltgot + load_bias);
    if (jmprel != 0 && pltrelsz != 0) {
        o->dt_jmprel = (ElfW_Reloc *) (jmprel + load_bias);
        o->plt_entries = pltrelsz / sizeof(ElfW_Reloc);
    }

    /* Parse symbol table and string table */
    uintptr_t symtab = get_dynamic_entry(dynamic, DT_SYMTAB);
    uintptr_t strtab = get_dynamic_entry(dynamic, DT_STRTAB);
    if (symtab)
        o->dt_symtab = (ElfW(Sym) *) (symtab + load_bias);
    if (strtab)
        o->dt_strtab = (const char *) (strtab + load_bias);
    o->dt_strsz = get_dynamic_entry(dynamic, DT_STRSZ);

    /* Parse ELF (SYSV) hash table */
    uintptr_t elf_ht = get_dynamic_entry(dynamic, DT_HASH);
    if (elf_ht) {
        const uint32_t *ht = (const uint32_t *) (elf_ht + load_bias);
        o->hash_nbuckets = ht[0];
        o->hash_nchains = ht[1];
        if (o->hash_nbuckets > 0) {
            o->hash_buckets = &ht[2];
            o->hash_chains = &ht[2 + o->hash_nbuckets];
        }
        o->sym_count = o->hash_nchains;
    } else if (symtab && strtab && strtab > symtab) {
        /* Fallback: derive from .dynsym/.dynstr adjacency */
        o->sym_count = (strtab - symtab) / sizeof(ElfW(Sym));
    }

    /* Parse GNU hash table */
    uintptr_t gnu_ht = get_dynamic_entry(dynamic, DT_GNU_HASH);
    if (gnu_ht) {
        const uint32_t *ght = (const uint32_t *) (gnu_ht + load_bias);
        o->gnu_nbuckets = ght[0];
        o->gnu_symndx = ght[1];
        uint32_t maskwords = ght[2];
        if (o->gnu_nbuckets > 0 && maskwords > 0) {
            /* Skip header (4 words) + bloom filter */
            const uint32_t *after_bloom =
                (const uint32_t *) ((const char *) &ght[4] +
                                    maskwords * sizeof(ElfW(Addr)));
            o->gnu_buckets = after_bloom;
            o->gnu_chains = after_bloom + o->gnu_nbuckets;
        }
    }

    /* Store relocation metadata for deferred resolution */
    o->dt_relocs = relocs_size ? relocs : NULL;
    o->dt_relocs_count = relocs_size / sizeof(ElfW_Reloc);
    o->phdr = (const ElfW(Phdr) *) (load_bias + ehdr.e_phoff);
    o->phnum = ehdr.e_phnum;
    o->pagesize = pagesize;

    close(fd);
    return o;
}

void *api_get_user_info(dloader_p o)
{
    return ((struct program_header *) (o->entry))->user_info;
}

void api_set_plt_resolver(dloader_p o, plt_resolver_t resolver, void *handle)
{
    struct program_header *PROG_HEADER = o->entry;
    *PROG_HEADER->plt_trampoline = (void *) plt_trampoline;
    *PROG_HEADER->plt_handle = o;
    o->user_plt_resolver = resolver;
    o->user_plt_resolver_handle = handle;
}

void api_set_plt_entry(dloader_p o, int import_id, void *func)
{
    struct program_header *ph = (struct program_header *) o->entry;
    size_t plt_count = get_plt_count(ph);
    if (import_id < 0 || (size_t) import_id >= plt_count)
        fail("<runtime>", "PLT import_id out of bounds!", "import_id",
             import_id);
    ph->pltgot[import_id] = func;
}

/*
 * Look up a symbol by name using the ELF hash table.
 * Returns NULL if no matching defined symbol is found.
 */
static void *lookup_by_sysv_hash(dloader_p o, const char *name)
{
    unsigned long h = elf_hash(name);
    uint32_t idx = o->hash_buckets[h % o->hash_nbuckets];
    while (idx != STN_UNDEF) {
        if (idx >= o->hash_nchains)
            return NULL;
        ElfW(Sym) *sym = &o->dt_symtab[idx];
        if (sym->st_name < o->dt_strsz &&
            my_strcmp(o->dt_strtab + sym->st_name, name) == 0) {
            if (sym->st_shndx != SHN_UNDEF)
                return (void *) (o->load_bias + sym->st_value);
        }
        idx = o->hash_chains[idx];
    }
    return NULL;
}

/*
 * Look up a symbol by name using the GNU hash table.
 * Returns NULL if no matching defined symbol is found.
 */
static void *lookup_by_gnu_hash(dloader_p o, const char *name)
{
    uint32_t h = gnu_hash_func(name);
    uint32_t idx = o->gnu_buckets[h % o->gnu_nbuckets];
    if (idx == 0 || idx < o->gnu_symndx)
        return NULL;
    const uint32_t *chain = &o->gnu_chains[idx - o->gnu_symndx];
    for (size_t safety = 0; safety < 65536; ++safety) {
        uint32_t chain_val = *chain;
        if ((chain_val | 1) == (h | 1)) {
            ElfW(Sym) *sym = &o->dt_symtab[idx];
            if (sym->st_name < o->dt_strsz &&
                my_strcmp(o->dt_strtab + sym->st_name, name) == 0) {
                if (sym->st_shndx != SHN_UNDEF)
                    return (void *) (o->load_bias + sym->st_value);
            }
        }
        if (chain_val & 1)
            break;
        ++idx;
        ++chain;
    }
    return NULL;
}

void *api_lookup_symbol(dloader_p o, const char *name)
{
    void *sym = NULL;

    if (!o->dt_symtab || !o->dt_strtab || !name)
        return NULL;
    if (o->hash_buckets)
        sym = lookup_by_sysv_hash(o, name);
    if (!sym && o->gnu_buckets)
        sym = lookup_by_gnu_hash(o, name);
    return sym;
}

/*
 * Resolve a single relocation that references a symbol (GLOB_DAT or
 * JUMP_SLOT).  Looks up the symbol locally first; falls back to the
 * user-supplied resolver for undefined symbols.
 */
static int resolve_one(dloader_p o,
                       ElfW_Reloc *reloc,
                       symbol_resolver_t resolver,
                       void *handle)
{
    unsigned sym_idx = ELFW_R_SYM(reloc->r_info);
    unsigned bind;
    if (!sym_idx)
        return 0;

    if (o->sym_count && sym_idx >= o->sym_count)
        return -1;

    ElfW(Sym) *sym = &o->dt_symtab[sym_idx];
    if (sym->st_name >= o->dt_strsz)
        return -1;
    bind = ELFW_ST_BIND(sym->st_info);

    const char *name = o->dt_strtab + sym->st_name;
    void *addr = NULL;

    /* Try local definition first */
    if (sym->st_shndx != SHN_UNDEF)
        addr = (void *) (o->load_bias + sym->st_value);
    else if (resolver)
        addr = resolver(handle, name);

    if (!addr && bind != STB_WEAK)
        return -1;

#if ELFW_DT_RELW == DT_RELA
    uintptr_t value = (uintptr_t) addr + reloc->r_addend;
#else
    uintptr_t value = (uintptr_t) addr;
#endif

    ElfW(Addr) *target = (ElfW(Addr) *) (o->load_bias + reloc->r_offset);
    const ElfW(Phdr) *seg =
        find_load_segment(o->phdr, o->phnum, reloc->r_offset);
    if (!seg)
        return -1;

    uintptr_t page = round_down((uintptr_t) target, o->pagesize);
    int need_mprotect = !(seg->p_flags & PF_W);

    if (need_mprotect &&
        mprotect((void *) page, o->pagesize, prot_from_phdr(seg) | PROT_WRITE))
        return -1;

    *target = value;

    if (need_mprotect &&
        mprotect((void *) page, o->pagesize, prot_from_phdr(seg)))
        return -1;

    return 0;
}

static int is_glob_dat_or_jump_slot(int type)
{
    switch (type) {
    case R_X86_64_GLOB_DAT:
    case R_ARM_GLOB_DAT:
    case R_AARCH64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT:
    case R_ARM_JUMP_SLOT:
    case R_AARCH64_JUMP_SLOT:
        return 1;
    default:
        return 0;
    }
}

static int resolve_table(dloader_p o,
                         ElfW_Reloc *table,
                         size_t count,
                         symbol_resolver_t resolver,
                         void *handle)
{
    for (size_t i = 0; i < count; i++) {
        ElfW_Reloc *reloc = &table[i];
        if (!is_glob_dat_or_jump_slot(ELFW_R_TYPE(reloc->r_info)))
            continue;
        if (resolve_one(o, reloc, resolver, handle))
            return -1;
    }
    return 0;
}

int api_resolve_symbols(dloader_p o, symbol_resolver_t resolver, void *handle)
{
    if (!o->dt_symtab || !o->dt_strtab)
        return -1;

    if (resolve_table(o, o->dt_relocs, o->dt_relocs_count, resolver, handle))
        return -1;
    return resolve_table(o, o->dt_jmprel, o->plt_entries, resolver, handle);
}

struct __DLoader_API__ DLoader = {
    .load = api_load,
    .get_info = api_get_user_info,
    .set_plt_resolver = api_set_plt_resolver,
    .set_plt_entry = api_set_plt_entry,
    .lookup_symbol = api_lookup_symbol,
    .resolve_symbols = api_resolve_symbols,
};
