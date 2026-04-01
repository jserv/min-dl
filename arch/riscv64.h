#ifndef _MIN_DL_RISCV64_H_
#define _MIN_DL_RISCV64_H_

#include <link.h>

#define _PUSH_S(x) addi sp, sp, -8 \n sd x, 0(sp)
#define _PUSH(x, y) la t0, x \n ld t1, 0(t0) \n addi sp, sp, -8 \n sd t1, 0(sp)
#define _PUSH_IMM(x) li t2, x \n addi sp, sp, -8 \n sd t2, 0(sp)
#define _PUSH_STACK_STATE addi sp, sp, -16 \n sd ra, 8(sp) \n sd s0, 0(sp)
#define _POP_STACK_STATE ld ra, 8(sp) \n ld s0, 0(sp) \n addi sp, sp, 16
#define _POP_S(x) ld x, 0(sp) \n addi sp, sp, 8
#define _POP(x, y) ld x, 0(y)
#define _JMP_S(x) j x
#define _JMP_REG(x) jr x
#define _JMP(x, y) la t0, x \n ld t1, 0(t0) \n jr t1
#define _CALL(x) call x
#define REG_IP t2
#define REG_ARG_1 a0
#define REG_ARG_2 a1
#define REG_RET a0
#define LABEL_PREFIX ""
#define SYS_ADDR_ATTR "quad"

typedef ElfW(Rela) ElfW_Reloc;
#define ELFW_DT_RELW DT_RELA
#define ELFW_DT_RELWSZ DT_RELASZ

#endif
