/*
 * port.h - architecture and stack model portability for kernel64
 */

#ifndef	_PORT_H
#define	_PORT_H

#include <stdint.h>

/* Architecture detection */
#if defined(__x86_64__) || defined(_M_X64)
#define	KERNEL64_ARCH_X86_64	1
#endif

#if defined(__aarch64__) || defined(__arm64__)
#define	KERNEL64_ARCH_ARM64	1
#endif

#if defined(__arm__)
#define	KERNEL64_ARCH_ARM32	1
#endif

#if defined(KERNEL64_ARCH_X86_64) || defined(KERNEL64_ARCH_ARM64)
#define	KERNEL64_WORD_BITS	64
#elif defined(KERNEL64_ARCH_ARM32)
#define	KERNEL64_WORD_BITS	32
#else
#define	KERNEL64_WORD_BITS	16	/* original 8086/8088 build */
#endif

#if defined(__TURBOC__)
/* Borland's Turbo C is strictly a 16-bit real mode compiler */
#define	KERNEL64_WORD_BITS	16	/* original 8086/8088 build */
#endif

/* Stack model */
#define	KERNEL64_STACK_GROWS_DOWN	1

#if defined(KERNEL64_ARCH_X86_64) || defined(KERNEL64_ARCH_ARM64)
#define	KERNEL64_HAVE_STACK_GUARD	1
#endif

#if 0
/* Generic stack pointer type */
#if KERNEL64_WORD_BITS == 16
/* TODO untested on 16 bit machine - maybe never will! */
typedef uint16_t kernel64_stack_word_t;
typedef kernel64_stack_word_t *kernel64_stack_ptr_t;
#else
typedef uintptr_t kernel64_stack_ptr_t;
#endif
#endif

#endif	/* _PORT_H */
