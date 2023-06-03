#ifndef _VSCC_OPTIMIZER_H_
#define _VSCC_OPTIMIZER_H_

#include <ir/intermediate.h>

#ifdef  __cplusplus
extern "C" {
#endif

int vscc_optfn_elim_dead_store(struct vscc_function *function);
int vscc_optfn_const_propagation(struct vscc_function *function);

#ifdef __cplusplus
}
#endif

#endif /* _VSCC_OPTIMIZER_H_ */