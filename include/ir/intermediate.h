#ifndef _VSCC_INTERMEDIATE_H_
#define _VSCC_INTERMEDIATE_H_

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#define MAX_SYMBOL_LEN 64

#define SIZEOF_I8 sizeof(unsigned char)
#define SIZEOF_I16 sizeof(unsigned short)
#define SIZEOF_I32 sizeof(unsigned int)
#define SIZEOF_I64 sizeof(unsigned long long)
#define SIZEOF_PTR sizeof(void*)
#define NO_RETURN 0

#define IS_PARAMETER true
#define NOT_PARAMETER false

#define IS_VOLATILE true
#define NOT_VOLATILE false

enum vscc_opcode {
    O_INVALID,
    O_ADD,
    O_LOAD,
    O_STORE,
    O_SUB,
    O_MUL,
    O_DIV,
    O_XOR,
    O_AND,
    O_OR,
    O_SHL,
    O_SHR,
    O_CMP,
    O_JMP,
    O_JE,
    O_JNE,
    O_JBE,
    O_JA,
    O_JS,
    O_JNS,
    O_JP,
    O_JNP,
    O_JL,
    O_JGE,
    O_JLE,
    O_JG,
    O_RET,
    O_DECLABEL,
    O_PSHARG,
    O_SYSCALL,
    O_SYSCALL_PSHARG,
    O_CALL,
    O_CALLADDR,
    O_LEA
};

enum vscc_data_movement {
    M_INVALID,
    M_REG_IMM,
    M_REG_REG,
    M_REG,
    M_IMM
};

enum vscc_data_scope {
    S_INVALID,
    S_LOCAL,
    S_GLOBAL,
    S_PARAMETER
};

struct vscc_register {
    struct vscc_register *next;

    char symbol_name[MAX_SYMBOL_LEN];
    enum vscc_data_scope scope;
    
    size_t index;
    size_t size;
    size_t value;
    size_t stackpos;

    bool is_volatile;
    bool is_read;
    bool is_blocked;
    bool is_return_storage;
};

struct vscc_instruction {
    struct vscc_instruction *next;

    enum vscc_opcode opcode;
    enum vscc_data_movement movement;

    size_t size;

    struct vscc_register *reg1;
    struct vscc_register *reg2;
    uint64_t imm1;
    uint64_t imm2;
};

struct vscc_function {
    struct vscc_function *next;
    struct vscc_instruction *instruction_stream;
    struct vscc_register *register_stream;

    char symbol_name[MAX_SYMBOL_LEN];

    size_t instruction_count;
    size_t register_count;
    size_t return_size;
};

/*
 * to-do: keep track of memory usage
 */
struct vscc_context {
    struct vscc_function *function_stream;
    struct vscc_register *global_register_stream;
};

struct vscc_syscall_args {
    size_t syscall_id;

    size_t values[8];
    size_t sizes[8];
    enum vscc_data_movement type[8];
    size_t count;
};

#ifdef  __cplusplus
extern "C" {
#endif

struct vscc_function *vscc_init_function(struct vscc_context *context, char *symbol_name, size_t return_size);

struct vscc_register *vscc_alloc(struct vscc_function *this, char *symbol_name, size_t size, bool is_parameter, bool is_volatile);
struct vscc_register *vscc_alloc_global(struct vscc_context *this_context, char *symbol_name, size_t size, bool is_volatile);

struct vscc_function *vscc_fetch_function_by_name(struct vscc_context *this_context, char *symbol_name);

struct vscc_register *vscc_fetch_register_by_name(struct vscc_function *this, char *symbol_name);
struct vscc_register *vscc_fetch_register_by_index(struct vscc_function *this, size_t index);
struct vscc_register *vscc_fetch_global_register_by_name(struct vscc_context *this_context, char *symbol_name);

void vscc_unlink_free_register(struct vscc_function *this, struct vscc_register *reg);
void vscc_unlink_free_instruction(struct vscc_function *this, struct vscc_instruction *ins);

void vscc_push0(struct vscc_function *this, enum vscc_opcode opcode, struct vscc_register *dst, size_t src);
void vscc_push1(struct vscc_function *this, enum vscc_opcode opcode, struct vscc_register *dst, struct vscc_register *src);

void vscc_push2(struct vscc_function *this, enum vscc_opcode opcode, size_t imm);
void vscc_push3(struct vscc_function *this, enum vscc_opcode opcode, struct vscc_register *reg);

void vscc_pushs(struct vscc_function *this, struct vscc_syscall_args *args);

#ifdef __cplusplus
}
#endif

#define _vscc_call(this, callee, result) vscc_push0(this, O_CALL, result, (uintptr_t)callee)

#endif /* _VSCC_INTERMEDIATE_H_ */
