#include <stdio.h>
#include <string.h>
#include <util/list.h>
#include <asm/symbol.h>

static struct vscc_symbol *symbol_add(struct vscc_symbol **root, enum vscc_symbol_type type, char *contents, void *symbol_for)
{
    struct vscc_symbol *symbol = vscc_list_alloc((void**)root, 0, sizeof(struct vscc_symbol));

    symbol->type = type;
    symbol->symbol_for = symbol_for;
    strcpy(symbol->symbol_name, contents);

    return symbol;
}

void vscc_symbol_generate_function(struct vscc_symbol **root, struct vscc_function *function, size_t offset)
{
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_FUNCTION, function->symbol_name, function);

    symbol->offset = offset;
}

void vscc_symbol_generate_register(struct vscc_symbol **root, struct vscc_function *owner, struct vscc_register *reg)
{
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_VARIABLE, owner->symbol_name, reg);

    strcat(symbol->symbol_name, "::");
    strcat(symbol->symbol_name, reg->symbol_name);

    symbol->offset = reg->stackpos;
}

void vscc_symbol_generate_global(struct vscc_symbol **root, struct vscc_register *reg, size_t offset)
{
    struct vscc_symbol *symbol = symbol_add(root, VSCC_SYMBOL_VARIABLE, reg->symbol_name, reg);

    symbol->offset = offset;
}
