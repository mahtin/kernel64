/*
 * kernel64util.h
 * for modern 64bit processor families: x86_64, aarch64/arm64.
 * plus some 32bit processor families: i386, armv6l/armv7l.
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
