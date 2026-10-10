/*
 * kernel64.c for modern 64 bit arch's (aarch64 & arm64) plus armv7l (32 bit)
 */

#include "port.h"

#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#include "kernel64.h"
#include "kernel64util.h"

#define	OPTION_START_OF_LIST	0		/* 1 if process should go at begining of process list */
#define	OPTION_DELETE_ZOMBIE	0		/* 1 if process should be deleted vs being left as zombie in process list */

static int debug_flag = 0;			/* turn on for debug */

struct proc64 *curproc = NULL;
struct proc64 *proclist = NULL;

static proc64pid pid_number = 1;		/* start with process id 1 */

int k64stack_overflowed(struct proc64 *p)
{
	unsigned char *top = (unsigned char *)p->stack_base;
	unsigned char *bottom = top + p->stack_size - 1;
	unsigned char *s;
	size_t ii, band = 256;			/* Check a small band at the bottom (near guard page) or one page */

	if (band > p->stack_size)
		band = p->stack_size;

	for (ii=0,s=top;ii<band;ii++,s++) {
		if (*s != STACK_GUARD_BYTE)
			return 1;		/* overflow into guard region */
	}
	return 0;
}

static int k64stack_sanity_check(struct proc64 *p)
{
	/* Sanity check */
	if (p->stack_base == NULL)
		return 1;
	if (p->stack_size <= 4*1024)
		return 1;
	size_t used = k64stack_used(p);
	if (debug_flag) {
		fprintf(stderr, "k64stack_sanity_check(): p->stack_size: %zu used: %zu\n", p->stack_size, used);
		fflush(stderr);
	}
	if ((used+4096) >= p->stack_size)
		return 1;

	/* all ok */
	return 0;
}

static void k64zombies_reap(void)
{
	struct proc64 *p = proclist, *next;

	while (p) {
		next = p->next;
		/* dont prune pid 1 or current process or something already pruned */
		if (p->pid > 1 && p != curproc && p->stack_base != NULL && (p->state == P_ZOMBIE || p->state == P_DESTROYED)) {
			if (debug_flag) {
				fprintf(stderr, "k64zombies_reap(%d) p->name=\"%s\", p->state=%c\n", p->pid, p->name, p->state);
				fflush(stderr);
			}
			/* remove stack memory as a zombie can never run anymore */
			k64stack_free(p);
#if OPTION_DELETE_ZOMBIE == 1
			/* unlink from list */
			if (p->prev)
				p->prev->next = p->next;
			else
				proclist = p->next;
			if (p->next)
				p->next->prev = p->prev;
			/* now safe to free stack + PCB */
			if (p->name) {
				free(p->name);
				p->name = NULL;
			}
			free(p);
#endif
		}
		p = next;
	}
}

static void k64idle_process(void *arg)
{
	for (;;) {
		/* cleanup any zombie process stack allocations - only do this is init processs */
		k64zombies_reap();
		/* Optional: low-power hint, logging, etc. */
		/* k64sleep(event); or k64yield(); depending on your design */
		k64print_ps();
		if (k64process_count(0) > 1) {
			/* still has processes running so lets quickly get them going again */
			k64yield();
		} else {
#if 0
			/* no-one around - so we just sleep */
			usleep(1000*1000);
#else
			/* screw it - exit the item process */
			k64exit();
#endif
		}
	}
}

/* Init */
void k64init(void)
{
	proclist = NULL;
	curproc = NULL;

	/* create idle process first: pid 1 */
	struct proc64 *idle = k64spawn("[idle]", k64idle_process, NULL, 16*1024, NULL);
	if (!idle) {
		fprintf(stderr, "k64init(): [init] process start failed - THIS SHOULD NOT HAPPEN\n");
		fflush(stderr);
		abort();	/* kernel cannot run without idle */
	}
	idle->priority = 255;	/* lowest priority */
}

static void k64process_bootstrap(struct proc64 *p)
{
	/* Called only once per process, on its first scheduling */
	k64_switch_to_stack(p->stack_base, p->stack_size, p->entry, p->arg);	/* Switch to the process's stack and now safely running on the process stack */
	/* process entry(arg) returned - call exit for it */
	k64exit();								/* Clean up and schedule next process */
	/* Never returns */
	abort();
}

static void k64enter(struct proc64 *next)
{
	assert(next->magic == PROC_MAGIC_NUMBER);
	if (debug_flag) {
		fprintf(stderr, "k64enter(%d) next->name=\"%s\", next->state=%c\n", next->pid, next->name, next->state);
		fflush(stderr);
	}
	if (next->state == P_GO) {
		/* first startup of process */
		next->state = P_RUNNING;
		k64process_bootstrap(next);
		/* never returns */
	}
	if (1 || debug_flag) {
		fprintf(stderr, "k64enter(%d): next->name=\"%s\" doing longjmp:\n", next->pid, next->name);
		fflush(stderr);
	}
	longjmp(next->context, 1);
	/* never returns */
}

static void k64switch(struct proc64 *next)
{
	struct proc64 *prev = curproc;
	int r = -1;

	assert(next->magic == PROC_MAGIC_NUMBER);
	if (next == prev) {
		if (debug_flag) {
			fprintf(stderr, "k64switch(): next == prev - returning\n");
			fflush(stderr);
		}
		return;
	}
	if (debug_flag) {
		fprintf(stderr, "k64switch() next->pid=%d next->name=%s prev->pid=%d\n", next->pid, next->name, prev?prev->pid:-1);
		fflush(stderr);
	}

	curproc = next;
	if (1 || debug_flag) {
		fprintf(stderr, "k64switch(%d): next->name=\"%s\" setjmp() prev->pid=%d prev->name=\"%s\" rsp=%p\n", next->pid, next->name, prev?prev->pid:-1, prev?prev->name:"-", __builtin_frame_address(0));
		fflush(stderr);
	}
	if (prev == NULL || (r=setjmp(prev->context)) == 0) {
		if (1 || debug_flag) {
			fprintf(stderr, "k64switch(): setjmp() returns 0 - next up k64enter ...\n");
			fflush(stderr);
		}
		k64enter(next);
		/* never returns */
		fprintf(stderr, "k64switch(): k64enter returned - SHOULD NOT HAPPEN (ABORTING!)\n");
		fflush(stderr);
		abort();
	}
	if (1 || debug_flag) {
		fprintf(stderr, "k64switch(): returning (at end)\n");
		fflush(stderr);
	}
}

/* Spawn a new process */
struct proc64 *k64spawn(const char *name, void (*entry)(void *), void *arg, size_t stack_size, void *v)
{
	struct proc64 *p;
	int r;

	if (debug_flag) {
		fprintf(stderr, "k64spawn(\"%s\")\n", name?name:"null");
		fflush(stderr);
	}

	p = (struct proc64 *)malloc(sizeof(struct proc64));
	if (p == NULL)
		return NULL;

	memset(p, 0x00, sizeof(*p));
	p->magic = PROC_MAGIC_NUMBER;
	p->state = P_GO;
	p->priority = 80;			/* mimic (somewhat) the norms for priority values */
	p->duration = 0;
	p->pid = pid_number++;
	p->entry = entry;			/* entry point for new process */
	p->arg = arg;				/* args for new process */
	name = name ? name : "[proc]";
	p->name = malloc(strlen(name)+1);
	if (p->name == NULL) {
		free(p);
		return NULL;
	}
	strncpy(p->name, name, strlen(name)+1);

	if (stack_size == 0)
		stack_size = 64 * 1024;
	k64stack_alloc(p, stack_size);
	if (p->stack_base == NULL || p->stack_size == 0) {
		free(p);
		return NULL;
	}
	memset(p->stack_base, STACK_GUARD_BYTE, p->stack_size);

	if (debug_flag) {
		fprintf(stderr, "k64spawn(%d): p->name=\"%s\" setjmp() - first setup - rsp=%p\n", p->pid, p->name, __builtin_frame_address(0));
		fflush(stderr);
	}
	r = setjmp(p->context);				/* only save context, never run child branch */
	if (debug_flag) {
		fprintf(stderr, "k64spawn(): SAVE p->pid=%d return=%d\n", p->pid, r);
		fflush(stderr);
	}

#if OPTION_START_OF_LIST == 1
	/* add to start of list */
	p->next = proclist;
	if (proclist)
		proclist->prev = p;
	proclist = p;
#else
	/* add to end of list */
	if (proclist == NULL) {
		proclist = p;
	} else {
		struct proc64 *pp = proclist;
		while (pp->next != NULL)
			pp = pp->next;
		pp->next = p;
	}
	p->next = NULL;
#endif
	return p;
}

/* Schedule - Simple round-robin scheduler - with priority checking */
void k64schedule(void)
{
	struct proc64 *p, *start, *phighest, *pidle;

	if (debug_flag) {
		if (curproc)
			fprintf(stderr, "k64schedule(): curproc->pid=%d curproc->name=\"%s\"\n", curproc->pid, curproc->name);
		else
			fprintf(stderr, "k64schedule(): curproc NULL\n");
		fflush(stderr);
	}

	/* Full circular scan */
	if (curproc == NULL)
		start = proclist;
	else
		start = curproc->next ? curproc->next : proclist;
	p = start;
	phighest = NULL;
	pidle = NULL;
	while (1) {
		assert(p->magic == PROC_MAGIC_NUMBER);
#if 0
		if (debug_flag) {
			fprintf(stderr, "k64schedule: testing p->pid=%d p->state=%c p->priority=%d\n", p->pid, p->state, p->priority);
			fflush(stderr);
		}
#endif
		/* while we are here - deal with delayed process */
		if (p->state == P_SLEEPING) {
			/* is it time to wake up */
			/* TODO */
			p->duration -= 1;
			if (p->duration <= 0)
				p->state = P_RUNNING;
		}
		/* only consider runnable processes */
		if (p->state == P_GO || p->state == P_RUNNING) {
			if (p->pid == 1)
				pidle = p;
			if (phighest == NULL)
				phighest = p;
			if (p->priority < phighest->priority)
				phighest = p;
		}
		p = p->next;
		if (p == NULL) {
			/* loop back to the first process */
			p = proclist;
		}
		if (p == start) {
			/* we have checked all processes */
			if (debug_flag) {
				fprintf(stderr, "k64schedule: no ready process\n");
				fflush(stderr);
			}
			if (pidle && pidle->state == P_GO) {
				/* give the idle process a chance to start */
				p = pidle;
				break;
			}
			/* if possible select the process with the highest priorty - which will be idle if nothing else around */
			p = phighest?phighest:NULL;
			if (p == NULL)
				p = pidle;
			break;
		}
	}
	if (p == NULL)
		return;	/* no processes is ready to run - we have nothing to schedule */

	/* kick off processs p if its stack is ok */
	if (k64stack_sanity_check(p) || k64stack_overflowed(p)) {
		/* stack corrupt on this process */
		size_t used = k64stack_used(p);
		fprintf(stderr, "k64schedule(%d): name=%s stack_used=%zu / %zu STACK CORRUPT\n", p->pid, p->name, used, p->stack_size);
		fflush(stderr);
		p->state = P_DESTROYED;
		if (p == curproc) {
			curproc = NULL;
		}
		k64print_ps();
		/* TODO - not really true - we may have another process ready to fly */
		p = pidle;
	}
	if (debug_flag) {
		fprintf(stderr, "k64schedule: doing k64switch!\n");
		fflush(stderr);
	}
	k64switch(p);
}

/* Exit this process */
void k64exit(void)
{
	struct proc64 *p;

	if (curproc != NULL) {

		p = curproc;
		assert(p->magic == PROC_MAGIC_NUMBER);
		if (debug_flag) {
			fprintf(stderr, "k64exit(): p->pid=%d p->state=%c set state P_ZOMBIE\n", p->pid, p->state);
			fflush(stderr);
		}
		p->state = P_ZOMBIE;
		/* stack is not quite done with - Never free/unmap the stack you’re currently running on. */
		/* see idle process */

#if OPTION_DELETE_ZOMBIE == 1
		/* TODO this should not be run here as its the current process */
#if 0
		/* Remove from process list if desired */
		if (p->prev)
			p->prev->next = p->next;
		else
			proclist = p->next;
		/* get rid of name */
		if (p->name) {
			free(p->name);
			p->name = NULL;
		}
		/* finally can get rid of p */
		free(p);
#endif
#endif
		curproc = NULL;			/* we were the running process */
	}

	/* no running process at this point */
	if (debug_flag) {
		fprintf(stderr, "k64exit(): calling k64schedule\n");
		fflush(stderr);
	}
	k64schedule();
	/* Never return */
	/* NOTREACHED: we always longjmp into another process (idle if nothing else) */
	fprintf(stderr, "k64exit(): k64schedule() returned - SHOULD NOT HAPPEN - EXITING CLEANLY\n");
	fflush(stderr);
	/* we cleanly exit becuase this only happens when someone destroyed the idle process */
	exit(0);
}

/* Nice this process */
void k64nice(proc64pri pri)
{
	if (curproc == NULL)
		return;
	curproc->priority = pri<255?pri:255;
	k64schedule();
}

/* Renice another process */
void k64renice(struct proc64 *p, proc64pri pri)
{
	assert(p->magic == PROC_MAGIC_NUMBER);
	p->priority = pri<255?pri:255;
	k64schedule();
}

/* Delay this process */
void k64delay(unsigned long msecs)
{
	if (curproc == NULL)
		return;
	curproc->state = P_SLEEPING;
	curproc->duration = msecs;
	k64schedule();
}

/* Sleep this process */
void k64sleep(void * event)
{
	if (curproc == NULL)
		return;
	curproc->event = event;
	curproc->state = P_WAITING;
	k64schedule();
}

/* Wakeup another process */
void k64wakeup(struct proc64 *p)
{
	assert(p->magic == PROC_MAGIC_NUMBER);
	if (p && p->state == P_WAITING)
		p->state = P_RUNNING;
	k64schedule();
}

struct proc64 *k64pid_to_proc(proc64pid pid)
{
	struct proc64 *p;
	for (p=proclist;p!=NULL;p=p->next) {
		assert(p->magic == PROC_MAGIC_NUMBER);
		if (pid == p->pid)
			return p;
		p = p->next;
	}
	return NULL;
}

/* Destroy another process */
void k64destroy(struct proc64 *p)
{
	assert(p->magic == PROC_MAGIC_NUMBER);

	if (p->state == P_GO || p->state == P_RUNNING || p->state == P_SLEEPING || p->state == P_WAITING) {
		p->state = P_DESTROYED;
	}
	if (p == curproc) {
		curproc = NULL;
	}
	/* k64kyelid(); */
}

/* Yield this process */
void k64yield(void)
{
	if (curproc == NULL)
		return;
	curproc->state = P_RUNNING;	/* redundant but why not */
	k64schedule();
}

int k64has_a_process_to_run(void)
{
	struct proc64 *p;
	for (p=proclist;p!=NULL;p=p->next) {
		if (p->state == P_GO || p->state == P_RUNNING || p->state == P_SLEEPING || p->state == P_WAITING)
			return 1;
	}
	return 0;
}

size_t k64process_count(int incl_zombie)
{
	size_t nn;

	nn = 0;
	for (struct proc64 *p=proclist;p!=NULL;p=p->next) {
		nn++;
		if ((p->state == P_ZOMBIE || p->state == P_DESTROYED) && !incl_zombie)
			nn--;
	}
	return nn;
}

size_t k64stack_used(struct proc64 *p)
{
	unsigned char *top, *bottom, *s;

	if (p->stack_base == NULL || p->stack_size == 0) {
		/* unallocated (i.e. zombie) */
		return 0;
	}
	bottom = (unsigned char *)p->stack_base;		/* lowest address */
	top = bottom + p->stack_size;				/* highest address + 1 */
#if 0
	if (debug_flag > 1) {
		fprintf(stderr, "Top   : 0x%012lx %02x %02x %02x %02x %02x %02x %02x %02x\n",
			(unsigned long)top, *(top-0), *(top-1), *(top-2), *(top-3), *(top-4), *(top-5), *(top-6), *(top-7)
		);
		fprintf(stderr, "Bottom: 0x%012lx %02x %02x %02x %02x %02x %02x %02x %02x\n",
			(unsigned long)bottom, *(bottom+0), *(bottom+1), *(bottom+2), *(bottom+3), *(bottom+4), *(bottom+5), *(bottom+6), *(bottom+7)
		);
		fflush(stderr);
	}
#endif
	s = bottom;
	while (s < top && *s == STACK_GUARD_BYTE)
		s++;
	return top - s;
}

#endif	/* KERNEL64_WORD_BITS */
