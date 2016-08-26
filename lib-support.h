#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdint.h>

typedef void *(*plt_resolver_t)(void *handle, int import_id);

struct program_header {
    void **plt_trampoline;
    void **plt_handle;
    void **pltgot;
    void *user_info;
};

extern void *pltgot_imports[];

#if defined(__x86_64__)
#define MDL_PLT_BEGIN                                     \
    asm(".pushsection \".text\",\"ax\",@progbits\n"       \
        "slowpath_common:\n"                              \
        "pushq plt_handle(%rip)\n"                        \
        "jmp *plt_trampoline(%rip)\n"                     \
        ".popsection\n" /* start of PLTGOT table. */      \
        ".pushsection \".my_pltgot\",\"aw\",@progbits\n"  \
        "pltgot_imports:\n"                               \
        ".popsection\n");

#define MDL_PLT_ENTRY(number, name)                       \
    asm(".pushsection \".text\",\"ax\",@progbits\n" #name \
        ":\n"                                             \
        "jmp *pltgot_" #name                              \
        "(%rip)\n"                                        \
        "slowpath_" #name                                 \
        ":\n"                                             \
        "pushq $" #number                                 \
        "\n"                                              \
        "jmp slowpath_common\n"                           \
        ".popsection\n" /* entry in PLTGOT table */       \
        ".pushsection \".my_pltgot\",\"aw\",@progbits\n"  \
        "pltgot_" #name                                   \
        ":\n"                                             \
        ".quad slowpath_" #name                           \
        "\n"                                              \
        ".popsection\n");

#else
#error Unsupported architecture
#endif

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
