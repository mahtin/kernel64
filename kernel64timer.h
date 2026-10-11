/*
 * kernel64timer.h for modern 64 bit arch's (aarch64 & arm64) plus armv7l (32 bit)
 */

#ifndef	_KERNEL64TIMER
#define	_KERNEL64TIMER
#include "port.h"
#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

void k64timer_init(void (*callback)(void));
size_t k64timer_ticks(void);
void k64timer_stop(void);
void k64timer_pause(void);

#define	MSECS_PER_TICK	50		/* as good as any value for now */

#endif	/* KERNEL64_WORD_BITS */
#endif	/* _KERNEL64TIMER */
