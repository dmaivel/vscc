#ifndef _VSCC_CODEGEN_H_
#define _VSCC_CODEGEN_H_

#include <ir/intermediate.h>
#include <asm/assembler.h>

#define DECLABEL_END_FUNCTION -1

typedef void(*vscc_func_setup)(struct vscc_compiled_data *out, struct vscc_asm_context *asmh, struct vscc_function *function, bool generate_symbols);
typedef void(*vscc_codegen_impl)(struct vscc_asm_context *asmh, struct vscc_instruction *instruction);

struct vscc_codegen_interface {
    vscc_func_setup startfn;
    vscc_func_setup endfn;

    vscc_codegen_impl addfn;
    vscc_codegen_impl loadfn;
    vscc_codegen_impl storefn;
    vscc_codegen_impl subfn;
    vscc_codegen_impl mulfn;
    vscc_codegen_impl divfn;
    vscc_codegen_impl xorfn;
    vscc_codegen_impl andfn;
    vscc_codegen_impl orfn;
    vscc_codegen_impl shlfn;
    vscc_codegen_impl shrfn;
    vscc_codegen_impl cmpfn;
    vscc_codegen_impl jmpfn;
    vscc_codegen_impl retfn;
    vscc_codegen_impl pshargfn;
    vscc_codegen_impl syscallfn;
    vscc_codegen_impl callfn;
    vscc_codegen_impl leafn;
};

void vscc_codegen(struct vscc_context *context, struct vscc_codegen_interface *interface, struct vscc_compiled_data *out, bool generate_symbols);

#endif