#ifndef _MIN_DL_X86_64_H_
#define _MIN_DL_X86_64_H_

#include <link.h>

#define _PUSH_S(x)   pushq x
#define _PUSH(x,y)   pushq x(y)
#define _PUSH_IMM(x) pushq $##x
#define _PUSH_STACK_STATE
#define _POP_STACK_STATE
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
#define SYS_ADDR_ATTR "quad"
#define LABEL_PREFIX

typedef ElfW(Rela) ElfW_Reloc;
#define ELFW_DT_RELW   DT_RELA
#define ELFW_DT_RELWSZ DT_RELASZ

#endif
