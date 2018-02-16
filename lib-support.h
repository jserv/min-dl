#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdint.h>

#define STR(x)  #x
#define XSTR(x) STR(x)

#if defined(__x86_64__)
#define _PUSH_S(x) pushq x
#define _PUSH(x,y) pushq x(y)
#define _JMP_S(x)  jmp x
#define _JMP(x,y)  jmp *x(y)
#define REG_IP     %rip

#elif defined(__arm__)
#define _PUSH_S(x) push x
#define _PUSH(x,y) ldr y, [x]
#define _JMP_S(x)  b x
#define _JMP(x,y)  b x
#define REG_IP     ip

#elif defined(__aarch64__)
#define _PUSH(x,y) stp x ,y ,[sp, #-16]!
#else
#error "Unsupported architecture"
#endif

#define PUSH_S(x) XSTR(_PUSH_S(x))
#define PUSH(x,y) XSTR(_PUSH(x,y))
#define JMP_S(x)  XSTR(_JMP_S(x))
#define JMP(x,y)  XSTR(_JMP(x,y))

typedef void *(*plt_resolver_t)(void *handle, int import_id);

struct program_header {
    void **plt_trampoline;
    void **plt_handle;
    void **pltgot;
    void *user_info;
};

extern void *pltgot_imports[];

#define MDL_PLT_BEGIN                                        \
    asm(".pushsection \".text\",\"ax\",@progbits"       "\n" \
        "slowpath_common:"                              "\n" \
        PUSH(plt_handle, REG_IP)                        "\n" \
        JMP(plt_trampoline, REG_IP)                     "\n" \
        ".popsection" /* start of PLTGOT table. */      "\n" \
        ".pushsection \".my_pltgot\",\"aw\",@progbits"  "\n" \
        "pltgot_imports:"                               "\n" \
        ".popsection"                                   "\n");

#define MDL_PLT_ENTRY(number, name)                          \
    asm(".pushsection \".text\",\"ax\",@progbits"       "\n" \
        #name ":"                                       "\n" \
        JMP(pltgot_ ##name, REG_IP)                     "\n" \
        "slowpath_" #name ":"                           "\n" \
        PUSH_S($ ##number)                              "\n" \
        JMP_S(slowpath_common)                          "\n" \
        ".popsection" /* entry in PLTGOT table */       "\n" \
        ".pushsection \".my_pltgot\",\"aw\",@progbits"  "\n" \
        "pltgot_" #name ":"                             "\n" \
        ".quad slowpath_" #name                         "\n" \
        ".popsection"                                   "\n");

#define MDL_DEFINE_HEADER(user_info_value)                \
    void *plt_trampoline;                                 \
    void *plt_handle;                                     \
    struct program_header PROG_HEADER = {                 \
        .plt_trampoline = &plt_trampoline,                \
        .plt_handle = &plt_handle,                        \
        .pltgot = pltgot_imports,                         \
        .user_info = user_info_value,                     \
    };

typedef struct __DLoader_Internal *dloader_p;
extern struct __DLoader_API__ {
    dloader_p (*load)(const char *filename);
    void *(*get_info)(dloader_p);
    void (*set_plt_resolver)(dloader_p, plt_resolver_t, void *handle);
    void (*set_plt_entry)(dloader_p, int import_id, void *func);
} DLoader;

#endif
