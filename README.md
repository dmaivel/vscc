# vscc ![license](https://img.shields.io/badge/license-MIT-blue)  <img style="float: right;" src="https://cdn-icons-png.flaticon.com/512/547/547433.png" alt=moon width="48" height="48">

An experimental, lightweight, fast x86-64 compiler backend library, with no dependencies, written in C99, with the ability generate isolated bytecode, compliant with the SYS-V ABI. 

## build (cmake)
```bash
git clone https://github.com/dmaivel/vscc.git
cd vscc
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
To build one of the examples, uncomment the `frontend` lines from the cmake file.

## examples
The primary example is [pyvscc](https://github.com/dmaivel/pyvscc), which is an experimental python compiler. However, this repository contains additional, smaller examples.
The `examples` directory contains the following subprojects:
- `basic_ir`: tests the intermediate language
- `codecheck`: tests the ir, optimization, symbol generation and code generation
- `mm_test`: test the memory manager (lists)
- `strlen_jit`: generate a `strlen` implementation at runtime

## to-do
This project is currently unfinished with a vast majority of the feature set either needing to be finished, polished, rewritten, or awaiting implementation. This is a list of priorities for future development:
- [x] Internal memory manager
- [ ] User memory manager (free code, symbols, ir, memory limits)
- [x] Refactor `codegen` implementation
    - create a codegen interface structure, contains function pointers
    - base for adding support for new ABIs/architectures
- [x] Save/load IR to and from storage
- [ ] Full Sys-V support
- [x] Basic support for all IR opcodes in `codegen`
- [ ] Finish implementing all IR opcodes in `codegen`
- [ ] Add/finish 8-bit & 16-bit support
- [ ] Add floating point support
- [ ] Add more optimization routines

Lower priority features:
- [ ] Support IR as source instead of just binary
- [ ] MS-ABI support
- [ ] x86 (32-bit) support
- [ ] AVX2 support

# features

## intermediate language
### opcodes
| Opcode | Description | Destination Operand | Source Operand | Support |
| --- | ----------- | --- | --- | --- |
| O_ADD | Addition | Register | Register/Immediate | :warning: |
| O_LOAD | Load value from memory into register | Register | Register/Immediate | :warning: |
| O_STORE | Store value into register | Register | Register/Immediate | :heavy_check_mark: |
| O_SUB | Subtraction | Register | Register/Immediate | :warning: |
| O_MUL | Multiplication | Register | Register/Immediate | :warning: |
| O_DIV | Division | Register | Register/Immediate | :warning: |
| O_XOR | Exclusive Or | Register | Register/Immediate | :warning: |
| O_AND | And | Register | Register/Immediate | :warning: |
| O_OR | Or | Register | Register/Immediate | :warning: |
| O_SHL | Left Shift | Register | Register/Immediate | :warning: |
| O_SHR | Right Shift | Register | Register/Immediate | :warning: |
| O_CMP | Compare register to value | Register | Register/Immediate | :warning: |
| O_JMP | Jump | Immediate |  | :heavy_check_mark: |
| O_JE | Jump if equal | Immediate |  | :heavy_check_mark: |
| O_JNE | Jump if not equal | Immediate |  | :heavy_check_mark: |
| O_JBE | Jump if below/equal | Immediate |  | :heavy_check_mark: |
| O_JA | Jump if above | Immediate |  | :heavy_check_mark: |
| O_JS | Jump if sign | Immediate |  | :heavy_check_mark: |
| O_JNS | Jump if not sign | Immediate |  | :heavy_check_mark: |
| O_JP | Jump if parity | Immediate |  | :heavy_check_mark: |
| O_JNP | Jump if not parity | Immediate |  | :heavy_check_mark: |
| O_JL | Jump if less | Immediate |  | :heavy_check_mark: |
| O_JGE | Jump if greater/equal | Immediate |  | :heavy_check_mark: |
| O_JLE | Jump if less/equal | Immediate |  | :heavy_check_mark: |
| O_JG | Jump if greater | Immediate |  | :heavy_check_mark: |
| O_RET | Return | Register/Immediate | | :heavy_check_mark: |
| O_DECLABEL | Declare a label within a function for jumps | Immediate | | :heavy_check_mark: |
| O_PSHARG | Push an argument | Register/Immediate | | :warning: |
| O_SYSCALL | Perform syscall (use vscc_pushs) | | | :heavy_check_mark: |
| O_SYSCALL_PSHARG | Push an argument for syscall (don't use unless not using vscc_pushs) | Register/Immediate | | :warning: |
| O_CALL | Call a function | Register (return value storage) | Immediate (pointer to function) | :heavy_check_mark: |
| O_LEA | Load effective address of register | Register | Register/Immediate | :warning: |

:x: no support
:warning: partial support
:heavy_check_mark: full support

### context initialization
```c
struct vscc_context ctx = { 0 };
```
### function initialization
```c
struct vscc_function *function = vscc_init_function(&ctx, "function_name", SIZEOF_I32); // NO_RETURN, SIZEOF_I8, SIZEOF_I16, SIZEOF_I32, SIZEOF_I64, SIZEOF_PTR
```
### register allocation
```c
struct vscc_register *variable = vscc_alloc(function, "variable_name", SIZEOF_I32, NOT_PARAMETER, NOT_VOLATILE); // [NOT_PARAMETER/IS_PARAMETER], [NOT_VOLATILE/IS_VOLATILE]
```
### global register allocation
```c
struct vscc_register *global_variable = vscc_alloc_global(&ctx, "my_global_name", SIZEOF_I32, NOT_VOLATILE);
```
### push instructions
```c
/* standard push instructions */ 
vscc_push0(function, O_..., reg, imm);
vscc_push1(function, O_..., reg, reg);
vscc_push2(function, O_..., imm);
vscc_push3(function, O_..., reg);

/* syscall */
vscc_pushs(function, &syscall_desc);

/* call */
vscc_push0(function, O_CALL, variable_return_storage, (uintptr_t)callee_function);
_vscc_call(function, callee_function, variable_return_storage); // alternative to above
```
### syscall descriptor example (write(...))
```c
struct vscc_syscall_args syscall_write = {
    .syscall_id = 1,
    .count = 3,

    .values = { 1, (uintptr_t)string, (size_t)len },
    .type = { M_IMM, M_REG, M_REG }
};
```
### saving/loading context
```c
bool status;

status = vscc_ir_save(&ctx, "/path/to/save.vscc");
status = vscc_ir_load(&ctx, "/path/to/save.vscc");
```

## code generation
### context initialization
```c
struct vscc_codegen_data compiled = { 0 }; 
struct vscc_codegen_interface interface = { 0 };
```
### compile context
```c
vscc_codegen_implement_x64(&interface, ABI_SYSV);
vscc_codegen(&ctx, &interface, &compiled, true); // true: generate symbols
```

## optimization
optimizing routines must be called before `codegen`
### eliminate dead store
```c
vscc_optfn_elim_dead_store(function);
```