#include <vscc.h>
#include <asm/assembler.h>
#include <util/list.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOT_FOUND 0xDEADBEEF
#define DECLABEL_END_FUNCTION -1

struct fill_in_jmp_label {
    struct fill_in_jmp_label *next;

    size_t cur_size;
    size_t id;
    uintptr_t idx;
    size_t instuction_len;
};

struct label {
    struct label *next;

    size_t id;
    uintptr_t address;
    bool is_function;
};

/*
 * to-do: support r8, r9, stack
 */
static const int argc_to_reg[] = {
    REG_DI,
    REG_SI,
    REG_DX,
    REG_CX
};

static bool equals(void *a, void *b)
{
    return a == b;
}

static bool equals_label(void *a, void *b)
{
    struct label *x = a;
    return !x->is_function;
}

static size_t codegen_get_size_of_assembly(struct vscc_asm_context *ctx)
{
    size_t size = 0;
    
    for (struct vscc_asm_instruction *ins = ctx->instruction_stream; ins; ins = ins->next)
        size += ins->length;
    
    return size;
}

static uintptr_t access_label_map(struct label *label_map, size_t id)
{
    while (label_map) {
        if (label_map->id == id)
            return label_map->address;
        label_map = label_map->next;
    }
    return NOT_FOUND;
}

static void push_label_map(struct label **label_map, size_t id, uintptr_t address, bool is_function)
{
    struct label *label = vscc_list_alloc((void**)label_map, 0, sizeof(struct label));

    label->id = id;
    label->address = address;
    label->is_function = is_function;
}

static void push_fill_in(struct fill_in_jmp_label **fill_in, size_t cur_size, size_t id, uintptr_t idx, size_t ins_len)
{
    struct fill_in_jmp_label *label = vscc_list_alloc((void**)fill_in, 0, sizeof(struct fill_in_jmp_label));

    label->cur_size = cur_size;
    label->id = id;
    label->idx = idx;
    label->instuction_len = ins_len;
}

static inline uint32_t codegen_get_jump_offset(struct vscc_asm_context *assembler, struct label *label_map, size_t id, size_t ins_length, bool override, size_t override_cur)
{
    return access_label_map(label_map, id) - ins_length - (!override ? codegen_get_size_of_assembly(assembler) : override_cur);
}

static void overwrite_assembler(struct vscc_asm_context *asmh, int dst_idx, void* src, size_t length)
{
    int current_idx = 0;
    for (struct vscc_asm_instruction *ins = asmh->instruction_stream; ins; ins = ins->next) {
        if (current_idx <= dst_idx && dst_idx <= current_idx + ins->length) {
            memcpy(&ins->buffer[dst_idx - current_idx], src, length);
        }
        current_idx += ins->length;
    }
}

static void fill_in_fill_ins(struct fill_in_jmp_label **fill_in, struct label *label_map, struct vscc_asm_context *assembler, size_t id)
{
    for (struct fill_in_jmp_label *fill = *fill_in; fill;) {
        if (fill->id == id) {
            int offset = codegen_get_jump_offset(assembler, label_map, fill->id, fill->instuction_len, true, fill->cur_size);
            overwrite_assembler(assembler, fill->idx, &offset, sizeof(offset));

            struct fill_in_jmp_label *next = fill->next;
            vscc_list_free_element((void**)fill_in, 0, equals, fill);
            fill = next;
        }
        else {
            fill = fill->next;
        }
    }
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
static void codegen_store(struct vscc_asm_context *asmh, struct vscc_instruction *instruction, struct fill_in_jmp_label **flabels)
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
                push_fill_in(flabels, codegen_get_size_of_assembly(asmh), (size_t)instruction->reg2, codegen_get_size_of_assembly(asmh) + (instruction->size == SIZEOF_I64 ? 3 : 2), (instruction->size == SIZEOF_I64 ? 7 : 6));
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
                push_fill_in(flabels, codegen_get_size_of_assembly(asmh), (size_t)instruction->reg1, codegen_get_size_of_assembly(asmh) + 2, 10);
                vscc_asm_encode(asmh, REX_NONE, 4, ENCODE_I8(ASM_MOV_DWORD_PTR_IMM), ENCODE_I8(0x05), ENCODE_I32(0), ENCODE_I32(instruction->imm1)); /* mov dword ptr [rip + 0], imm1 */
            }
            else {
                if (unlikely(instruction->imm1 > UINT32_MAX)) {
                    vscc_asm_encode(asmh, REX_W, 2, ENCODE_I8(0xbf), ENCODE_I64(instruction->imm1)); /* movabs rdi, imm1:uint64_t */

                    push_fill_in(flabels, codegen_get_size_of_assembly(asmh), (size_t)instruction->reg1, codegen_get_size_of_assembly(asmh) + 3, 7);
                    vscc_asm_encode(asmh, REX_W, 3, ENCODE_I8(ASM_MOV_DWORD_PTR_REG), ENCODE_I8(0x3d), ENCODE_I32(0)); /* mov qword ptr [rip + 0], rdi */
                }
                else {
                    push_fill_in(flabels, codegen_get_size_of_assembly(asmh), (size_t)instruction->reg1, codegen_get_size_of_assembly(asmh) + 3, 11);
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

static void codegen_lea(struct vscc_asm_context *asmh, struct vscc_instruction *instruction, struct fill_in_jmp_label **flabels)
{
    uint16_t dst_stackpos = 0x100 - instruction->reg1->stackpos;
    uint16_t src_stackpos = 0x100 - (instruction->reg2 != NULL ? instruction->reg2->stackpos : 0);

    /*
     * label map required as global variables are stored at the end
     * to-do: maybe support globals at the top?
     */
    push_fill_in(flabels, codegen_get_size_of_assembly(asmh), (size_t)instruction->reg2, codegen_get_size_of_assembly(asmh) + 3, 7);

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
}

static struct vscc_symbol *symbol_add(struct vscc_symbol **root, enum vscc_symbol_type type, char *contents)
{
    struct vscc_symbol *symbol = vscc_list_alloc((void**)root, 0, sizeof(struct vscc_symbol));

    symbol->type = type;
    strcpy(symbol->symbol_name, contents);

    return symbol;
}

static void symbol_generate_function(struct vscc_symbol **root, struct vscc_function *function, size_t offset)
{
    static char _str_temp[15] = { 0 };
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_FUNCTION, function->symbol_name);

    sprintf(_str_temp, "<i%ld>()", function->return_size * 8);
    strcat(symbol->symbol_name, _str_temp);

    symbol->offset = offset;
}

static void symbol_generate_register(struct vscc_symbol **root, struct vscc_function *owner, struct vscc_register *reg)
{
    static char _str_temp[15] = { 0 };
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_VARIABLE, owner->symbol_name);

    strcat(symbol->symbol_name, "::");
    strcat(symbol->symbol_name, reg->symbol_name);

    sprintf(_str_temp, "<i%ld, %s>", reg->size * 8, (reg->scope == S_PARAMETER) ? "param" : "local");
    strcat(symbol->symbol_name, _str_temp);

    symbol->offset = reg->stackpos;
}

static void symbol_generate_global(struct vscc_symbol **root, struct vscc_register *reg, size_t offset)
{
    static char _str_temp[15] = { 0 };
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_VARIABLE, reg->symbol_name);

    bool dt = (reg->size == 1 || reg->size == 2 || reg->size == 4 || reg->size == 8);
    sprintf(_str_temp, "<%s%ld%s, globl>", dt ? "i" : "", reg->size * (dt ? 8 : 1), dt ? "" : "b");
    strcat(symbol->symbol_name, _str_temp);

    symbol->offset = offset;
}

void vscc_codegen_x64(struct vscc_context *context, struct vscc_compiled_data *out, bool generate_symbols)
{
    struct vscc_asm_context assembler = { 0 };
    struct label *label_map = NULL;
    struct fill_in_jmp_label *fill_in = NULL;
    char _str_temp[15] = { 0 };

    for (struct vscc_function *function = context->function_stream; function; function = function->next_function) {

        /* to-do: determine release/debug for perf? maybe impact no big deal */
        if (unlikely(generate_symbols))
            symbol_generate_function(&out->symbols, function, codegen_get_size_of_assembly(&assembler));

        push_label_map(&label_map, (size_t)function, codegen_get_size_of_assembly(&assembler), true);
        assembler.argc = 0;
        
        /* check for any labels that can be filled with the proper relative offsets */
        /* to-do: clear when found? */
        fill_in_fill_ins(&fill_in, label_map, &assembler, (size_t)function);

        /* register allocation */
        size_t stack_allocation_size = 0;
        if (likely(function->register_stream != NULL)) {
            /* write stack positions per register */
            size_t determined_stackpos = 0;
            for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next_register) {
                reg->stackpos = determined_stackpos + reg->size;
                determined_stackpos += reg->size;

                if (unlikely(generate_symbols))
                    symbol_generate_register(&out->symbols, function, reg);
            }

            /* startup code */
            vscc_x64_push_rbp(&assembler);
            vscc_x64_reg_reg(&assembler, REX_W, ASM_MOV_REG_REG, MOD_RM | REG_SP | RM_BP);

            /* determine how much of the stack the function requires */
            stack_allocation_size = determined_stackpos;
            vscc_asm_encode(&assembler, REX_W, 3, ENCODE_I8(0x83), ENCODE_I8(0xEC), ENCODE_I8(stack_allocation_size)); /* sub rsp, ... */

            /* move arguments onto stack */
            determined_stackpos = 0x100 - function->register_stream->size;
            size_t argc = 0;
            for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next_register) {
                if (reg->scope == S_PARAMETER) {
                    vscc_x64_ptr_reg(&assembler, reg->size == SIZEOF_I32 ? 0 : REX_W, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | RM_BP | argc_to_reg[argc++], determined_stackpos);
                    determined_stackpos -= reg->size;
                }
            }
        }

        /* instruction translation */
        for (struct vscc_instruction *instruction = function->instruction_stream; instruction; instruction = instruction->next_instruction) {
            switch (instruction->opcode) {
            case O_LOAD: 
                codegen_load(&assembler, instruction); 
                break;
            case O_STORE: 
                codegen_store(&assembler, instruction, &fill_in); 
                break;
            case O_CMP: 
                codegen_cmp(&assembler, instruction); 
                break;
            case O_ADD: 
                codegen_add(&assembler, instruction); 
                break;
            case O_SUB: 
                codegen_sub(&assembler, instruction); 
                break;
            case O_RET: 
                codegen_ret(&assembler, instruction);
                if (unlikely(instruction->next_instruction != NULL)) {
                    vscc_asm_encode(&assembler, REX_NONE, 2, ENCODE_I8(ASM_JMP), ENCODE_I32(0));
                    push_fill_in(&fill_in, codegen_get_size_of_assembly(&assembler) - 6, DECLABEL_END_FUNCTION, codegen_get_size_of_assembly(&assembler) - 4, 6);
                }
                break;
            case O_PSHARG: 
            case O_SYSCALL_PSHARG: 
                codegen_psharg(&assembler, instruction); 
                break;
            case O_LEA:
                codegen_lea(&assembler, instruction, &fill_in);
                break;

            case O_SYSCALL: 
                vscc_x64_reg_imm(&assembler, REX_W, ASM_MOV_REG_IMM, MOD_RM | RM_AX, instruction->imm1); 
                vscc_x64_syscall(&assembler); 
                assembler.syscall_argc = 0; 
                break;

            case O_DECLABEL:
                push_label_map(&label_map, instruction->imm1, codegen_get_size_of_assembly(&assembler), false);
                fill_in_fill_ins(&fill_in, label_map, &assembler, instruction->imm1);
                break;

            /* 
             * encoded opcode = ASM_JE + (instruction->opcode - O_JE)
             */
            case O_JE:
            case O_JNE:
            case O_JBE:
            case O_JA:
            case O_JS:
            case O_JNS:
            case O_JP:
            case O_JNP:
            case O_JL:
            case O_JGE:
            case O_JLE:
            case O_JG:
                vscc_asm_encode(&assembler, REX_NONE, 3, ENCODE_I8(ASM_TWO_BYTES), ENCODE_I8(instruction->opcode + 118), ENCODE_I32(codegen_get_jump_offset(&assembler, label_map, instruction->imm1, 6, false, 0)));
                if (access_label_map(label_map, instruction->imm1) == NOT_FOUND)
                    push_fill_in(&fill_in, codegen_get_size_of_assembly(&assembler) - 6, instruction->imm1, codegen_get_size_of_assembly(&assembler) - 4, 6);
                break;

            case O_JMP:
                vscc_asm_encode(&assembler, REX_NONE, 2, ENCODE_I8(ASM_JMP), ENCODE_I32(codegen_get_jump_offset(&assembler, label_map, instruction->imm1, 5, false, 0)));
                if (access_label_map(label_map, instruction->imm1) == NOT_FOUND)
                    push_fill_in(&fill_in, codegen_get_size_of_assembly(&assembler) - 5, instruction->imm1, codegen_get_size_of_assembly(&assembler) - 4, 5);
                break;

            case O_CALL:
                vscc_asm_encode(&assembler, REX_NONE, 2, ENCODE_I8(ASM_CALL), ENCODE_I32(codegen_get_jump_offset(&assembler, label_map, instruction->imm1, 5, false, 0)));
                if (access_label_map(label_map, instruction->imm1) == NOT_FOUND)
                    push_fill_in(&fill_in, codegen_get_size_of_assembly(&assembler) - 5, instruction->imm1, codegen_get_size_of_assembly(&assembler) - 4, 5);
                if (instruction->reg1 != NULL && ((struct vscc_function*)instruction->imm1)->return_size != 0)
                    vscc_x64_ptr_reg(&assembler, REX_W, ASM_MOV_DWORD_PTR_REG, MOD_DISP8 | RM_BP | REG_AX, 0x100 - instruction->reg1->stackpos);
                assembler.argc = 0;
                break;

            default:
                assert(false && "unsupported intermediate opcode");
                break;
            }
        }

        /*
         * to-do: maybe determine ret count and decide whether these two lines require execution
         */
        push_label_map(&label_map, DECLABEL_END_FUNCTION, codegen_get_size_of_assembly(&assembler), false);
        fill_in_fill_ins(&fill_in, label_map, &assembler, DECLABEL_END_FUNCTION);

        if (likely(function->register_stream != NULL)) {
            vscc_asm_encode(&assembler, REX_W, 3, ENCODE_I8(0x83), ENCODE_I8(0xC4), ENCODE_I8(stack_allocation_size)); /* add rsp, ... */
            vscc_x64_pop_rbp(&assembler);
        }

        vscc_x64_ret(&assembler);
        vscc_list_free_element((void**)&label_map, 0, equals_label, NULL);
    }

    /* global registers */
    /* to-do: support global registers being stored at the top of the buffer (should be simpler) */
    size_t offs = codegen_get_size_of_assembly(&assembler);
    if (likely(context->global_register_stream != NULL)) {
        for (struct vscc_register *reg = context->global_register_stream; reg; reg = reg->next_register) {
            if (unlikely(generate_symbols))
                symbol_generate_global(&out->symbols, reg, offs);

            push_label_map(&label_map, (size_t)reg, offs, false);
            offs += reg->size;
        }

        for (struct vscc_register *reg = context->global_register_stream; reg; reg = reg->next_register)
            fill_in_fill_ins(&fill_in, label_map, &assembler, (size_t)reg);
    }

    /* write compiled code to output */
    out->length = offs;
    out->buffer = calloc(1, out->length);

    /* copy memory contents */
    size_t idx = 0;
    for (struct vscc_asm_instruction *ins = assembler.instruction_stream; ins; ins = ins->next) {
        memcpy(&out->buffer[idx], ins->buffer, ins->length);
        idx += ins->length;
    }

    /* free memory */
    vscc_list_free((void**)&assembler.instruction_stream, 0);
    vscc_list_free((void**)&label_map, 0);
    vscc_list_free((void**)&fill_in, 0);
}