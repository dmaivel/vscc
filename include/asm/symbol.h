#ifndef _VSCC_SYMBOL_H_
#define _VSCC_SYMBOL_H_

#include <ir/intermediate.h>

#define GENERATE_SYMBOLS true
#define DONT_GENERATE_SYMBOLS false

enum vscc_symbol_type {
    VSCC_SYMBOL_FUNCTION,
    VSCC_SYMBOL_VARIABLE,
};

struct vscc_symbol {
    struct vscc_symbol *next;

    char symbol_name[MAX_SYMBOL_LEN];

    enum vscc_symbol_type type;

    /* absolute offset for functions & global variables, relative stack offset for local/param variables */
    uintptr_t offset;
};

void vscc_symbol_generate_function(struct vscc_symbol **root, struct vscc_function *function, size_t offset);
void vscc_symbol_generate_register(struct vscc_symbol **root, struct vscc_function *owner, struct vscc_register *reg);
void vscc_symbol_generate_global(struct vscc_symbol **root, struct vscc_register *reg, size_t offset);

#endif