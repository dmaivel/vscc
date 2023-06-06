#include "asm/codegen.h"
#include "asm/codegen/x64.h"
#include <stdio.h>
#include <string.h>
#include <vscc.h>
#include <asm/assembler.h>
#include <util/list.h>

#define NOT_FOUND 0xDEADBEEF

/*
 * to-do: support r8, r9, stack
 */
static const int argc_to_reg[] = {
    REG_DI,
    REG_SI,
    REG_DX,
    REG_CX
};

static void codegen_function_start(struct vscc_compiled_data *out, struct vscc_asm_context *asmh, struct vscc_function *function, bool generate_symbols)
{
    size_t stack_allocation_size = 0;
    if (likely(function->register_stream != NULL)) {
        /* write stack positions per register */
        size_t determined_stackpos = 0;
        for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next) {
            reg->stackpos = determined_stackpos + reg->size;
            determined_stackpos += reg->size;

            if (unlikely(generate_symbols))
                vscc_symbol_generate_register(&out->symbols, function, reg);
        }

        /* startup code */
        vscc_x64_push_rbp(asmh);
        vscc_x64_reg_reg(asmh, REX_W, ASM_MOV_REG_REG, MOD_RM | REG_SP | RM_BP);

        /* determine how much of the stack the function requires */
        stack_allocation_size = determined_stackpos;
        vscc_asm_encode(asmh, REX_W, 3, ENCODE_I8(0x83), ENCODE_I8(0xEC), ENCODE_I8(stack_allocation_size)); /* sub rsp, ... */

        /* move arguments onto stack */
        determined_stackpos = 0x100 - function->register_stream->size;
        size_t argc = 0;
        for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next) {
            if (reg->scope == S_PARAMETER) {
                vscc_x64_ptr_reg(asmh, reg->size == SIZEOF_I32 ? 0 : REX_W, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | RM_BP | argc_to_reg[argc++], determined_stackpos);
                determined_stackpos -= reg->size;
            }
        }
    }

    asmh->scratch[0] = stack_allocation_size;
}

static void codegen_function_end(struct vscc_compiled_data *out, struct vscc_asm_context *asmh, struct vscc_function *function, bool generate_symbols)
{
    if (likely(function->register_stream != NULL)) {
        vscc_asm_encode(asmh, REX_W, 3, ENCODE_I8(0x83), ENCODE_I8(0xC4), ENCODE_I8(asmh->scratch[0])); /* add rsp, ... */
        vscc_x64_pop_rbp(asmh);
    }

    vscc_x64_ret(asmh);
}

/* to-do: globals support */
static void codegen_load(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - instruction->reg1->stackpos;
    uint16_t src_stackpos = 0x100 - (instruction->reg2 != NULL ? instruction->reg2->stackpos : 0);

    switch (instruction->movement) {
    case M_REG_IMM:
        /* unimplemented */
        break;
    case M_REG_REG:
        /* to-do: every function needs to check instruction size */
        if (instruction->size == SIZEOF_I32) {
            vscc_x64_reg_ptr(asmh, REX_NONE, ASM_MOV_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, src_stackpos);
            vscc_x64_ptr_reg(asmh, REX_NONE, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
        }
        else {
            vscc_x64_reg_ptr(asmh, REX_W, ASM_MOV_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, src_stackpos);
            vscc_asm_encode(asmh, REX_NONE, 2, ENCODE_I8(0x8a), ENCODE_I8(0x00)); /* mov al, byte ptr [rax] */
            vscc_x64_ptr_reg(asmh, REX_NONE, ASM_MOV_BYTE_PTR_REG, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
        }
        break;
    default:
        break;
    }
}

/* to-do: globals support */
static void codegen_store(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - instruction->reg1->stackpos;
    uint16_t src_stackpos = 0x100 - (instruction->reg2 != NULL ? instruction->reg2->stackpos : 0);

    /* 
     * == 0x100 is the equivalent of checking if the register is a global or not 
     */
    if (dst_stackpos != 0x100) {
        switch (instruction->movement) {
        case M_REG_IMM:
            if (instruction->size == SIZEOF_I32) {
                vscc_x64_ptr_imm(asmh, 0, ASM_MOV_DWORD_PTR_IMM, MOD_DISP8 | RM_BP, dst_stackpos, ENCODE_I32(instruction->imm1));
            }
            else {
                if (unlikely(instruction->imm1 > UINT32_MAX)) {
                    vscc_asm_encode(asmh, REX_W, 2, ENCODE_I8(0xb8), ENCODE_I64(instruction->imm1)); /* movabs rax, imm1:uint64_t */
                    vscc_x64_ptr_reg(asmh, REX_W, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
                }
                else {
                    vscc_x64_ptr_imm(asmh, REX_W, ASM_MOV_DWORD_PTR_IMM, MOD_DISP8 | RM_BP, dst_stackpos, ENCODE_I32(instruction->imm1));
                }
            }
            break;
        case M_REG_REG:
            /* local = local */
            if (likely(src_stackpos != 0x100)) {
                vscc_x64_reg_ptr(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_MOV_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, src_stackpos);
                vscc_x64_ptr_reg(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
            } 
            /* local = global */
            else {
                vscc_asm_push_fill_in(asmh, (size_t)instruction->reg2, instruction->size == SIZEOF_I64 ? 3 : 2, instruction->size == SIZEOF_I64 ? 7 : 6);
                vscc_asm_encode(asmh, REX_NONE, 3, ENCODE_I8(ASM_MOV_REG_DWORD_PTR), ENCODE_I8(0x05), ENCODE_I32(0)); /* mov eax, dword ptr [rip + 0] */

                vscc_x64_ptr_reg(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
            }
            break;
        default:
            break;
        }
    }
    else {
        switch (instruction->movement) {
        case M_REG_IMM:
            if (instruction->size == SIZEOF_I32) {
                vscc_asm_push_fill_in(asmh, (size_t)instruction->reg1, 2, 10);
                vscc_asm_encode(asmh, REX_NONE, 4, ENCODE_I8(ASM_MOV_DWORD_PTR_IMM), ENCODE_I8(0x05), ENCODE_I32(0), ENCODE_I32(instruction->imm1)); /* mov dword ptr [rip + 0], imm1 */
            }
            else {
                if (unlikely(instruction->imm1 > UINT32_MAX)) {
                    vscc_asm_encode(asmh, REX_W, 2, ENCODE_I8(0xbf), ENCODE_I64(instruction->imm1)); /* movabs rdi, imm1:uint64_t */

                    vscc_asm_push_fill_in(asmh, (size_t)instruction->reg1, 3, 7);
                    vscc_asm_encode(asmh, REX_W, 3, ENCODE_I8(ASM_MOV_DWORD_PTR_REG), ENCODE_I8(0x3d), ENCODE_I32(0)); /* mov qword ptr [rip + 0], rdi */
                }
                else {
                    vscc_asm_push_fill_in(asmh, (size_t)instruction->reg1, 3, 11);
                    vscc_asm_encode(asmh, REX_W, 4, ENCODE_I8(ASM_MOV_DWORD_PTR_IMM), ENCODE_I8(0x05), ENCODE_I32(0), ENCODE_I32(instruction->imm1)); /* mov qword ptr [rip + 0], imm1 */
                }
            }
            break;
        case M_REG_REG:
            /* global = local */
            if (likely(src_stackpos != 0x100)) {
                
            }
            /* global = global */
            else {

            }
            break;
        default:
            break;
        }
    }
}

static void codegen_lea(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - instruction->reg1->stackpos;
    uint16_t src_stackpos = 0x100 - (instruction->reg2 != NULL ? instruction->reg2->stackpos : 0);

    /*
     * label map required as global variables are stored at the end
     * to-do: maybe support globals at the top?
     */
    vscc_asm_push_fill_in(asmh, (size_t)instruction->reg2, 3, 7);

    switch (instruction->movement) {
    case M_REG_IMM:
        /* to-do: support? */
        break;
    case M_REG_REG:
        if (instruction->size == SIZEOF_I64) {
            vscc_asm_encode(asmh, REX_W, 3, ENCODE_I8(0x8d), ENCODE_I8(0x05), ENCODE_I32(0)); /* lea rax, qword ptr [rip + 0] */
            vscc_x64_ptr_reg(asmh, REX_W, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
        }
        break;
    default:
        break;
    }
}

static void codegen_cmp(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - instruction->reg1->stackpos;

    switch (instruction->movement) {
    case M_REG_IMM:
        if (instruction->size == SIZEOF_I8)
            vscc_x64_ptr_imm(asmh, REX_NONE, ASM_CMP_BYTE_PTR_IMM, MOD_DISP8 | 0b00111000 /* cmp */ | RM_BP, dst_stackpos, ENCODE_I8(instruction->imm1));
        else if (instruction->size == SIZEOF_I32 || instruction->size == SIZEOF_I64)
            vscc_x64_ptr_imm(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_CMP_DWORD_PTR_IMM, MOD_DISP8 | RM_BP | 0b00111000, dst_stackpos, ENCODE_I8(instruction->imm1));
        break;
    case M_REG_REG:
        /* unimplemented */
        break;
    default:
        break;
    }
}

static void codegen_add(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - instruction->reg1->stackpos;
    uint16_t src_stackpos = 0x100 - (instruction->reg2 != NULL ? instruction->reg2->stackpos : 0);

    switch (instruction->movement) {
    case M_REG_IMM:
        vscc_x64_ptr_imm(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_ADD_DWORD_PTR_IMM, MOD_DISP8 | RM_BP, dst_stackpos, ENCODE_I8(instruction->imm1));
        break;
    case M_REG_REG:
        vscc_x64_reg_ptr(asmh, 0, ASM_MOV_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, src_stackpos);
        vscc_x64_reg_ptr(asmh, 0, ASM_ADD_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
        break;
    default:
        break;
    }
}

static void codegen_sub(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - instruction->reg1->stackpos;
    uint16_t src_stackpos = 0x100 - (instruction->reg2 != NULL ? instruction->reg2->stackpos : 0);

    switch (instruction->movement) {
    case M_REG_IMM:
        vscc_x64_ptr_imm(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_SUB_DWORD_PTR_IMM, MOD_DISP8 | RM_BP | REG_BP, dst_stackpos, ENCODE_I8(instruction->imm1));
        break;
    case M_REG_REG:
        vscc_x64_reg_ptr(asmh, 0, ASM_MOV_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, src_stackpos);
        vscc_x64_reg_ptr(asmh, 0, ASM_SUB_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
        break;
    default:
        break;
    }
}

/* to-do: support r8 & r9, stack arguments */
static inline uint8_t codegen_psharg_determine_register(int argc, bool rm) 
{
    assert(argc < sizeof(argc_to_reg) / sizeof(*argc_to_reg));
    return rm ? argc_to_reg[argc] >> 3 : argc_to_reg[argc];
}

static void codegen_psharg(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - (instruction->reg1 != NULL ? instruction->reg1->stackpos : 0);
    bool syscall = instruction->opcode == O_SYSCALL_PSHARG;

    int argc = syscall ? asmh->syscall_argc : asmh->argc;

    switch (instruction->movement) {
    case M_REG:
        vscc_x64_reg_ptr(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_MOV_REG_DWORD_PTR, MOD_DISP8 | RM_BP | codegen_psharg_determine_register(argc, false), dst_stackpos);
        break;
    case M_IMM:
        /* to-do: support movabs on immediates larger than UINT32_MAX */
        vscc_x64_reg_imm(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_MOV_REG_IMM, MOD_RM | codegen_psharg_determine_register(argc, true), instruction->imm1);
        break;
    default:
        break;
    }

    syscall ? asmh->syscall_argc++ : asmh->argc++;
}

static void codegen_ret(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    uint16_t dst_stackpos = 0x100 - (instruction->reg1 != NULL ? instruction->reg1->stackpos : 0);

    switch (instruction->movement) {
    case M_REG:
        vscc_x64_reg_ptr(asmh, instruction->size == SIZEOF_I64 ? REX_W : 0, ASM_MOV_REG_DWORD_PTR, MOD_DISP8 | REG_AX | RM_BP, dst_stackpos);
        break;
    case M_IMM:
        vscc_x64_mov_eax(asmh, instruction->imm1);
        break;
    default:
        break;
    }

    if (unlikely(instruction->next != NULL)) {
        vscc_asm_push_fill_in(asmh, DECLABEL_END_FUNCTION, 1, 5);
        vscc_asm_encode(asmh, REX_NONE, 2, ENCODE_I8(ASM_JMP), ENCODE_I32(0));
    }
}

static void codegen_syscall(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    vscc_x64_reg_imm(asmh, REX_W, ASM_MOV_REG_IMM, MOD_RM | RM_AX, instruction->imm1); 
    vscc_x64_syscall(asmh); 
    asmh->syscall_argc = 0; 
}

static void codegen_jmp(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    if (instruction->opcode >= O_JE && instruction->opcode <= O_JG) {
        if (vscc_asm_get_label(asmh, instruction->imm1) == NOT_FOUND)
            vscc_asm_push_fill_in(asmh, instruction->imm1, 2, 6);
        vscc_asm_encode(asmh, REX_NONE, 3, ENCODE_I8(ASM_TWO_BYTES), ENCODE_I8(instruction->opcode + 118), ENCODE_I32(vscc_asm_get_offset(asmh, instruction->imm1, 6, false, 0)));
    }
    else {
        if (vscc_asm_get_label(asmh, instruction->imm1) == NOT_FOUND)
            vscc_asm_push_fill_in(asmh, instruction->imm1, 1, 5);
        vscc_asm_encode(asmh, REX_NONE, 2, ENCODE_I8(ASM_JMP), ENCODE_I32(vscc_asm_get_offset(asmh, instruction->imm1, 5, false, 0)));
    }
}

static void codegen_call(struct vscc_asm_context *asmh, struct vscc_instruction *instruction)
{
    if (vscc_asm_get_label(asmh, instruction->imm1) == NOT_FOUND)
        vscc_asm_push_fill_in(asmh, instruction->imm1, 1, 5);
    vscc_asm_encode(asmh, REX_NONE, 2, ENCODE_I8(ASM_CALL), ENCODE_I32(vscc_asm_get_offset(asmh, instruction->imm1, 5, false, 0)));
    if (instruction->reg1 != NULL && ((struct vscc_function*)instruction->imm1)->return_size != 0)
        vscc_x64_ptr_reg(asmh, REX_W, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | RM_BP | REG_AX, 0x100 - instruction->reg1->stackpos);
    asmh->argc = 0;
}

void vscc_codegen_implement_x64(struct vscc_codegen_interface *interface, enum vscc_x64_abi abi)
{
    if (abi != ABI_SYSV)
        assert(false && "only sys-v abi currently supported");

    *interface = (struct vscc_codegen_interface) {
        .startfn = codegen_function_start,
        .endfn = codegen_function_end,

        .addfn = codegen_add,
        .loadfn = codegen_load,
        .storefn = codegen_store,
        .subfn = codegen_sub,
        .mulfn = NULL,
        .divfn = NULL,
        .xorfn = NULL,
        .andfn = NULL,
        .orfn = NULL,
        .shlfn = NULL,
        .shrfn = NULL,
        .cmpfn = codegen_cmp,
        .jmpfn = codegen_jmp,
        .retfn = codegen_ret,
        .pshargfn = codegen_psharg,
        .syscallfn = codegen_syscall,
        .callfn = codegen_call,
        .leafn = codegen_lea,
    };
}