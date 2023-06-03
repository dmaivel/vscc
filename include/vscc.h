#ifndef _VSCC_H_
#define _VSCC_H_

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#include <assert.h>

#include <ir/intermediate.h>
#include <ir/fmt.h>
#include <asm/assembler.h>
#include <opt/opt.h>

#endif /* _VSCC_H_ */