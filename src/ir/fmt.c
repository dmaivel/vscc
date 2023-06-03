#include <vscc.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char *sprintf_wrapper(char *__s, const char *__format, ...)
{
    va_list __arg;
    va_start(__arg, __format);
    vsprintf(__s, __format, __arg);
    return __s;
}

/*
 * to-do: do a complete revision
 */
char *vscc_fmt_ir_str(struct vscc_context *ctx, size_t max_strlen)
{
    char *str = calloc(1, max_strlen);
    char sprint[64] = { 0 };

    /* matches order of 'enum vscc_opcode' */
    static const char opcode_str[][17] = { 
        "__invalid",
        "add",
        "load",
        "store",
        "sub",
        "mul",
        "div",
        "xor",
        "and",
        "or",
        "shl",
        "shr",
        "cmp",
        "jmp",
        "je",
        "jne",
        "jbe",
        "ja",
        "js",
        "jns",
        "jp",
        "jnp",
        "jl",
        "jge",
        "jle",
        "jg",
        "return",
        "__declabel",
        "psharg",
        "syscall",
        "psharg(sys)",
        "__call",
        "lea"
    };
    
    for (struct vscc_function *function = ctx->function_stream; function; function = function->next_function) {
        strcat(str, sprintf_wrapper(sprint, "%s<i%ld>(", function->symbol_name, function->return_size * 8));

        bool first = false;
        for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next_register) {
            if (reg->scope == S_PARAMETER) {
                if (first)
                    strcat(str, ", ");
                strcat(str, sprintf_wrapper(sprint, "i%ld %%%ld", reg->size * 8, reg->index));
                first = true;               
            }
        }

        strcat(str, ") {\n");
        for (struct vscc_instruction *instruction = function->instruction_stream; instruction; instruction = instruction->next_instruction) {
            switch (instruction->opcode) {
            case O_DECLABEL:
                strcat(str, sprintf_wrapper(sprint, "%ld:\n", instruction->imm1));
                continue;
            case O_CALL:
                strcat(str, sprintf_wrapper(sprint, "\tcall @%s", ((struct vscc_function*)instruction->imm1)->symbol_name));
                if (instruction->reg1 != NULL)
                    strcat(str, sprintf_wrapper(sprint, " -> i%ld %%%ld\n", instruction->reg1->size * 8, instruction->reg1->index));
                else
                    strcat(str, "\n");
                continue;
            default:
                break;
            };

            strcat(str, sprintf_wrapper(sprint, "\t%s ", opcode_str[instruction->opcode]));
            switch (instruction->movement) {
            case M_INVALID:
                strcat(str, "__m_invalid: possible corruption\n");
                break;
            case M_REG_REG:
                strcat(str, sprintf_wrapper(sprint, "i%ld %%%ld %%%ld\n", instruction->size * 8, instruction->reg1->index, instruction->reg2->index));
                break;
            case M_REG_IMM:
                strcat(str, sprintf_wrapper(sprint, "i%ld %%%ld %ld\n", instruction->size * 8, instruction->reg1->index, instruction->imm1));
                break;
            case M_REG:
                strcat(str, sprintf_wrapper(sprint, "i%ld %%%ld\n", instruction->size * 8, instruction->reg1->index));
                break;
            case M_IMM:
                strcat(str, sprintf_wrapper(sprint, "i%ld %ld\n", instruction->size * 8, instruction->imm1));
                break;
            }
        }
        strcat(str, "}\n\n");
    }

    str = realloc(str, strlen(str) + 1);
    assert(str);
    return str;
}

void vscc_util_cvt_ir_file(struct vscc_context *ctx, char *path, bool is_src)
{
    /* unimplemented */
}

void vscc_util_cvt_file_ir(struct vscc_context *ctx, char *path, bool is_src)
{
    /* unimplemented */
}