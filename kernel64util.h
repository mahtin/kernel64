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
size_t k64stack_used(const struct proc64 *p);
int k64stack_back_check(const struct proc64 *p);

typedef unsigned long long k64stack_guard;
#define	STACK_GUARD_NUMBER	0x4b3634535441434b	/* "K64STACK" as hex (64 bits for number i.e. long long) */

#endif	/* KERNEL64_WORD_BITS */
#endif	/* _KERNEL64UTIL_H */
