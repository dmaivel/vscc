#include <vscc.h>
#include <util/list.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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