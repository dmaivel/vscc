#include <vscc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mman.h>

typedef uint64_t(*strlen_t)(char*);

static uintptr_t get_offset_from_symbol(struct vscc_symbol *symbols, char *symbol_name)
{
    for (struct vscc_symbol *symbol = symbols; symbol; symbol = symbol->next) {
        if (strstr(symbol->symbol_name, symbol_name) != NULL)
            return symbol->offset;
    }
    return 0;
}

int main()
{
    /* context */
    struct vscc_context ctx = { 0 };

    /* strlen & local variable initialization */
    struct vscc_function *my_strlen = vscc_init_function(&ctx, "strlen", SIZEOF_I64);
    struct vscc_register *my_strlen_string = vscc_alloc(my_strlen, "str", SIZEOF_PTR, IS_PARAMETER, NOT_VOLATILE);
    struct vscc_register *my_strlen_len = vscc_alloc(my_strlen, "length", SIZEOF_I64, NOT_PARAMETER, NOT_VOLATILE);
    struct vscc_register *my_strlen_deref = vscc_alloc(my_strlen, "deref_char", SIZEOF_I8, NOT_PARAMETER, NOT_VOLATILE);

    /* compiled context */
    struct vscc_codegen_data compiled = { 0 };

    /* strlen definition */
    vscc_push0(my_strlen, O_STORE, my_strlen_len, 0);
    vscc_push2(my_strlen, O_DECLABEL, 0);
    vscc_push1(my_strlen, O_LOAD, my_strlen_deref, my_strlen_string);
    vscc_push0(my_strlen, O_CMP, my_strlen_deref, 0);
    vscc_push2(my_strlen, O_JE, 1);
    vscc_push0(my_strlen, O_ADD, my_strlen_string, 1);
    vscc_push0(my_strlen, O_ADD, my_strlen_len, 1);
    vscc_push2(my_strlen, O_JMP, 0);
    vscc_push2(my_strlen, O_DECLABEL, 1);
    vscc_push3(my_strlen, O_RET, my_strlen_len);

    /* compile */
    struct vscc_codegen_interface interface = { 0 };
    vscc_codegen_implement_x64(&interface, ABI_SYSV);
    vscc_codegen(&ctx, &interface, &compiled, true);

    /* map executable into memory */
    void *exe = mmap(NULL, compiled.length, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    memcpy(exe, compiled.buffer, compiled.length);

    /* get specific function from executable */
    strlen_t my_strlen_fn = (strlen_t)(exe + get_offset_from_symbol(compiled.symbols, "strlen"));

    /* run executable */
    printf("result: %ld\n", my_strlen_fn("Hello, world!"));

    /* free executable */
    munmap(exe, compiled.length);
    return 0;
}
