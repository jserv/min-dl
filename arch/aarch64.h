#ifndef _MIN_DL_AARCH64_H_
#define _MIN_DL_AARCH64_H_

#include <link.h>

#define _PUSH_S(x) str x, [sp, ES_HASH()-16]!
#define _PUSH(x,y) \
  ldr x3, =x   \n  \
  ldr x2, [x3] \n  \
  str x2, [sp, ES_HASH()-16]!
#define _PUSH_IMM(x) \
  mov x0, ES_HASH()x  \n  \
  str x0, [sp, ES_HASH()-16]!
#define _PUSH_STACK_STATE stp x29, x30, [sp, #-16]!
#define _POP_STACK_STATE  ldp x29, x30, [sp] \n \
                          add sp, sp, ES_HASH()16
#define _POP_S(x)   ldr x, [sp] \n \
                    add sp, sp, ES_HASH()16
#define _POP(x,y)   ldr x, [y]
#define _JMP_S(x)   b x
#define _JMP_REG(x) br x
#define _JMP(x,y) \
  ldr x3, =x   \n   \
  ldr x2, [x3] \n  \
  br x2
#define _CALL(x)    bl x
#define REG_IP      ip
#define REG_ARG_1   x0
#define REG_ARG_2   x1
#define REG_RET     x0
#define LABEL_PREFIX  "="
#define SYS_ADDR_ATTR "quad"

typedef ElfW(Rela) ElfW_Reloc;
#define ELFW_DT_RELW   DT_RELA
#define ELFW_DT_RELWSZ DT_RELASZ

#endif
