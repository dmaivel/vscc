#include <ir/intermediate.h>
#include <util/list.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * to-do: check if symbol names are less than 64 bytes
 */
 
struct vscc_function *vscc_init_function(struct vscc_context *context, char *symbol_name, size_t return_size)
{
    struct vscc_function *function = vscc_list_alloc((void**)&context->function_stream, 0, sizeof(struct vscc_function));
    
    strcpy(function->symbol_name, symbol_name);
    function->return_size = return_size;

    return function;
}

struct vscc_register *vscc_alloc(struct vscc_function *this, char *symbol_name, size_t size, bool is_parameter, bool is_volatile)
{
    struct vscc_register *reg = vscc_list_alloc((void**)&this->register_stream, 0, sizeof(struct vscc_register));

    strcpy(reg->symbol_name, symbol_name);
    reg->index = this->register_count++;
    reg->is_volatile = is_volatile;
    reg->scope = is_parameter ? S_PARAMETER : S_LOCAL;
    reg->size = size;

    return reg;
}

struct vscc_register *vscc_alloc_global(struct vscc_context *this_context, char *symbol_name, size_t size, bool is_volatile)
{
    struct vscc_register *reg = vscc_list_alloc((void**)&this_context->global_register_stream, 0, sizeof(struct vscc_register));
    
    strcpy(reg->symbol_name, symbol_name);
    reg->is_volatile = is_volatile;
    reg->scope = S_GLOBAL;
    reg->size = size;

    return reg;
}

struct vscc_function *vscc_fetch_function_by_name(struct vscc_context *this_context, char *symbol_name)
{
    for (struct vscc_function *fn = this_context->function_stream; fn; fn = fn->next_function)
        if (strcmp(fn->symbol_name, symbol_name) == 0)
            return fn;
    return NULL;
}

struct vscc_register *vscc_fetch_register_by_name(struct vscc_function *this, char *symbol_name)
{
    for (struct vscc_register *reg = this->register_stream; reg; reg = reg->next_register)
        if (strcmp(reg->symbol_name, symbol_name) == 0)
            return reg;
    return NULL;
}

struct vscc_register *vscc_fetch_register_by_index(struct vscc_function *this, size_t index)
{
    for (struct vscc_register *reg = this->register_stream; reg; reg = reg->next_register)
        if (reg->index == index)
            return reg;
    return NULL;
}

struct vscc_register *vscc_fetch_global_register_by_name(struct vscc_context *this_context, char *symbol_name)
{
    for (struct vscc_register *reg = this_context->global_register_stream; reg; reg = reg->next_register)
        if (strcmp(reg->symbol_name, symbol_name) == 0)
            return reg;
    return NULL;
}

static bool vscc_unlink_matcher_equal(void *element, void *data) 
{
    return element == data;
}

void vscc_unlink_free_register(struct vscc_function *this, struct vscc_register *reg)
{
    vscc_list_free_element((void**)&this->register_stream, 0, vscc_unlink_matcher_equal, reg);
}

void vscc_unlink_free_instruction(struct vscc_function *this, struct vscc_instruction *ins)
{
    vscc_list_free_element((void**)&this->instruction_stream, 0, vscc_unlink_matcher_equal, ins);
}

#define vscc_push(o, m) \
    struct vscc_instruction *ins = vscc_list_alloc((void**)&this->instruction_stream, 0, sizeof(struct vscc_instruction)); \
    ins->opcode = o; \
    ins->movement = m; \

void vscc_push0(struct vscc_function *this, enum vscc_opcode opcode, struct vscc_register *dst, size_t src)
{
    vscc_push(opcode, M_REG_IMM);
    ins->reg1 = dst;
    ins->imm1 = src;
    ins->size = dst != NULL ? dst->size : 0;
}

void vscc_push1(struct vscc_function *this, enum vscc_opcode opcode, struct vscc_register *dst, struct vscc_register *src)
{
    vscc_push(opcode, M_REG_REG);
    ins->reg1 = dst;
    ins->reg2 = src;
    ins->size = dst->size;
}

void vscc_push2(struct vscc_function *this, enum vscc_opcode opcode, size_t imm)
{
    vscc_push(opcode, M_IMM);
    ins->imm1 = imm;
    ins->size = 8; /* to-do: fix, maybe add parameter specifing size */
}

void vscc_push3(struct vscc_function *this, enum vscc_opcode opcode, struct vscc_register *reg)
{
    vscc_push(opcode, M_REG);
    ins->reg1 = reg;
    ins->size = reg->size;
}

void vscc_pushs(struct vscc_function *this, struct vscc_syscall_args *args)
{
    for (int i = 0; i < args->count; i++) {
        if (args->type[i] == M_IMM)
            vscc_push2(this, O_SYSCALL_PSHARG, args->values[i]);
        else
            vscc_push3(this, O_SYSCALL_PSHARG, (struct vscc_register*)args->values[i]);
    }

    vscc_push2(this, O_SYSCALL, args->syscall_id);
}