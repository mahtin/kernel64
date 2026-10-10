/*
 * kernel64util.h for modern 64 bit arch's (aarch64 & arm64) plus armv7l (32 bit)
 */

#ifndef	_KERNEL64UTIL_H
#define	_KERNEL64UTIL_H
#include "port.h"
#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stddef.h>

void k64stack_alloc(struct proc64 *p, size_t stack_size);
void k64stack_free(struct proc64 *p);

#endif	/* KERNEL64_WORD_BITS */
#endif	/* _KERNEL64UTIL_H */
