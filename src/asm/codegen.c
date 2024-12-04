#include <vscc.h>
#include <asm/assembler.h>
#include <util/list.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool equals_label(void *a, void *b)
{
    struct vscc_label *x = a;
    return !x->is_function;
}

void vscc_codegen(struct vscc_context *context, struct vscc_codegen_interface *interface, struct vscc_codegen_data *out, bool generate_symbols)
{
    vscc_codegen_impl call_table[] = {
        NULL, /* invalid */
        interface->addfn,
        interface->loadfn,
        interface->storefn,
        interface->subfn,
        interface->mulfn,
        interface->divfn,
        interface->xorfn,
        interface->andfn,
        interface->orfn,
        interface->shlfn,
        interface->shrfn,
        interface->cmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->jmpfn,
        interface->retfn,
        NULL, /* declabel */
        interface->pshargfn,
        interface->syscallfn,
        interface->pshargfn,
        interface->callfn,
        interface->callfn,
        interface->leafn,
    };

    struct vscc_asm_context assembler = { 0 };
    char _str_temp[15] = { 0 };

    for (struct vscc_function *function = context->function_stream; function; function = function->next) {

        /* to-do: determine release/debug for perf? maybe impact no big deal */
        if (unlikely(generate_symbols))
            vscc_symbol_generate_function(&out->symbols, function, vscc_asm_size(&assembler));

        vscc_asm_push_label(&assembler, (size_t)function, vscc_asm_size(&assembler), true);
        assembler.argc = 0;
        
        /* check for any labels that can be filled with the proper relative offsets */
        vscc_asm_fill_ins(&assembler, (size_t)function);

        /* startup */
        interface->prologuefn(out, &assembler, function, generate_symbols);

        /* instruction translation */
        for (struct vscc_instruction *instruction = function->instruction_stream; instruction; instruction = instruction->next) {
            switch (instruction->opcode) {
            case O_DECLABEL:
                vscc_asm_push_label(&assembler, instruction->imm1, vscc_asm_size(&assembler), false);
                vscc_asm_fill_ins(&assembler, instruction->imm1);
                break;
            default:;
                vscc_codegen_impl fn = call_table[instruction->opcode];
                assert(fn);
                fn(&assembler, instruction);;
                break;
            }
        }

        /*
         * to-do: maybe determine ret count and decide whether these two lines require execution
         */
        vscc_asm_push_label(&assembler, DECLABEL_END_FUNCTION, vscc_asm_size(&assembler), false);
        vscc_asm_fill_ins(&assembler, DECLABEL_END_FUNCTION);

        interface->epiloguefn(out, &assembler, function, generate_symbols);

        vscc_list_free_element((void**)&assembler.label_map, 0, equals_label, NULL);
    }

    /* global registers */
    /* to-do: support global registers being stored at the top of the buffer (should be simpler) */
    size_t offs = vscc_asm_size(&assembler);
    if (likely(context->global_register_stream != NULL)) {
        for (struct vscc_register *reg = context->global_register_stream; reg; reg = reg->next) {
            if (unlikely(generate_symbols))
                vscc_symbol_generate_global(&out->symbols, reg, offs);

            vscc_asm_push_label(&assembler, (size_t)reg, offs, false);
            offs += reg->size;
        }

        for (struct vscc_register *reg = context->global_register_stream; reg; reg = reg->next)
            vscc_asm_fill_ins(&assembler, (size_t)reg);
    }

    /* write compiled code to output */
    out->length = offs;
    out->buffer = calloc(1, out->length);

    /* copy memory contents */
    size_t idx = 0;
    for (struct vscc_asm_instruction *ins = assembler.instruction_stream; ins; ins = ins->next) {
        memcpy(&out->buffer[idx], ins->buffer, ins->length);
        idx += ins->length;
    }

    /* free memory */
    vscc_list_free((void**)&assembler.instruction_stream, 0);
    vscc_list_free((void**)&assembler.label_map, 0);
    vscc_list_free((void**)&assembler.fill_in, 0);
}
