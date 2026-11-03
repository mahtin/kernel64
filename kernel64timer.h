/*
 * kernel64timer.h
 * for modern 64bit processor families: x86_64, aarch64/arm64.
 * plus some 32bit processor families: i386, armv6l/armv7l.
 */

#ifndef	_KERNEL64TIMER
#define	_KERNEL64TIMER
#include "port.h"
#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

void k64timer_init(void (*callback)(size_t msecs));
size_t k64timer_time_in_msecs(void);
void k64timer_stop(void);
void k64timer_pause(size_t msecs);

#define	TICKS_PER_SECOND	50		/* as good as any value for now */

#endif	/* KERNEL64_WORD_BITS */
#endif	/* _KERNEL64TIMER */
