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

#define MAX_PHNUM 12

#define ELFW_R_TYPE(x) ELFW(R_TYPE)(x)
#define ELFW_R_SYM(x) ELFW(R_SYM)(x)

struct __DLoader_Internal {
    uintptr_t load_bias;
    void *entry;
    ElfW(Dyn) *pt_dynamic;

    void **dt_pltgot;
    ElfW_Reloc *dt_jmprel;
    size_t plt_entries;

    plt_resolver_t user_plt_resolver;
    void *user_plt_resolver_handle;
};

/*
 * In recent glibc, even simple functions like memset and strlen can
 * depend on complex startup code, because they are defined using
 * STT_GNU_IFUNC.
 */
static inline
void my_bzero(void *buf, size_t n)
{
    char *p = buf;
    while (n-- > 0)
        *p++ = 0;
}

static inline
size_t my_strlen(const char *s)
{
    size_t n = 0;
    while (*s++ != '\0')
        ++n;
    return n;
}

/*
 * We're avoiding libc, so no printf.  The only nontrivial thing we need
 * is rendering numbers, which is, in fact, pretty trivial.
 * bufsz of course must be enough to hold INT_MIN in decimal.
 */
static void iov_int_string(int value, struct iovec *iov,
                           char *buf, size_t bufsz)
{
    static const char * const lookup = "9876543210123456789" + 9;
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
    { (void *) string_constant, cond ? (sizeof(string_constant) - 1) : 0 }

__attribute__((noreturn))
static void fail(const char *filename, const char *message,
                 const char *item, int value)
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

static int prot_from_phdr(const ElfW(Phdr) *phdr)
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

static inline
uintptr_t round_up(uintptr_t value, uintptr_t size)
{
    return (value + size - 1) & -size;
}

static inline
uintptr_t round_down(uintptr_t value, uintptr_t size)
{
    return value & -size;
}

/*
 * Handle the "bss" portion of a segment, where the memory size
 * exceeds the file size and we zero-fill the difference.
 * For any whole pages in this region, we over-map anonymous pages.
 * For the sub-page remainder, we zero-fill bytes directly.
 */
static void handle_bss(const ElfW(Phdr) *ph, ElfW(Addr) load_bias,
                       size_t pagesize)
{
    if (ph->p_memsz > ph->p_filesz) {
        ElfW(Addr) file_end = ph->p_vaddr + load_bias + ph->p_filesz;
        ElfW(Addr) file_page_end = round_up(file_end, pagesize);
        ElfW(Addr) page_end =
            round_up(ph->p_vaddr + load_bias + ph->p_memsz, pagesize);
        if (page_end > file_page_end)
            mmap((void *) file_page_end,
                    page_end - file_page_end, prot_from_phdr(ph),
                    MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0);
        if (file_page_end > file_end && (ph->p_flags & PF_W))
            my_bzero((void *) file_end, file_page_end - file_end);
    }
}

ElfW(Word) get_dynamic_entry(ElfW(Dyn) *dynamic, int field)
{
    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        if (dynamic->d_tag == field)
            return dynamic->d_un.d_val;
    }
    return 0;
}

/*
 * Use pre-defined macro in arch/ to support different
 * system-specific assembly, see lib-support.h.
 */
void plt_trampoline();
asm(".pushsection .text,\"ax\",\"progbits\""  "\n"
    "plt_trampoline:"                         "\n"
    POP_S(REG_ARG_1) /* Argument 1 */         "\n"
    POP_S(REG_ARG_2) /* Argument 2 */         "\n"
    PUSH_STACK_STATE                          "\n"
    CALL(system_plt_resolver)                 "\n"
    POP_STACK_STATE                           "\n"
    JMP_REG(REG_RET)                          "\n"
    ".popsection"                             "\n");

void *system_plt_resolver(dloader_p o, int import_id)
{
    return o->user_plt_resolver(o->user_plt_resolver_handle, import_id);
}

dloader_p api_load(const char *filename)
{
    size_t pagesize = 0x1000;
    int fd = open(filename, O_RDONLY);
    ElfW(Ehdr) ehdr;
    pread(fd, &ehdr, sizeof(ehdr), 0);

    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr.e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr.e_version != EV_CURRENT || ehdr.e_ehsize != sizeof(ehdr) ||
        ehdr.e_phentsize != sizeof(ElfW(Phdr)))
        fail(filename, "File has no valid ELF header!", NULL, 0);

    switch (ehdr.e_machine) {
    case EM_X86_64:
    case EM_ARM:
    case EM_AARCH64:
        break;
    default:
        fail(filename, "ELF file has wrong architecture!  ",
             "e_machine", ehdr.e_machine);
        break;
    }

    ElfW(Phdr) phdr[MAX_PHNUM];
    if (ehdr.e_phnum > sizeof(phdr) / sizeof(phdr[0]) || ehdr.e_phnum < 1)
        fail(filename, "ELF file has unreasonable ", "e_phnum", ehdr.e_phnum);

    if (ehdr.e_type != ET_DYN)
        fail(filename, "ELF file not ET_DYN!  ", "e_type", ehdr.e_type);

    pread(fd, phdr, sizeof(phdr[0]) * ehdr.e_phnum, ehdr.e_phoff);

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
    size_t span = last_load->p_vaddr + last_load->p_memsz - first_load->p_vaddr;

    /*
     * Map the first segment and reserve the space used for the rest and
     * for holes between segments.
     */
    const uintptr_t mapping =
        (uintptr_t) mmap((void *) round_down(first_load->p_vaddr, pagesize),
                         span, prot_from_phdr(first_load), MAP_PRIVATE, fd,
                         round_down(first_load->p_offset, pagesize));

    /*
     * Mapping will not always equal to round_down(first_load->p_vaddr, pagesize).
     */
    const ElfW(Addr) load_bias =
        mapping - round_down(first_load->p_vaddr, pagesize);

    if (first_load->p_offset > ehdr.e_phoff ||
        first_load->p_filesz <
            ehdr.e_phoff + (ehdr.e_phnum * sizeof(ElfW(Phdr))))
        fail(filename, "First load segment of ELF does not contain phdrs!",
             NULL, 0);

    const ElfW(Phdr) *ro_load = NULL;
    if (!(first_load->p_flags & PF_W))
        ro_load = first_load;

    handle_bss(first_load, load_bias, pagesize);

    ElfW(Addr) last_end = first_load->p_vaddr + load_bias +
                          first_load->p_memsz;

    /* Map the remaining segments, and protect any holes between them. */
    for (const ElfW(Phdr) *ph = first_load + 1; ph <= last_load; ++ph) {
        if (ph->p_type == PT_LOAD) {
            ElfW(Addr) last_page_end = round_up(last_end, pagesize);

            last_end = ph->p_vaddr + load_bias + ph->p_memsz;
            ElfW(Addr) start = round_down(ph->p_vaddr + load_bias, pagesize);
            ElfW(Addr) end = round_up(last_end, pagesize);

            if (start > last_page_end)
                mprotect((void *) last_page_end,
                         start - last_page_end, PROT_NONE);

            mmap((void *) start, end - start,
                 prot_from_phdr(ph), MAP_PRIVATE | MAP_FIXED, fd,
                 round_down(ph->p_offset, pagesize));

            handle_bss(ph, load_bias, pagesize);
            if (!(ph->p_flags & PF_W) && !ro_load)
                ro_load = ph;
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
    assert(dynamic != NULL);

    ElfW(Addr) ro_start = ro_load->p_offset + load_bias;
    ElfW(Addr) ro_end = ro_start + ro_load->p_memsz;
    ElfW_Reloc *relocs =
        (ElfW_Reloc *)(load_bias + get_dynamic_entry(dynamic, ELFW_DT_RELW));
    size_t relocs_size = get_dynamic_entry(dynamic, ELFW_DT_RELWSZ);
    for (i = 0; i < relocs_size / sizeof(ElfW_Reloc); i++) {
        ElfW_Reloc *reloc = &relocs[i];
        int reloc_type = ELFW_R_TYPE(reloc->r_info);
        switch (reloc_type) {
        case R_X86_64_RELATIVE:
        case R_ARM_RELATIVE:
        case R_AARCH64_RELATIVE:
        {
            ElfW(Addr) *addr = (ElfW(Addr) *)(load_bias + reloc->r_offset);
            /*
             * If addr loactes in read-only PT_LOAD section, i.e., .text, then
             * we give the memory fragment WRITE permission during relocating
             * its address. Reset its access permission after relocation to
             * avoid some secure issue.
             */
            if ((intptr_t) addr < ro_end && (intptr_t) addr >= ro_start) {
                mprotect((void*) round_down((intptr_t) addr, pagesize),
                         pagesize, PROT_WRITE);
                *addr += load_bias;
                mprotect((void*) round_down((intptr_t) addr, pagesize),
                         pagesize, prot_from_phdr(ro_load));
            }
            else
                *addr += load_bias;
            break;
        }
        default:
            assert(0);
        }
    }

    dloader_p o = malloc(sizeof(struct __DLoader_Internal));
    assert(o != NULL);

    o->load_bias = load_bias;
    o->entry = (void *)(ehdr.e_entry + load_bias);
    o->pt_dynamic = dynamic;
    o->dt_pltgot = NULL;
    o->plt_entries = 0;
    uintptr_t pltgot = get_dynamic_entry(dynamic, DT_PLTGOT);
    if (pltgot != 0) {
        o->dt_pltgot = (void **) (pltgot + load_bias);
        o->dt_jmprel = (ElfW_Reloc *) (get_dynamic_entry(dynamic, DT_JMPREL) +
                                       load_bias);
    }

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
    ((struct program_header *) (o->entry))->pltgot[import_id] = func;
}

struct __DLoader_API__ DLoader = {
    .load = api_load,
    .get_info = api_get_user_info,
    .set_plt_resolver = api_set_plt_resolver,
    .set_plt_entry = api_set_plt_entry,
};
