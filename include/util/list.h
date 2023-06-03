#ifndef _VSCC_LIST_H_
#define _VSCC_LIST_H_

#include <stddef.h>
#include <stdbool.h>

typedef bool(*vscc_match_fn)(void *elem, void *data);

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * allocate element in list
 *  *root may be null
 */
void *vscc_list_alloc(void **root, size_t next_offset, size_t size);

/*
 * free specific elements in list
 */
void vscc_list_free_element(void **root, size_t next_offset, vscc_match_fn matcher, void *data);

/*
 * free entire list
 */
void vscc_list_free(void **root, size_t next_offset);

#ifdef __cplusplus
}
#endif

#endif /* _VSCC_LIST_H_ */