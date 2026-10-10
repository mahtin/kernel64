/*
 * kernel64.h for modern 64 bit arch's (aarch64 & arm64) plus armv7l (32 bit)
 */

#ifndef	_KERNEL64_H
#define	_KERNEL64_H
#include "port.h"
#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stddef.h>
#include <setjmp.h>

/* state of process (as a single character to make life easy) */
typedef enum {
	P_GO		= 'G',	/* proc setup but never run yet */
	P_RUNNING	= 'R',	/* proc up and running */
	P_WAITING	= 'W',	/* proc waiting (for wakeup) */
	P_SLEEPING	= 'S',	/* proc delayed/sleeping */
	P_DESTROYED	= 'D',	/* proc destroyed (because error) */
	P_ZOMBIE	= 'Z',	/* proc dead/zombie */
} proc64state;

typedef unsigned long long proc64magic;
typedef unsigned short proc64pri;
typedef unsigned int proc64pid;

struct proc64 {
	proc64magic magic;			/* proc64 structure magic number for sanity checks */
	struct proc64 *prev;			/* processes are link listed */
	struct proc64 *next;
	void *v;				/* transparent pointer to calling process's structure - for tying into exiting systems */
	proc64pid pid;				/* process ID */
	char *name;				/* process name */
	proc64state state;			/* process state */
	proc64pri priority;			/* process priority */
	jmp_buf context;			/* process stack and context for setjmp/longjmp */
	void *stack_real_base;			/* process actual mmap base (including guard) */
	size_t stack_real_size;			/* process total mmap size (usable + guard) */
	void *stack_base;			/* process stack */
	size_t stack_size;			/* process stack size */
	void (*entry)(void *);			/* store entry function and argument */
	void *arg;
	void *event;				/* process wait event */
	unsigned long duration;			/* process duration of timer, in ticks */
	unsigned long expiration;		/* process clock time at expiration */
};

extern struct proc64 *curproc;
extern struct proc64 *proclist;

extern void k64init(void);
struct proc64 *k64spawn(const char *name, void (*entry)(void *), void *arg, size_t stack_size, void *v);
void k64schedule(void);
void k64exit(void);
void k64nice(proc64pri pri);
void k64renice(struct proc64 *p, proc64pri pri);
void k64delay(unsigned long msecs);
void k64sleep(void * event);
void k64wakeup(struct proc64 *p);
struct proc64 *k64pid_to_proc(proc64pid pid);
void k64destroy(struct proc64 *p);
void k64yield(void);
int k64has_a_process_to_run(void);
size_t k64process_count(int incl_zombie);
size_t k64stack_used(struct proc64 *p);

void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg);	/* process stack switch primitive (implemented per-arch in assembly) */

void k64print_ps(void);
char **k64ps(void);

#define	STACK_GUARD_BYTE	0xAA		/* could be nearly any random value */
#define	PROC_MAGIC_NUMBER	0x70726f633634	/* "proc64" as hex */

#endif	/* KERNEL64_WORD_BITS */
#endif	/* _KERNEL64_H */
