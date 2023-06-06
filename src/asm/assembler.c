#include <vscc.h>
#include <util/list.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static bool equals(void *a, void *b)
{
    return a == b;
}

uintptr_t vscc_asm_get_label(struct vscc_label *label_map, size_t id)
{
    while (label_map) {
        if (label_map->id == id)
            return label_map->address;
        label_map = label_map->next;
    }
    return NOT_FOUND;
}

void vscc_asm_push_label(struct vscc_label **label_map, size_t id, uintptr_t address, bool is_function)
{
    struct vscc_label *label = vscc_list_alloc((void**)label_map, 0, sizeof(struct vscc_label));

    label->id = id;
    label->address = address;
    label->is_function = is_function;
}

void vscc_asm_push_fill_in(struct vscc_fill_in_jmp_label **fill_in, size_t cur_size, size_t id, uintptr_t idx, size_t ins_len)
{
    struct vscc_fill_in_jmp_label *label = vscc_list_alloc((void**)fill_in, 0, sizeof(struct vscc_fill_in_jmp_label));

    label->cur_size = cur_size;
    label->id = id;
    label->idx = idx;
    label->instuction_len = ins_len;
}

uint32_t vscc_asm_get_offset(struct vscc_asm_context *assembler, struct vscc_label *label_map, size_t id, size_t ins_length, bool override, size_t override_cur)
{
    return vscc_asm_get_label(label_map, id) - ins_length - (!override ? vscc_asm_size(assembler) : override_cur);
}

void vscc_asm_fill_ins(struct vscc_fill_in_jmp_label **fill_in, struct vscc_label *label_map, struct vscc_asm_context *assembler, size_t id)
{
    for (struct vscc_fill_in_jmp_label *fill = *fill_in; fill;) {
        if (fill->id == id) {
            int offset = vscc_asm_get_offset(assembler, label_map, fill->id, fill->instuction_len, true, fill->cur_size);
            vscc_asm_overwrite(assembler, fill->idx, &offset, sizeof(offset));

            struct vscc_fill_in_jmp_label *next = fill->next;
            vscc_list_free_element((void**)fill_in, 0, equals, fill);
            fill = next;
        }
        else {
            fill = fill->next;
        }
    }
}

size_t vscc_asm_size(struct vscc_asm_context *ctx)
{
    size_t size = 0;
    
    for (struct vscc_asm_instruction *ins = ctx->instruction_stream; ins; ins = ins->next)
        size += ins->length;
    
    return size;
}

void vscc_asm_overwrite(struct vscc_asm_context *asmh, int dst_idx, void* src, size_t length)
{
    int current_idx = 0;
    for (struct vscc_asm_instruction *ins = asmh->instruction_stream; ins; ins = ins->next) {
        if (current_idx <= dst_idx && dst_idx <= current_idx + ins->length) {
            memcpy(&ins->buffer[dst_idx - current_idx], src, length);
        }
        current_idx += ins->length;
    }
}

void vscc_asm_encode(struct vscc_asm_context *context, int rex, int count, ...)
{
    struct vscc_asm_instruction *ins = vscc_list_alloc((void**)&context->instruction_stream, 0, sizeof(struct vscc_asm_instruction));
    va_list list;

    /*
     * pre-process to get allocation size
     */
    size_t alloc_size = 0;
    va_start(list, count);
    for (int i = 0; i < count * 2; i += 2) {
        va_arg(list, size_t); // discard
        alloc_size += va_arg(list, size_t);
    }
    va_end(list);

    if (rex)
        alloc_size++;

    /*
     * allocate buffer
     */
    ins->buffer = calloc(1, alloc_size);
    ins->length = alloc_size;

    /*
     * copy contents
     */
    va_start(list, count);
    int offset = 0;

    if (rex) {
        ins->buffer[0] = rex;
        offset = 1;
    }

    for (int i = 0; i < count * 2; i += 2) {
        size_t value = va_arg(list, size_t);
        size_t size = va_arg(list, size_t); 
        memcpy(&ins->buffer[offset], &value, size);
        offset += size;
    }
    va_end(list);
}