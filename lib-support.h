#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdint.h>

#define STR(...)  #__VA_ARGS__
#define XSTR(...) STR(__VA_ARGS__)
#define _ES_HASH  #
#define ES_HASH() _ES_HASH

#if defined(__x86_64__)
#define _PUSH_S(x)   pushq x
#define _PUSH(x,y)   pushq x(y)
#define _PUSH_IMM(x) _PUSH_S(x)
#define _POP_S(x)  pop x
#define _POP(x,y)  pop x
#define _JMP_S(x)  jmp x
#define _JMP_REG(x) _JMP_S(x)
#define _JMP(x,y)  jmp *x(y)
#define _CALL(x)   call x
#define REG_IP     %rip
#define REG_ARG_1  %rdi
#define REG_ARG_2  %rsi
#define REG_RET    *%rax
#define LABEL_PREFIX

#elif defined(__arm__)
#define _PUSH_S(x) push x
#define _PUSH(x,y)                \
  ldr r3 , =x                \n   \
  add r3, r3, y              \n   \
  ldr r2, [r3]               \n   \
  push {r2}                  \n   \
  ldr r2, [r3, ES_HASH()4]   \n   \
  push {r2}
#define _PUSH_IMM(x) \
  sub r0, r0, r0    \n  \
  push {r0}
#define _POP_S(x)   pop x
#define _POP(x,y)   ldr x, [y]
#define _JMP_S(x)   b x
#define _JMP_REG(x) bx x
#define _JMP(x,y)   b x
#define _CALL(x)    bl x
#define REG_IP      ip
#define REG_ARG_1   {r0}
#define REG_ARG_2   {r1}
#define REG_RET  r0
#define LABEL_PREFIX "="

#elif defined(__aarch64__)
#define _PUSH(x,y) stp x, y, [sp, #-16]!
#define _CALL(x)   bl x
#else
#error "Unsupported architecture"
#endif

#define PUSH_S(x)   XSTR(_PUSH_S(x))
#define PUSH(x,y)   XSTR(_PUSH(x,y))
#define PUSH_IMM(x) XSTR(_PUSH_IMM(x))
#define JMP_S(x)    XSTR(_JMP_S(x))
#define JMP_REG(x)  XSTR(_JMP_REG(x))
#define JMP(x,y)    XSTR(_JMP(x,y))
#define POP_S(x)    XSTR(_POP_S(x))
#define POP(x,y)    XSTR(_POP(x,y))
#define CALL(x)     XSTR(_CALL(x))

typedef void *(*plt_resolver_t)(void *handle, int import_id);

struct program_header {
    void **plt_trampoline;
    void **plt_handle;
    void **pltgot;
    void *user_info;
};

extern void *pltgot_imports[];

#define MDL_PLT_BEGIN                                        \
    asm(".pushsection .text,\"ax\", \"progbits\""       "\n" \
        "slowpath_common:"                              "\n" \
        PUSH(plt_handle, REG_IP)                        "\n" \
        JMP(plt_trampoline, REG_IP)                     "\n" \
        ".popsection" /* start of PLTGOT table. */      "\n" \
        ".pushsection .my_pltgot,\"aw\",\"progbits\""   "\n" \
        "pltgot_imports:"                               "\n" \
        ".popsection"                                   "\n");

#define MDL_PLT_ENTRY(number, name)                          \
    asm(".pushsection .text,\"ax\", \"progbits\""       "\n" \
        #name ":"                                       "\n" \
        JMP(pltgot_ ##name, REG_IP)                     "\n" \
        "slowpath_" #name ":"                           "\n" \
        PUSH_IMM($ ##number)                            "\n" \
        JMP_S(slowpath_common)                          "\n" \
        ".popsection" /* entry in PLTGOT table */       "\n" \
        ".pushsection .my_pltgot,\"aw\",\"progbits\""   "\n" \
        "pltgot_" #name ":"                             "\n" \
        ".quad " LABEL_PREFIX "slowpath_" #name         "\n" \
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
