#include "asm/codegen.h"
#include "asm/codegen/x64.h"
#include "ir/fmt.h"
#include <vscc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/mman.h>

#define LOG_DEFAULT "\033[0m"
#define LOG_RED "\033[0;31m"
#define LOG_GREEN "\033[0;32m"
#define LOG_YELLOW "\033[0;33m"
#define LOG_CYAN "\033[0;36m"

typedef uint64_t(*strlen_t)(char*);
typedef uint64_t(*print_t)(char*);

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

    /* attempt load */
    bool status = vscc_ir_load(&ctx, "codecheck.vscc");

    /* load failed, generate ir */
    if (!status) {
        printf("fail to find existing save of ir, re-generating...\n\n");

        /* main & local variable initialization */
        struct vscc_function *my_main = vscc_init_function(&ctx, "main", SIZEOF_I64);
        struct vscc_register *my_main_string = vscc_alloc(my_main, "string", SIZEOF_PTR, NOT_PARAMETER, NOT_VOLATILE);
        struct vscc_register *my_main_result = vscc_alloc(my_main, "result", SIZEOF_I32, NOT_PARAMETER, NOT_VOLATILE);

        /* strlen & local variable initialization */
        struct vscc_function *my_strlen = vscc_init_function(&ctx, "strlen", SIZEOF_I64);
        struct vscc_register *my_strlen_string = vscc_alloc(my_strlen, "str", SIZEOF_PTR, IS_PARAMETER, NOT_VOLATILE);
        struct vscc_register *my_strlen_len = vscc_alloc(my_strlen, "length", SIZEOF_I64, NOT_PARAMETER, NOT_VOLATILE);
        struct vscc_register *my_strlen_deref = vscc_alloc(my_strlen, "deref_char", SIZEOF_I8, NOT_PARAMETER, NOT_VOLATILE);

        /* print & local variable initialization */
        struct vscc_function *my_print = vscc_init_function(&ctx, "print", SIZEOF_I64);
        struct vscc_register *my_print_string = vscc_alloc(my_print, "str", SIZEOF_PTR, IS_PARAMETER, NOT_VOLATILE);
        struct vscc_register *my_print_len = vscc_alloc(my_print, "length", SIZEOF_I64, NOT_PARAMETER, NOT_VOLATILE);

        /* global variable for message */
        struct vscc_register *my_message = vscc_alloc_global(&ctx, "my_message", 15, NOT_VOLATILE);
        struct vscc_register *my_test_var = vscc_alloc_global(&ctx, "test_var", SIZEOF_I64, NOT_VOLATILE);

        /* syscall info for 'write' */
        struct vscc_syscall_args syscall_write = {
            .syscall_id = 1,
            .count = 3,

            .values = { 1, (uintptr_t)my_print_string, (size_t)my_print_len },
            .type = { M_IMM, M_REG, M_REG }
        };

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

        /* print definition */
        vscc_push3(my_print, O_PSHARG, my_print_string);
        vscc_push0(my_print, O_CALL, my_print_len, (uintptr_t)my_strlen);
        vscc_pushs(my_print, &syscall_write);
        vscc_push3(my_print, O_RET, my_print_len);

        /* main definition */
        vscc_push1(my_main, O_LEA, my_main_string, my_message);
        vscc_push3(my_main, O_PSHARG, my_main_string);
        vscc_push0(my_main, O_CALL, my_main_result, (uintptr_t)my_print);
        vscc_push0(my_main, O_STORE, my_test_var, 1234);
        vscc_push1(my_main, O_STORE, my_main_result, my_test_var);
        vscc_push0(my_main, O_DIV, my_main_result, 10);
        vscc_push3(my_main, O_RET, my_main_result);

        /* save */
        vscc_ir_save(&ctx, "codecheck.vscc");
    }
    else {
        printf("found existing save of ir, re-using\n\n");
    }

    /* compiled context */
    struct vscc_compiled_data compiled = { 0 };

    /* print intermediate representation */
    printf("%s", vscc_ir_str(&ctx, 512)); 

    /* print optimizations */
    for (struct vscc_function *fn = ctx.function_stream; fn; fn = fn->next)
        printf("elim_dead_store  [" LOG_CYAN "%-40s" LOG_DEFAULT " @ " LOG_RED "-%-5d" LOG_DEFAULT "]\n", fn->symbol_name, vscc_optfn_elim_dead_store(fn));    
    puts("");

    /* compile */
    struct vscc_codegen_interface interface = { 0 };
    vscc_codegen_implement_x64(&interface, ABI_SYSV);
    vscc_codegen(&ctx, &interface, &compiled, true);

    /* print generated symbols */
    for (struct vscc_symbol *symbol = compiled.symbols; symbol; symbol = symbol->next) {
        printf("generated symbol [" LOG_CYAN "%-40s" LOG_DEFAULT " @ " LOG_GREEN "+%-5ld" LOG_DEFAULT "]\n", symbol->symbol_name, symbol->offset);
    }
    puts("");

    /* print executable bytecode */
    for (int i = 0; i < compiled.length; i++) {
        printf("%02hhx ", compiled.buffer[i]);
    }
    puts("\n");
    fflush(stdout);

    /* map executable into memory */
    void *exe = mmap(NULL, compiled.length, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    memcpy(exe, compiled.buffer, compiled.length);

    /* get specific function from executable */
    strlen_t my_strlen_fn = (strlen_t)(exe + get_offset_from_symbol(compiled.symbols, "strlen<i64>()"));

    /* map string into executable memory and call specific function */
    char *message = "Hello, world!\n";
    memcpy(exe + get_offset_from_symbol(compiled.symbols, "my_message<15b, globl>"), message, my_strlen_fn(message));

    /* run executable */
    printf("program result: %d\n", ((int(*)())exe)());

    /* call another specific function from executable */
    print_t my_print_fn = (print_t)(exe + get_offset_from_symbol(compiled.symbols, "print<i64>()"));
    my_print_fn("Hello from the main main!\n");

    /* value of */
    printf("my_test_var = %ld\n", *(size_t*)(exe + get_offset_from_symbol(compiled.symbols, "test_var<i64, globl>")));

    /* free executable */
    munmap(exe, compiled.length);
    return 0;
}
