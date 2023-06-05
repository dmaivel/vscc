#include "ir/intermediate.h"
#include <vscc.h>
#include <util/list.h>

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
char *vscc_ir_str(struct vscc_context *ctx, size_t max_strlen)
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
    
    for (struct vscc_function *function = ctx->function_stream; function; function = function->next) {
        strcat(str, sprintf_wrapper(sprint, "%s<i%ld>(", function->symbol_name, function->return_size * 8));

        bool first = false;
        for (struct vscc_register *reg = function->register_stream; reg; reg = reg->next) {
            if (reg->scope == S_PARAMETER) {
                if (first)
                    strcat(str, ", ");
                strcat(str, sprintf_wrapper(sprint, "i%ld %%%ld", reg->size * 8, reg->index));
                first = true;               
            }
        }

        strcat(str, ") {\n");
        for (struct vscc_instruction *instruction = function->instruction_stream; instruction; instruction = instruction->next) {
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

bool vscc_ir_save(struct vscc_context *ctx, char *path, bool is_src)
{
    static uint64_t zero = 0;
    if (is_src)
        assert(false && "cannot save ir as source");
    
    /* 
     * open file, write magic
     */
    FILE *f = fopen(path, "wb");
    if (f == NULL)
        return false;

    int magic = VSCC_IR_FMT_MAGIC;
    fwrite(&magic, sizeof(magic), 1, f);

    /*
     * determine function count & global register counts
     */
    int fcount = 0;
    int gcount = 0;
    
    for (struct vscc_function *fn = ctx->function_stream; fn; fn = fn->next) { fcount++; }
    for (struct vscc_register *rg = ctx->global_register_stream; rg; rg = rg->next) { gcount++; }

    /*
     * notice:
     * the code below makes use of sizeof(uint8_t) and sizeof(uint16_t) as
     * an attempt at cutting down the total size of the binaries produced.
     */

    fwrite(&fcount, sizeof(uint16_t), 1, f);
    fwrite(&gcount, sizeof(uint16_t), 1, f);
    
    /*
     * global register data
     */
    for (struct vscc_register *rg = ctx->global_register_stream; rg; rg = rg->next) {
        size_t len = strlen(rg->symbol_name);
        fwrite(&len, sizeof(uint8_t), 1, f);
        fwrite(rg->symbol_name, len, 1, f);
        /*
         * scope & index set by loader
         */
        fwrite(&rg->size, sizeof(uint8_t), 1, f);
        fwrite(&rg->is_volatile, sizeof(uint8_t), 1, f);
    }

    /*
     * start writing function data
     */
    for (struct vscc_function *fn = ctx->function_stream; fn; fn = fn->next) {
        size_t len = strlen(fn->symbol_name);
        fwrite(&len, sizeof(uint8_t), 1, f);
        fwrite(fn->symbol_name, len, 1, f);
        fwrite(&fn->instruction_count, sizeof(uint16_t), 1, f);
        fwrite(&fn->register_count, sizeof(uint16_t), 1, f);
        fwrite(&fn->return_size, sizeof(uint8_t), 1, f);

        for (struct vscc_register *rg = fn->register_stream; rg; rg = rg->next) {
            size_t len = strlen(rg->symbol_name);
            fwrite(&len, sizeof(uint8_t), 1, f);
            fwrite(rg->symbol_name, len, 1, f);
            fwrite(&rg->scope, sizeof(uint8_t), 1, f);
            /*
             * index set by loader
             */
            fwrite(&rg->size, sizeof(uint8_t), 1, f);
            fwrite(&rg->is_volatile, sizeof(uint8_t), 1, f);
        }

        for (struct vscc_instruction *in = fn->instruction_stream; in; in = in->next) {
            fwrite(&in->opcode, sizeof(uint8_t), 1, f);
            fwrite(&in->movement, sizeof(uint8_t), 1, f);
            fwrite(&in->size, sizeof(uint8_t), 1, f);

            bool x = in->reg1 != NULL;
            bool y = in->reg2 != NULL;

            fwrite(&x, 1, 1, f);
            if (x) {
                size_t len = strlen(in->reg1->symbol_name);
                fwrite(&len, sizeof(uint8_t), 1, f);
                fwrite(in->reg1->symbol_name, len, 1, f);
            }

            fwrite(&y, 1, 1, f);
            if (y) {
                size_t len = strlen(in->reg2->symbol_name);
                fwrite(&len, sizeof(uint8_t), 1, f);
                fwrite(in->reg2->symbol_name, len, 1, f);
            }

            fwrite(&in->imm1, sizeof(in->imm1), 1, f);
            fwrite(&in->imm2, sizeof(in->imm2), 1, f);
        }
    }

    fclose(f);
    return true;
}

bool vscc_ir_load(struct vscc_context *ctx, char *path, bool is_src)
{
    if (is_src)
        assert(false && "cannot load ir as source");

    /* 
     * open file, read magic
     */
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return false;

    int magic;
    fread(&magic, sizeof(magic), 1, f);
    assert(magic == VSCC_IR_FMT_MAGIC);

    /*
     * to-do: implement
     */

    fclose(f);
    return true;
}