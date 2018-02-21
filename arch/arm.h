#ifndef _MIN_DL_ARM_H_
#define _MIN_DL_ARM_H_

#include <link.h>

#define _PUSH_S(x) push x
#define _PUSH(x,y) \
  ldr r3, =x   \n  \
  ldr r2, [r3] \n  \
  push {r2}
#define _PUSH_IMM(x) \
  mov r0, ES_HASH()x  \n  \
  push {r0}
#define _PUSH_STACK_STATE push {r11, lr}
#define _POP_STACK_STATE  pop {r11, lr}
#define _POP_S(x)   pop x
#define _POP(x,y)   ldr x, [y]
#define _JMP_S(x)   b x
#define _JMP_REG(x) bx x
#define _JMP(x,y) \
  ldr r3, =x        \n   \
  ldr r2, [r3]      \n   \
  bx r2
#define _CALL(x)    bl x
#define REG_IP      ip
#define REG_ARG_1   {r0}
#define REG_ARG_2   {r1}
#define REG_RET  r0
#define LABEL_PREFIX  "="
#define SYS_ADDR_ATTR "word"

typedef ElfW(Rel) ElfW_Reloc;
#define ELFW_DT_RELW   DT_REL
#define ELFW_DT_RELWSZ DT_RELSZ

#endif
