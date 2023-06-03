#include <util/list.h>
#include <vscc.h>

#include <stdlib.h>

static bool vscc_optfn_remove_register_matcher(void *element, void *data)
{
    struct vscc_register *reg = element;
    bool result = reg->is_read == false && reg->is_volatile == false;

    /* result is either 0 or 1 */
    *(int*)data += (int)result;
    return result;
}

int vscc_optfn_elim_dead_store(struct vscc_function *function)
{
    int optimized_out = 0;

    /* make sure reserved 'is_read' field is false so it won't conflict the optimizer */
    for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next_register)
        reg->is_read = false;

    /* loop through all instructions to see which registers are read */
    for (struct vscc_instruction *instruction = function->instruction_stream; instruction; instruction = instruction->next_instruction) {
        switch (instruction->movement) {
        case M_INVALID:
            break;
        case M_REG_IMM:
            if (instruction->opcode == O_LOAD)
                instruction->reg1->is_read = true;
            break;
        case M_REG_REG:
            if (instruction->opcode == O_LOAD)
                instruction->reg1->is_read = true;
            instruction->reg2->is_read = true; /* to-do: maybe fix, else caused issues before so look into that */
            break;
        case M_REG:
            instruction->reg1->is_read = true;
            break;
        case M_IMM:
            break;
        }
    }

    /* remove registers */
    vscc_list_free_element((void**)&function->register_stream, 0, vscc_optfn_remove_register_matcher, &optimized_out);

    /* adjust indices */
    size_t idx = 0;
    for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next_register)
        reg->index = idx++;

    /* return count of how many registers were optimized out of the function */
    return optimized_out;
}

/*
 * constant propagation analysis
 */
int vscc_optfn_const_propagation(struct vscc_function *function)
{
    /* unimplemented */
    return 0;
}