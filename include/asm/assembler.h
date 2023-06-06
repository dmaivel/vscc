#ifndef _VSCC_ASSEMBLER_H_
#define _VSCC_ASSEMBLER_H_

#include <ir/intermediate.h>
#include <asm/symbol.h>

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>

#define NOT_FOUND 0xDEADBEEF

struct vscc_fill_in_jmp_label {
    struct vscc_fill_in_jmp_label *next;

    size_t cur_size;
    size_t id;
    uintptr_t idx;
    size_t instuction_len;
};

struct vscc_label {
    struct vscc_label *next;

    size_t id;
    uintptr_t address;
    bool is_function;
};

struct vscc_asm_instruction {
    struct vscc_asm_instruction *next;
    
    uint8_t *buffer;
    size_t length;
};

struct vscc_asm_context {
    struct vscc_asm_instruction *instruction_stream;

    struct vscc_label *label_map;
    struct vscc_fill_in_jmp_label *fill_in;

    size_t syscall_argc;
    size_t argc;

    uint64_t scratch[4];
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

#ifdef  __cplusplus
extern "C" {
#endif

uintptr_t vscc_asm_get_label(struct vscc_label *label_map, size_t id);
void vscc_asm_push_label(struct vscc_label **label_map, size_t id, uintptr_t address, bool is_function);
void vscc_asm_push_fill_in(struct vscc_fill_in_jmp_label **fill_in, size_t cur_size, size_t id, uintptr_t idx, size_t ins_len);
uint32_t vscc_asm_get_offset(struct vscc_asm_context *assembler, struct vscc_label *label_map, size_t id, size_t ins_length, bool override, size_t override_cur);
void vscc_asm_fill_ins(struct vscc_fill_in_jmp_label **fill_in, struct vscc_label *label_map, struct vscc_asm_context *assembler, size_t id);

size_t vscc_asm_size(struct vscc_asm_context *ctx);
void vscc_asm_overwrite(struct vscc_asm_context *asmh, int dst_idx, void* src, size_t length);
void vscc_asm_encode(struct vscc_asm_context *context, int rex, int count, ...);

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