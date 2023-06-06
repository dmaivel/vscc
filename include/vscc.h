#ifndef _VSCC_H_
#define _VSCC_H_

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define VSCC_TARGET_X64

#include <assert.h>

#include <ir/intermediate.h>
#include <ir/fmt.h>
#include <asm/assembler.h>
#include <asm/codegen.h>
#include <opt/opt.h>

#ifdef VSCC_TARGET_X64
#include <asm/codegen/x64.h>
#endif

#endif /* _VSCC_H_ */