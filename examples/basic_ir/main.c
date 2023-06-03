#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <vscc.h>

int main()
{
    struct vscc_context ctx = { 0 };

    struct vscc_function *my_main = vscc_init_function(&ctx, "main", SIZEOF_I64);
    struct vscc_register *my_main_a = vscc_alloc(my_main, "a", SIZEOF_I64, NOT_PARAMETER, NOT_VOLATILE);
    struct vscc_register *my_main_b = vscc_alloc(my_main, "b", SIZEOF_I64, NOT_PARAMETER, NOT_VOLATILE);
    struct vscc_register *my_main_c = vscc_alloc(my_main, "c", SIZEOF_I64, NOT_PARAMETER, NOT_VOLATILE);

    vscc_push0(my_main, O_LOAD, my_main_a, 9);
    vscc_push0(my_main, O_LOAD, my_main_b, 9);
    vscc_push0(my_main, O_ADD, my_main_a, 16);
    vscc_push1(my_main, O_ADD, my_main_b, my_main_a);
    vscc_push3(my_main, O_RET, my_main_b); // return 34

    printf("before:\n%s\n", vscc_fmt_ir_str(&ctx, 512)); 

    printf("eliminated %d registers from function @%s:i%ld\n\n", vscc_optfn_elim_dead_store(my_main), my_main->symbol_name, my_main->return_size * 8);
    printf("eliminated %d instructions from function @%s:i%ld\n\n", vscc_optfn_const_propagation(my_main), my_main->symbol_name, my_main->return_size * 8);

    printf("after:\n%s\n", vscc_fmt_ir_str(&ctx, 512)); 
    return 0;
}