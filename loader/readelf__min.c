//sig handling
#include <signal.h>
#include <ucontext.h>
#include <setjmp.h>
#include <errno.h>

jmp_buf restore_point;
struct sigaction sa;

void Handler(int sig, siginfo_t *si, ucontext_t *context)
{
    if (sig == SIGSEGV)
    {
        void * h = &Handler;
        signal(SIGSEGV, h);
        longjmp(restore_point, SIGSEGV);
    }
}

#include <stdio.h>
void
init_handler() {
    sa.sa_flags = SA_SIGINFO|SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = Handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
        perror("failed to set handler");
}

int test(char * address)
{
    init_handler();
    int fault_code = setjmp(restore_point);
    if (fault_code == 0)
    {
        /*
        if this seg faults in gdb, pass "handle SIGSEGV nostop pass noprint" to gdb command line to allow the hander init_handler() to handle this instead of gdb:
        (gdb) handle SIGSEGV nostop pass noprint
        <enter>
        (gdb) r
        <enter>
        
        if u use pwndbg the instructions are the same:
        pwndbg> handle SIGSEGV nostop pass noprint
        <enter>
        pwndbg> r
        <enter>
            
        alternatively start gdb like this (this assumes this is run inside a script and the executable this is compiled into is named ./loader and compiled with  test_loader.c containing a
        main() { 
            ... ;
            return 0;
        }
        with return 0; being on line 22, note the ... signifies a variable amount of text as we do not know what code main() {} can contain) :

        gdb ./loader -ex "set args $1" -ex "break test_loader.c:22" -ex "handle SIGSEGV nostop pass noprint" -ex "r"

        else this works fine:

        gdb <file> -ex "handle SIGSEGV nostop pass noprint" -ex "r"


        */
        printf("value: %15d\t", *(int*)address);
        return 0;
    }
    else
    {
        printf("value: %s\t", "     is not int");
        return -1;
    }
}


// from libstring:
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
void nl() {
    printf("\n");
}

int bcmp_q(void const *vp, size_t n, void const *vp2, size_t n2)
{
    int string_match = 0;
    if (n == n2) {
        unsigned char const *p = vp;
        unsigned char const *p2 = vp2;
        for (size_t i=0; i<n; i++)
            if (p[i] == p2[i]) {
//                 printf("p[%d] = %c\n", i, p[i]);
                string_match = 1;
            } else { string_match = 0; break; }
        if (string_match == 0) {
            return -1;
        } else return 0;
    } else
    {
        return -1;
    }
}


int bytecmpq(void const * p, void const * pp) { return bcmp_q(p, strlen(p), pp, strlen(pp)); }

uintptr_t round_down(uintptr_t value, uintptr_t size)
{
    printf("called round_down\nreturning %014p\n", value ? size * (value / size) : value);
    return value ? size * (value / size) : value;
}

uintptr_t round_up(uintptr_t value, uintptr_t size)
{
    printf("called round_up\nreturning %014p\n", value ? size * ((value + (size - 1)) / size) : size);
//     return size * ((value + (size - 1)) / size);
    return value ? size * ((value + (size - 1)) / size) : size;
}

void abort_() {
    printf("cannot continue, pausing execution to allow for debugging\nif you do now know how to debug this process as paused execute the following in a new terminal:\n\n    sudo gdb -p %d\n\n", getpid());
    pause();
}

void
print_maps(void)
{
	char rbuf[1024];
	int fd, cc;

	fd = open("/proc/self/maps", 0, 0);
	while (0 < (cc = read(fd, rbuf, sizeof(rbuf))))
		write(1, rbuf, cc);
	close(fd);
}

// reads a string instead of a file descriptor, verifies length
int read_fast_verify(const char *src, int len_of_source, char **dest, int requested_len) {
    *dest = malloc(requested_len+4096);
    if (len_of_source < requested_len) memcpy(*dest, src, len_of_source);
    else memcpy(*dest, src, requested_len);
    *dest = memmove(round_up(*dest, 4096), *dest, requested_len);
    return requested_len;
}

// special version specifically for PT_LOAD handling
int read_fast_verifyb(const char *src, int len_of_source, char **dest, int requested_len, Elf64_Phdr PT_LOAD_F, Elf64_Phdr PT_LOAD_L) {
    void * align = 0x10000000;
    *dest = malloc(requested_len+align+PT_LOAD_L.p_align);
    if (len_of_source < requested_len) memcpy(*dest, src, len_of_source);
    else memcpy(*dest, src, requested_len);
    printf("memmove: round_up(%014p, %014p)+%014p = %014p\n", *dest, align, PT_LOAD_L.p_align, round_up(*dest, align)+PT_LOAD_L.p_align);
    *dest = memmove(round_up(*dest, align)+PT_LOAD_L.p_align, *dest, requested_len);
    printf("dest = %014p\n", *dest);
    *dest = memmove(*dest-PT_LOAD_L.p_align, *dest, PT_LOAD_F.p_memsz);
    printf("dest = %014p\n", *dest);
    return requested_len;
}

char *strjoin_(const char *_a, const char *_b) {
    size_t na = strlen(_a);
    size_t nb = strlen(_b);
    char *p = malloc(na + nb + 1);
    memcpy(p, _a, na);
    memcpy(p + na, _b, nb);
    p[na + nb] = 0;
    return p;
}

// end of libstring

int init__ = 0;
int init_(const char * filename);
int initv_(const char * filename);


void * lookup_symbol_by_name_(const char * lib, const char * name);
// for C++ symbol name demangling should libirty become incompatible
// http://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
// https://itanium-cxx-abi.github.io/cxx-abi/gcc-cxxabi.h
// https://github.com/xaxxon/xl/blob/master/include/xl/demangle.h
// gdb and c++filt use demangler provided by libiberty

// allow for demangling of C++ symbols, link with -liberty
#include <libiberty/libiberty.h>
#include <libiberty/demangle.h>
#include <libiberty/safe-ctype.h>
int flags = DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE;
int strip_underscore = 0;
char * demangle_it (char *mangled_name)
{
  char *result;
  unsigned int skip_first = 0;

  /* _ and $ are sometimes found at the start of function names
     in assembler sources in order to distinguish them from other
     names (eg register names).  So skip them here.  */
  if (mangled_name[0] == '.' || mangled_name[0] == '$')
    ++skip_first;
  if (strip_underscore && mangled_name[skip_first] == '_')
    ++skip_first;

  result = cplus_demangle (mangled_name + skip_first, flags);
//   bytecmp(mangled_name, mangled_name);
//   printf("\n\nmangled_name[%d] = %c , mangled_name[%d] = %c\n\n", strlen(mangled_name)-2, strlen(mangled_name)-1, mangled_name[strlen(mangled_name)-2], mangled_name[strlen(mangled_name)-1]);
//     if ( mangled_name[-2] == '(' && mangled_name[-1] == ')')
//         mangled_name[-2] = '\0';
  if (result == NULL) return mangled_name;
  else if (mangled_name[0] == '.') return strjoin_(".", result); else return result;
}

char * __print_quoted_string__(const char *str, unsigned int size, const unsigned int style, const char * return_type);
#define _GNU_SOURCE
#define __USE_GNU

// ELF Spec     FULL:  http://refspecs.linuxbase.org/elf/elf.pdf
// ELF Spec ABI FULL:  https://github.com/hjl-tools/x86-psABI/wiki/x86-64-psABI-1.0.pdf

#include <link.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <errno.h>
uintptr_t mappingb;
Elf64_Ehdr *_elf_header;
Elf64_Phdr *_elf_program_header;
Elf64_Shdr *_elf_symbol_table;
char * program_hdr;
size_t len;
size_t RELA_PLT_SIZE = 0;
char * array;
int PT_DYNAMIC_ = NULL;
char * tmp99D;
int First_Load_Header_index = NULL;
int Last_Load_Header_index = NULL;
size_t align;
ElfW(Addr) base_address = 0x00000000;
ElfW(Addr) mappingb_end = 0x00000000;
char * last_lib = "";
int is_mapped = 0;
#define symbol_mode_S 1
#define symbol_mode_Z 2
char * init(char * lib) {
    if (bytecmpq(lib, last_lib) != 0) { array = NULL; is_mapped = 0; init__ = 0; }
    last_lib=lib;
    if (array == NULL) {
        int fd = open(lib, O_RDONLY);
        if (fd < 0) {
            printf("cannot open \"%s\", returned %i\n", lib, fd);
            return -1;
        }
        len = 0;
        len = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, 0);
        array = mmap (NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
        if (array == MAP_FAILED) {
            printf ("map failed\n");
            exit;
        } else {
            printf ("map succeded with address: %014p\n", array);
            return 0;
        }
    } else return 0;
    return -1;
}

int prot_from_phdr(const int p_flags)
{
    int prot = 0;
    if (p_flags & PF_R)
    {
        printf("PROT_READ|");
        prot |= PROT_READ;
    }
    if (p_flags & PF_W)
    {
        printf("PROT_WRITE|");
        prot |= PROT_WRITE;
    }
    if (p_flags & PF_X)
    {
        printf("PROT_EXEC|");
        prot |= PROT_EXEC;
    }
    return prot;
}

void map() {
    if (is_mapped == 0) {
        Elf64_Ehdr * _elf_header = (Elf64_Ehdr *) array;
        _elf_program_header = (Elf64_Phdr *)((unsigned long)_elf_header + _elf_header->e_phoff);

// the very first thing we do is obtain the base address

// Base Address
// The virtual addresses in the program headers might not represent the actual virtual addresses
// of the program's memory image. Executable files typically contain absolute code. To let the
// process execute correctly, the segments must reside at the virtual addresses used to build the
// executable file. On the other hand, shared object segments typically contain
// position-independent code. This lets a segment's virtual address change from one process to
// another, without invalidating execution behavior. Though the system chooses virtual addresses
// for individual processes, it maintains the segments’ relative positions. Because
// position-independent code uses relative addressing between segments, the difference between
// virtual addresses in memory must match the difference between virtual addresses in the file.
// 
// The difference between the virtual address of any segment in memory and the corresponding
// virtual address in the file is thus a single constant value for any one executable or shared object
// in a given process. This difference is the base address. One use of the base address is to relocate
// the memory image of the program during dynamic linking.
// 
// An executable or shared object file's base address is calculated during execution from three
// values: the virtual memory load address, the maximum page size, and the lowest virtual address
// of a program's loadable segment. To compute the base address, one determines the memory
// address associated with the lowest p_vaddr value for a PT_LOAD segment. This address is
// truncated to the nearest multiple of the maximum page size. The corresponding p_vaddr value
// itself is also truncated to the nearest multiple of the maximum page size. The base address is
// the difference between the truncated memory address and the truncated p_vaddr value.

        int PT_LOADS=0;
        for (int i = 0; i < _elf_header->e_phnum; ++i) {
            switch(_elf_program_header[i].p_type)
            {
                case PT_LOAD:
//                         printf("i = %d\n", i);
//                         printf("PT_LOADS = %d\n", PT_LOADS);
                    if (!PT_LOADS)  {
//                             printf("saving first load\n");
                        First_Load_Header_index = i;
                    }
                    if (PT_LOADS) {
//                             printf("saving last load\n");
                        Last_Load_Header_index = i;
                    }
                    PT_LOADS=PT_LOADS+1;
                    break;
            }
        }
        size_t span = _elf_program_header[Last_Load_Header_index].p_vaddr + _elf_program_header[Last_Load_Header_index].p_memsz - _elf_program_header[First_Load_Header_index].p_vaddr;

        size_t pagesize = 0x1000;

        read_fast_verifyb(array, len, &mappingb, span, _elf_program_header[First_Load_Header_index], _elf_program_header[Last_Load_Header_index]);

        align = round_down(_elf_program_header[Last_Load_Header_index].p_vaddr, pagesize);
        base_address = mappingb - align;
        mappingb_end = mappingb+span;

//             printf("base address range = %014p - %014p\nmapping = %014p\n", mappingb, mappingb_end, mapping);

// base address aquired, map all PT_LOAD segments adjusting by base address then continue with the rest
        printf("\n\n\nfind %014p, %014p, (int) 1239\n\n\n\n", mappingb, mappingb_end);

        if (mappingb == 0x00000000) abort_();
        int PT_LOADS_CURRENT = 0;
        for (int i = 0; i < _elf_header->e_phnum; ++i) {
            switch(_elf_program_header[i].p_type)
            {
                case PT_LOAD:
                    PT_LOADS_CURRENT = PT_LOADS_CURRENT + 1;
//                         printf ("mapping PT_LOAD number %d\n", PT_LOADS_CURRENT);
//                         printf("\t\tp_flags:  %014p\n", _elf_program_header[i].p_flags);
//                         printf("\t\tp_offset: %014p\n", _elf_program_header[i].p_offset);
//                         printf("\t\tp_vaddr:  %014p\n", _elf_program_header[i].p_vaddr+mappingb);
//                         printf("\t\tp_paddr:  %014p\n", _elf_program_header[i].p_paddr);
//                         printf("\t\tp_filesz: %014p\n", _elf_program_header[i].p_filesz);
//                         printf("\t\tp_memsz:  %014p\n", _elf_program_header[i].p_memsz);
//                         printf("\t\tp_align:  %014p\n\n", _elf_program_header[i].p_align);
// 
//                         printf("\tp_flags: %014p", _elf_program_header[i].p_flags);
//                         printf(" p_offset: %014p", _elf_program_header[i].p_offset);
//                         printf(" p_vaddr: %014p", _elf_program_header[i].p_vaddr+mappingb);
//                         printf(" p_paddr: %014p", _elf_program_header[i].p_paddr);
//                         printf(" p_filesz: %014p", _elf_program_header[i].p_filesz);
//                         printf(" p_memsz: %014p", _elf_program_header[i].p_memsz);
//                         printf(" p_align: %014p\n\n\n", _elf_program_header[i].p_align);

                    printf("mprotect(%014p+round_down(%014p, %014p), %014p, ", mappingb, _elf_program_header[i].p_vaddr, _elf_program_header[i].p_align, _elf_program_header[i].p_memsz);
                    prot_from_phdr(_elf_program_header[i].p_flags);
                    printf(");\n");
                    
                    int check_map_success = mprotect(mappingb+round_down(_elf_program_header[i].p_vaddr, _elf_program_header[i].p_align), round_up(_elf_program_header[i].p_memsz, _elf_program_header[i].p_align), _elf_program_header[i].p_flags);
                    if (errno == 0)
                    {
                        printf ("mprotect on %014p succeded with size: %014p\n", mappingb+round_down(_elf_program_header[i].p_vaddr, _elf_program_header[i].p_align), round_up(_elf_program_header[i].p_memsz, _elf_program_header[i].p_align));
                        print_maps();
                    }
                    else
                    {
                        printf ("mprotect failed with: %s (errno: %d)\n", strerror(errno), errno);
                        print_maps();
                        abort_();
                    }
                    break;
            }
        }
        is_mapped = 1;
    }
}

void __lseek_string__(char **src, int len, int offset) {
    char *p = malloc(len);
    memcpy(p, *src+offset, len);
    *src = p;
}

int read_section_header_table_(const char * arrayb, Elf64_Ehdr * eh, Elf64_Shdr * sh_table[])
{
    *sh_table = (Elf64_Shdr *)(arrayb + eh->e_shoff);
    if(!sh_table) {
        printf("Failed to read table\n");
        return -1;
    }
    return 0;
}

char * read_section_(char * ar, Elf64_Shdr sh) {
    char * buff = (char *)(ar + sh.sh_offset);
    return buff ;
}

char * obtain_rela_plt_size(char * sourcePtr, Elf64_Ehdr * eh, Elf64_Shdr sh_table[]) {
    char * sh_str = read_section_(sourcePtr, sh_table[eh->e_shstrndx]); // will fail untill section header table can be read
    for(int i=0; i<eh->e_shnum; i++) if (bytecmpq((sh_str + sh_table[i].sh_name), ".rela.plt") == 0) RELA_PLT_SIZE=_elf_symbol_table[i].sh_size;
}

char * print_section_headers_(char * sourcePtr, Elf64_Ehdr * eh, Elf64_Shdr sh_table[]) {
    printf ("\n");
    printf("eh->e_shstrndx = 0x%x (%d)\n", eh->e_shstrndx+mappingb, eh->e_shstrndx);
    char * sh_str;
    sh_str = read_section_(sourcePtr, sh_table[eh->e_shstrndx]); // will fail untill section header table can be read
    printf("\t========================================");
    printf("========================================\n");
    printf("\tidx offset     load-addr  size       algn type       flags      section\n");
    printf("\t========================================");
    printf("========================================\n");

    for(int i=0; i<eh->e_shnum; i++) { // will fail untill section header table can be read
        printf("\t%03d ", i);
        printf("%014p ", _elf_symbol_table[i].sh_offset); // not sure if this should be adjusted to base address
        printf("%014p ", _elf_symbol_table[i].sh_addr+mappingb);
        printf("%014p ", _elf_symbol_table[i].sh_size);
        printf("%4d ", _elf_symbol_table[i].sh_addralign);
        printf("%014p ", _elf_symbol_table[i].sh_type);
        printf("%014p ", _elf_symbol_table[i].sh_flags);
        printf("%s\t", (sh_str + sh_table[i].sh_name));
        printf("\n");
        if (bytecmpq((sh_str + sh_table[i].sh_name), ".rela.plt") == 0) RELA_PLT_SIZE=_elf_symbol_table[i].sh_size;
    }
    printf("\t========================================");
    printf("========================================\n");
    printf("\n");
}

int symbol(char * arrayc, Elf64_Shdr sh_table[], uint64_t symbol_table) {
    char *str_tbl;
    Elf64_Sym* sym_tbl;
    uint64_t i, symbol_count;
    printf("symbol_table = %d\n", symbol_table);
    sym_tbl = (Elf64_Sym*)read_section_(arrayc, sh_table[symbol_table]);

    /* Read linked string-table
    * Section containing the string table having names of
    * symbols of this section
    */
    uint64_t str_tbl_ndx = sh_table[symbol_table].sh_link;
    printf("string/symbol table index = %d\n", str_tbl_ndx);
    str_tbl = read_section_(arrayc, sh_table[str_tbl_ndx]);

    symbol_count = (sh_table[symbol_table].sh_size/sizeof(Elf64_Sym));
    int link_ = sh_table[symbol_table].sh_link;
    link_ = sh_table[link_].sh_link;
    int linkn = 0;
    while (link_ != 0) {
        link_ = sh_table[link_].sh_link;
        linkn++;
    }
    printf("links: %d\n", linkn);
    printf("%d symbols\n", symbol_count);

//   Elf64_Word	st_name;		/* Symbol name (string tbl index) */
//   unsigned char	st_info;		/* Symbol type and binding */
//   unsigned char st_other;		/* Symbol visibility */
//   Elf64_Section	st_shndx;		/* Section index */
//   Elf64_Addr	st_value;		/* Symbol value */
//   Elf64_Xword	st_size;		/* Symbol size */
    for(int i=0; i< symbol_count; i++) {
        printf("index: %d\t", i);
        printf("size: %10d \t", sym_tbl[i].st_size);
// /* Legal values for ST_BIND subfield of st_info (symbol binding).  */
// 
// #define STB_LOCAL	0		/* Local symbol */
// #define STB_GLOBAL	1		/* Global symbol */
// #define STB_WEAK	2		/* Weak symbol */
// #define	STB_NUM		3		/* Number of defined types.  */
// #define STB_LOOS	10		/* Start of OS-specific */
// #define STB_GNU_UNIQUE	10		/* Unique symbol.  */
// #define STB_HIOS	12		/* End of OS-specific */
// #define STB_LOPROC	13		/* Start of processor-specific */
// #define STB_HIPROC	15		/* End of processor-specific */
        printf("binding: ");
        switch (ELF64_ST_BIND(sym_tbl[i].st_info)) {
            case STB_LOCAL:
                printf("LOCAL   ( Local  symbol )  ");
                break;
            case STB_GLOBAL:
                printf("GLOBAL  ( Global symbol )  ");
                break;
            case STB_WEAK:
                printf("WEAK    (  Weak symbol  )  ");
                break;
            default:
                printf("UNKNOWN (%d)                ", ELF64_ST_BIND(sym_tbl[i].st_info));
                break;
        }
// /* Legal values for ST_TYPE subfield of st_info (symbol type).  */
// 
// #define STT_NOTYPE	0		/* Symbol type is unspecified */
// #define STT_OBJECT	1		/* Symbol is a data object */
// #define STT_FUNC	2		/* Symbol is a code object */
// #define STT_SECTION	3		/* Symbol associated with a section */
// #define STT_FILE	4		/* Symbol's name is file name */
// #define STT_COMMON	5		/* Symbol is a common data object */
// #define STT_TLS		6		/* Symbol is thread-local data object*/
// #define	STT_NUM		7		/* Number of defined types.  */
// #define STT_LOOS	10		/* Start of OS-specific */
// #define STT_GNU_IFUNC	10		/* Symbol is indirect code object */
// #define STT_HIOS	12		/* End of OS-specific */
// #define STT_LOPROC	13		/* Start of processor-specific */
// #define STT_HIPROC	15		/* End of processor-specific */
// /* Symbol visibility specification encoded in the st_other field.  */
// #define STV_DEFAULT	0		/* Default symbol visibility rules */
// #define STV_INTERNAL	1		/* Processor specific hidden class */
// #define STV_HIDDEN	2		/* Sym unavailable in other modules */
// #define STV_PROTECTED	3		/* Not preemptible, not exported */
        printf("visibility: ");
        switch (ELF64_ST_VISIBILITY(sym_tbl[i].st_other)) {
            case STV_DEFAULT:
                printf("default   (Default symbol visibility rules)      ");
                break;
            case STV_INTERNAL:
                printf("internal  (Processor specific hidden class)      ");
                break;
            case STV_HIDDEN:
                printf("hidden    (Symbol unavailable in other modules)  ");
                break;
            case STV_PROTECTED:
                printf("protected (Not preemptible, not exported)        ");
                break;
        }
        char * address = sym_tbl[i].st_value+mappingb;
        printf("address: %014p\t", address);
        if ( address > mappingb && address < mappingb_end ) test(address);
        else printf("value: %015p\t", sym_tbl[i].st_value);
        printf("type: ");
        switch (ELF64_ST_TYPE(sym_tbl[i].st_info)) {
            case STT_NOTYPE:
                printf("NOTYPE   (Symbol type is unspecified)             ");
                break;
            case STT_OBJECT:
                printf("OBJECT   (Symbol is a data object)                ");
                break;
                case STT_FUNC:
                printf("FUNCTION (Symbol is a code object)                ");
                break;
                case STT_SECTION:
                printf("SECTION  (Symbol associated with a section)       ");
                break;
                case STT_FILE:
                printf("FILE     (Symbol's name is file name)             ");
                break;
                case STT_COMMON:
                printf("COMMON   (Symbol is a common data object)         ");
                break;
                case STT_TLS:
                printf("TLS      (Symbol is thread-local data object)     ");
                break;
            default:
                printf("UNKNOWN  (%d)                                     ", ELF64_ST_TYPE(sym_tbl[i].st_info));
                break;
        }
        char * name = str_tbl + sym_tbl[i].st_name;
        printf("name: %s\n", demangle_it(name));
        nl();
//         if (bytecmp(name,"t") == 0) {
// 
//             printf("t found\n");
//                         
// // #define JMP_ADDR(x) asm("\tjmp  *%0\n" :: "r" (x))
// //             printf("(%014p+%014p=%014p)\n", mappingb, sym_tbl[i].st_value, sym_tbl[i].st_value+mappingb);
// //             printf("JMP_ADDR(%014p);\n", address);
// //             JMP_ADDR(address);
//                         printf("int (*testb)()                               =%014p\n", address);
// // 
//             printf("(%014p+%014p=%014p)\n", mappingb, sym_tbl[i].st_value, sym_tbl[i].st_value+mappingb);
// // 
//             int (*testb)() = lookup_symbol_by_name_("/chakra/home/universalpackagemanager/chroot/arch-chroot/arch-pkg-build/packages/glibc/repos/core-x86_64/min-dl/loader/files/test_lib.so", "t");
//             printf("testb = %014p\n", testb);
//             printf("testb() returned %d;\n",
//             testb()
//             );
// 
//             nl();
// //             int (*testc)() = mappingb+sym_tbl[i].st_value;
// //             printf("int (*testc)()                =%014p ; testc();\n", mappingb+sym_tbl[i].st_value);
// //             testc();
// //             nl();
// //             int foo(int i){ return i + 1;}
// // 
// //             typedef int (*g)(int);  // Declare typedef
// // 
// //             g func = mappingb+sym_tbl[i].st_value;          // Define function-pointer variable, and initialise
// // 
// //             int hvar = func(3);     // Call function through pointer
//             nl();
//             print_maps();
//         }
    }
}

int relocation(char * arrayc, Elf64_Shdr sh_table[], uint64_t symbol_table) {
    char *str_tbl;
    Elf64_Sym* sym_tbl;
    uint64_t i, symbol_count;

    sym_tbl = (Elf64_Sym*)read_section_(arrayc, sh_table[symbol_table]);

    /* Read linked string-table
    * Section containing the string table having names of
    * symbols of this section
    */
    uint64_t str_tbl_ndx = sh_table[symbol_table].sh_link;
    printf("string/symbol table index = %d\n", str_tbl_ndx);
    str_tbl = read_section_(arrayc, sh_table[str_tbl_ndx]);

    symbol_count = (sh_table[symbol_table].sh_size/sizeof(Elf64_Sym));
    int link_ = sh_table[symbol_table].sh_link;
    link_ = sh_table[link_].sh_link;
    int linkn = 0;
    while (link_ != 0) {
        link_ = sh_table[link_].sh_link;
        linkn++;
    }
    printf("links: %d\n", linkn);
    printf("%d symbols\n", symbol_count);

//   Elf64_Word	st_name;		/* Symbol name (string tbl index) */
//   unsigned char	st_info;		/* Symbol type and binding */
//   unsigned char st_other;		/* Symbol visibility */
//   Elf64_Section	st_shndx;		/* Section index */
//   Elf64_Addr	st_value;		/* Symbol value */
//   Elf64_Xword	st_size;		/* Symbol size */
    for(int i=0; i< symbol_count; i++) {
        printf("index: %d\t", i);
        printf("size: %10d \t", sym_tbl[i].st_size);
// /* Legal values for ST_BIND subfield of st_info (symbol binding).  */
// 
// #define STB_LOCAL	0		/* Local symbol */
// #define STB_GLOBAL	1		/* Global symbol */
// #define STB_WEAK	2		/* Weak symbol */
// #define	STB_NUM		3		/* Number of defined types.  */
// #define STB_LOOS	10		/* Start of OS-specific */
// #define STB_GNU_UNIQUE	10		/* Unique symbol.  */
// #define STB_HIOS	12		/* End of OS-specific */
// #define STB_LOPROC	13		/* Start of processor-specific */
// #define STB_HIPROC	15		/* End of processor-specific */
        printf("binding: ");
        switch (ELF64_ST_BIND(sym_tbl[i].st_info)) {
            case STB_LOCAL:
                printf("LOCAL   ( Local  symbol )  ");
                break;
            case STB_GLOBAL:
                printf("GLOBAL  ( Global symbol )  ");
                break;
            case STB_WEAK:
                printf("WEAK    (  Weak symbol  )  ");
                break;
            default:
                printf("UNKNOWN (%d)                ", ELF64_ST_BIND(sym_tbl[i].st_info));
                break;
        }
// /* Legal values for ST_TYPE subfield of st_info (symbol type).  */
// 
// #define STT_NOTYPE	0		/* Symbol type is unspecified */
// #define STT_OBJECT	1		/* Symbol is a data object */
// #define STT_FUNC	2		/* Symbol is a code object */
// #define STT_SECTION	3		/* Symbol associated with a section */
// #define STT_FILE	4		/* Symbol's name is file name */
// #define STT_COMMON	5		/* Symbol is a common data object */
// #define STT_TLS		6		/* Symbol is thread-local data object*/
// #define	STT_NUM		7		/* Number of defined types.  */
// #define STT_LOOS	10		/* Start of OS-specific */
// #define STT_GNU_IFUNC	10		/* Symbol is indirect code object */
// #define STT_HIOS	12		/* End of OS-specific */
// #define STT_LOPROC	13		/* Start of processor-specific */
// #define STT_HIPROC	15		/* End of processor-specific */
// /* Symbol visibility specification encoded in the st_other field.  */
// #define STV_DEFAULT	0		/* Default symbol visibility rules */
// #define STV_INTERNAL	1		/* Processor specific hidden class */
// #define STV_HIDDEN	2		/* Sym unavailable in other modules */
// #define STV_PROTECTED	3		/* Not preemptible, not exported */
        printf("visibility: ");
        switch (ELF64_ST_VISIBILITY(sym_tbl[i].st_other)) {
            case STV_DEFAULT:
                printf("default   (Default symbol visibility rules)      ");
                break;
            case STV_INTERNAL:
                printf("internal  (Processor specific hidden class)      ");
                break;
            case STV_HIDDEN:
                printf("hidden    (Symbol unavailable in other modules)  ");
                break;
            case STV_PROTECTED:
                printf("protected (Not preemptible, not exported)        ");
                break;
        }
        char * address;
        if ( ELF64_ST_TYPE(sym_tbl[i].st_info) == STT_FUNC)
        {
            address = sym_tbl[i].st_value+mappingb+align;
            printf("address: %014p\t", address);
        }
        else
        {
            address = sym_tbl[i].st_value+mappingb;
            printf("address: %014p\t", address);
        }
        if ( address > mappingb && address < mappingb_end ) test(address);
        else printf("value: %15s\t", "invalid range");
        printf("type: ");
        switch (ELF64_ST_TYPE(sym_tbl[i].st_info)) {
            case STT_NOTYPE:
                printf("NOTYPE   (Symbol type is unspecified)             ");
                break;
            case STT_OBJECT:
                printf("OBJECT   (Symbol is a data object)                ");
                break;
                case STT_FUNC:
                printf("FUNCTION (Symbol is a code object)                ");
                break;
                case STT_SECTION:
                printf("SECTION  (Symbol associated with a section)       ");
                break;
                case STT_FILE:
                printf("FILE     (Symbol's name is file name)             ");
                break;
                case STT_COMMON:
                printf("COMMON   (Symbol is a common data object)         ");
                break;
                case STT_TLS:
                printf("TLS      (Symbol is thread-local data object)     ");
                break;
            default:
                printf("UNKNOWN  (%d)                                     ", ELF64_ST_TYPE(sym_tbl[i].st_info));
                break;
        }
        printf("name: [Not obtained due to it may crash this program]\n");
//         printf("\n");
    }
}

void print_elf_symbol_table(char * arrayc, Elf64_Ehdr * eh, Elf64_Shdr sh_table[], uint64_t symbol_table)
{
    int level = 0;
        switch(sh_table[symbol_table].sh_type) {
            case SHT_NULL:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_PROGBITS:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_SYMTAB:
                symbol(arrayc, sh_table, symbol_table);
                break;
            case SHT_STRTAB:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_RELA:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_HASH:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_DYNAMIC:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_NOTE:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_NOBITS:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_REL:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_SHLIB:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_DYNSYM:
                symbol(arrayc, sh_table, symbol_table);
                break;
            case SHT_INIT_ARRAY:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_FINI_ARRAY:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_PREINIT_ARRAY:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_GROUP:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_SYMTAB_SHNDX:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_NUM:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_LOOS:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_GNU_ATTRIBUTES:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_GNU_HASH:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_GNU_LIBLIST:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_CHECKSUM:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_LOSUNW:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_SUNW_COMDAT:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_SUNW_syminfo:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_GNU_verdef:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_GNU_verneed:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_GNU_versym:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_LOPROC:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_HIPROC:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_LOUSER:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            case SHT_HIUSER:
                if (level == 3) relocation(arrayc, sh_table, symbol_table);
                break;
            default:
                printf("UNKNOWN ");
                break;
        }
}

void print_symbols(char * arrayd, Elf64_Ehdr * eh, Elf64_Shdr sh_table[])
{
// /* Legal values for sh_type (section type).  */
// 
// #define SHT_NULL	  0		/* Section header table entry unused */
// #define SHT_PROGBITS	  1		/* Program data */
// #define SHT_SYMTAB	  2		/* Symbol table */
// #define SHT_STRTAB	  3		/* String table */
// #define SHT_RELA	  4		/* Relocation entries with addends */
// #define SHT_HASH	  5		/* Symbol hash table */
// #define SHT_DYNAMIC	  6		/* Dynamic linking information */
// #define SHT_NOTE	  7		/* Notes */
// #define SHT_NOBITS	  8		/* Program space with no data (bss) */
// #define SHT_REL		  9		/* Relocation entries, no addends */
// #define SHT_SHLIB	  10		/* Reserved */
// #define SHT_DYNSYM	  11		/* Dynamic linker symbol table */
// #define SHT_INIT_ARRAY	  14		/* Array of constructors */
// #define SHT_FINI_ARRAY	  15		/* Array of destructors */
// #define SHT_PREINIT_ARRAY 16		/* Array of pre-constructors */
// #define SHT_GROUP	  17		/* Section group */
// #define SHT_SYMTAB_SHNDX  18		/* Extended section indeces */
// #define	SHT_NUM		  19		/* Number of defined types.  */
// #define SHT_LOOS	  0x60000000	/* Start OS-specific.  */
// #define SHT_GNU_ATTRIBUTES 0x6ffffff5	/* Object attributes.  */
// #define SHT_GNU_HASH	  0x6ffffff6	/* GNU-style hash table.  */
// #define SHT_GNU_LIBLIST	  0x6ffffff7	/* Prelink library list */
// #define SHT_CHECKSUM	  0x6ffffff8	/* Checksum for DSO content.  */
// #define SHT_LOSUNW	  0x6ffffffa	/* Sun-specific low bound.  */
// #define SHT_SUNW_move	  0x6ffffffa
// #define SHT_SUNW_COMDAT   0x6ffffffb
// #define SHT_SUNW_syminfo  0x6ffffffc
// #define SHT_GNU_verdef	  0x6ffffffd	/* Version definition section.  */
// #define SHT_GNU_verneed	  0x6ffffffe	/* Version needs section.  */
// #define SHT_GNU_versym	  0x6fffffff	/* Version symbol table.  */
// #define SHT_HISUNW	  0x6fffffff	/* Sun-specific high bound.  */
// #define SHT_HIOS	  0x6fffffff	/* End OS-specific type */
// #define SHT_LOPROC	  0x70000000	/* Start of processor-specific */
// #define SHT_HIPROC	  0x7fffffff	/* End of processor-specific */
// #define SHT_LOUSER	  0x80000000	/* Start of application-specific */
// #define SHT_HIUSER	  0x8fffffff	/* End of application-specific */
    int ii = 0;
    for(int i=0; i<eh->e_shnum; i++) {
        printf("\n[");
        switch(sh_table[i].sh_type) {
            case SHT_NULL:
                printf("NULL                     (Section header table entry unused)                   ");
                break;
            case SHT_PROGBITS:
                printf("PROGBITS                 (Program data)                                        ");
                break;
            case SHT_SYMTAB: 
                printf("SYMTAB                   (Symbol table)                                        ");
                break;
            case SHT_STRTAB:
                printf("STRTAB                   (String table)                                        ");
                break;
            case SHT_RELA:
                printf("RELA                     (Relocation entries with addends)                     ");
                break;
            case SHT_HASH:
                printf("HASH                     (Symbol hash table)                                   ");
                break;
            case SHT_DYNAMIC:
                printf("DYNAMIC                  (Dynamic linking information)                         ");
                break;
            case SHT_NOTE:
                printf("NOTE                     (Notes)                                               ");
                break;
            case SHT_NOBITS:
                printf("NOBITS                   (Program space with no data (bss))                    ");
                break;
            case SHT_REL:
                printf("REL                      (Relocation entries, no addends)                      ");
                break;
            case SHT_SHLIB:
                printf("SHLIB                    (Reserved)                                            ");
                break;
            case SHT_DYNSYM:
                printf("DYNSYM                   (Dynamic linker symbol table)                         ");
                break;
            case SHT_INIT_ARRAY:
                printf("INIT_ARRAY               (Array of constructors)                               ");
                break;
            case SHT_FINI_ARRAY:
                printf("FINI_ARRAY               (Array of destructors)                                ");
                break;
            case SHT_PREINIT_ARRAY:
                printf("PREINIT_ARRAY            (Array of pre-constructors)                           ");
                break;
            case SHT_GROUP:
                printf("GROUP                    (Section group)                                       ");
                break;
            case SHT_SYMTAB_SHNDX:
                printf("SYMTAB_SHNDX             (Extended section indeces)                            ");
                break;
            case SHT_NUM:
                printf("NUM                      (Number of defined types)                             ");
                break;
            case SHT_LOOS:
                printf("LOOS                     (Start OS-specific)                                   ");
                break;
            case SHT_GNU_ATTRIBUTES:
                printf("GNU_ATTRIBUTES           (Object attributes)                                   ");
                break;
            case SHT_GNU_HASH:
                printf("GNU_HASH                 (GNU-style hash table)                                ");
                break;
            case SHT_GNU_LIBLIST:
                printf("GNU_LIBLIST              (Prelink library list)                                ");
                break;
            case SHT_CHECKSUM:
                printf("CHECKSUM                 (Checksum for DSO content)                            ");
                break;
            case SHT_LOSUNW:
                printf("LOSUNW or SUNW_move                                                            ");
                break;
            case SHT_SUNW_COMDAT:
                printf("SUNW_COMDAT                                                                    ");
                break;
            case SHT_SUNW_syminfo:
                printf("SUNW_syminfo                                                                   ");
                break;
            case SHT_GNU_verdef:
                printf("GNU_verdef               (Version definition section)                          ");
                break;
            case SHT_GNU_verneed:
                printf("GNU_verneed              (Version needs section)                               ");
                break;
            case SHT_GNU_versym:
                printf("GNU_versym               (Version symbol table) or HISUNW (Sun-specific high bound) or HIOS (End OS-specific type) ");
                break;
            case SHT_LOPROC:
                printf("LOPROC                   (Start of processor-specific)                         ");
                break;
            case SHT_HIPROC:
                printf("HIPROC                   (End of processor-specific)                           ");
                break;
            case SHT_LOUSER:
                printf("LOUSER                   (Start of application-specific)                       ");
                break;
            case SHT_HIUSER:
                printf("HIUSER                   (End of application-specific)                         ");
                break;
            default:
                printf("UNKNOWN                                                                        ");
        }
        printf("Section %d, Index %d]\n", ii, i);
        print_elf_symbol_table(arrayd, eh, sh_table, i);
        ii++;
    }
}

char * symbol_lookup(char * arrayc, Elf64_Shdr sh_table[], uint64_t symbol_table, int index, int mode) {
    printf("looking up index %d of table %d\n", index, symbol_table);
    Elf64_Sym* sym_tbl = (Elf64_Sym*)read_section_(arrayc, sh_table[symbol_table]);
    uint64_t str_tbl_ndx = sh_table[symbol_table].sh_link;
    char *str_tbl = read_section_(arrayc, sh_table[str_tbl_ndx]);
    uint64_t symbol_count = (sh_table[symbol_table].sh_size/sizeof(Elf64_Sym));
    printf("requested symbol name for index %d is %s\n", index, demangle_it(str_tbl + sym_tbl[index].st_name));
    if ( mode == 1) return sym_tbl[index].st_value;
    else if (mode == 2) return sym_tbl[index].st_size;
}

char * symbol_lookup_name(char * arrayc, Elf64_Shdr sh_table[], uint64_t symbol_table, char * name_) {
    char *str_tbl;
    Elf64_Sym* sym_tbl;
    uint64_t i, symbol_count;
    sym_tbl = (Elf64_Sym*)read_section_(arrayc, sh_table[symbol_table]);

    uint64_t str_tbl_ndx = sh_table[symbol_table].sh_link;
    str_tbl = read_section_(arrayc, sh_table[str_tbl_ndx]);

    symbol_count = (sh_table[symbol_table].sh_size/sizeof(Elf64_Sym));

    for(int i=0; i< symbol_count; i++) {
        char * name = demangle_it(str_tbl + sym_tbl[i].st_name);
        if (bytecmpq(name,name_) == 0) {
            char * address = sym_tbl[i].st_value+mappingb;
            printf("requested symbol name \"%s\" found in table %d at address %014p is \"%s\"\n", name_, symbol_table, address, name);
            return address;
        }
    }
    printf("\nrequested symbol name \"%s\" could not be found in table %d\n\n", name_, symbol_table);
    return NULL;
}

char * print_elf_symbol_table_lookup(char * arrayc, Elf64_Ehdr * eh, Elf64_Shdr sh_table[], uint64_t symbol_table, int index, int mode)
{
        switch(sh_table[symbol_table].sh_type) {
            case SHT_DYNSYM:
                return symbol_lookup(arrayc, sh_table, symbol_table, index, mode);
                break;
            default:
                return (int) -1;
                break;
        }
}

char * print_elf_symbol_table_lookup_name(char * arrayc, Elf64_Ehdr * eh, Elf64_Shdr sh_table[], uint64_t symbol_table, char * index)
{
        char * name_;
        switch(sh_table[symbol_table].sh_type) {
            case SHT_DYNSYM:
                name_ = symbol_lookup_name(arrayc, sh_table, symbol_table, index);
                if (name_ != NULL) {
                    return name_;
                }
                else {
                    return NULL;
                }
                break;
            case SHT_SYMTAB:
                name_ = symbol_lookup_name(arrayc, sh_table, symbol_table, index);
                if (name_ != NULL) {
                    return name_;
                }
                else {
                    return NULL;
                }
                break;
            default:
                return NULL;
                break;
        }
}

char * print_symbols_lookup(char * arrayd, Elf64_Ehdr * eh, Elf64_Shdr sh_table[], int index, int mode)
{
    for(int i=0; i<eh->e_shnum; i++) {
        int value = print_elf_symbol_table_lookup(arrayd, eh, sh_table, i, index, mode);
        if ( value != -1 ) return value;
    }
}

char * print_symbols_lookup_name(char * arrayd, Elf64_Ehdr * eh, Elf64_Shdr sh_table[], char * index)
{
    char * value;
    for(int i=0; i<eh->e_shnum; i++) {
        value = print_elf_symbol_table_lookup_name(arrayd, eh, sh_table, i, index);
        if ( value != NULL ) {
            return value;
        }
    }
    if (value == NULL) return NULL;

}

void * lookup_symbol_by_name(const char * arrayb, Elf64_Ehdr * eh, char * name) {

        read_section_header_table_(arrayb, eh, &_elf_symbol_table);
        char * symbol = print_symbols_lookup_name(arrayb, eh, _elf_symbol_table, name);
        return symbol;
}

void * lookup_symbol_by_name_(const char * lib, const char * name) {
        init_(lib);
        const char * arrayb = array;
        Elf64_Ehdr * eh = (Elf64_Ehdr *) arrayb;
        Elf64_Shdr *_elf_symbol_tableb;
        if(!strncmp((char*)eh->e_ident, "\177ELF", 4)) {
            if ( read_section_header_table_(arrayb, eh, &_elf_symbol_tableb) == 0) {
                char * symbol = print_symbols_lookup_name(arrayb, eh, _elf_symbol_tableb, name);
                return symbol;
            }
        }
        else abort_();
}

void * lookup_symbol_by_index(const char * arrayb, Elf64_Ehdr * eh, int symbol_index, int mode) {
        printf("attempting to look up symbol, index = %d\n", symbol_index);

        read_section_header_table_(arrayb, eh, &_elf_symbol_table);
        nl();
        nl();
        char * symbol = print_symbols_lookup(arrayb, eh, _elf_symbol_table, symbol_index, mode);
        printf("symbol = %d (%014p)\n", symbol, symbol);
        return symbol;
}

ElfW(Word)
get_dynamic_entry(ElfW(Dyn) *dynamic, int field)
{
    printf("called get_dynamic_entry\n");

// Name        Value       d_un        Executable      Shared Object
// DT_NULL     0           ignored     mandatory       mandatory
// DT_NEEDED   1           d_val       optional        optional
// DT_PLTRELSZ 2           d_val       optional        optional
// DT_PLTGOT   3           d_ptr       optional        optional
// DT_HASH     4           d_ptr       mandatory       mandatory
// DT_STRTAB   5           d_ptr       mandatory       mandatory
// DT_SYMTAB   6           d_ptr       mandatory       mandatory
// DT_RELA     7           d_ptr       mandatory       optional
// DT_RELASZ   8           d_val       mandatory       optional
// DT_RELAENT  9           d_val       mandatory       optional
// DT_STRSZ    10          d_val       mandatory       mandatory
// DT_SYMENT   11          d_val       mandatory       mandatory
// DT_INIT     12          d_ptr       optional        optional
// DT_FINI     13          d_ptr       optional        optional
// DT_SONAME   14          d_val       ignored         optional
// DT_RPATH    15          d_val       optional        ignored
// DT_SYMBOLIC 16          ignored     ignored         optional
// DT_REL      17          d_ptr       mandatory       optional
// DT_RELSZ    18          d_val       mandatory       optional
// DT_RELENT   19          d_val       mandatory       optional
// DT_PLTREL   20          d_val       optional        optional
// DT_DEBUG    21          d_ptr       optional        ignored
// DT_TEXTREL  22          ignored     optional        optional
// DT_JMPREL   23          d_ptr       optional        optional
// DT_BIND_NOW 24          ignored     optional        optional
// DT_LOPROC   0x70000000  unspecified unspecified     unspecified
// DT_HIPROC   0x7fffffff  unspecified unspecified     unspecified
// 
// DT_NULL         An entry with a DT_NULL tag marks the end of the _DYNAMIC array.
// DT_NEEDED       This element holds the string table offset of a null-terminated string, giving
//                 the name of a needed library. The offset is an index into the table recorded
//                 in the DT_STRTAB entry. See "Shared Object Dependencies'' for more
//                 information about these names. The dynamic array may contain multiple
//                 entries with this type. These entries' relative order is significant, though
//                 their relation to entries of other types is not.
// 
// DT_PLTRELSZ     This element holds the total size, in bytes, of the relocation entries
//                 associated with the procedure linkage table. If an entry of type DT_JMPREL
//                 is present, a DT_PLTRELSZ must accompany it.
// 
// DT_PLTGOT       This element holds an address associated with the procedure linkage table
//                 and/or the global offset table.
// 
// DT_HASH         This element holds the address of the symbol hash table, described in "Hash
//                 Table". This hash table refers to the symbol table referenced by the
//                 DT_SYMTAB element.
// 
// DT_STRTAB       This element holds the address of the string table, described in Chapter 1.
//                 Symbol names, library names, and other strings reside in this table.
// 
// DT_SYMTAB       This element holds the address of the symbol table, described in
//                 Chapter 1, with Elf32_Sym entries for the 32-bit class of files.
// 
// DT_RELA         This element holds the address of a relocation table, described in
//                 Chapter 1. Entries in the table have explicit addends, such as Elf32_Rela
//                 for the 32-bit file class. An object file may have multiple relocation
//                 sections. When building the relocation table for an executable or shared
//                 object file, the link editor catenates those sections to form a single table.
//                 Although the sections remain independent in the object file, the dynamic
//                 linker sees a single table. When the dynamic linker creates the process
//                 image for an executable file or adds a shared object to the process image,
//                 it reads the relocation table and performs the associated actions. If this
//                 element is present, the dynamic structure must also have DT_RELASZ and
//                 DT_RELAENT elements. When relocation is "mandatory" for a file, either
//                 DT_RELA or DT_REL may occur (both are permitted but not required).
// 
// DT_RELASZ       This element holds the total size, in bytes, of the DT_RELA relocation table.
// 
// DT_RELAENT      This element holds the size, in bytes, of the DT_RELA relocation entry.
// 
// DT_STRSZ        This element holds the size, in bytes, of the string table.
// 
// DT_SYMENT       This element holds the size, in bytes, of a symbol table entry.
// 
// DT_INIT         This element holds the address of the initialization function, discussed in
//                 "Initialization and Termination Functions" below.
// 
// DT_FINI         This element holds the address of the termination function, discussed in
//                 "Initialization and Termination Functions" below.
// 
// DT_SONAME       This element holds the string table offset of a null-terminated string, giving
//                 the name of the shared object. The offset is an index into the table recorded
//                 in the DT_STRTAB entry. See "Shared Object Dependencies" below for
//                 more information about these names.
// 
// DT_RPATH        This element holds the string table offset of a null-terminated search library
//                 search path string, discussed in "Shared Object Dependencies". The offset
//                 is an index into the table recorded in the DT_STRTAB entry.
// 
// DT_SYMBOLIC     This element's presence in a shared object library alters the dynamic linker's
//                 symbol resolution algorithm for references within the library. Instead of
//                 starting a symbol search with the executable file, the dynamic linker starts
//                 from the shared object itself. If the shared object fails to supply the
//                 referenced symbol, the dynamic linker then searches the executable file and
//                 other shared objects as usual.
// 
// DT_REL          This element is similar to DT_RELA, except its table has implicit addends,
//                 such as Elf32_Rel for the 32-bit file class. If this element is present, the
//                 dynamic structure must also have DT_RELSZ and DT_RELENT elements.
// 
// DT_RELSZ        This element holds the total size, in bytes, of the DT_REL relocation table.
// 
// DT_RELENT       This element holds the size, in bytes, of the DT_REL relocation entry.
// 
// DT_PLTREL       This member specifies the type of relocation entry to which the procedure
//                 linkage table refers. The d_val member holds DT_REL or DT_RELA , as
//                 appropriate. All relocations in a procedure linkage table must use the same
//                 relocation.
// 
// DT_DEBUG        This member is used for debugging. Its contents are not specified in this
//                 document.
// 
// DT_TEXTREL      This member's absence signifies that no relocation entry should cause a
//                 modification to a non-writable segment, as specified by the segment
//                 permissions in the program header table. If this member is present, one or
//                 more relocation entries might request modifications to a non-writable
//                 segment, and the dynamic linker can prepare accordingly.
// 
// DT_JMPREL       If present, this entries d_ptr member holds the address of relocation
//                 entries associated solely with the procedure linkage table. Separating these
//                 relocation entries lets the dynamic linker ignore them during process
//                 initialization, if lazy binding is enabled. If this entry is present, the related
//                 entries of types DT_PLTRELSZ and DT_PLTREL must also be present.
// 
// DT_BIND_NOW     If present in a shared object or executable, this entry instructs the dynamic
//                 linker to process all relocations for the object containing this entry before
//                 transferring control to the program. The presence of this entry takes
//                 precedence over a directive to use lazy binding for this object when
//                 specified through the environment or via dlopen( BA_LIB).
// 
// DT_LOPROC through DT_HIPROC
//                 Values in this inclusive range are reserved for processor-specific semantics.
//                 If meanings are specified, the processor supplement explains them.
// 
// Except for the DT_NULL element at the end of the array, and the relative order of DT_NEEDED
// elements, entries may appear in any order. Tag values not appearing in the table are reserved.


    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        printf("testing if ");
/* Legal values for d_tag (dynamic entry type).  */

// #define DT_NULL		0		/* Marks end of dynamic section */
// #define DT_NEEDED	1		/* Name of needed library */
// #define DT_PLTRELSZ	2		/* Size in bytes of PLT relocs */
// #define DT_PLTGOT	3		/* Processor defined value */
// #define DT_HASH		4		/* Address of symbol hash table */
// #define DT_STRTAB	5		/* Address of string table */
// #define DT_SYMTAB	6		/* Address of symbol table */
// #define DT_RELA		7		/* Address of Rela relocs */
// #define DT_RELASZ	8		/* Total size of Rela relocs */
// #define DT_RELAENT	9		/* Size of one Rela reloc */
// #define DT_STRSZ	10		/* Size of string table */
// #define DT_SYMENT	11		/* Size of one symbol table entry */
// #define DT_INIT		12		/* Address of init function */
// #define DT_FINI		13		/* Address of termination function */
// #define DT_SONAME	14		/* Name of shared object */
// #define DT_RPATH	15		/* Library search path (deprecated) */
// #define DT_SYMBOLIC	16		/* Start symbol search here */
// #define DT_REL		17		/* Address of Rel relocs */
// #define DT_RELSZ	18		/* Total size of Rel relocs */
// #define DT_RELENT	19		/* Size of one Rel reloc */
// #define DT_PLTREL	20		/* Type of reloc in PLT */
// #define DT_DEBUG	21		/* For debugging; unspecified */
// #define DT_TEXTREL	22		/* Reloc might modify .text */
// #define DT_JMPREL	23		/* Address of PLT relocs */
// #define	DT_BIND_NOW	24		/* Process relocations of object */
// #define	DT_INIT_ARRAY	25		/* Array with addresses of init fct */
// #define	DT_FINI_ARRAY	26		/* Array with addresses of fini fct */
// #define	DT_INIT_ARRAYSZ	27		/* Size in bytes of DT_INIT_ARRAY */
// #define	DT_FINI_ARRAYSZ	28		/* Size in bytes of DT_FINI_ARRAY */
// #define DT_RUNPATH	29		/* Library search path */
// #define DT_FLAGS	30		/* Flags for the object being loaded */
// #define DT_ENCODING	32		/* Start of encoded range */
// #define DT_PREINIT_ARRAY 32		/* Array with addresses of preinit fct*/
// #define DT_PREINIT_ARRAYSZ 33		/* size in bytes of DT_PREINIT_ARRAY */
// #define	DT_NUM		34		/* Number used */
// #define DT_LOOS		0x6000000d	/* Start of OS-specific */
// #define DT_HIOS		0x6ffff000	/* End of OS-specific */
// #define DT_LOPROC	0x70000000	/* Start of processor-specific */
// #define DT_HIPROC	0x7fffffff	/* End of processor-specific */
// #define	DT_PROCNUM	DT_MIPS_NUM	/* Most used by any processor */
        switch (dynamic->d_tag) {
            case DT_NULL:
                printf("DT_NULL");
                break;
            case DT_NEEDED:
                printf("DT_NEEDED");
                break;
            case DT_PLTRELSZ:
                printf("DT_PLTRELSZ");
                break;
            case DT_PLTGOT:
                printf("DT_PLTGOT");
                break;
            case DT_HASH:
                printf("DT_HASH");
                break;
            case DT_STRTAB:
                printf("DT_STRTAB");
                break;
            case DT_SYMTAB:
                printf("DT_SYMTAB");
                break;
            case DT_RELA:
                printf("DT_RELA");
                break;
            case DT_RELASZ:
                printf("DT_RELASZ");
                break;
            case DT_RELAENT:
                printf("DT_RELAENT");
                break;
            case DT_STRSZ:
                printf("DT_STRSZ");
                break;
            case DT_SYMENT:
                printf("DT_SYMENT");
                break;
            case DT_INIT:
                printf("DT_INIT");
                break;
            case DT_FINI:
                printf("DT_FINI");
                break;
            case DT_SONAME:
                printf("DT_SONAME");
                break;
            case DT_RPATH:
                printf("DT_RPATH");
                break;
            case DT_SYMBOLIC:
                printf("DT_SYMBOLIC");
                break;
            case DT_REL:
                printf("DT_REL");
                break;
            case DT_RELSZ:
                printf("DT_RELSZ");
                break;
            case DT_RELENT:
                printf("DT_RELENT");
                break;
            case DT_PLTREL:
                printf("DT_PLTREL");
                break;
            case DT_DEBUG:
                printf("DT_DEBUG");
                break;
            case DT_TEXTREL:
                printf("DT_TEXTREL");
                break;
            case DT_JMPREL:
                printf("DT_JMPREL");
                break;
            case DT_BIND_NOW:
                printf("DT_BIND_NOW");
                break;
            case DT_INIT_ARRAY:
                printf("DT_INIT_ARRAY");
                break;
            case DT_FINI_ARRAY:
                printf("DT_FINI_ARRAY");
                break;
            case DT_INIT_ARRAYSZ:
                printf("DT_INIT_ARRAYSZ");
                break;
            case DT_FINI_ARRAYSZ:
                printf("DT_FINI_ARRAYSZ");
                break;
            case DT_RUNPATH:
                printf("DT_RUNPATH");
                break;
            case DT_FLAGS:
                printf("DT_FLAGS");
                break;
            case DT_ENCODING:
                printf("DT_ENCODING (or DT_PREINIT_ARRAY)");
                break;
            case DT_PREINIT_ARRAYSZ:
                printf("DT_PREINIT_ARRAYSZ");
                break;
            case DT_NUM:
                printf("DT_NUM");
                break;
            case DT_LOOS:
                printf("DT_LOOS");
                break;
            case DT_HIOS:
                printf("DT_HIOS");
                break;
            case DT_LOPROC:
                printf("DT_LOPROC");
                break;
            case DT_HIPROC:
                printf("DT_HIPROC (or DT_FILTER)");
                break;
            case DT_PROCNUM:
                printf("DT_PROCNUM");
                break;
            case DT_VERSYM:
                printf("DT_VERSYM");
                break;
            case DT_RELACOUNT:
                printf("DT_RELACOUNT");
                break;
            case DT_RELCOUNT:
                printf("DT_RELCOUNT");
                break;
            case DT_FLAGS_1:
                printf("DT_FLAGS_1");
                break;
            case DT_VERDEF:
                printf("DT_VERDEF");
                break;
            case DT_VERDEFNUM:
                printf("DT_VERDEFNUM");
                break;
            case DT_VERNEED:
                printf("DT_VERNEED");
                break;
            case DT_VERNEEDNUM:
                printf("DT_VERNEEDNUM");
                break;
            case DT_AUXILIARY:
                printf("DT_AUXILIARY");
                break;
            default:
                printf("%d", dynamic->d_tag);
                break;
        }
        printf(" == ");
        switch (field) {
            case DT_NULL:
                printf("DT_NULL");
                break;
            case DT_NEEDED:
                printf("DT_NEEDED");
                break;
            case DT_PLTRELSZ:
                printf("DT_PLTRELSZ");
                break;
            case DT_PLTGOT:
                printf("DT_PLTGOT");
                break;
            case DT_HASH:
                printf("DT_HASH");
                break;
            case DT_STRTAB:
                printf("DT_STRTAB");
                break;
            case DT_SYMTAB:
                printf("DT_SYMTAB");
                break;
            case DT_RELA:
                printf("DT_RELA");
                break;
            case DT_RELASZ:
                printf("DT_RELASZ");
                break;
            case DT_RELAENT:
                printf("DT_RELAENT");
                break;
            case DT_STRSZ:
                printf("DT_STRSZ");
                break;
            case DT_SYMENT:
                printf("DT_SYMENT");
                break;
            case DT_INIT:
                printf("DT_INIT");
                break;
            case DT_FINI:
                printf("DT_FINI");
                break;
            case DT_SONAME:
                printf("DT_SONAME");
                break;
            case DT_RPATH:
                printf("DT_RPATH");
                break;
            case DT_SYMBOLIC:
                printf("DT_SYMBOLIC");
                break;
            case DT_REL:
                printf("DT_REL");
                break;
            case DT_RELSZ:
                printf("DT_RELSZ");
                break;
            case DT_RELENT:
                printf("DT_RELENT");
                break;
            case DT_PLTREL:
                printf("DT_PLTREL");
                break;
            case DT_DEBUG:
                printf("DT_DEBUG");
                break;
            case DT_TEXTREL:
                printf("DT_TEXTREL");
                break;
            case DT_JMPREL:
                printf("DT_JMPREL");
                break;
            case DT_BIND_NOW:
                printf("DT_BIND_NOW");
                break;
            case DT_INIT_ARRAY:
                printf("DT_INIT_ARRAY");
                break;
            case DT_FINI_ARRAY:
                printf("DT_FINI_ARRAY");
                break;
            case DT_INIT_ARRAYSZ:
                printf("DT_INIT_ARRAYSZ");
                break;
            case DT_FINI_ARRAYSZ:
                printf("DT_FINI_ARRAYSZ");
                break;
            case DT_RUNPATH:
                printf("DT_RUNPATH");
                break;
            case DT_FLAGS:
                printf("DT_FLAGS");
                break;
            case DT_ENCODING:
                printf("DT_ENCODING (or DT_PREINIT_ARRAY)");
                break;
            case DT_PREINIT_ARRAYSZ:
                printf("DT_PREINIT_ARRAYSZ");
                break;
            case DT_NUM:
                printf("DT_NUM");
                break;
            case DT_LOOS:
                printf("DT_LOOS");
                break;
            case DT_HIOS:
                printf("DT_HIOS");
                break;
            case DT_LOPROC:
                printf("DT_LOPROC");
                break;
            case DT_HIPROC:
                printf("DT_HIPROC (or DT_FILTER)");
                break;
            case DT_PROCNUM:
                printf("DT_PROCNUM");
                break;
            case DT_VERSYM:
                printf("DT_VERSYM");
                break;
            case DT_RELACOUNT:
                printf("DT_RELACOUNT");
                break;
            case DT_RELCOUNT:
                printf("DT_RELCOUNT");
                break;
            case DT_FLAGS_1:
                printf("DT_FLAGS_1");
                break;
            case DT_VERDEF:
                printf("DT_VERDEF");
                break;
            case DT_VERDEFNUM:
                printf("DT_VERDEFNUM");
                break;
            case DT_VERNEED:
                printf("DT_VERNEED");
                break;
            case DT_VERNEEDNUM:
                printf("DT_VERNEEDNUM");
                break;
            case DT_AUXILIARY:
                printf("DT_AUXILIARY");
                break;
            default:
                printf("%d (unknown)", field);
                break;
        }
        printf("\n");
        if (dynamic->d_tag == field) {
            printf("returning %014p\n", dynamic->d_un.d_val);
            return dynamic->d_un.d_val;
        }
    }
    printf("returning 0\n");
    return 0;
}

r(Elf64_Rela *relocs, size_t relocs_size) {
/*

Relocation
Relocation Types
Relocation entries describe how to alter the following instruction and data fields (bit numbers
appear in the lower box corners).


        word32
31                  0

word32      This specifies a 32-bit field occupying 4 bytes with arbitrary byte alignment. These
            values use the same byte order as other word values in the Intel architecture.

                         3    2    1    0
0x01020304           01   02   03   04
                 31                     0

Calculations below assume the actions are transforming a relocatable file into either an
executable or a shared object file. Conceptually, the link editor merges one or more relocatable
files to form the output. It first decides how to combine and locate the input files, then updates
the symbol values, and finally performs the relocation. Relocations applied to executable or
shared object files are similar and accomplish the same result. Descriptions below use the
following notation.

A       This means the addend used to compute the value of the relocatable field.

B       This means the base address at which a shared object has been loaded into memory
        during execution. Generally, a shared object file is built with a 0 base virtual address,
        but the execution address will be different.

G       This means the offset into the global offset table at which the address of the
        relocation entry's symbol will reside during execution. See "Global Offset Table''
        below for more information.

GOT     This means the address of the global offset table. See "Global Offset Table'' below
        for more information.

L       This means the place (section offset or address) of the procedure linkage table entry
        for a symbol. A procedure linkage table entry redirects a function call to the proper
        destination. The link editor builds the initial procedure linkage table, and the
        dynamic linker modifies the entries during execution. See "Procedure Linkage
        Table'' below for more information.

P       This means the place (section offset or address) of the storage unit being relocated
        (computed using r_offset ).
        
S       This means the value of the symbol whose index resides in the relocation entry.

A relocation entry's r_offset value designates the offset or virtual address of the first byte
of the affected storage unit. The relocation type specifies which bits to change and how to
calculate their values. The Intel architecture uses only Elf32_Rel relocation entries, the field
to be relocated holds the addend. In all cases, the addend and the computed result use the same
byte order.

Name                Value       Field       Calculation
R_386_NONE          0           none        none
R_386_32            1           word32      S + A
R_386_PC32          2           word32      S + A - P
R_386_GOT32         3           word32      G + A
R_386_PLT32         4           word32      L + A - P
R_386_COPY          5           none        none
R_386_GLOB_DAT      6           word32      S
R_386_JMP_SLOT      7           word32      S
R_386_RELATIVE      8           word32      B + A
R_386_GOTOFF        9           word32      S + A - GOT
R_386_GOTPC         10          word32      GOT + A - P

Some relocation types have semantics beyond simple calculation.

R_386_GLOB_DAT      This relocation type is used to set a global offset table entry to the address
                    of the specified symbol. The special relocation type allows one to determine
                    the correspondence between symbols and global offset table entries.

R_386_JMP_SLOT      The link editor creates this relocation type for dynamic linking. Its offset
                    member gives the location of a procedure linkage table entry. The dynamic
                    linker modifies the procedure linkage table entry to transfer control to the
                    designated symbol's address [see "Procedure Linkage Table'' below].

R_386_RELATIVE      The link editor creates this relocation type for dynamic linking. Its offset
                    member gives a location within a shared object that contains a value
                    representing a relative address. The dynamic linker computes the
                    corresponding virtual address by adding the virtual address at which the
                    shared object was loaded to the relative address. Relocation entries for this
                    type must specify 0 for the symbol table index.

R_386_GOTOFF        This relocation type computes the difference between a symbol's value and
                    the address of the global offset table. It additionally instructs the link editor
                    to build the global offset table.
                    
R_386_GOTPC         This relocation type resembles R_386_PC32, except it uses the address
                    of the global offset table in its calculation. The symbol referenced in this
                    relocation normally is _GLOBAL_OFFSET_TABLE_, which additionally
                    instructs the link editor to build the global offset table.

*/

/*

Relocation
Relocation Types
Relocation entries describe how to alter the following instruction and data fields (bit numbers
appear in the lower box corners).

              word8  
            7       0
            
                  word16
            15              0
            
                          word32
            31                              0

                                          word64
            63                                                              0

word8       This specifies a 8-bit field occupying 1 byte.

word16      This specifies a 16-bit field occupying 2 bytes with arbitrary
            byte alignment. These values use the same byte order as
            other word values in the AMD64 architecture.

word32      This specifies a 32-bit field occupying 4 bytes with arbitrary
            byte alignment. These values use the same byte order as
            other word values in the AMD64 architecture.

word64      This specifies a 64-bit field occupying 8 bytes with arbitrary
            byte alignment. These values use the same byte order as
            other word values in the AMD64 architecture.

wordclass   This specifies word64 for LP64 and specifies word32 for
            ILP32.

Calculations below assume the actions are transforming a relocatable file into either an
executable or a shared object file. Conceptually, the link editor merges one or more relocatable
files to form the output. It first decides how to combine and locate the input files, then updates
the symbol values, and finally performs the relocation. Relocations applied to executable or
shared object files are similar and accomplish the same result. Descriptions below use the
following notation.

A           Represents the addend used to compute the value of the relocatable field.

B           Represents the base address at which a shared object has been loaded into
            memory during execution. Generally, a shared object is built with a 0 base
            virtual address, but the execution address will be different.

G           Represents the offset into the global offset table at which the relocation
            entry’s symbol will reside during execution.

GOT         Represents the address of the global offset table.

L           Represents the place (section offset or address) of the Procedure Linkage Table
            entry for a symbol.

P           Represents the place (section offset or address) of the storage unit being
            relocated (computed using r_offset).

S           Represents the value of the symbol whose index resides in the relocation entry.

Z           Represents the size of the symbol whose index resides in the relocation entry.

A relocation entry's r_offset value designates the offset or virtual address of the first byte
of the affected storage unit. The relocation type specifies which bits to change and how to
calculate their values. The Intel architecture uses only Elf32_Rel relocation entries, the field
to be relocated holds the addend. In all cases, the addend and the computed result use the same
byte order.

The AMD64 LP64 ABI architecture uses only Elf64_Rela relocation entries with explicit addends.
The r_addend member serves as the relocation addend.

The AMD64 ILP32 ABI architecture uses only Elf32_Rela relocation entries in relocatable files.
Executable files or shared objects may use either Elf32_Rela or Elf32_Rel relocation entries.

Name                        Value       Field       Calculation
R_X86_64_NONE               0           none        none
R_X86_64_64                 1           word64      S + A
R_X86_64_PC32               2           word32      S + A - P
R_X86_64_GOT32              3           word32      G + A
R_X86_64_PLT32              4           word32      L + A - P
R_X86_64_COPY               5           none        none
R_X86_64_GLOB_DAT           6           wordclass   S
R_X86_64_JUMP_SLOT          7           wordclass   S
R_X86_64_RELATIVE           8           wordclass   B + A
R_X86_64_GOTPCREL           9           word32      G + GOT + A - P
R_X86_64_32                 10          word32      S + A
R_X86_64_32S                11          word32      S + A
R_X86_64_16                 12          word16      S + A
R_X86_64_PC16               13          word16      S + A - P
R_X86_64_8                  14          word8       S + A
R_X86_64_PC8                15          word8       S + A - P
R_X86_64_DTPMOD64           16          word64      none
R_X86_64_DTPOFF64           17          word64      none
R_X86_64_TPOFF64            18          word64      none
R_X86_64_TLSGD              19          word32      none
R_X86_64_TLSLD              20          word32      none
R_X86_64_DTPOFF32           21          word32      none
R_X86_64_GOTTPOFF           22          word32      none
R_X86_64_TPOFF32            23          word32      none                †
R_X86_64_PC64               24          word64      S + A - P           †
R_X86_64_GOTOFF64           25          word64      S + A - GOT
R_X86_64_GOTPC32            26          word32      GOT + A - P
R_X86_64_GOT64              27          word64      G + A
R_X86_64_GOTPCREL64         28          word64      G + GOT - P + A
R_X86_64_GOTPC64            29          word64      GOT - P + A
Deprecated                  30          none        none
R_X86_64_PLTOFF64           31          word64      L - GOT + A
R_X86_64_SIZE32             32          word32      Z + A               †
R_X86_64_SIZE64             33          word64      Z + A
R_X86_64_GOTPC32_TLSDESC    34          word32      none
R_X86_64_TLSDESC_CALL       35          none        none
R_X86_64_TLSDESC            36          word64×2    none
R_X86_64_IRELATIVE          37          wordclass   indirect (B + A)    ††
R_X86_64_RELATIVE64         38          word64      B + A
Deprecated                  39          none        none
Deprecated                  40          none        none
R_X86_64_GOTPCRELX          41          word32      G + GOT + A - P
R_X86_64_REX_GOTPCRELX      42          word32      G + GOT + A - P

†   This relocation is used only for LP64.
††  This relocation only appears in ILP32 executable files or shared objects.

Some relocation types have semantics beyond simple calculation.

R_386_GLOB_DAT      This relocation type is used to set a global offset table entry to the address
                    of the specified symbol. The special relocation type allows one to determine
                    the correspondence between symbols and global offset table entries.

R_386_JMP_SLOT      The link editor creates this relocation type for dynamic linking. Its offset
                    member gives the location of a procedure linkage table entry. The dynamic
                    linker modifies the procedure linkage table entry to transfer control to the
                    designated symbol's address [see "Procedure Linkage Table'' below].

R_386_RELATIVE      The link editor creates this relocation type for dynamic linking. Its offset
                    member gives a location within a shared object that contains a value
                    representing a relative address. The dynamic linker computes the
                    corresponding virtual address by adding the virtual address at which the
                    shared object was loaded to the relative address. Relocation entries for this
                    type must specify 0 for the symbol table index.

R_386_GOTOFF        This relocation type computes the difference between a symbol's value and
                    the address of the global offset table. It additionally instructs the link editor
                    to build the global offset table.
                    
R_386_GOTPC         This relocation type resembles R_386_PC32, except it uses the address
                    of the global offset table in its calculation. The symbol referenced in this
                    relocation normally is _GLOBAL_OFFSET_TABLE_, which additionally
                    instructs the link editor to build the global offset table.

*/
    if (relocs != mappingb && relocs_size != 0) {
        for (int i = 0; i < relocs_size  / sizeof(Elf64_Rela); i++) {
            Elf64_Rela *reloc = &relocs[i];
            int reloc_type = ELF64_R_TYPE(reloc->r_info);
            printf("i = %d,\t\tELF64_R_TYPE(reloc->r_info)\t= ", i);
            switch (reloc_type) {
                #if defined(__x86_64__)
                    
                case R_X86_64_NONE:
                {
                    printf("\n\n\nR_X86_64_NONE                calculation: none\n");
                    break;
                }
                case R_X86_64_64:
                {
                    printf("\n\n\nR_X86_64_64                  calculation: S + A (symbol value + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_PC32:
                {
                    printf("\n\n\nR_X86_64_PC32                calculation: S + A - P (symbol value + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_GOT32:
                {
                    printf("\n\n\nR_X86_64_GOT32               calculation: G + A (address of global offset table + r_addend)\n");
                    break;
                }
                case R_X86_64_PLT32:
                {
                    printf("\n\n\nR_X86_64_PLT32               calculation: L + A - P ((L: This means the place (section offset or address) of the procedure linkage table entry for a symbol) + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).) \n");
                    break;
                }
                case R_X86_64_COPY:
                {
                    printf("\n\n\nR_X86_64_COPY                calculation: none\n");
                    break;
                }
                case R_X86_64_GLOB_DAT:
                {
                    printf("\n\n\nR_X86_64_GLOB_DAT            calculation: S (symbol value)\n");
                    printf("reloc->r_offset = %014p+%014p=%014p\n", mappingb, reloc->r_offset, mappingb+reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S)+mappingb;
                    char ** addr = reloc->r_offset + mappingb;
                    printf("%014p = %014p\n", addr, *addr);
                    break;
                }
                case R_X86_64_JUMP_SLOT:
                {
                    printf("\n\n\nR_X86_64_JUMP_SLOT           calculation: S (symbol value)\n");
                    printf("mappingb    = %014p\n", mappingb);
                    printf("reloc->r_offset = %014p+%014p=%014p\n", mappingb, reloc->r_offset, mappingb+reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S)+mappingb;
                    char ** addr = reloc->r_offset + mappingb;
                    printf("%014p = %014p\n", addr, *addr);
                    break;
                }
                case R_X86_64_RELATIVE:
                {
                    printf("\n\n\nR_X86_64_RELATIVE            calculation: B + A (base address + r_addend)\n");
                    printf("mappingb    = %014p\n", mappingb);
                    printf("reloc->r_offset = %014p+%014p=%014p\n", mappingb, reloc->r_offset, mappingb+reloc->r_offset);
                    printf("reloc->r_addend = %014p+%014p=%014p\n", mappingb, reloc->r_addend, ((char*)mappingb + reloc->r_addend) );
                    *((char**)((char*)mappingb + reloc->r_offset)) = ((char*)mappingb + reloc->r_addend);
                    char ** addr = reloc->r_offset + mappingb;
                    printf("%014p = %014p\n", addr, *addr);
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_GOTPCREL:
                {
                    printf("\n\n\nR_X86_64_GOTPCREL            calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).))) \n");
                    break;
                }
                case R_X86_64_32:
                {
                    printf("\n\n\nR_X86_64_32                  calculation: S + A (symbol value + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_32S:
                {
                    printf("\n\n\nR_X86_64_32S                 calculation: S + A (symbol value + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_16:
                {
                    printf("\n\n\nR_X86_64_16                  calculation: S + A (symbol value + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_PC16:
                {
                    printf("\n\n\nR_X86_64_PC16                calculation: S + A - P (symbol value + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).))\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_8:
                {
                    printf("\n\n\nR_X86_64_8                   calculation: S + A (symbol value + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_PC8:
                {
                    printf("\n\n\nR_X86_64_PC8                 calculation: S + A - P (symbol value + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).))\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_DTPMOD64:
                {
                    printf("\n\n\nR_X86_64_DTPMOD64\n");
                    break;
                }
                case R_X86_64_DTPOFF64:
                {
                    printf("\n\n\nR_X86_64_DTPOFF64\n");
                    break;
                }
                case R_X86_64_TPOFF64:
                {
                    printf("\n\n\nR_X86_64_TPOFF64\n");
                    break;
                }
                case R_X86_64_TLSGD:
                {
                    printf("\n\n\nR_X86_64_TLSGD\n");
                    break;
                }
                case R_X86_64_TLSLD:
                {
                    printf("\n\n\nR_X86_64_TLSLD\n");
                    break;
                }
                case R_X86_64_DTPOFF32:
                {
                    printf("\n\n\nR_X86_64_DTPOFF32\n");
                    break;
                }
                case R_X86_64_GOTTPOFF:
                {
                    printf("\n\n\nR_X86_64_GOTTPOFF\n");
                    break;
                }
                case R_X86_64_TPOFF32:
                {
                    printf("\n\n\nR_X86_64_TPOFF32\n");
                    break;
                }
                case R_X86_64_PC64:
                {
                    printf("\n\n\nR_X86_64_PC64                calculation: S + A (symbol value + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_GOTOFF64:
                {
                    printf("\n\n\nR_X86_64_GOTOFF64            calculation: S + A - GOT (symbol value + r_addend - address of global offset table)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_S) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_GOTPC32:
                {
                    printf("\n\n\nR_X86_64_GOTPC32             calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_GOT64:
                {
                    printf("\n\n\nR_X86_64_GOT64               calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_GOTPCREL64:
                {
                    printf("\n\n\nR_X86_64_GOTPCREL64          calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_GOTPC64:
                {
                    printf("\n\n\nR_X86_64_GOTPC64             calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_GOTPLT64:
                {
                    printf("\n\n\nR_X86_64_GOTPLT64            calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_PLTOFF64:
                {
                    printf("\n\n\nR_X86_64_PLTOFF64\n");
                    break;
                }
                case R_X86_64_SIZE32:
                {
                    printf("\n\n\nR_X86_64_SIZE32                 calculation: Z + A (symbol size + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_Z) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_SIZE64:
                {
                    printf("\n\n\nR_X86_64_SIZE64                 calculation: Z + A (symbol size + r_addend)\n");
                    printf("reloc->r_offset = %014p\n", reloc->r_offset);
                    *((char**)((char*)mappingb + reloc->r_offset)) = lookup_symbol_by_index(array, _elf_header, ELF64_R_SYM(reloc->r_info), symbol_mode_Z) + reloc->r_addend+mappingb;
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_GOTPC32_TLSDESC:
                {
                    printf("\n\n\nR_X86_64_GOTPC32_TLSDESC     calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_TLSDESC_CALL:
                {
                    printf("\n\n\nR_X86_64_TLSDESC_CALL\n");
                    break;
                }
                case R_X86_64_TLSDESC:
                {
                    printf("\n\n\nR_X86_64_TLSDESC\n");
                    break;
                }
                case R_X86_64_IRELATIVE:
                {
                    printf("\n\n\nR_X86_64_IRELATIVE                 calculation: (indirect) B + A (base address + r_addend)\n");
                    printf("mappingb    = %014p\n", mappingb);
                    printf("reloc->r_offset = %014p+%014p=%014p\n", mappingb, reloc->r_offset, mappingb+reloc->r_offset);
                    printf("reloc->r_addend = %014p+%014p=%014p\n", mappingb, reloc->r_addend, ((char*)mappingb + reloc->r_addend) );
                    *((char**)((char*)mappingb + reloc->r_offset)) = ((char*)mappingb + reloc->r_addend);
                    char ** addr = reloc->r_offset + mappingb;
                    printf("%014p = %014p\n", addr, *addr);
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_RELATIVE64:
                {
                    printf("\n\n\nR_X86_64_RELATIVE64                 calculation: B + A (base address + r_addend)\n");
                    printf("mappingb    = %014p\n", mappingb);
                    printf("reloc->r_offset = %014p+%014p=%014p\n", mappingb, reloc->r_offset, mappingb+reloc->r_offset);
                    printf("reloc->r_addend = %014p+%014p=%014p\n", mappingb, reloc->r_addend, ((char*)mappingb + reloc->r_addend) );
                    *((char**)((char*)mappingb + reloc->r_offset)) = ((char*)mappingb + reloc->r_addend);
                    char ** addr = reloc->r_offset + mappingb;
                    printf("%014p = %014p\n", addr, *addr);
                    printf("((char*)mappingb + reloc->r_offset)            = %014p\n", ((char*)mappingb + reloc->r_offset));
                    break;
                }
                case R_X86_64_GOTPCRELX:
                {
                    printf("\n\n\nR_X86_64_GOTPCRELX           calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_REX_GOTPCRELX:
                {
                    printf("\n\n\nR_X86_64_REX_GOTPCRELX       calculation: (_GOTPC: GOT + A - P (address of global offset table + r_addend - (P: This means the place (section offset or address) of the storage unit being relocated (computed using r_offset ).)))\n");
                    break;
                }
                case R_X86_64_NUM:
                {
                    printf("\n\n\nR_X86_64_NUM\n");
                    break;
                }
                #endif
                default:
                    printf("unknown type, got %d\n", reloc_type);
                    break;
            }
        }
    }
    nl();
    nl();
    nl();
}

int
init_(const char * filename) {
    init(filename);
    if (init__ == 1) return 0;
    _elf_header = (Elf64_Ehdr *) array;
    read_section_header_table_(array, _elf_header, &_elf_symbol_table);
    obtain_rela_plt_size(array, _elf_header, _elf_symbol_table);
    if(!strncmp((char*)_elf_header->e_ident, "\177ELF", 4)) {
        map();
        for (int i = 0; i < _elf_header->e_phnum; ++i) {
            char * section_;
            switch(_elf_program_header[i].p_type)
            {
                case PT_NULL:
                    section_="PT_NULL";
                    break;
                case PT_LOAD:
                    section_="PT_LOAD";
                    break;
                case PT_DYNAMIC:
                    section_="PT_DYNAMIC";
                    PT_DYNAMIC_=i;
                    break;
                case PT_INTERP:
                    section_="PT_INTERP";
                    break;
                case PT_NOTE:
                    section_="PT_NOTE";
                    break;
                case PT_SHLIB:
                    section_="PT_SHLIB";
                    break;
                case PT_PHDR:
                    section_="PT_PHDR";
                    break;
                case PT_TLS:
                    section_="PT_TLS";
                    break;
                case PT_NUM:
                    section_="PT_NUM";
                    break;
                case PT_LOOS:
                    section_="PT_LOOS";
                    break;
                case PT_GNU_EH_FRAME:
                    section_="PT_GNU_EH_FRAME";
                    break;
                case PT_GNU_STACK:
                    section_="PT_GNU_STACK";
                    break;
                case PT_GNU_RELRO:
                    section_="PT_GNU_RELRO";
                    break;
                case PT_SUNWBSS:
                    section_="PT_SUNWBSS";
                    break;
                case PT_SUNWSTACK:
                    section_="PT_SUNWSTACK";
                    break;
                case PT_HIOS:
                    section_="PT_HIOS";
                    break;
                case PT_LOPROC:
                    section_="PT_LOPROC";
                    break;
                case PT_HIPROC:
                    section_="PT_HIPROC";
                    break;
                default:
                    section_="Unknown";
                    break;
            }
            if (section_ == "PT_DYNAMIC")
            {
                read_fast_verify(array, len, &tmp99D, (_elf_program_header[i].p_memsz + _elf_program_header[i].p_offset));
                __lseek_string__(&tmp99D, _elf_program_header[i].p_memsz, _elf_program_header[i].p_offset);
            }
        }
        if (PT_DYNAMIC_ != 0) {
            ElfW(Dyn) * dynamic = tmp99D;
            r(mappingb + get_dynamic_entry(dynamic, DT_RELA), get_dynamic_entry(dynamic, DT_RELASZ));
            r(mappingb + get_dynamic_entry(dynamic, DT_JMPREL), RELA_PLT_SIZE);
        }
    }
    else return -1;
    init__ = 1;
    return 0;
}

int
initv_(const char * filename) {
    init(filename);
    if (init__ == 1) return 0;
    _elf_header = (Elf64_Ehdr *) array;
    read_section_header_table_(array, _elf_header, &_elf_symbol_table);
    obtain_rela_plt_size(array, _elf_header, _elf_symbol_table);
    if(!strncmp((char*)_elf_header->e_ident, "\177ELF", 4)) {
        printf("Name:\t\t %s\n", filename);
        printf("ELF Identifier\t %s\n", _elf_header->e_ident);

        printf("Architecture\t ");
        switch(_elf_header->e_ident[EI_CLASS])
        {
            case ELFCLASSNONE:
                printf("None\n");
                break;

            case ELFCLASS32:
                printf("32-bit\n");
                break;

            case ELFCLASS64:
                printf("64-bit\n");
                break;
                
            case ELFCLASSNUM:
                printf("NUM ( unspecified )\n");
                break;

            default:
                printf("Unknown CLASS\n");
                break;
        }

        printf("Data Type\t ");
        switch(_elf_header->e_ident[EI_DATA])
        {
            case ELFDATANONE:
                printf("None\n");
                break;

            case ELFDATA2LSB:
                printf("2's complement, little endian\n");
                break;

            case ELFDATA2MSB:
                printf("2's complement, big endian\n");
                break;
                
            case ELFDATANUM:
                printf("NUM ( unspecified )\n");
                break;

            default:
                printf("Unknown \n");
                break;
        }

        printf("Version\t\t ");
        switch(_elf_header->e_ident[EI_VERSION])
        {
            case EV_NONE:
                printf("None\n");
                break;

            case EV_CURRENT:
                printf("Current\n");
                break;

            case EV_NUM:
                printf("NUM ( Unspecified )\n");
                break;

            default:
                printf("Unknown \n");
                break;
        }

        printf("OS ABI\t\t ");
        switch(_elf_header->e_ident[EI_OSABI])
        {
            case ELFOSABI_NONE:
                printf("UNIX System V ABI\n");
                break;

//                     case ELFOSABI_SYSV:
//                         printf("SYSV\n");
//                         break;
// 
            case ELFOSABI_HPUX:
                printf("HP-UX\n");
                break;

            case ELFOSABI_NETBSD:
                printf("NetBSD\n");
                break;

            case ELFOSABI_GNU:
                printf("GNU\n");
                break;

//                     case ELFOSABI_LINUX:
//                         printf("Linux\n");
//                         break;
// 
            case ELFOSABI_SOLARIS:
                printf("Sun Solaris\n");
                break;

            case ELFOSABI_AIX:
                printf("ABM AIX\n");
                break;

            case ELFOSABI_FREEBSD:
                printf("FreeBSD\n");
                break;

            case ELFOSABI_TRU64:
                printf("Compaq Tru64\n");
                break;

            case ELFOSABI_MODESTO:
                printf("Novell Modesto\n");
                break;

            case ELFOSABI_OPENBSD:
                printf("OpenBSD\n");
                break;

            case ELFOSABI_ARM_AEABI:
                printf("ARM EABI\n");
                break;

            case ELFOSABI_ARM:
                printf("ARM\n");
                break;

            case ELFOSABI_STANDALONE:
                printf("Standalone (embedded) application\n");
                break;

            default:
                printf("Unknown \n");
                break;
        }

        printf("File Type\t ");
        switch(_elf_header->e_type)
        {
            case ET_NONE:
                printf("None\n");
                break;

            case ET_REL:
                printf("Relocatable file\n");
                break;

            case ET_EXEC:
                printf("Executable file\n");
                break;

            case ET_DYN:
                printf("Shared object file\n");
                break;

            case ET_CORE:
                printf("Core file\n");
                break;

            case ET_NUM:
                printf("Number of defined types\n");
                break;

            case ET_LOOS:
                printf("OS-specific range start\n");
                break;

            case ET_HIOS:
                printf("OS-specific range end\n");
                break;

            case ET_LOPROC:
                printf("Processor-specific range start\n");
                break;

            case ET_HIPROC:
                printf("Processor-specific range end\n");
                break;

            default:
                printf("Unknown \n");
                break;
        }

        printf("Machine\t\t ");
        switch(_elf_header->e_machine)
        {
            case EM_NONE:
                printf("None\n");
                break;

            case EM_386:
                    printf("INTEL x86\n");
                    break;

            case EM_X86_64:
                    printf("AMD x86-64 architecture\n");
                    break;

            case EM_ARM:
                    printf("ARM\n");
                    break;
            default:
                    printf("Unknown\n");
            break;
        }
        
        /* Entry point */
        int entry=_elf_header->e_entry;
        printf("Entry point\t %014p\n", _elf_header->e_entry);
        

        /* ELF header size in bytes */
        printf("ELF header size\t %014p\n", _elf_header->e_ehsize);

        /* Program Header */
        printf("Program Header\t %014p (%d entries with a total of %d bytes)\n",
        _elf_header->e_phoff,
        _elf_header->e_phnum,
        _elf_header->e_phentsize
        );
        map();
        for (int i = 0; i < _elf_header->e_phnum; ++i) {
            char * section_;
            printf("p_type:\t\t\t/* Segment type */\t\t= ");
            switch(_elf_program_header[i].p_type)
            {
                case PT_NULL:
                    printf("PT_NULL\t\t/* Program header table entry unused */\n");
                    section_="PT_NULL";
                    break;
                case PT_LOAD:
                    printf("PT_LOAD\t\t/* Loadable program segment */\n");
                    section_="PT_LOAD";
                    break;
                case PT_DYNAMIC:
                    printf("PT_DYNAMIC\t\t/* Dynamic linking information */\n");
                    section_="PT_DYNAMIC";
                    PT_DYNAMIC_=i;
                    break;
                case PT_INTERP:
                    printf("PT_INTERP\t\t/* Program interpreter */\n");
                    section_="PT_INTERP";
                    break;
                case PT_NOTE:
                    printf("PT_NOTE\t\t/* Auxiliary information */\n");
                    section_="PT_NOTE";
                    break;
                case PT_SHLIB:
                    printf("PT_SHLIB\t\t/* Reserved */\n");
                    section_="PT_SHLIB";
                    break;
                case PT_PHDR:
                    printf("PT_PHDR\t\t/* Entry for header table itself */\n");
                    section_="PT_PHDR";
                    break;
                case PT_TLS:
                    printf("PT_TLS\t\t/* Thread-local storage segment */\n");
                    section_="PT_TLS";
                    break;
                case PT_NUM:
                    printf("PT_NUM\t\t/* Number of defined types */\n");
                    section_="PT_NUM";
                    break;
                case PT_LOOS:
                    printf("PT_LOOS\t\t/* Start of OS-specific */\n");
                    section_="PT_LOOS";
                    break;
                case PT_GNU_EH_FRAME:
                    printf("PT_GNU_EH_FRAME\t/* GCC .eh_frame_hdr segment */\n");
                    section_="PT_GNU_EH_FRAME";
                    break;
                case PT_GNU_STACK:
                    printf("PT_GNU_STACK\t\t/* Indicates stack executability */\n");
                    section_="PT_GNU_STACK";
                    break;
                case PT_GNU_RELRO:
                    printf("PT_GNU_RELRO\t\t/* Read-only after relocation */\n");
                    section_="PT_GNU_RELRO";
                    break;
                case PT_SUNWBSS:
                    printf("PT_SUNWBSS\t\t/* Sun Specific segment */\n");
                    section_="PT_SUNWBSS";
                    break;
                case PT_SUNWSTACK:
                    printf("PT_SUNWSTACK\t\t/* Stack segment */\n");
                    section_="PT_SUNWSTACK";
                    break;
                case PT_HIOS:
                    printf("PT_HIOS\t\t/* End of OS-specific */\n");
                    section_="PT_HIOS";
                    break;
                case PT_LOPROC:
                    printf("PT_LOPROC\t\t/* Start of processor-specific */\n");
                    section_="PT_LOPROC";
                    break;
                case PT_HIPROC:
                    printf("PT_HIPROC\t\t/* End of processor-specific */\n");
                    section_="PT_HIPROC";
                    break;
                default:
                    printf("Unknown\n");
                    section_="Unknown";
                    break;
            }
            if (section_ == "PT_DYNAMIC")
            {
                // obtain PT_DYNAMIC into seperate array for use later
                read_fast_verify(array, len, &tmp99D, (_elf_program_header[i].p_memsz + _elf_program_header[i].p_offset));
                __lseek_string__(&tmp99D, _elf_program_header[i].p_memsz, _elf_program_header[i].p_offset);
            }
            printf("p_flags:\t\t/* Segment flags */\t\t= %014p\np_offset:\t\t/* Segment file offset */\t= %014p\np_vaddr:\t\t/* Segment virtual address */\t= %014p\np_paddr:\t\t/* Segment physical address */\t= %014p\np_filesz:\t\t/* Segment size in file */\t= %014p\np_memsz:\t\t/* Segment size in memory */\t= %014p\np_align:\t\t/* Segment alignment */\t\t= %014p\n\n\n", _elf_program_header[i].p_flags, _elf_program_header[i].p_offset, _elf_program_header[i].p_vaddr+mappingb, _elf_program_header[i].p_paddr, _elf_program_header[i].p_filesz, _elf_program_header[i].p_memsz, _elf_program_header[i].p_align);
            nl();
            printf("\t\tp_flags: %014p", _elf_program_header[i].p_flags);
            printf(" p_offset: %014p", _elf_program_header[i].p_offset);
            printf(" p_vaddr:  %014p", _elf_program_header[i].p_vaddr+mappingb);
            printf(" p_paddr: %014p", _elf_program_header[i].p_paddr);
            printf(" p_filesz: %014p", _elf_program_header[i].p_filesz);
            printf(" p_memsz: %014p", _elf_program_header[i].p_memsz);
            printf(" p_align: %014p\n", _elf_program_header[i].p_align);
        }

        if (PT_DYNAMIC_ != 0) {
// A PT_DYNAMIC program header element points at the .dynamic section, explained in
// "Dynamic Section" below. The .got and .plt sections also hold information related to
// position-independent code and dynamic linking. Although the .plt appears in a text segment
// above, it may reside in a text or a data segment, depending on the processor.
// 
// As "Sections" describes, the .bss section has the type SHT_NOBITS. Although it occupies no
// space in the file, it contributes to the segment's memory image. Normally, these uninitialized
// data reside at the end of the segment, thereby making p_memsz larger than p_filesz.
// 

            printf("PT_LOAD 1 = \n");
            printf("p_flags:\t\t/* Segment flags */\t\t= %014p\np_offset:\t\t/* Segment file offset */\t= %014p\np_vaddr:\t\t/* Segment virtual address */\t= %014p\np_paddr:\t\t/* Segment physical address */\t= %014p\np_filesz:\t\t/* Segment size in file */\t= %014p\np_memsz:\t\t/* Segment size in memory */\t= %014p\np_align:\t\t/* Segment alignment */\t\t= %014p\n\n\n", _elf_program_header[First_Load_Header_index].p_flags, _elf_program_header[First_Load_Header_index].p_offset, _elf_program_header[First_Load_Header_index].p_vaddr+mappingb, _elf_program_header[First_Load_Header_index].p_paddr, _elf_program_header[First_Load_Header_index].p_filesz, _elf_program_header[First_Load_Header_index].p_memsz, _elf_program_header[First_Load_Header_index].p_align);
            printf("PT_LOAD 2 = \n");
            printf("p_flags:\t\t/* Segment flags */\t\t= %014p\np_offset:\t\t/* Segment file offset */\t= %014p\np_vaddr:\t\t/* Segment virtual address */\t= %014p\np_paddr:\t\t/* Segment physical address */\t= %014p\np_filesz:\t\t/* Segment size in file */\t= %014p\np_memsz:\t\t/* Segment size in memory */\t= %014p\np_align:\t\t/* Segment alignment */\t\t= %014p\n\n\n", _elf_program_header[Last_Load_Header_index].p_flags, _elf_program_header[Last_Load_Header_index].p_offset, _elf_program_header[Last_Load_Header_index].p_vaddr+mappingb, _elf_program_header[Last_Load_Header_index].p_paddr, _elf_program_header[Last_Load_Header_index].p_filesz, _elf_program_header[Last_Load_Header_index].p_memsz, _elf_program_header[Last_Load_Header_index].p_align);
            printf("first PT_LOAD _elf_program_header[%d]->p_paddr = \n%014p\n", First_Load_Header_index, _elf_program_header[First_Load_Header_index].p_paddr+mappingb);
            printf("Second PT_LOAD _elf_program_header[%d]->p_paddr = \n%014p\n", Last_Load_Header_index, _elf_program_header[Last_Load_Header_index].p_paddr+mappingb);
            ElfW(Dyn) * dynamic = tmp99D;

//                 printf("printing symbol data\n");
//                 Elf64_Sym *syms = mappingb + get_dynamic_entry(dynamic, DT_SYMTAB);
//                 symbol1(array, syms, 0);
            printf("examining current entries:\n");
            get_dynamic_entry(dynamic, -1);
            printf("printing relocation data\n");
            // needs to be the address of the mapping itself, not the base address
            r(mappingb + get_dynamic_entry(dynamic, DT_RELA), get_dynamic_entry(dynamic, DT_RELASZ));
            r(mappingb + get_dynamic_entry(dynamic, DT_JMPREL), RELA_PLT_SIZE);
        }
//             nl();
        
        printf("Section Header\t \
_elf_header->e_shstrndx %014p (\
_elf_header->e_shnum = %d entries with a total of \
_elf_header->e_shentsize = %d (should match %d) bytes, offset is \
_elf_header->e_shoff = %014p)\n",\
        _elf_header->e_shstrndx,\
        _elf_header->e_shnum,\
        _elf_header->e_shentsize,\
        sizeof(Elf64_Shdr),\
        _elf_header->e_shoff,\
        (char *)array + _elf_header->e_shoff\
        );
        read_section_header_table_(array, _elf_header, &_elf_symbol_table);
        print_section_headers_(array, _elf_header, _elf_symbol_table);
        print_symbols(array, _elf_header, _elf_symbol_table);
    } else {
        return -1;
    }
    init__ = 1;
    return 0;
}
