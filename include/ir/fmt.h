#ifndef _VSCC_UTIL_H_
#define _VSCC_UTIL_H_

#include <ir/intermediate.h>

#define VSCC_FMT_MAGIC 0x63637376

#define VSCC_FMT_IS_SOURCE 0
#define VSCC_FMT_IS_BINARY 1

struct vscc_ir_file_fmt {
    int magic;
    int type;
    void *data;
};

#define VSCC_CALCULATE_DATA_SIZE(len) (len - sizeof(struct vscc_ir_file_fmt) - sizeof(void*))

#ifdef  __cplusplus
extern "C" {
#endif

char *vscc_fmt_ir_str(struct vscc_context *ctx, size_t max_strlen);
void vscc_fmt_cvt_ir_file(struct vscc_context *ctx, char *path, bool is_src);
void vscc_fmt_cvt_file_ir(struct vscc_context *ctx, char *path, bool is_src);

#ifdef __cplusplus
}
#endif

#endif /* _VSCC_UTIL_H_ */