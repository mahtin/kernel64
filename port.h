/*
 * port.h - architecture and stack model portability for kernel64
 */

#ifndef	_PORT_H
#define	_PORT_H

/* Architecture detection */

#if defined(__x86_64__) || defined(_M_X64)
#define	KERNEL64_WORD_BITS	64
#endif

#ifdef _WIN32
#define	KERNEL64_WORD_BITS	64
#endif

#if defined(__aarch64__) || defined(__arm64__)
#define	KERNEL64_WORD_BITS	64
#endif

#if defined(__riscv) && __riscv_xlen == 64
#define	KERNEL64_WORD_BITS	64
#endif
#if defined(__riscv) && __riscv_xlen == 32
#define	KERNEL64_WORD_BITS	32
#endif

#if defined(__arm__)
#define	KERNEL64_WORD_BITS	32
#endif

#if defined(__i386__) || defined(_M_IX86)
#define	KERNEL64_WORD_BITS	32
#endif

#endif	/* _PORT_H */
