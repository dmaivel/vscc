#ifndef _VSCC_ASSEMBLER_H_
#define _VSCC_ASSEMBLER_H_

#include <ir/intermediate.h>

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>

#define GENERATE_SYMBOLS true
#define DONT_GENERATE_SYMBOLS false

#define MOD_SIB     0b00000000
#define MOD_DISP8   0b01000000
#define MOD_DISP32  0b10000000
#define MOD_RM      0b11000000

#define REG_AX      0b00000000
#define REG_CX      0b00001000
#define REG_DX      0b00010000
#define REG_BX      0b00011000
#define REG_SP      0b00100000
#define REG_BP      0b00101000
#define REG_SI      0b00110000
#define REG_DI      0b00111000

#define RM_AX       0b00000000
#define RM_CX       0b00000001
#define RM_DX       0b00000010
#define RM_BX       0b00000011
#define RM_SP       0b00000100
#define RM_BP       0b00000101
#define RM_SI       0b00000110
#define RM_DI       0b00000111

#define RM_XMM0     0b00000000
#define RM_XMM1     0b00000001
#define RM_XMM2     0b00000010
#define RM_XMM3     0b00000011
#define RM_XMM4     0b00000100
#define RM_XMM5     0b00000101
#define RM_XMM6     0b00000110
#define RM_XMM7     0b00000111
#define RM_XMM8     RM_XMM0
#define RM_XMM9     RM_XMM1
#define RM_XMM10    RM_XMM2
#define RM_XMM11    RM_XMM3
#define RM_XMM12    RM_XMM4
#define RM_XMM13    RM_XMM5
#define RM_XMM14    RM_XMM6
#define RM_XMM15    RM_XMM7
#define RM_YMM0     RM_XMM0
#define RM_YMM1     RM_XMM1
#define RM_YMM2     RM_XMM2
#define RM_YMM3     RM_XMM3
#define RM_YMM4     RM_XMM4
#define RM_YMM5     RM_XMM5
#define RM_YMM6     RM_XMM6
#define RM_YMM7     RM_XMM7
#define RM_YMM8     RM_XMM0
#define RM_YMM9     RM_XMM1
#define RM_YMM10    RM_XMM2
#define RM_YMM11    RM_XMM3
#define RM_YMM12    RM_XMM4
#define RM_YMM13    RM_XMM5
#define RM_YMM14    RM_XMM6
#define RM_YMM15    RM_XMM7

#define REX_NONE    0b00000000
#define REX_W       0b01001000
#define REX_R       0b01000100
#define REX_X       0b01000010
#define REX_B       0b01000001

#define ASM_TWO_BYTES            0x0F

#define ASM_MOV_DWORD_PTR_IMM    0xC7
#define ASM_MOV_REG_IMM          0xC7
#define ASM_MOV_DWORD_PTR_REG    0x89
#define ASM_MOV_REG_REG          0x89
#define ASM_MOV_REG_DWORD_PTR    0x8B

#define ASM_MOV_BYTE_PTR_REG     0x88
#define ASM_MOV_REG_BYTE_PTR     0x8A

#define ASM_ADD_DWORD_PTR_IMM    0x83
#define ASM_ADD_REG_DWORD_PTR    0x01

#define ASM_SUB_DWORD_PTR_IMM    0x83
#define ASM_SUB_REG_DWORD_PTR    0x29

#define ASM_CMP_DWORD_PTR_IMM    0x83
#define ASM_CMP_BYTE_PTR_IMM     0x80

#define ASM_MOV_EAX              0xB8

#define ASM_PUSH_RBP             0x55
#define ASM_POP_RBP              0x5D

#define ASM_SYSCALL              0x05

#define ASM_CALL                 0xE8
#define ASM_JMP                  0xE9

#define ASM_JE                   0x84
#define ASM_JNE                  0x85
#define ASM_JBE                  0x86
#define ASM_JA                   0x87
#define ASM_JS                   0x88
#define ASM_JNS                  0x89
#define ASM_JP                   0x8A
#define ASM_JNP                  0x8B
#define ASM_JL                   0x8C
#define ASM_JGE                  0x8D
#define ASM_JLE                  0x8E
#define ASM_JG                   0x8F

#define ASM_RET                  0xC3

enum vscc_symbol_type {
    VSCC_SYMBOL_FUNCTION,
    VSCC_SYMBOL_VARIABLE,
};

struct vscc_asm_instruction {
    struct vscc_asm_instruction *next;
    
    uint8_t *buffer;
    size_t length;
};

struct vscc_asm_context {
    struct vscc_asm_instruction *instruction_stream;

    size_t syscall_argc;
    size_t argc;
};

struct vscc_symbol {
    struct vscc_symbol *next;

    char symbol_name[MAX_SYMBOL_LEN];

    enum vscc_symbol_type type;

    /* absolute offset for functions & global variables, relative stack offset for local/param variables */
    uintptr_t offset;
};

struct vscc_compiled_data {
    uint8_t *buffer;
    size_t length;

    struct vscc_symbol *symbols;
};

#define ENCODE_I8(x) x, SIZEOF_I8
#define ENCODE_I16(x) x, SIZEOF_I16
#define ENCODE_I32(x) x, SIZEOF_I32
#define ENCODE_I64(x) x, SIZEOF_I64

#define vscc_x64_mov_eax(ctx, imm)  vscc_asm_encode(ctx, REX_NONE, 2, ENCODE_I8(ASM_MOV_EAX), ENCODE_I32(imm))
#define vscc_x64_push_rbp(ctx)      vscc_asm_encode(ctx, REX_NONE, 1, ENCODE_I8(ASM_PUSH_RBP))
#define vscc_x64_pop_rbp(ctx)       vscc_asm_encode(ctx, REX_NONE, 1, ENCODE_I8(ASM_POP_RBP))
#define vscc_x64_syscall(ctx)       vscc_asm_encode(ctx, REX_NONE, 2, ENCODE_I8(ASM_TWO_BYTES), ENCODE_I8(ASM_SYSCALL))
#define vscc_x64_ret(ctx)           vscc_asm_encode(ctx, REX_NONE, 1, ENCODE_I8(ASM_RET))
#define vscc_x64_call(ctx, rel32)   vscc_asm_encode(ctx, REX_NONE, 2, ENCODE_I8(ASM_CALL), ENCODE_I32(rel32))

#define vscc_x64_ptr_imm(ctx, rex, op, modrm, stackpos, imm)    vscc_asm_encode(ctx, rex, 4, ENCODE_I8(op), ENCODE_I8(modrm), ENCODE_I8(stackpos), imm)
#define vscc_x64_ptr_reg(ctx, rex, op, modrm, stackpos)         vscc_asm_encode(ctx, rex, 3, ENCODE_I8(op), ENCODE_I8(modrm), ENCODE_I8(stackpos))
#define vscc_x64_reg_ptr(ctx, rex, op, modrm, stackpos)         vscc_asm_encode(ctx, rex, 3, ENCODE_I8(op), ENCODE_I8(modrm), ENCODE_I8(stackpos))

#define vscc_x64_reg_imm(ctx, rex, op, modrm, imm)              vscc_asm_encode(ctx, rex, 3, ENCODE_I8(op), ENCODE_I8(modrm), ENCODE_I32(imm))
#define vscc_x64_reg_reg(ctx, rex, op, modrm)                   vscc_asm_encode(ctx, rex, 2, ENCODE_I8(op), ENCODE_I8(MOD_RM | modrm))

#ifdef  __cplusplus
extern "C" {
#endif

void vscc_asm_encode(struct vscc_asm_context *context, int rex, int count, ...);
void vscc_codegen_x64(struct vscc_context *context, struct vscc_compiled_data *out, bool generate_symbols);

#ifdef __cplusplus
}
#endif

/*
* some encoding
* reg_di = 7 (0b111), 8th possible function of the opcode '0x80' 
* http://ref.x86asm.net/coder64.html#x80
*/
// #define vscc_cmp_byte_ptr_imm(ctx, rex, modrm, stackpos, imm) vscc_asm_encode(ctx, rex, 4, ENCODE_I8(ASM_CMP_BYTE_PTR_IMM), ENCODE_I8(REG_DI | modrm), ENCODE_I8(stackpos), ENCODE_I8(imm))

#endif /* _VSCC_ASSEMBLER_H_ */