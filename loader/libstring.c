// TODO: move all of this into a seperate library called libstring.so
#include <syscall.h>
#include <link.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <alloca.h>
#include <sys/auxv.h>
void nl() {
    printf("\n");
}

// bytecmp compares two strings char by char for an EXACT full string match, returns -1 if strings differ in length or do not match but are of same length

int bcmp_(void const *vp, size_t n, void const *vp2, size_t n2)
{
    printf("----------------------------------------------------------------------->called %s() at line %d from %s\n", __func__, __LINE__, __FILE__);
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
            printf("ERROR: strings do not match\n");
            return -1;
        } else return 0;
    } else
    {
        printf("ERROR: different length string comparision, might want to use strcmp instead\n");
        return -1;
    }
}

int bytecmp(void const * p, void const * pp) { return bcmp_(p, strlen(p), pp, strlen(pp)); }

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

#define QUOTE_0_TERMINATED			0x01
#define QUOTE_OMIT_LEADING_TRAILING_QUOTES	0x02
#define QUOTE_OMIT_TRAILING_0			0x08
#define QUOTE_FORCE_HEX				0x10
#define QUOTE_FORCE_HEXOLD				9998
#define QUOTE_FORCE_LEN				9999
#define error_msg printf
int argc;
char **argv;
char ** saved_environ = NULL;
void libauxv_initv (void);
void init_env();
void init_argX();
void * getaux(void * type);
void * setaux(void * value, void * type);
void * currentaux();

void abort_() {
    printf("%s: cannot continue, pausing execution to allow for debugging\nif you do now know how to debug this process as paused execute the following in a new terminal:\n\n    sudo gdb -p %d\n\n", getaux(AT_EXECFN), getpid());
    pause();
}

void
init_env (void)
{
    printf("----------------------------------------------------------------------->called %s() at line %d from %s\n", __func__, __LINE__, __FILE__);
    assert(__environ!=NULL);
    saved_environ = __environ;
    assert(saved_environ!=NULL);
}

void
init_argX() {
    printf("----------------------------------------------------------------------->called %s() at line %d from %s\n", __func__, __LINE__, __FILE__);
    size_t i;
    char **p = &saved_environ[-2];
    for (i = 1; i != *(size_t*)(p-1); i++) {
        p--;
    }
    argc = (int)i;
    argv = p;
}

void __attribute__ ((__constructor__ (0)))
init_aux() {
    printf("----------------------------------------------------------------------->called %s() at line %d from %s\n", __func__, __LINE__, __FILE__);
    init_env();
    init_argX();
}
void *
getaux(void * type) {
    printf("----------------------------------------------------------------------->called %s() at line %d from %s\n", __func__, __LINE__, __FILE__);
    #include <elf.h>
    #include <unistd.h>
    #ifndef AUX_CNT
        #define AUX_CNT 32
    #endif
	size_t i;
    int num = -1;
    void * AUXV_TYPE;
    char * AUXV_NAME;
    for (i=argc+1; argv[i]; i++);
    for (int ii=0; ii<=(argv+i+1)[0]+(2*20); ii+=2) { // 20 extra incase auxv does not have the same vectors for every machine (could have more than +4)
        size_t tmp = (size_t)(argv+i+1+ii)[0];
        switch(tmp)
        {
                case 0:
                    AUXV_NAME = "AT_NULL";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 0;
                    break;
                case 1:
                    AUXV_NAME = "AT_IGNORE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 1;
                    break;
                case 2:
                    AUXV_NAME = "AT_EXECFD";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 2;
                    break;
                case 3:
                    AUXV_NAME = "AT_PHDR";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 3;
                    break;
                case 4:
                    AUXV_NAME = "AT_PHENT";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 4;
                    break;
                case 5:
                    AUXV_NAME = "AT_PHNUM";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 5;
                    break;
                case 6:
                    AUXV_NAME = "AT_PAGESZ";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 6;
                    break;
                case 7:
                    AUXV_NAME = "AT_BASE";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 7;
                    break;
                case 8:
                    AUXV_NAME = "AT_FLAGS";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 8;
                    break;
                case 9:
                    AUXV_NAME = "AT_ENTRY";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 9;
                    break;
                case 10:
                    AUXV_NAME = "AT_NOTELF";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 10;
                    break;
                case 11:
                    AUXV_NAME = "AT_UID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 11;
                    break;
                case 12:
                    AUXV_NAME = "AT_EUID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 12;
                    break;
                case 13:
                    AUXV_NAME = "AT_GID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 13;
                    break;
                case 14:
                    AUXV_NAME = "AT_EGID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 14;
                    break;
                case 15:
                    AUXV_NAME = "AT_PLATFORM";
                    AUXV_TYPE = "CHAR*";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 15;
                    break;
                case 16:
                    AUXV_NAME = "AT_HWCAP";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 16;
                    break;
                case 17:
                    AUXV_NAME = "AT_CLKTCK";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 17;
                    break;
                case 18:
                    AUXV_NAME = "AT_FPUCW";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 18;
                    break;
                case 19:
                    AUXV_NAME = "AT_DCACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 19;
                    break;
                case 20:
                    AUXV_NAME = "AT_ICACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 20;
                    break;
                case 21:
                    AUXV_NAME = "AT_UCACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 21;
                    break;
                case 22:
                    AUXV_NAME = "AT_IGNOREPPC";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 22;
                    break;
                case 23:
                    AUXV_NAME = "AT_SECURE";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 23;
                    break;
                case 24:
                    AUXV_NAME = "AT_BASE_PLATFORM";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 24;
                    break;
                case 25:
                    AUXV_NAME = "AT_RANDOM";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 25;
                    break;
                case 26:
                    AUXV_NAME = "AT_HWCAP2";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 26;
                    break;
                case 31:
                    AUXV_NAME = "AT_EXECFN";
                    AUXV_TYPE = "CHAR*";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 31;
                    break;
                case 32:
                    AUXV_NAME = "AT_SYSINFO";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 32;
                    break;
                case 33:
                    AUXV_NAME = "AT_SYSINFO_EHDR";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 33;
                    break;
                case 34:
                    AUXV_NAME = "AT_L1I_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 34;
                    break;
                case 35:
                    AUXV_NAME = "AT_L1D_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 35;
                    break;
                case 36:
                    AUXV_NAME = "AT_L2_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 36;
                    break;
                case 37:
                    AUXV_NAME = "AT_L3_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 37;
                    break;
                default:
                    AUXV_NAME = "UNDEFINED";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = -1;
                    break;
        }
        if (num == type) {
            if (AUXV_TYPE == "CHAR*") {
            printf("%s = %s\n", AUXV_NAME, (void *)(argv+i+1+ii)[1]);
            } else if (AUXV_TYPE == "INT") {
            printf("%s = %d\n", AUXV_NAME, (void *)(argv+i+1+ii)[1]);
            } else if (AUXV_TYPE == "ADDRESS") {
            printf("%s = %p\n", AUXV_NAME, (void *)(argv+i+1+ii)[1]);
            }
            return (argv+i+1+ii)[1];
        }
    }
}

void *
setaux(void * value, void * type) {
    #include <elf.h>
    #include <unistd.h>
    #ifndef AUX_CNT
        #define AUX_CNT 32
    #endif
	size_t i;
    int num = -1;
    void * AUXV_TYPE;
    char * AUXV_NAME;
    for (i=argc+1; argv[i]; i++);
    for (int ii=0; ii<=(argv+i+1)[0]+(2*20); ii+=2) { // 20 extra incase auxv does not have the same vectors for every machine (could have more than +4)
        size_t tmp = (size_t *)(argv+i+1+ii)[0];
        switch(tmp)
        {
                case 0:
                    AUXV_NAME = "AT_NULL";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 0;
                    break;
                case 1:
                    AUXV_NAME = "AT_IGNORE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 1;
                    break;
                case 2:
                    AUXV_NAME = "AT_EXECFD";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 2;
                    break;
                case 3:
                    AUXV_NAME = "AT_PHDR";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 3;
                    break;
                case 4:
                    AUXV_NAME = "AT_PHENT";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 4;
                    break;
                case 5:
                    AUXV_NAME = "AT_PHNUM";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 5;
                    break;
                case 6:
                    AUXV_NAME = "AT_PAGESZ";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 6;
                    break;
                case 7:
                    AUXV_NAME = "AT_BASE";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 7;
                    break;
                case 8:
                    AUXV_NAME = "AT_FLAGS";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 8;
                    break;
                case 9:
                    AUXV_NAME = "AT_ENTRY";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 9;
                    break;
                case 10:
                    AUXV_NAME = "AT_NOTELF";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 10;
                    break;
                case 11:
                    AUXV_NAME = "AT_UID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 11;
                    break;
                case 12:
                    AUXV_NAME = "AT_EUID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 12;
                    break;
                case 13:
                    AUXV_NAME = "AT_GID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 13;
                    break;
                case 14:
                    AUXV_NAME = "AT_EGID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 14;
                    break;
                case 15:
                    AUXV_NAME = "AT_PLATFORM";
                    AUXV_TYPE = "CHAR*";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 15;
                    break;
                case 16:
                    AUXV_NAME = "AT_HWCAP";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 16;
                    break;
                case 17:
                    AUXV_NAME = "AT_CLKTCK";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 17;
                    break;
                case 18:
                    AUXV_NAME = "AT_FPUCW";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 18;
                    break;
                case 19:
                    AUXV_NAME = "AT_DCACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 19;
                    break;
                case 20:
                    AUXV_NAME = "AT_ICACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 20;
                    break;
                case 21:
                    AUXV_NAME = "AT_UCACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 21;
                    break;
                case 22:
                    AUXV_NAME = "AT_IGNOREPPC";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 22;
                    break;
                case 23:
                    AUXV_NAME = "AT_SECURE";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 23;
                    break;
                case 24:
                    AUXV_NAME = "AT_BASE_PLATFORM";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 24;
                    break;
                case 25:
                    AUXV_NAME = "AT_RANDOM";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 25;
                    break;
                case 26:
                    AUXV_NAME = "AT_HWCAP2";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 26;
                    break;
                case 31:
                    AUXV_NAME = "AT_EXECFN";
                    AUXV_TYPE = "CHAR*";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 31;
                    break;
                case 32:
                    AUXV_NAME = "AT_SYSINFO";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 32;
                    break;
                case 33:
                    AUXV_NAME = "AT_SYSINFO_EHDR";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 33;
                    break;
                case 34:
                    AUXV_NAME = "AT_L1I_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 34;
                    break;
                case 35:
                    AUXV_NAME = "AT_L1D_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 35;
                    break;
                case 36:
                    AUXV_NAME = "AT_L2_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 36;
                    break;
                case 37:
                    AUXV_NAME = "AT_L3_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 37;
                    break;
                default:
                    AUXV_NAME = "UNDEFINED";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = -1;
                    break;
        }
        if (num == type) {
        if (AUXV_TYPE == "CHAR*") {
            char * string = value;
            char *j = (void *)(argv+i+1+ii)[1];
            int len = strlen((void *)(argv+i+1+ii)[1]);
            int len_ = strlen(string);
            for (int g = 0; g<=len_; g++) {
                *j = string[g];
                j+=1;
            }
            for (int g = 0; g<=len-len_; g++) { // NULL the rest of the string
                *j = '\0';
                j+=1;
            }
        } else if (AUXV_TYPE == "INT") {
            (argv+i+1+ii)[1] = value;
        } else if (AUXV_TYPE == "ADDRESS") {
            (argv+i+1+ii)[1] = value;
        }
        }
    }
    type = -1;
}

void * __attribute__ ((__constructor__ (1)))
currentaux() {
    printf("----------------------------------------------------------------------->called %s() at line %d from %s\n", __func__, __LINE__, __FILE__);
    #include <elf.h>
    #include <unistd.h>
    #ifndef AUX_CNT
        #define AUX_CNT 32
    #endif
	size_t i;
    int num = -1;
    void * AUXV_TYPE;
    char * AUXV_NAME;
    for (i=argc+1; argv[i]; i++);
    for (int ii=0; ii<=(argv+i+1)[0]+(2*20); ii+=2) { // 20 extra incase auxv does not have the same vectors for every machine (could have more than +4)
        size_t tmp = (size_t)(argv+i+1+ii)[0];
        switch(tmp)
        {
                case 0:
                    AUXV_NAME = "AT_NULL";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 0;
                    break;
                case 1:
                    AUXV_NAME = "AT_IGNORE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 1;
                    break;
                case 2:
                    AUXV_NAME = "AT_EXECFD";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 2;
                    break;
                case 3:
                    AUXV_NAME = "AT_PHDR";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 3;
                    break;
                case 4:
                    AUXV_NAME = "AT_PHENT";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 4;
                    break;
                case 5:
                    AUXV_NAME = "AT_PHNUM";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 5;
                    break;
                case 6:
                    AUXV_NAME = "AT_PAGESZ";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 6;
                    break;
                case 7:
                    AUXV_NAME = "AT_BASE";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 7;
                    break;
                case 8:
                    AUXV_NAME = "AT_FLAGS";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 8;
                    break;
                case 9:
                    AUXV_NAME = "AT_ENTRY";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 9;
                    break;
                case 10:
                    AUXV_NAME = "AT_NOTELF";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 10;
                    break;
                case 11:
                    AUXV_NAME = "AT_UID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 11;
                    break;
                case 12:
                    AUXV_NAME = "AT_EUID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 12;
                    break;
                case 13:
                    AUXV_NAME = "AT_GID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 13;
                    break;
                case 14:
                    AUXV_NAME = "AT_EGID";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 14;
                    break;
                case 15:
                    AUXV_NAME = "AT_PLATFORM";
                    AUXV_TYPE = "CHAR*";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 15;
                    break;
                case 16:
                    AUXV_NAME = "AT_HWCAP";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 16;
                    break;
                case 17:
                    AUXV_NAME = "AT_CLKTCK";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 17;
                    break;
                case 18:
                    AUXV_NAME = "AT_FPUCW";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 18;
                    break;
                case 19:
                    AUXV_NAME = "AT_DCACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 19;
                    break;
                case 20:
                    AUXV_NAME = "AT_ICACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 20;
                    break;
                case 21:
                    AUXV_NAME = "AT_UCACHEBSIZE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 21;
                    break;
                case 22:
                    AUXV_NAME = "AT_IGNOREPPC";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 22;
                    break;
                case 23:
                    AUXV_NAME = "AT_SECURE";
                    AUXV_TYPE = "INT";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 23;
                    break;
                case 24:
                    AUXV_NAME = "AT_BASE_PLATFORM";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 24;
                    break;
                case 25:
                    AUXV_NAME = "AT_RANDOM";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 25;
                    break;
                case 26:
                    AUXV_NAME = "AT_HWCAP2";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 26;
                    break;
                case 31:
                    AUXV_NAME = "AT_EXECFN";
                    AUXV_TYPE = "CHAR*";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 31;
                    break;
                case 32:
                    AUXV_NAME = "AT_SYSINFO";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 32;
                    break;
                case 33:
                    AUXV_NAME = "AT_SYSINFO_EHDR";
                    AUXV_TYPE = "ADDRESS";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 33;
                    break;
                case 34:
                    AUXV_NAME = "AT_L1I_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 34;
                    break;
                case 35:
                    AUXV_NAME = "AT_L1D_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 35;
                    break;
                case 36:
                    AUXV_NAME = "AT_L2_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 36;
                    break;
                case 37:
                    AUXV_NAME = "AT_L3_CACHESHAPE";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = 37;
                    break;
                default:
                    AUXV_NAME = "UNDEFINED";
                    AUXV_TYPE = "";
                    // printf("AUXV_NAME = %s, AUXV_TYPE = %s\n", AUXV_NAME, AUXV_TYPE);
                    num = -1;
                    break;
        }
//         if (num == 31) {
            if (AUXV_TYPE == "CHAR*") {
            printf("%s = %s\n", AUXV_NAME, (void *)(argv+i+1+ii)[1]);
            } else if (AUXV_TYPE == "INT") {
            printf("%s = %d\n", AUXV_NAME, (void *)(argv+i+1+ii)[1]);
            } else if (AUXV_TYPE == "ADDRESS") {
            printf("%s = %p\n", AUXV_NAME, (void *)(argv+i+1+ii)[1]);
            }
//             return (void *)(argv+i+1+ii)[1];
//         }
    }
}
/// strace
#define error_msg printf
// these may be required?
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif
#include <sys/uio.h>
/*
* Copyright (c) 1991, 1992 Paul Kranenburg <pk@cs.few.eur.nl>
* Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
* Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
* Copyright (c) 2001-2017 The strace developers.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
* Quote string `instr' of length `size'
* Write up to (3 + `size' * 4) bytes to `outstr' buffer.
*
* If QUOTE_0_TERMINATED `style' flag is set,
* treat `instr' as a NUL-terminated string,
* checking up to (`size' + 1) bytes of `instr'.
*
* If QUOTE_OMIT_LEADING_TRAILING_QUOTES `style' flag is set,
* do not add leading and trailing quoting symbols.
*
* Returns 0 if QUOTE_0_TERMINATED is set and NUL was seen, 1 otherwise.
* Note that if QUOTE_0_TERMINATED is not set, always returns 1.
*/
int
string_quote_catraw(const char *instr, char *outstr, const unsigned int size, const unsigned int style)
{
    const unsigned char *ustr = (const unsigned char *) instr;
    char *s = outstr;
    unsigned int i;
    int usehex, usehexX, uselen, c;

    int xflag = 0;
    usehex = 0;
    usehexX = 0;
    uselen = 0;
    if ((style == 9998)) {
        usehexX = 1;
    } else if ((style == 9999)) {
        uselen = 1;
    } else if ((xflag > 1) || (style & QUOTE_FORCE_HEX)) {
        usehex = 1;
    } else if (xflag) {
        /* Check for presence of symbol which require
        to hex-quote the whole string. */
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                break;

            /* Force hex unless c is printable or whitespace */
            if (c > 0x7e) {
                usehex = 1;
                break;
            }
            /* In ASCII isspace is only these chars: "\t\n\v\f\r".
            * They happen to have ASCII codes 9,10,11,12,13.
            */
            if (c < ' ' && (unsigned)(c - 9) >= 5) {
                usehex = 1;
                break;
            }
        }
    }

    if (!(style & QUOTE_OMIT_LEADING_TRAILING_QUOTES))
        *s++ = '\"';

    if (usehexX) {
        /* Hex-quote the whole string. */
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                goto asciz_ended;
            // print hex in " 00 00" format instead of "\x00\x00" format
//             *s++ = '\\';
            *s++ = ' ';
            *s++ = "0123456789abcdef"[c >> 4];
            *s++ = "0123456789abcdef"[c & 0xf];
        }
    } else if (usehex) {
        /* Hex-quote the whole string. */
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                goto asciz_ended;
            *s++ = '\\';
            *s++ = 'x';
            *s++ = "0123456789abcdef"[c >> 4];
            *s++ = "0123456789abcdef"[c & 0xf];
        }
    } else if (uselen) {
        /* Hex-quote the whole string. */
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                goto asciz_ended;
            *s++ = '1';
        }
    } else {
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                goto asciz_ended;
            if ((i == (size - 1)) &&
                (style & QUOTE_OMIT_TRAILING_0) && (c == '\0'))
                goto asciz_ended;
                int pass_one = 0;
                int pass_two = 0;
                int pass_three = 0;
                int pass_four = 0;
                if (c == '\f') {
                    *s++ = '\\';
                    *s++ = 'f';
                    pass_one = 1;
                    pass_three = 1;
                    pass_four= 1;
                }
                int executable_format = 1;
                if (pass_one == 0 && executable_format == 0) { // nessisary to omit this otherwise it complains with stuff like invalid format characters
                    if (c == '%'/*FOR PRINTF*/) {
                        *s++ = '%';
                        *s++ = '%';
                        pass_two = 1;
                        pass_three = 1;
                        pass_four= 1;
                    } else {
                        pass_two = 1;
                    }
                }
                if (pass_two == 0) {
                    if (c == '\"') {
                        /*FOR PRINTF/SHELL*/
                        *s++ = '\\';
                        *s++ = '\"';
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '\\') {
                        /*FOR PRINTF/SHELL*/
                        *s++ = '\\';
                        *s++ = '\\';
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '`'/*FOR PRINTF*/|| c == '$'/*FOR BASH*/) {
//                             *s++ = '\\';
                        *s++ = c;
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '\''/*FOR PRINTF*/) {
//                             *s++ = '\\';
//                             *s++ = 'x';
//                             *s++ = '2';
                        *s++ = c;
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '!'/*FOR BASH*/ || c ==  '-'/*FOR PRINTF*/) {
//                             *s++ = '"';
//                             *s++ = '\'';
                        *s++ = c;
//                             *s++ = '\'';
//                             *s++ = '"';
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '%'/*FOR PRINTF*/) {
                        *s++ = '%';
                        *s++ = '%';
                        *s++ = '%';
                        *s++ = '%';
                        pass_three = 1;
                        pass_four= 1;
                    }
                }
                if (pass_three == 0) {
                    if (c == '\n') {
                        *s++ = '\\';
                        *s++ = 'n';
                        pass_four = 1;
                    } else if (c == '\r') {
                        *s++ = '\\';
                        *s++ = 'r';
                        pass_four = 1;
                    } else if (c == '\t') {
                        *s++ = '\\';
                        *s++ = 't';
                        pass_four = 1;
                    } else if (c == '\v') {
                        *s++ = '\\';
                        *s++ = 'v';
                        pass_four = 1;
                    }
                }
                if (pass_four == 0) {
                    if (c >= ' ' && c <= 0x7e)
                        *s++ = c;
                    else {
                        /* Print \octal */
                        *s++ = '\\';
                        if (i + 1 < size
                            && ustr[i + 1] >= '0'
                            && ustr[i + 1] <= '9'
                        ) {
                            /* Print \ooo */
                            *s++ = '0' + (c >> 6);
                            *s++ = '0' + ((c >> 3) & 0x7);
                        } else {
                            /* Print \[[o]o]o */
                            if ((c >> 3) != 0) {
                                if ((c >> 6) != 0)
                                    *s++ = '0' + (c >> 6);
                                *s++ = '0' + ((c >> 3) & 0x7);
                            }
                        }
                        *s++ = '0' + (c & 0x7);
                    }
            }
        }
    }

    if (!(style & QUOTE_OMIT_LEADING_TRAILING_QUOTES))
        *s++ = '\"';
    *s = '\0';

    /* Return zero if we printed entire ASCIZ string (didn't truncate it) */
    if (style & QUOTE_0_TERMINATED && ustr[i] == '\0') {
        /* We didn't see NUL yet (otherwise we'd jump to 'asciz_ended')
        * but next char is NUL.
        */
        return 0;
    }

    return 1;

asciz_ended:
    if (!(style & QUOTE_OMIT_LEADING_TRAILING_QUOTES))
        *s++ = '\"';
    *s = '\0';
    /* Return zero: we printed entire ASCIZ string (didn't truncate it) */
    return 0;
}

#ifndef ALLOCA_CUTOFF
# define ALLOCA_CUTOFF	4032
#endif
#define use_alloca(n) ((n) <= ALLOCA_CUTOFF)

/*
* Quote string `str' of length `size' and print the result.
*
* If QUOTE_0_TERMINATED `style' flag is set,
* treat `str' as a NUL-terminated string and
* quote at most (`size' - 1) bytes.
*
* If QUOTE_OMIT_LEADING_TRAILING_QUOTES `style' flag is set,
* do not add leading and trailing quoting symbols.
*
* Returns 0 if QUOTE_0_TERMINATED is set and NUL was seen, 1 otherwise.
* Note that if QUOTE_0_TERMINATED is not set, always returns 1.
*/
int
print_quoted_string_catraw(const char *str, unsigned int size, const unsigned int style, const char * return_type)
{
    char *buf;
    char *outstr;
    unsigned int alloc_size;
    int rc;

    if (size && style & QUOTE_0_TERMINATED)
        --size;

    alloc_size = 4 * size;
    if (alloc_size / 4 != size) {
        error_msg("Out of memory");
        printf("???");
        return -1;
    }
    alloc_size += 1 + (style & QUOTE_OMIT_LEADING_TRAILING_QUOTES ? 0 : 2);

    if (use_alloca(alloc_size)) {
        outstr = alloca(alloc_size);
        buf = NULL;
    } else {
        outstr = buf = malloc(alloc_size);
        if (!buf) {
            error_msg("Out of memory");
            printf("???");
            return -1;
        }
    }

//         rc = string_quote(str, outstr, size, style);
    string_quote_catraw(str, outstr, size, style);
    if ( return_type == "return") {
        return outstr;
    } else if ( return_type == "print") {
        printf(outstr);
    }

    free(buf);
//         return rc;
}

// function definitions to avoid using glibc 

// https://github.com/vendu/OS-Zero/blob/master/usr/lib/c/string.c

size_t
strlenb(const char *str)
{
    size_t len = 0;
    
    while (*str++) {
        len++;
    }
    
    return len;
}

#if ((defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 700))                 \
     || (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200809L)))

size_t
strnlenb(const char *str, size_t maxlen)
{
    size_t len = 0;

    while (str[0] && (maxlen--)) {
        len++;
    }

    return len;
}

#endif

/* TESTED OK */

void *
strchrb(const char *str,
       int ch)
{
    char *cptr = (char *)str;
    char  c = (char)ch;
    void *retval = NULL;
    
    while ((*cptr) && *cptr != c) {
        cptr++;
    }
    if (*cptr == c) {
        retval = cptr;
    }

    return retval;
}

char *
strcpyb(char *dest,
       const char *src)
{
    char *cptr = dest;

    while (*src) {
        *cptr++ = *src++;
    }
    *cptr = *src;

    return dest;
}

/* TESTED OK */

char *
  strncpyb(char *dest,
          const char *src,
          size_t n)
  {
      char *cptr = dest;
  
      if (n) {
          while ((*src) && (n--)) {
              *cptr++ = *src++;
          }
          if (n) {
              *cptr = *src;
          }
      }
  
      return dest;
  }

char *
strcatb(char *dest,
       const char *src)
{
    char *cptr = dest;

    while (*cptr) {
        cptr++;
    }
    while (*src) {
        *cptr++ = *src++;
    }
    *cptr = *src;

    return dest;
}

/* TESTED OK */
char *
strncatb(char *dest,
        const char *src,
        size_t n)
{
    char *cptr = dest;

    if (n) {
        while (*cptr) {
            cptr++;
        }
        while ((*src) && (n--)) {
            *cptr++ = *src++;
        }
        if (n) {
            *cptr = *src;
        }
    }

    return dest;
}

/* TESTED OK */
int
strcmpb(const char *str1,
       const char *str2)
{
    unsigned char *ucptr1 = (unsigned char *)str1;
    unsigned char *ucptr2 = (unsigned char *)str2;
    int            retval = 0;

    while ((*ucptr1) && *ucptr1 == *ucptr2) {
        ucptr1++;
        ucptr2++;
    }
    if (*ucptr1) {
        retval = (int)*ucptr1 - (int)*ucptr2;
    }

    return retval;
}

/* TESTED OK */
int
strncmpb(const char *str1,
        const char *str2,
        size_t n)
{
    unsigned char *ucptr1 = (unsigned char *)str1;
    unsigned char *ucptr2 = (unsigned char *)str2;
    int            retval = 0;

    if (n) {
        while ((*ucptr1) && (*ucptr1 == *ucptr2) && (n--)) {
            ucptr1++;
            ucptr2++;
        }
        if (n) {
            retval = (int)*ucptr1 - (int)*ucptr2;
        }
    }

    return retval;
}

int
strcollb(const char *str1,
        const char *str2)
{
////      _exit(1);
    return 1;
}

char* strstrb(const char *str, const char *target) {
  char * stra = (char *)malloc(strlen(str) + 1);
  strcpyb(stra,str);
  if (!*target) return stra;
  char *p1 = (char*)str, *p2 = (char*)target;
  char *p1Adv = (char*)str;
  while (*++p2)
    p1Adv++;
  while (*p1Adv) {
    char *p1Begin = p1;
    p2 = (char*)target;
    while (*p1 && *p2 && *p1 == *p2) {
      p1++;
      p2++;
    }
    if (!*p2)
      return p1Begin;
    p1 = p1Begin + 1;
    p1Adv++;
  }
  return NULL;
}

#include <string.h>

char *
  getenv_debug(const char *envtarget)
  {
    extern char **environ;
    const char * STTR = envtarget;
    int i = 1;
    char *s = *environ;

    for (; s; i++) {
        if (s)
        {
//                 printf("%s\n", s);
            char * pch;
            char * y = (char *)malloc(strlenb(s) + 1);
            strcpyb(y,s);
            pch = strtok (y,"=");
            while (pch != '\0')
                {
                    char * NAME = pch;
                        printf("trying \"%s\"\n", y);
                    if (strcmpb(NAME,STTR) == 0 )
                        {
                                printf("  MATCH FOUND=\"%s\"\n", y);
                            char * pch = strtok (NULL, "=");
                            char * VALUE = pch;
                                printf ("VALUE = %s\n",VALUE);
                            return VALUE;
                        }
                    if (strcmpb(NAME,STTR) != 0 )
                        {
                                printf("  \"%s\" did not match \"%s\"\n", NAME,STTR);
                        }
                    break;
                }
            free(y);
        }
        s = *(environ+i);
    }
printf("  \"%s\" COULD NOT BE FOUND\n", STTR);
free(s);
return 0;
}

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

char *
resolve_debug(char * path, char * save_to_variable)
{
printf("called resolve_debug()\n");
        char *cptr = save_to_variable;

    if(!access(path, F_OK)) {
        } else {
        return(0);
        }

    char * pathb = (char *)malloc(strlen(path) + 1);
    char * strcpyb(char *dest, const char *src);
    strcpyb(pathb,path);
    char save_pwd[PATH_MAX];
    getcwd(save_pwd, sizeof(save_pwd));
    char path_separator='/';
    char relpathdot_separator[4]="/./";
    char relpathdotdor_separator[5]="/../";
    char newpathb[PATH_MAX+256];
    char newpathc[PATH_MAX+256];
    char linkb[PATH_MAX+256];
    char linkd[PATH_MAX+256];
    char tmp_pwd[PATH_MAX];
    char current_pwd[PATH_MAX];
    getcwd(current_pwd, sizeof(current_pwd));
        #include <sys/types.h>
        #include <sys/stat.h>
    char* resolvedir(const char * pathb)
    {
        printf("chdir(%s)\n", pathb);
        chdir(pathb);
        printf("getcwd(%s, sizeof(%s))\n", tmp_pwd, tmp_pwd);
        getcwd(tmp_pwd, sizeof(tmp_pwd));
        printf("%s points to %s\n\n", pathb, tmp_pwd);
        printf("chdir(%s)\n", current_pwd);
        chdir(current_pwd);
        printf("return %s\n", tmp_pwd);
        return tmp_pwd;
    }
    char* resolvefile(char * pathb)
    {
        printf("strncpyb(%s, %s, sizeof(%s)\n", linkb, pathb, linkb);
        strncpyb(linkb, pathb, sizeof(linkb));
        printf("linkb[sizeof(%s)-1]=0\n", linkb);
        linkb[sizeof(linkb)-1]=0;
        printf("strncpyb(%s, %s, sizeof(%s)\n", linkd, pathb, linkb);
        strncpyb(linkd, pathb, sizeof(linkb));
        printf("linkb[sizeof(%s)-1]=0\n", linkb);
        linkb[sizeof(linkb)-1]=0;
        printf("dirname(%s)\n", linkd);
        dirname(linkd);
        printf("strncatb(%s, \"/\", sizeof(%s));\n", linkd, linkd);
        strncatb(linkd, "/", sizeof(linkd));
        printf("linkd[sizeof(%s)-1]=0\n", linkd);
        linkd[sizeof(linkd)-1]=0;
        printf("chdir(%s)\n", linkd);
        chdir(linkd);
        printf("getcwd(%s, sizeof(%s))\n", tmp_pwd, tmp_pwd);
        getcwd(tmp_pwd, sizeof(tmp_pwd));
        printf("strncatb(%s, \"/\", sizeof(%s));\n", tmp_pwd, tmp_pwd);
        strncatb(tmp_pwd, "/", sizeof(tmp_pwd));
        printf("tmp_pwd[sizeof(%s)-1]=0\n", tmp_pwd);
        tmp_pwd[sizeof(tmp_pwd)-1]=0;
        printf("strncpyb(%s, %s, sizeof(%s));\n", linkb, basename(pathb), linkb);
        strncpyb(linkb, basename(pathb), sizeof(linkb));
        printf("linkb[sizeof(%s)-1]=0\n", linkb);
        linkb[sizeof(linkb)-1]=0;
        printf("strncatb(%s, %s, sizeof(%s));\n", tmp_pwd, linkb, tmp_pwd);
        strncatb(tmp_pwd, linkb, sizeof(tmp_pwd));
        printf("tmp_pwd[sizeof(%s)-1]=0\n", tmp_pwd);
        tmp_pwd[sizeof(tmp_pwd)-1]=0;
        printf("%s points to %s\n\n", pathb, tmp_pwd);
        printf("chdir(%s)\n", current_pwd);
        chdir(current_pwd);
        printf("return %s\n", tmp_pwd);
        return tmp_pwd;
    }
    #include <sys/types.h>
    #include <sys/stat.h>
    char * getlink(const char * link)
    {
        struct stat p_statbuf;
        if (lstat(link,&p_statbuf)==0) {
                printf("%s type is <int>\n",link, S_ISLNK(p_statbuf.st_mode));
            if (S_ISLNK(p_statbuf.st_mode)==1)
            {
                printf("%s is symbolic link \n", link);
            } else
            {
                printf("%s is not symbolic link \n", link);
                return 0;
            }
        }
        struct stat sb;
        char *linkname;
        ssize_t r;

        if (lstat(link, &sb) == -1)
        {
                _exit(EXIT_FAILURE);
        }

        linkname = malloc(sb.st_size + 1);
            if (linkname == NULL)
            {
                _exit(EXIT_FAILURE);
            }

        r = readlink(link, linkname, sb.st_size + 1);

        if (r < 0)
        {
                _exit(EXIT_FAILURE);
        }

        if (r > sb.st_size)
        {
                _exit(EXIT_FAILURE);
        }

        linkname[sb.st_size] = '\0';

    printf("\"%s\" points to '%s'\n", link, linkname);

        path = linkname;
        char * checkifsymlink(const char * tlink)
        {
            struct stat p_statbuf;
            if (lstat(tlink,&p_statbuf)==0)
            {
                    printf("%s type is <int>\n",tlink, S_ISLNK(p_statbuf.st_mode));
                if (S_ISLNK(p_statbuf.st_mode)==1)
                {
                    printf("%s is symbolic link \n", tlink);
                    printf("called getlink()\n");
                    getlink(tlink);
                } else
                {
                    printf("%s is not symbolic link \n", tlink);
                    return 0;
                }
            }
        return 0;
        }
    printf("called checkifsymlink()\n");
        checkifsymlink(path);
        return 0;
    }
printf("called getlink()\n");
    getlink(path);
    char * testtype(const char * patha)
    {
        int is_regular_file(const char *patha)
        {
            struct stat path_stat;
            stat(patha, &path_stat);
            return S_ISREG(path_stat.st_mode);
        }
        int isDirectory(const char *patha)
        {
            struct stat statbuf;
            if (stat(patha, &statbuf) != 0)
                return 0;
            return S_ISDIR(statbuf.st_mode);
        }
        if (is_regular_file(patha)==1)
        {
        printf("%s is file \n", patha);
            if (path[0]==path_separator)
            {
                if ( strstrb(path, relpathdot_separator ))
                {
                    printf("    %s is an absolute path which contains a dot relative path\n", path);
                    printf("called Rresolvefile()\n");
                    return resolvefile(path);
                } else if ( strstrb(path, relpathdotdor_separator ))
                {
                    printf("    %s is an absolute path which contains a dot dot relative path\n", path);
                    printf("called resolvefile()\n");
                return resolvefile(path);
                } else
                {
                    printf("    %s is an absolute path with no relative paths\n", path);
                    return path;
                }
                return 0;
            } else if ( strchrb(path, path_separator ))
            {
                printf("    %s is a relative path\n", path);
                strncpyb(newpathb, current_pwd, sizeof(newpathb));
                newpathb[sizeof(newpathb)-1]=0;
                strncatb(newpathb, "/", sizeof(newpathb));
                newpathb[sizeof(newpathb)-1]=0;
                strncatb(newpathb, path, sizeof(newpathb));
                newpathb[sizeof(newpathb)-1]=0;
                printf("called resolvefile()\n");
                resolvefile(newpathb);
                return 0;
            } else
            {
                printf("could not determine path type of %s\n", path);
                return 1;
            }
        } else if (isDirectory(patha)==1)
        {
        printf("%s is a directory \n", patha);
            if (path[0]==path_separator)
            {
                if ( strstrb(path, relpathdot_separator ))
                {
                    printf("    %s is an absolute path which contains a dot relative path\n", path);
                    printf("called resolvedir()\n");
                    resolvedir(path);
                } else if ( strstrb(path, relpathdotdor_separator ))
                {
                    printf("    %s is an absolute path which contains a dot dot relative path\n", path);
                    printf("called resolvedir()\n");
                    resolvedir(path);
                } else
                {
                    printf("    %s is an absolute path with no relative paths\n", path);
                    return path;
                }
                return 0;
            } else if ( strchrb(path, path_separator ))
            {
                printf("    %s is a relative path\n", path);
                printf("    strncpyb(%s, %s, sizeof(%s));\n", newpathc, current_pwd, newpathc);
                strncpyb(newpathc, current_pwd, sizeof(newpathc));
                printf("    newpath2[sizeof(%s)-1]=0;\n", newpathc);
                newpathc[sizeof(newpathc)-1]=0;
                printf("    strncatb(%s, %s, sizeof(%s));\n", newpathc, "/", newpathc);
                strncatb(newpathc, "/", sizeof(newpathc));
                printf("    newpathc[sizeof(%s)-1]=0;\n", newpathc);
                newpathc[sizeof(newpathc)-1]=0;
                printf("    strncatb(%s, %s, sizeof(%s));\n", newpathc, path, newpathc);
                strncatb(newpathc, path, sizeof(newpathc));
                printf("    newpathc[sizeof(%s)-1]=0;\n", newpathc);
                newpathc[sizeof(newpathc)-1]=0;
                printf("called resolvedir()\n");
                resolvedir(newpathc);
                return 0;
            } else
            {
                printf("could not determine path type of %s\n", path);
                return 1;
            }
        }
        return 0;
    }
printf("called testtype()\n");
    save_to_variable = testtype(path);
printf("print save_to_variable from resolve_debug() %s\n", save_to_variable);
    while (*cptr) {
        cptr++;
    }
    while (*save_to_variable) {
        *cptr++ = *save_to_variable++;
    }
    *cptr = *save_to_variable;

    return cptr;
}


// "look deep into yourself, Clarice"  -- Hanibal Lector
char findyourself_save_pwd[PATH_MAX];
char findyourself_save_argv0[PATH_MAX];
char findyourself_save_path[PATH_MAX];
char findyourself_path_separator='/';
char findyourself_path_separator_as_string[2]="/";
char findyourself_path_list_separator[8]=":";  // could be ":; "
char findyourself_debug=2;
char findyourself_verbose=1;

int findyourself_initialized=0;

void findyourself_init_debug(char *argv0)
{

if(findyourself_verbose) printf("  getcwd(%s,sizeof(%s));\n", findyourself_save_pwd, findyourself_save_pwd);
  getcwd(findyourself_save_pwd, sizeof(findyourself_save_pwd));
if(findyourself_verbose) printf("  getcwd(%s,sizeof(%s));\n", findyourself_save_pwd, findyourself_save_pwd);

if(findyourself_verbose) printf("  strncpyb(%s, %s, sizeof(%s));\n", findyourself_save_argv0, argv0, findyourself_save_argv0);
  strncpyb(findyourself_save_argv0, argv0, sizeof(findyourself_save_argv0));
if(findyourself_verbose) printf("  findyourself_save_argv0[sizeof(%s)-1)=0;\n", findyourself_save_argv0);
  findyourself_save_argv0[sizeof(findyourself_save_argv0)-1]=0;

if(findyourself_verbose) printf("  strncpyb(%s, getenv_debug(\"PATH\"), sizeof(%s));\n", findyourself_save_path, findyourself_save_path);
  strncpyb(findyourself_save_path, getenv_debug("PATH"), sizeof(findyourself_save_path));
if(findyourself_verbose) printf("  findyourself_save_path[sizeof(%s)-1)=0;\n", findyourself_save_path);
  findyourself_save_path[sizeof(findyourself_save_path)-1]=0;
if(findyourself_verbose) printf("  findyourself_initialized=1\n");
  findyourself_initialized=1;
}


int find_yourself_debug(char *result, size_t size_of_result)
{
  char newpath[PATH_MAX+256];
  char newpath2[PATH_MAX+256];

if(findyourself_verbose) printf("  assert(1)\n");
  assert(findyourself_initialized);
if(findyourself_verbose) printf("  result[0]=0\n");
  result[0]=0;
if(findyourself_verbose) printf("  if(%s==<char>) {\n",findyourself_save_argv0, findyourself_path_separator);
  if(findyourself_save_argv0[0]==findyourself_path_separator) {
    if(findyourself_debug) printf("     absolute path\n");
if(findyourself_verbose) printf("     resolve_debug(%s);\n",findyourself_save_argv0);
     resolve_debug(findyourself_save_argv0, newpath);
     if(findyourself_debug) printf("     newpath=\"%s\"\n", newpath);
if(findyourself_verbose) printf("     if(!access(%s, F_OK)) {\n", newpath);
     if(!access(newpath, F_OK)) {
if(findyourself_verbose) printf("        strncpyb(%s, %s, <int>);\n", result, newpath, size_of_result);
        strncpyb(result, newpath, size_of_result);
if(findyourself_verbose) printf("        result[<int>-1]=0;\n", size_of_result);
        result[size_of_result-1]=0;
if(findyourself_verbose) printf("        return(0);\n");
        return(0);
     } else {
if(findyourself_verbose) printf("     } else {\n");
if(findyourself_verbose) printf("    perror(\"access failed 1\");\n");
if(findyourself_verbose) printf("      }\n");
      }
  } else if( strchrb(findyourself_save_argv0, findyourself_path_separator )) {
if(findyourself_verbose) printf("  } else if( strchrb(%s, <char> )) {\n",findyourself_save_argv0, findyourself_path_separator );
    if(findyourself_debug) printf("    relative path to pwd\n");
if(findyourself_verbose) printf("    strncpyb(%s, %s, sizeof(%s));\n", newpath2, findyourself_save_pwd, newpath2);
    strncpyb(newpath2, findyourself_save_pwd, sizeof(newpath2));
if(findyourself_verbose) printf("    newpath2[sizeof(%s)-1]=0;\n", newpath2);
    newpath2[sizeof(newpath2)-1]=0;
if(findyourself_verbose) printf("    strncatb(%s, %s, sizeof(%s));\n", newpath2, findyourself_path_separator_as_string, newpath2);
    strncatb(newpath2, findyourself_path_separator_as_string, sizeof(newpath2));
if(findyourself_verbose) printf("    newpath2[sizeof(%s)-1]=0;\n", newpath2);
    newpath2[sizeof(newpath2)-1]=0;
if(findyourself_verbose) printf("    strncatb(%s, %s, sizeof(%s));\n", newpath2, findyourself_save_argv0, newpath2);
    strncatb(newpath2, findyourself_save_argv0, sizeof(newpath2));
if(findyourself_verbose) printf("    newpath2[sizeof(%s)-1]=0;\n", newpath2);
    newpath2[sizeof(newpath2)-1]=0;
if(findyourself_verbose) printf("    resolve_debug(%s);\n",newpath2);
    resolve_debug(newpath2, newpath);
    if(findyourself_debug) printf("    newpath=\"%s\"\n", newpath);
if(findyourself_verbose) printf("    if(!access(%s, F_OK)) {\n", newpath);
    if(!access(newpath, F_OK)) {
if(findyourself_verbose) printf("        strncpyb(%s, %s, <int>);\n", result, newpath, size_of_result);
        strncpyb(result, newpath, size_of_result);
if(findyourself_verbose) printf("        result[<int>-1]=0;\n", size_of_result);
        result[size_of_result-1]=0;
if(findyourself_verbose) printf("        return(0);\n");
        return(0);
     } else {
if(findyourself_verbose) printf("     } else {\n");
if(findyourself_verbose) printf("    perror(\"access failed 2\");\n");
if(findyourself_verbose) printf("      }\n");
      }
  } else {
if(findyourself_verbose) printf("  } else {\n");
    if(findyourself_debug) printf("    searching $PATH\n");
    char *saveptr;
    char *pathitem;
if(findyourself_verbose) printf("    for(pathitem=strtok_r(%s, %s,  %s); %s; pathitem=strtok_r(NULL, %s, %s) ) {;\n", findyourself_save_path, findyourself_path_list_separator,  &saveptr, pathitem, findyourself_path_list_separator, &saveptr);
    for(pathitem=strtok_r(findyourself_save_path, findyourself_path_list_separator,  &saveptr); pathitem; pathitem=strtok_r(NULL, findyourself_path_list_separator, &saveptr) ) {
       if(findyourself_debug>=2) printf("       pathitem=\"%s\"\n", pathitem);
if(findyourself_verbose) printf("       strncpyb(%s, %s, sizeof(%s));\n", newpath2, pathitem, newpath2);
       strncpyb(newpath2, pathitem, sizeof(newpath2));
if(findyourself_verbose) printf("       newpath2[sizeof(%s)-1]=0;\n", newpath2);
       newpath2[sizeof(newpath2)-1]=0;
if(findyourself_verbose) printf("       strncatb(%s, %s, sizeof(%s));\n", newpath2, findyourself_path_separator_as_string, newpath2);
       strncatb(newpath2, findyourself_path_separator_as_string, sizeof(newpath2));
if(findyourself_verbose) printf("       newpath2[sizeof(%s)-1]=0;\n", newpath2);
       newpath2[sizeof(newpath2)-1]=0;
if(findyourself_verbose) printf("       strncatb(%s, %s, sizeof(%s));\n", newpath2, findyourself_save_argv0, newpath2);
       strncatb(newpath2, findyourself_save_argv0, sizeof(newpath2));
if(findyourself_verbose) printf("       newpath2[sizeof(%s)-1]=0;\n", newpath2);
       newpath2[sizeof(newpath2)-1]=0;
if(findyourself_verbose) printf("       resolve_debug(%s);\n",newpath2);
       resolve_debug(newpath2, newpath);
       if(findyourself_debug) printf("       newpath=\"%s\"\n", newpath);
if(findyourself_verbose) printf("       if(!access(%s, F_OK)) {\n", newpath);
       if(!access(newpath, F_OK)) {
if(findyourself_verbose) printf("          strncpyb(%s, %s, <int>);\n", result, newpath, size_of_result);
          strncpyb(result, newpath, size_of_result);
if(findyourself_verbose) printf("          result[<int>-1]=0;\n", size_of_result);
          result[size_of_result-1]=0;
if(findyourself_verbose) printf("          return(0);\n");
if(findyourself_verbose) printf("      }\n");
          return(0);
      }
if(findyourself_verbose) printf("    }\n");
    } // end for
if(findyourself_verbose) printf("    perror(\"access failed 3\");\n");
if(findyourself_verbose) printf("  }\n");
  } // end else
  // if we get here, we have tried all three methods on argv[0] and still haven't succeeded.   Include fallback methods here.
if(findyourself_verbose) printf("  return(1);\n");
if(findyourself_verbose) printf("}\n");
  return(1);
}

#define error_msg printf
int
string_quote(const char *instr, char *outstr, const unsigned int size, const unsigned int style);

#ifndef ALLOCA_CUTOFF
# define ALLOCA_CUTOFF	4032
#endif
#define use_alloca(n) ((n) <= ALLOCA_CUTOFF)

/*
* Quote string `str' of length `size' and print the result.
*
* If QUOTE_0_TERMINATED `style' flag is set,
* treat `str' as a NUL-terminated string and
* quote at most (`size' - 1) bytes.
*
* If QUOTE_OMIT_LEADING_TRAILING_QUOTES `style' flag is set,
* do not add leading and trailing quoting symbols.
*
* Returns 0 if QUOTE_0_TERMINATED is set and NUL was seen, 1 otherwise.
* Note that if QUOTE_0_TERMINATED is not set, always returns 1.
*/
char *
print_quoted_string(const char *str, unsigned int size, const unsigned int style, const char * return_type)
{
    char *buf;
    char *outstr;
    unsigned int alloc_size;
    int rc;

    if (size && style & QUOTE_0_TERMINATED)
        --size;

    alloc_size = 4 * size;
    if (alloc_size / 4 != size) {
        error_msg("Out of memory");
        printf("???");
        return "-1";
    }
    alloc_size += 1 + (style & QUOTE_OMIT_LEADING_TRAILING_QUOTES ? 0 : 2);

    if (use_alloca(alloc_size)) {
        outstr = alloca(alloc_size);
        buf = NULL;
    } else {
        outstr = buf = malloc(alloc_size);
        if (!buf) {
            error_msg("Out of memory");
            printf("???");
            return "-1";
        }
    }

//         rc = string_quote(str, outstr, size, style);
    string_quote(str, outstr, size, style);
    if ( return_type == "return") {
        return outstr;
    } else if ( return_type == "print") {
        printf(outstr);
    }

    free(buf);
//         return rc;
}
void lseek_string(char **src, int len, int offset) {
    char *p = malloc(len);
    memcpy(p, *src+offset, len);
    *src = p;
}

// not used but kept incase needed, a version of lseek_string that has an offset multiplier as so this does not need to be specified multiple times, eg if offset is 64 and multiplier is 2 the offset is then 128, this is intended for loops and related
void lseek_stringb(char **src, int len, int offset, int offset_multiplier) {
    char *p = malloc(len);
    int off;
    off=((len*offset_multiplier));
    memcpy(p, *src+offset+off, len);
    *src = p;
}
// TODO: impliment pread:
// [ ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset); ]
// [ The pread() function shall be equivalent to read(), except that it shall read from a given position in the file without changing the file pointer. The first three arguments to pread() are the same as read() with the addition of a fourth argument offset for the desired position inside the file. An attempt to perform a pread() on a file that is incapable of seeking shall result in an error. ]


// reads a string instead of a file descriptor
// [ The  read()  function shall attempt to read nbyte bytes from the file associated with the open file descriptor, fildes, into the buffer pointed to by buf. The behavior of multiple concurrent reads on the same pipe, FIFO, or terminal device is unspecified. ]
ssize_t read_(const char *src, char **dest, size_t len) {
    char *p = malloc(len + 1);
    memcpy(p, src, len);
    p[len] = 0;
    *dest = p;
    return len;
}

ssize_t read_mem(const char *src, void *dest, size_t len) {
    memcpy(dest, src, len);
    src += len;
    return len;
}

// reads a string instead of a file descriptor
int read_fast(const char *src, char *dest, int len) {
    memcpy(dest, src, len);
    return len;
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

// struct fd_ { const char *src; }; ssize_t read_fd_mem(struct fd_ *fd, void *dest, size_t len) { memcpy(dest, fd->src, len); fd->src += len; return len; }


// TODO: adapt read_ to fread
// [ size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream); ]
// [ The function fread() reads nmemb elements of data, each size bytes long, from the stream pointed to by stream, storing them at the location given by ptr. ]
// basically means if i do [ fread(ptr, 5, 5, stdin); ] it would read the first 25 bytes from stdin into ptr
size_t sizeof_(const char str[]) {
    return sizeof(str);
}

void lseek_stringc(char **src, int len, int offset, int offsetT) {
    char *p = malloc(len);
    int off;
    off=((len*offsetT));
    fprintf(stderr, "off = %d * %d, = %d\n", len, offsetT, off);
    memcpy(p, *src+offset+off, strlen(*src));
    *src = p;
}

// [ size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream); ]
// [ The function fread() reads nmemb elements of data, each size bytes long, from the stream pointed to by stream, storing them at the location given by ptr. ]
// basically means if i do [ fread(ptr, 5, 5, stdin); ] it would read the first 25 bytes from stdin into ptr
size_t fread_(void **dest, size_t size, size_t num, const char *src) {
    fprintf(stderr, "1\n");
    size_t len = size;
    fprintf(stderr, "2\n");
    char *p = malloc(len + 1);
    fprintf(stderr, "3\n");
    memcpy(p, src, len);
    fprintf(stderr, "4\n");
    p[len] = 0;
    fprintf(stderr, "5\n");
    *dest = p;
    fprintf(stderr, "6, dest = %c\n", *p);
    sleep(1);
    lseek_stringc(&src, 1, 0, 1);
    return num;
}



char *strjoin(const char *_a, const char *_b, int _a_len, int len) {
    size_t na = _a_len;
    size_t nb = len;
    char *p = malloc(na + nb + 1);
    memcpy(p, _a, na);
    memcpy(p + na, _b, nb);
    p[na + nb] = 0;
    return p;
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

int stream__(char *file, char **p, int *q, int LINES_TO_READ) {
            const char *filename = file;
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                printf("cannot open \"%s\", returned %i\n", filename, fd);
                return -1;
            }
            char * array;
            char ch;
            size_t lines = 1;
            // Read the file byte by byte
            int bytes=1;
            int count=1;
            array = malloc(sizeof(char) * 2048);
            char *array_tmp;
            while (read(fd, &ch, 1) == 1) {
            printf("\rbytes read: %'i", bytes);
                if (count == 1024) { array_tmp = realloc(array, bytes+1024);
                    if (array_tmp == NULL) {
                        printf("failed to allocate array to new size");
                        free(array);
                        exit(1);
                    } else {
                        array = array_tmp;
                    }
                    count=1;
                }
                array[bytes-1] = ch;
                if (ch == '\n') {
                    if (lines == LINES_TO_READ) {
                        break;
                    }
                    lines++;
                }
                count++;
                bytes++;
            }
            bytes--;
            array_tmp = realloc(array, bytes);
            if (array_tmp == NULL) {
                printf("failed to allocate array to new size");
                free(array);
                exit(1);
            } else {
                array = array_tmp;
            }
            printf("\rbytes read: %'i\n", bytes);
    *p = array;
    *q = bytes;
    return bytes;
}

// not used but kept incase needed, a version of stream__ that only outputs the last line read
int stream__o(char *file, char **p, int *q, int LINES_TO_READ) {
            const char *filename = file;
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                printf("cannot open \"%s\", returned %i\n", filename, fd);
                return -1;
            }
            char * array;
            char * array_tmp;
            char * array_lines;
            char * array_lines_tmp;
            char ch;
            size_t lines = 1;
            // Read the file byte by byte
            int bytes=1;
            int count=1;
            array = malloc(sizeof(char) * 2048);
            while (read(fd, &ch, 1) == 1) {
            printf("\rbytes read: %'i", bytes);
                if (count == 1024) { array_tmp = realloc(array, bytes+1024);
                    if (array_tmp == NULL) {
                        printf("failed to allocate array to new size");
                        free(array);
                        exit(1);
                    } else {
                        array = array_tmp;
                    }
                    count=1;
                }
                array[bytes-1] = ch;
                if (ch == '\n') {
                    printf("attempting to reset array\n");
                    if (lines == LINES_TO_READ) {
                        break;
                    } else {
                        // reset array to as if we just executed this function
                        int y;
                        for (y=0; y<bytes; y++) {
                            array[y] = 0;
                        }
                        free(array);
                        array = malloc(sizeof(char) * 2048);
                        bytes=1;
                        count=1;
                    }
                    lines++;
                }
//                 count++;
                bytes++;
            }
            bytes--;
            array_tmp = realloc(array, bytes);
            if (array_tmp == NULL) {
                printf("failed to allocate array to new size");
                free(array);
                exit(1);
            } else {
                array = array_tmp;
            }
            printf("\rbytes read: %'i\n", bytes);
    *p = array;
    *q = bytes;
    return bytes;
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

// reads a entire file
int read__(char *file, char **p, size_t *q) {
    int fd;
    size_t len = 0;
    char *o;
    if (!(fd = open(file, O_RDONLY)))
    {
        fprintf(stderr, "open() failure\n");
        return (1);
    }
    len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, 0);
    if (!(o = malloc(len))) {
        fprintf(stderr, "failure to malloc()\n");
    }
    if ((read(fd, o, len)) == -1) {
        fprintf(stderr, "failure to read()\n");
    }
    int cl = close(fd);
    if (cl < 0) {
        printf("cannot close \"%s\", returned %i\n", file, cl);
        return -1;
    }
    *p = o;
    *q = len;
    return len;
}

void findyourself_init_(char *argv0)
{

  getcwd(findyourself_save_pwd, sizeof(findyourself_save_pwd));

  strncpy(findyourself_save_argv0, argv0, sizeof(findyourself_save_argv0));
  findyourself_save_argv0[sizeof(findyourself_save_argv0)-1]=0;

  strncpy(findyourself_save_path, getenv("PATH"), sizeof(findyourself_save_path));
  findyourself_save_path[sizeof(findyourself_save_path)-1]=0;
  findyourself_initialized=1;
}


int find_yourself_(char *result, size_t size_of_result)
{
  char newpath[PATH_MAX+256];
  char newpath2[PATH_MAX+256];

  assert(findyourself_initialized);
  result[0]=0;

  if(findyourself_save_argv0[0]==findyourself_path_separator) {
    if(findyourself_debug) printf("  absolute path\n");
     realpath(findyourself_save_argv0, newpath);
     if(findyourself_debug) printf("  newpath=\"%s\"\n", newpath);
     if(!access(newpath, F_OK)) {
        strncpy(result, newpath, size_of_result);
        result[size_of_result-1]=0;
        return(0);
     } else {
    perror("access failed 1");
      }
  } else if( strchr(findyourself_save_argv0, findyourself_path_separator )) {
    if(findyourself_debug) printf("  relative path to pwd\n");
    strncpy(newpath2, findyourself_save_pwd, sizeof(newpath2));
    newpath2[sizeof(newpath2)-1]=0;
    strncat(newpath2, findyourself_path_separator_as_string, sizeof(newpath2));
    newpath2[sizeof(newpath2)-1]=0;
    strncat(newpath2, findyourself_save_argv0, sizeof(newpath2));
    newpath2[sizeof(newpath2)-1]=0;
    realpath(newpath2, newpath);
    if(findyourself_debug) printf("  newpath=\"%s\"\n", newpath);
    if(!access(newpath, F_OK)) {
        strncpy(result, newpath, size_of_result);
        result[size_of_result-1]=0;
        return(0);
     } else {
    perror("access failed 2");
      }
  } else {
    if(findyourself_debug) printf("  searching $PATH\n");
    char *saveptr;
    char *pathitem;
    for(pathitem=strtok_r(findyourself_save_path, findyourself_path_list_separator,  &saveptr); pathitem; pathitem=strtok_r(NULL, findyourself_path_list_separator, &saveptr) ) {
       if(findyourself_debug>=2) printf("pathitem=\"%s\"\n", pathitem);
       strncpy(newpath2, pathitem, sizeof(newpath2));
       newpath2[sizeof(newpath2)-1]=0;
       strncat(newpath2, findyourself_path_separator_as_string, sizeof(newpath2));
       newpath2[sizeof(newpath2)-1]=0;
       strncat(newpath2, findyourself_save_argv0, sizeof(newpath2));
       newpath2[sizeof(newpath2)-1]=0;
       realpath(newpath2, newpath);
       if(findyourself_debug) printf("  newpath=\"%s\"\n", newpath);
      if(!access(newpath, F_OK)) {
          strncpy(result, newpath, size_of_result);
          result[size_of_result-1]=0;
          return(0);
      } 
    } // end for
    perror("access failed 3");

  } // end else
  // if we get here, we have tried all three methods on argv[0] and still haven't succeeded.   Include fallback methods here.
  return(1);
}

char * get_full_path() {
    findyourself_init_((char *)getauxval(AT_EXECFN));
    char * auxAT = (char *)getauxval(AT_EXECFN);
    char newpath[PATH_MAX];
    find_yourself_(newpath, sizeof(newpath));
    if(1 || strcmp((char *)getauxval(AT_EXECFN),newpath)) { }
    char *fullpath  = strdup( newpath );
    char *directorypath = dirname( strdup( newpath ) );
    printf("current = %s\nfullpath = %s\ndirname = %s\n", auxAT, fullpath, directorypath);
    return fullpath;
}

int *
resolve_and_re_execute_debug (char * path)
{
printf("called resolve_debug()\n");
    if(!access(path, F_OK)) {
        } else {
        return -1;
        }
    char * pathb = (char *)malloc(strlen(path) + 1);
    char * strcpyb(char *dest, const char *src);
    strcpyb(pathb,path);
    char save_pwd[PATH_MAX];
    getcwd(save_pwd, sizeof(save_pwd));
    char path_separator='/';
    char relpathdot_separator[4]="/./";
    char relpathdotdor_separator[5]="/../";
    char newpathb[PATH_MAX+256];
    char newpathc[PATH_MAX+256];
    char linkb[PATH_MAX+256];
    char linkd[PATH_MAX+256];
    char tmp_pwd[PATH_MAX];
    char current_pwd[PATH_MAX];
    getcwd(current_pwd, sizeof(current_pwd));
        #include <sys/types.h>
        #include <sys/stat.h>
    char* resolvedir(const char * pathb)
    {
        printf("chdir(%s)\n", pathb);
        chdir(pathb);
        printf("getcwd(%s, sizeof(%s))\n", tmp_pwd, tmp_pwd);
        getcwd(tmp_pwd, sizeof(tmp_pwd));
        printf("%s points to %s\n\n", pathb, tmp_pwd);
        printf("chdir(%s)\n", current_pwd);
        chdir(current_pwd);
        printf("return %s\n", tmp_pwd);
        return tmp_pwd;
    }
    char* resolvefile(char * pathb)
    {
        printf("strncpyb(%s, %s, sizeof(%s)\n", linkb, pathb, linkb);
        strncpyb(linkb, pathb, sizeof(linkb));
        printf("linkb[sizeof(%s)-1]=0\n", linkb);
        linkb[sizeof(linkb)-1]=0;
        printf("strncpyb(%s, %s, sizeof(%s)\n", linkd, pathb, linkb);
        strncpyb(linkd, pathb, sizeof(linkb));
        printf("linkb[sizeof(%s)-1]=0\n", linkb);
        linkb[sizeof(linkb)-1]=0;
        printf("dirname(%s)\n", linkd);
        dirname(linkd);
        printf("strncatb(%s, \"/\", sizeof(%s));\n", linkd, linkd);
        strncatb(linkd, "/", sizeof(linkd));
        printf("linkd[sizeof(%s)-1]=0\n", linkd);
        linkd[sizeof(linkd)-1]=0;
        printf("chdir(%s)\n", linkd);
        chdir(linkd);
        printf("getcwd(%s, sizeof(%s))\n", tmp_pwd, tmp_pwd);
        getcwd(tmp_pwd, sizeof(tmp_pwd));
        printf("strncatb(%s, \"/\", sizeof(%s));\n", tmp_pwd, tmp_pwd);
        strncatb(tmp_pwd, "/", sizeof(tmp_pwd));
        printf("tmp_pwd[sizeof(%s)-1]=0\n", tmp_pwd);
        tmp_pwd[sizeof(tmp_pwd)-1]=0;
        printf("strncpyb(%s, %s, sizeof(%s));\n", linkb, basename(pathb), linkb);
        strncpyb(linkb, basename(pathb), sizeof(linkb));
        printf("linkb[sizeof(%s)-1]=0\n", linkb);
        linkb[sizeof(linkb)-1]=0;
        printf("strncatb(%s, %s, sizeof(%s));\n", tmp_pwd, linkb, tmp_pwd);
        strncatb(tmp_pwd, linkb, sizeof(tmp_pwd));
        printf("tmp_pwd[sizeof(%s)-1]=0\n", tmp_pwd);
        tmp_pwd[sizeof(tmp_pwd)-1]=0;
        printf("%s points to %s\n\n", pathb, tmp_pwd);
        printf("chdir(%s)\n", current_pwd);
        chdir(current_pwd);
        printf("return %s\n", tmp_pwd);
        return tmp_pwd;
    }
    #include <sys/types.h>
    #include <sys/stat.h>
    char * getlink(const char * link)
    {
        struct stat p_statbuf;
        if (lstat(link,&p_statbuf)==0) {
                printf("%s type is <int>\n",link, S_ISLNK(p_statbuf.st_mode));
            if (S_ISLNK(p_statbuf.st_mode)==1)
            {
                printf("%s is symbolic link \n", link);
            } else
            {
                printf("%s is not symbolic link \n", link);
                return 0;
            }
        }
        struct stat sb;
        char *linkname;
        ssize_t r;

        if (lstat(link, &sb) == -1)
        {
                _exit(EXIT_FAILURE);
        }

        linkname = malloc(sb.st_size + 1);
            if (linkname == NULL)
            {
                _exit(EXIT_FAILURE);
            }

        r = readlink(link, linkname, sb.st_size + 1);

        if (r < 0)
        {
                _exit(EXIT_FAILURE);
        }

        if (r > sb.st_size)
        {
                _exit(EXIT_FAILURE);
        }

        linkname[sb.st_size] = '\0';

        printf("\"%s\" points to '%s'\n", link, linkname);

        path = linkname;
        char * checkifsymlink(const char * tlink)
        {
            struct stat p_statbuf;
            if (lstat(tlink,&p_statbuf)==0)
            {
                    printf("%s type is <int>\n",tlink, S_ISLNK(p_statbuf.st_mode));
                if (S_ISLNK(p_statbuf.st_mode)==1)
                {
                        printf("%s is symbolic link \n", tlink);
                        printf("called getlink()\n");
                    getlink(tlink);
                } else
                {
                        printf("%s is not symbolic link \n", tlink);
                    return 0;
                }
            }
        return 0;
        }
        printf("called checkifsymlink()\n");
        checkifsymlink(path);
        return 0;
    }
    printf("called getlink()\n");
    getlink(path);
    char * testtype(const char * patha)
    {
        int is_regular_file(const char *patha)
        {
            struct stat path_stat;
            stat(patha, &path_stat);
            return S_ISREG(path_stat.st_mode);
        }
        int isDirectory(const char *patha)
        {
            struct stat statbuf;
            if (stat(patha, &statbuf) != 0)
                return 0;
            return S_ISDIR(statbuf.st_mode);
        }
        if (is_regular_file(patha)==1)
        {
        printf("%s is file \n", patha);
            if (path[0]==path_separator)
            {
                if ( strstrb(path, relpathdot_separator ))
                {
                    printf("%s is an absolute path which contains a dot relative path\n", path);
                    printf("called Rresolvefile()\n");
                    return resolvefile(path);
                } else if ( strstrb(path, relpathdotdor_separator ))
                {
                    printf("%s is an absolute path which contains a dot dot relative path\n", path);
                    printf("called resolvefile()\n");
                return resolvefile(path);
                } else
                {
                    printf("%s is an absolute path with no relative paths\n", path);
                    return path;
                }
            } else if ( strchrb(path, path_separator ))
            {
                printf("%s is a relative path\n", path);
                strncpyb(newpathb, current_pwd, sizeof(newpathb));
                newpathb[sizeof(newpathb)-1]=0;
                strncatb(newpathb, "/", sizeof(newpathb));
                newpathb[sizeof(newpathb)-1]=0;
                strncatb(newpathb, path, sizeof(newpathb));
                newpathb[sizeof(newpathb)-1]=0;
                    printf("called resolvefile()\n");
                printf("need to re execute\n");
                char * new_aux = resolvefile(newpathb);
                printf("executing with %s\n\n\n\n", new_aux);
                int ret = execv(new_aux, NULL);
                printf("ret = %d\n\n", ret);
                return "ERROR";
            } else
            {
                    printf("could not determine path type of %s\n", path);
                return "NULL";
            }
        } else if (isDirectory(patha)==1)
        {
                printf("%s is a directory \n", patha);
            if (path[0]==path_separator)
            {
                if ( strstrb(path, relpathdot_separator ))
                {
                        printf("%s is an absolute path which contains a dot relative path\n", path);
                        printf("called resolvedir()\n");
                    resolvedir(path);
                } else if ( strstrb(path, relpathdotdor_separator ))
                {
                        printf("%s is an absolute path which contains a dot dot relative path\n", path);
                        printf("called resolvedir()\n");
                    resolvedir(path);
                } else
                {
                        printf("%s is an absolute path with no relative paths\n", path);
                    return path;
                }
            } else if ( strchrb(path, path_separator ))
            {
                printf("%s is a relative path\n", path);
                printf("strncpyb(%s, %s, sizeof(%s));\n", newpathc, current_pwd, newpathc);
                strncpyb(newpathc, current_pwd, sizeof(newpathc));
                printf("newpath2[sizeof(%s)-1]=0;\n", newpathc);
                newpathc[sizeof(newpathc)-1]=0;
                printf("strncatb(%s, %s, sizeof(%s));\n", newpathc, "/", newpathc);
                strncatb(newpathc, "/", sizeof(newpathc));
                printf("newpathc[sizeof(%s)-1]=0;\n", newpathc);
                newpathc[sizeof(newpathc)-1]=0;
                printf("strncatb(%s, %s, sizeof(%s));\n", newpathc, path, newpathc);
                strncatb(newpathc, path, sizeof(newpathc));
                printf("newpathc[sizeof(%s)-1]=0;\n", newpathc);
                newpathc[sizeof(newpathc)-1]=0;
                printf("called resolvedir()\n");
                return resolvedir(newpathc);
            } else
            {
                printf("could not determine path type of %s\n", path);
                return "NULL";
            }
        }
        return "FAILED";
    }
printf("called testtype()\n");
    return testtype(path);
}

int
string_quote(const char *instr, char *outstr, const unsigned int size, const unsigned int style)
{
    const unsigned char *ustr = (const unsigned char *) instr;
    char *s = outstr;
    unsigned int i;
    int usehex, uselen, c;

    int xflag = 0;
    usehex = 0;
    uselen = 0;
    if ((style == 9999)) {
        uselen = 1;
    } else if ((xflag > 1) || (style & QUOTE_FORCE_HEX)) {
        usehex = 1;
    } else if (xflag) {
        /* Check for presence of symbol which require
        to hex-quote the whole string. */
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                break;

            /* Force hex unless c is printable or whitespace */
            if (c > 0x7e) {
                usehex = 1;
                break;
            }
            /* In ASCII isspace is only these chars: "\t\n\v\f\r".
            * They happen to have ASCII codes 9,10,11,12,13.
            */
            if (c < ' ' && (unsigned)(c - 9) >= 5) {
                usehex = 1;
                break;
            }
        }
    }

    if (!(style & QUOTE_OMIT_LEADING_TRAILING_QUOTES))
        *s++ = '\"';

    if (usehex == 1) {
        /* Hex-quote the whole string. */
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                goto asciz_ended;
            // print hex in " 00 00" format instead of "\x00\x00" format
//             *s++ = '\\';
            *s++ = ' ';
            *s++ = "0123456789abcdef"[c >> 4];
            *s++ = "0123456789abcdef"[c & 0xf];
        }
    } else if (uselen == 1) {
        /* Hex-quote the whole string. */
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                goto asciz_ended;
            *s++ = '1';
        }
    } else {
        for (i = 0; i < size; ++i) {
            c = ustr[i];
            /* Check for NUL-terminated string. */
            if (c == 0x100)
                goto asciz_ended;
            if ((i == (size - 1)) &&
                (style & QUOTE_OMIT_TRAILING_0) && (c == '\0'))
                goto asciz_ended;
                int pass_one = 0;
                int pass_two = 0;
                int pass_three = 0;
                int pass_four = 0;
                if (c == '\f') {
                    *s++ = '\\';
                    *s++ = 'f';
                    pass_one = 1;
                    pass_three = 1;
                    pass_four= 1;
                }
                if (pass_one == 0) {
                    if (c == '%'/*FOR PRINTF*/) {
                        *s++ = '%';
                        *s++ = '%';
                        pass_two = 1;
                        pass_three = 1;
                        pass_four= 1;
                    } else {
                        pass_two = 1;
                    }
                }
                if (pass_two == 0) {
                    if (c == '\"') {
                        /*FOR PRINTF/SHELL*/
                        *s++ = '\\';
                        *s++ = '\"';
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '\\') {
                        /*FOR PRINTF/SHELL*/
                        *s++ = '\\';
                        *s++ = '\\';
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '`'/*FOR PRINTF*/|| c == '$'/*FOR BASH*/) {
//                             *s++ = '\\';
                        *s++ = c;
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '\''/*FOR PRINTF*/) {
//                             *s++ = '\\';
//                             *s++ = 'x';
//                             *s++ = '2';
                        *s++ = c;
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '!'/*FOR BASH*/ || c ==  '-'/*FOR PRINTF*/) {
//                             *s++ = '"';
//                             *s++ = '\'';
                        *s++ = c;
//                             *s++ = '\'';
//                             *s++ = '"';
                        pass_three = 1;
                        pass_four= 1;
                    } else if (c == '%'/*FOR PRINTF*/) {
                        *s++ = '%';
                        *s++ = '%';
                        *s++ = '%';
                        *s++ = '%';
                        pass_three = 1;
                        pass_four= 1;
                    }
                }
                if (pass_three == 0) {
                    if (c == '\n') {
                        *s++ = '\\';
                        *s++ = 'n';
                        pass_four = 1;
                    } else if (c == '\r') {
                        *s++ = '\\';
                        *s++ = 'r';
                        pass_four = 1;
                    } else if (c == '\t') {
                        *s++ = '\\';
                        *s++ = 't';
                        pass_four = 1;
                    } else if (c == '\v') {
                        *s++ = '\\';
                        *s++ = 'v';
                        pass_four = 1;
                    }
                }
                if (pass_four == 0) {
                    if (c >= ' ' && c <= 0x7e)
                        *s++ = c;
                    else {
                        /* Print \octal */
                        *s++ = '\\';
                        if (i + 1 < size
                            && ustr[i + 1] >= '0'
                            && ustr[i + 1] <= '9'
                        ) {
                            /* Print \ooo */
                            *s++ = '0' + (c >> 6);
                            *s++ = '0' + ((c >> 3) & 0x7);
                        } else {
                            /* Print \[[o]o]o */
                            if ((c >> 3) != 0) {
                                if ((c >> 6) != 0)
                                    *s++ = '0' + (c >> 6);
                                *s++ = '0' + ((c >> 3) & 0x7);
                            }
                        }
                        *s++ = '0' + (c & 0x7);
                    }
            }
        }
    }

    if (!(style & QUOTE_OMIT_LEADING_TRAILING_QUOTES))
        *s++ = '\"';
    *s = '\0';

    /* Return zero if we printed entire ASCIZ string (didn't truncate it) */
    if (style & QUOTE_0_TERMINATED && ustr[i] == '\0') {
        /* We didn't see NUL yet (otherwise we'd jump to 'asciz_ended')
        * but next char is NUL.
        */
        return 0;
    }

    return 1;

asciz_ended:
    if (!(style & QUOTE_OMIT_LEADING_TRAILING_QUOTES))
        *s++ = '\"';
    *s = '\0';
    /* Return zero: we printed entire ASCIZ string (didn't truncate it) */
    return 0;
}
