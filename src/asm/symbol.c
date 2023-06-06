#include <stdio.h>
#include <string.h>
#include <util/list.h>
#include <asm/symbol.h>

static struct vscc_symbol *symbol_add(struct vscc_symbol **root, enum vscc_symbol_type type, char *contents)
{
    struct vscc_symbol *symbol = vscc_list_alloc((void**)root, 0, sizeof(struct vscc_symbol));

    symbol->type = type;
    strcpy(symbol->symbol_name, contents);

    return symbol;
}

void vscc_symbol_generate_function(struct vscc_symbol **root, struct vscc_function *function, size_t offset)
{
    static char _str_temp[15] = { 0 };
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_FUNCTION, function->symbol_name);

    sprintf(_str_temp, "<i%ld>()", function->return_size * 8);
    strcat(symbol->symbol_name, _str_temp);

    symbol->offset = offset;
}

void vscc_symbol_generate_register(struct vscc_symbol **root, struct vscc_function *owner, struct vscc_register *reg)
{
    static char _str_temp[15] = { 0 };
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_VARIABLE, owner->symbol_name);

    strcat(symbol->symbol_name, "::");
    strcat(symbol->symbol_name, reg->symbol_name);

    sprintf(_str_temp, "<i%ld, %s>", reg->size * 8, (reg->scope == S_PARAMETER) ? "param" : "local");
    strcat(symbol->symbol_name, _str_temp);

    symbol->offset = reg->stackpos;
}

void vscc_symbol_generate_global(struct vscc_symbol **root, struct vscc_register *reg, size_t offset)
{
    static char _str_temp[15] = { 0 };
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_VARIABLE, reg->symbol_name);

    bool dt = (reg->size == 1 || reg->size == 2 || reg->size == 4 || reg->size == 8);
    sprintf(_str_temp, "<%s%ld%s, globl>", dt ? "i" : "", reg->size * (dt ? 8 : 1), dt ? "" : "b");
    strcat(symbol->symbol_name, _str_temp);

    symbol->offset = offset;
}