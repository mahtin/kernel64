/*
 * kernel64.h
 * for modern 64bit processor families: x86_64, aarch64/arm64.
 * plus some 32bit processor families: i386, armv6l/armv7l.
 */

#ifndef	_KERNEL64_H
#define	_KERNEL64_H
#include "port.h"
#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stddef.h>

/* state of process (as a single character to make life easy) */
typedef enum {
	P_GO		= 'G',	/* proc setup but never run yet */
	P_RUNNING	= 'R',	/* proc up and running */
	P_WAITING	= 'W',	/* proc waiting (for wakeup) */
	P_DELAYED	= 'S',	/* proc delayed (i.e sleep in unix) */
	P_DESTROYED	= 'D',	/* proc destroyed (because error) */
	P_ZOMBIE	= 'Z',	/* proc dead/zombie */
} proc64state;

typedef unsigned long long proc64magic;
typedef unsigned short proc64pri;
typedef unsigned int proc64pid;

#include <setjmp.h>
typedef jmp_buf proc64context;

struct proc64 {
	proc64magic magic;			/* proc64 structure magic number for sanity checks */
	struct proc64 *prev;			/* processes are link listed */
	struct proc64 *next;
	void *v;				/* transparent pointer to calling process's structure - for tying into exiting systems */
	proc64pid pid;				/* process ID */
	char *name;				/* process name */
	proc64state state;			/* process state */
	proc64pri priority;			/* process priority */
	proc64context context;			/* process stack and context for setjmp/longjmp */
	size_t msec_elapsed;			/* count of processor time */
	void *stack_real_base;			/* process actual mmap base (including guard) */
	size_t stack_real_size;			/* process total mmap size (usable + guard) */
	void *stack_base;			/* process stack */
	size_t stack_size;			/* process stack size */
	const void (*entry)(const void *);	/* store entry function and argument */
	const void *arg;
	void *event;				/* process wait event */
	size_t expiration;			/* process clock time at expiration of sleep */
};

extern struct proc64 *curproc;
extern struct proc64 *proclist;

extern void k64init(const int debug);
struct proc64 *k64spawn(const char *name, void (*entry)(const void *), const void *arg, size_t stack_size, const void *v);
void k64schedule(void);
void k64exit(void);
void k64nice(const proc64pri pri);
void k64renice(struct proc64 *p, const proc64pri pri);
void k64delay(const size_t msecs);
void k64sleep(const void * event);
void k64wakeup(struct proc64 *p);
void k64pause(void);
struct proc64 *k64pid_to_proc(const proc64pid pid);
void k64destroy(struct proc64 *p);
void k64yield(void);
int k64has_a_process_to_run(void);
size_t k64process_count(const int incl_zombie);
int k64stack_overflowed(const struct proc64 *p);

void k64_switch_to_stack(void *stack_base, size_t stack_size, const void (*entry)(const void *), const void *arg);	/* process stack switch primitive (implemented per-arch in assembly) */

void k64print_ps(void);

#define	PROC_MAGIC_NUMBER	0x5f70726f6336345f 	/* "_proc64_" as hex (64 bits for number i.e. long long) */

#endif	/* KERNEL64_WORD_BITS */
#endif	/* _KERNEL64_H */
