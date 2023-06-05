#ifndef _VSCC_UTIL_H_
#define _VSCC_UTIL_H_

#include <ir/intermediate.h>

#define VSCC_IR_FMT_MAGIC 0x43435356 // VSCC

#ifdef  __cplusplus
extern "C" {
#endif

char *vscc_ir_str(struct vscc_context *ctx, size_t max_strlen);
bool vscc_ir_save(struct vscc_context *ctx, char *path, bool is_src);
bool vscc_ir_load(struct vscc_context *ctx, char *path, bool is_src);

#ifdef __cplusplus
}
#endif

#endif /* _VSCC_UTIL_H_ */