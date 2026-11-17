/*
 * kernel64.c
 * for modern 64bit processor families: x86_64, aarch64/arm64.
 * plus some 32bit processor families: i386, armv6l/armv7l.
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
#include "kernel64timer.h"

#define	OPTION_START_OF_LIST	0		/* 1 if process should go at begining of process list */
#define	OPTION_DELETE_ZOMBIE	0		/* 1 if process should be deleted vs being left as zombie in process list */

static int debug_flag = 0;			/* turn on for debug */

static int init_done_flag = 0;			/* 1 after init */
struct proc64 *curproc = NULL;
struct proc64 *proclist = NULL;

static proc64pid pid_number = 1;		/* start with process id 1 */

static int timer_started = 0;			/* only start the timer once */

static size_t next_expiration = 0;		/* we keep track of all the delayed processes so we can pause longer than a single tick */

static void k64zombies_reap(void)
{
	struct proc64 *p = proclist, *next;

	while (p) {
		next = p->next;
		/* dont prune pid 1 or current process or something already pruned */
		if (p->pid > 1 && p != curproc && p->stack_base != NULL && (p->state == P_ZOMBIE || p->state == P_DESTROYED)) {
			if (debug_flag) {
				fprintf(stderr, "k64zombies_reap(%u) p->name=\"%s\", p->state=%c\n", p->pid, p->name, p->state);
				fflush(stderr);
			}
			if (p->state == P_DESTROYED) {
				/* report a stack overflow - do it from a safe stack - i.e. here in idle */
				size_t used = k64stack_used(p);
				fprintf(stderr, "k64zombies_reap(%u): p->name=%s stack_used=%zu / %zu -- STACK CORRUPT!\n", p->pid, p->name, used, p->stack_size);
				fflush(stderr);
				k64print_ps();
			}
			/* remove stack memory from that process as a zombie can never run anymore */
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
			p->magic = 0;			/* we do this as the process is done for and we don't want to reference free'ed memory */
			free(p);			/* finally can get rid of p's memory */
#endif
		}
		p = next;
	}
}

static void k64idle_process(const void *arg)
{
	for (;;) {
		/* cleanup any zombie process stack allocations - only do this is init processs */
		k64zombies_reap();

		if (k64process_count(0) <= 1) {
			/* all processes exited (except idle) - so exit the idle process and end everything */
			k64exit();
		}

		/* Optional: low-power hint, logging, etc. */

		if (debug_flag) {
			fprintf(stderr, "k64idle_process(): pause!\n");
			fflush(stderr);
		}
		/* idle is only called when no process is able to run - so we wait for the tick timer to go off as we have nothing else to do */
		if (next_expiration != 0) {
			/* we have an active timer */
			if (next_expiration <= k64timer_time_in_msecs()) {
				if (debug_flag) {
					fprintf(stderr, "k64idle_process(): pause - %zu %zu - pause is up!\n", next_expiration, k64timer_time_in_msecs());
					fflush(stderr);
				}
				next_expiration = 0;
			} else {
				/* caculate the pause we should take! */
				size_t how_many_more_msecs = next_expiration - k64timer_time_in_msecs();
				if (debug_flag) {
					fprintf(stderr, "k64idle_process(): pause - %zu %zu - pause = %zu msecs\n", next_expiration, k64timer_time_in_msecs(), how_many_more_msecs);
					fflush(stderr);
				}
				/* we have time to wait */
				k64timer_pause(how_many_more_msecs);
			}
		} else {
			if (!k64has_a_process_to_run())
				k64timer_pause(0);
		}
		k64yield();
	}
}

static void k64timer_expired(const size_t msecs)
{
	/* timer has gone off! it's been msecs since the last timer (i.e. it's possibly not every tick) */
	if (curproc) {
		/* this process has accumulated time */
		curproc->msec_elapsed += msecs;
	}
	/* we explictly don't do any more processing in this routine so that we never have to lock/unlock data structures */
	/* everything is done within k64schedule() and/or the idle process */
}

/* Init */
void k64init(const int debug)
{
	proclist = NULL;
	curproc = NULL;

	debug_flag = debug;
	init_done_flag = 1;							/* set it now to get init process going */

	/* create idle process first: pid 1 */
	struct proc64 *idle = k64spawn("[idle]", k64idle_process, NULL, 8*1024, NULL);
	if (!idle) {
		fprintf(stderr, "k64init(): [init] process start failed - THIS SHOULD NOT HAPPEN\n");
		fflush(stderr);
		abort();	/* kernel cannot run without idle */
	}
	k64renice(idle, 255);	/* lowest priority */
}

static void k64process_bootstrap(const struct proc64 *p)
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
	assert(next != NULL && next->magic == PROC_MAGIC_NUMBER);
	if (debug_flag) {
		fprintf(stderr, "k64enter(%u) next->name=\"%s\", next->state=%c\n", next->pid, next->name, next->state);
		fflush(stderr);
	}
	if (next->state == P_GO) {
		/* as P_GO to P_RUNNING is indication of a process start; now's a good time to get timers going. */
		if (!timer_started) {
			/* only happens once! */
			k64timer_init(&k64timer_expired);
			timer_started = 1;
		}
		/* first startup of process */
		next->state = P_RUNNING;
		k64process_bootstrap(next);
		/* never returns */
	}
	if (debug_flag) {
		fprintf(stderr, "k64enter(%u): next->name=\"%s\" doing longjmp:\n", next->pid, next->name);
		fflush(stderr);
	}
	longjmp(next->context, 1);
	/* never returns */
}

static void k64switch(struct proc64 *next)
{
	struct proc64 *prev = curproc;
	int r = -1;

	assert(next != NULL && next->magic == PROC_MAGIC_NUMBER);
	if (next == prev) {
		if (debug_flag) {
			fprintf(stderr, "k64switch(): next == prev - returning\n");
			fflush(stderr);
		}
		return;
	}
	if (debug_flag) {
		fprintf(stderr, "k64switch() next->pid=%u next->name=%s prev->pid=%u\n", next->pid, next->name, prev?prev->pid:0);
		fflush(stderr);
	}

	curproc = next;
	if (debug_flag) {
		fprintf(stderr, "k64switch(%u): next->name=\"%s\" setjmp() prev->pid=%u prev->name=\"%s\" rsp=%p\n", next->pid, next->name, prev?prev->pid:0, prev?prev->name:"-", __builtin_frame_address(0));
		fflush(stderr);
	}
	if (prev == NULL || (r=setjmp(prev->context)) == 0) {
		if (debug_flag) {
			fprintf(stderr, "k64switch(): setjmp() returns %d (should be 0) - next up k64enter ...\n", r);
			fflush(stderr);
		}
		k64enter(next);
		/* never returns */
		fprintf(stderr, "k64switch(): k64enter returned - SHOULD NOT HAPPEN (ABORTING!)\n");
		fflush(stderr);
		abort();
	}
	if (debug_flag) {
		fprintf(stderr, "k64switch(): returning (at end)\n");
		fflush(stderr);
	}
}

/* Spawn a new process */
struct proc64 *k64spawn(const char *name, void (*entry)(const void *), const void *arg, size_t stack_size, const void *v)
{
	struct proc64 *p;
	int r;

	name = name ? name : "[null]";

	if (debug_flag) {
		fprintf(stderr, "k64spawn(\"%s\")\n", name);
		fflush(stderr);
	}

	/* just in case nothing is init'ed */
	if (!init_done_flag) {
		k64init(0);
	}

	p = (struct proc64 *)malloc(sizeof(struct proc64));
	if (p == NULL) {
		fprintf(stderr, "k64spawn(\"%s\") - malloc failed\n", name);
		fflush(stderr);
		return NULL;
	}

	memset(p, 0x00, sizeof(*p));
	p->magic = PROC_MAGIC_NUMBER;
	p->prev = NULL;
	p->next = NULL;
	p->state = P_GO;
	p->priority = 80;				/* mimic (somewhat) the norms for priority values */
	p->expiration = 0;
	p->pid = pid_number++;
	p->msec_elapsed = 0;
	p->entry = (const void (*)(const void *))entry;	/* entry point for new process */
	p->arg = arg;					/* args for new process */
	p->name = malloc(strlen(name)+1);
	if (p->name == NULL) {
		fprintf(stderr, "k64spawn(\"%s\") - malloc failed\n", name);
		fflush(stderr);
		p->magic = 0;			/* we do this as the process is done for and we don't want to reference free'ed memory */
		free(p);
		return NULL;
	}
	strncpy(p->name, name, strlen(name)+1);

	k64stack_alloc(p, stack_size);
	if (p->stack_base == NULL || p->stack_size == 0) {
		p->magic = 0;			/* we do this as the process is done for and we don't want to reference free'ed memory */
		free(p);
		fprintf(stderr, "k64spawn(\"%s\") - k64stack_alloc() failed\n", name);
		fflush(stderr);
		return NULL;
	}

	if (debug_flag) {
		fprintf(stderr, "k64spawn(%u): p->name=\"%s\" setjmp() - first setup - rsp=%p\n", p->pid, p->name, __builtin_frame_address(0));
		fflush(stderr);
	}
	r = setjmp(p->context);				/* only save context, never run child branch */
	if (debug_flag) {
		fprintf(stderr, "k64spawn(): SAVE p->pid=%u return=%d\n", p->pid, r);
		fflush(stderr);
	}

#if OPTION_START_OF_LIST == 1
	/* add to start of list */
	p->next = proclist;
	if (proclist) {
		proclist->prev = p;
	}
	proclist = p;
	p->prev = NULL;
#else
	/* add to end of list */
	if (proclist == NULL) {
		proclist = p;
		p->next = NULL;
		p->prev = NULL;
	} else {
		struct proc64 *pp = proclist;
		while (pp->next != NULL)
			pp = pp->next;
		pp->next = p;
		p->next = NULL;
		p->prev = pp;
	}
#endif
	return p;
}

/* Delay check - maybe this process needs to wakeup from a timer */
void k64delay_check(struct proc64 *p)
{
	if (p->expiration > k64timer_time_in_msecs())
		return;
	/* the processes timer has expired */
	p->state = P_RUNNING;
	p->expiration = 0;
	if (debug_flag) {
		fprintf(stderr, "k64delay_check(): wakeup p->pid=%u p->name=\"%s\"\n", p->pid, p->name);
		fflush(stderr);
	}
}

/* Schedule - Simple round-robin scheduler - with priority checking */
void k64schedule(void)
{
	struct proc64 *p, *p_start, *p_highest, *p_idle;

	if (debug_flag) {
		if (curproc)
			fprintf(stderr, "k64schedule(): curproc->pid=%u curproc->name=\"%s\"\n", curproc->pid, curproc->name);
		else
			fprintf(stderr, "k64schedule(): curproc NULL\n");
		fflush(stderr);
	}

	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	p_highest = NULL;
	p_idle = NULL;

	/* Full circular scan */
	p_start = curproc ? (curproc->next ? curproc->next : proclist) : proclist;

	/* check all processes */
	p = p_start;
	do {
		assert(p != NULL && p->magic == PROC_MAGIC_NUMBER);
		/* while we are here - deal with delayed process */
		if (p->state == P_DELAYED)
			k64delay_check(p);
		/* only consider runnable processes */
		if (p->state == P_GO || p->state == P_RUNNING) {
			if (p->pid == 1) {
				p_idle = p;
			} else {
				if (p_highest == NULL)
					p_highest = p;
				if (p->priority < p_highest->priority)
					p_highest = p;
			}
		}
		/* loop back to the first process if needed */
		p = p->next ? p->next : proclist;
	} while (p != p_start);
	/* we have gone thru all the processes - now decide what to run */

	if (p_idle && p_idle->state == P_GO) {
		/* give the idle process a chance to start up - this is a special case */
		k64switch(p_idle);
		return;
	}

	/* if possible select the process with the highest priorty */
	p = p_highest ? p_highest : NULL;

	if (p == NULL) {
		/* this could mean only idle process is left - so run that */
		p = p_idle;
	}

	if (p == NULL) {
		/* no processes is able to run - we have nothing to schedule anymore */
		if (debug_flag) {
			fprintf(stderr, "k64schedule(): no more processes - EXITING CLEANLY\n");
			fflush(stderr);
		}
		/* clean up timers */
		k64timer_stop();
		/* we cleanly exit becuase this only happens when someone destroyed the idle process */
		exit(0);
	}

	/* kick off processs p if its stack is ok */
	if (k64stack_sanity_check(p) || k64stack_overflowed(p)) {
		/* stack corrupt on this process */
		/* we don't report here - 'cause we don't have a stack! */
		p->state = P_DESTROYED;
		/* now we don't have that process anymore - what if it's the running process? */
		if (p == curproc) {
			curproc = NULL;
		}
		/* TODO - not really true - we may have another process ready to fly, but idle is safe */
		p = p_idle;
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

	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	if (curproc != NULL) {
		p = curproc;
		assert(p != NULL && p->magic == PROC_MAGIC_NUMBER);
		if (debug_flag) {
			fprintf(stderr, "k64exit(): p->pid=%u p->state=%c set state P_ZOMBIE\n", p->pid, p->state);
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
		p->magic = 0;			/* we do this as the process is done for and we don't want to reference free'ed memory */
		free(p);			/* finally can get rid of p's memory */
#endif
#endif
		curproc = NULL;			/* we were the running process */
	}

	/* no running process at this point - which is hard to see this call happen */
	if (debug_flag) {
		fprintf(stderr, "k64exit(): calling k64schedule\n");
		fflush(stderr);
	}
	k64schedule();
	/* Never return */
	/* NOTREACHED: we always longjmp into another process (idle if nothing else) */
}

/* Nice this process */
void k64nice(const proc64pri pri)
{
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	if (curproc == NULL)
		return;
	k64renice(curproc, pri);
	/* call k64schedule() becuase we are this process */
	k64schedule();
}

/* Renice another process */
void k64renice(struct proc64 *p, const proc64pri pri)
{
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	assert(p != NULL && p->magic == PROC_MAGIC_NUMBER);
	p->priority = (proc64pri)(pri < 255 ? pri : 255);
	/* we don't call k64schedule() because we have affected a different process */
}

/* Delay this process */
void k64delay(const size_t msecs)
{
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	if (curproc == NULL)
		return;
	curproc->state = P_DELAYED;
	curproc->expiration = k64timer_time_in_msecs() + msecs;
	if (next_expiration == 0 || next_expiration > curproc->expiration)
		next_expiration = curproc->expiration;
	k64schedule();
}

/* Sleep this process */
void k64sleep(const void * event)
{
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	if (curproc == NULL)
		return;
	curproc->event = (void *)event;
	curproc->state = P_WAITING;
	k64schedule();
}

/* Wakeup another process */
void k64wakeup(struct proc64 *p)
{
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	assert(p != NULL && p->magic == PROC_MAGIC_NUMBER);
	if (p && p->state == P_WAITING)
		p->state = P_RUNNING;
	k64schedule();
}

/* Pause this process */
void k64pause(void)
{
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	if (curproc == NULL)
		return;
	/* We don't really "pause" - we simply wait for a tick to happen */
	k64delay(1);
	k64schedule();
}

struct proc64 *k64pid_to_proc(const proc64pid pid)
{
	struct proc64 *p;
	for (p=proclist;p!=NULL;p=p->next) {
		assert(p != NULL && p->magic == PROC_MAGIC_NUMBER);
		if (pid == p->pid)
			return p;
		p = p->next;
	}
	return NULL;
}

/* Destroy another process */
void k64destroy(struct proc64 *p)
{
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	assert(p != NULL && p->magic == PROC_MAGIC_NUMBER);

	if (p->state == P_GO || p->state == P_RUNNING || p->state == P_DELAYED || p->state == P_WAITING) {
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
	/* only proceed if we are init'ed and ready to roll */
	assert(proclist != NULL);

	if (curproc == NULL)
		return;
	curproc->state = P_RUNNING;	/* redundant but why not */
	k64schedule();
}

int k64has_a_process_to_run(void)
{
	const struct proc64 *p;
	for (p=proclist;p!=NULL;p=p->next) {
		if (p->state == P_GO || p->state == P_RUNNING || p->state == P_DELAYED || p->state == P_WAITING)
			return 1;
	}
	return 0;
}

size_t k64process_count(const int incl_zombie)
{
	const struct proc64 *p;
	size_t nn;

	nn = 0;
	for (p=proclist;p!=NULL;p=p->next) {
		nn++;
		if ((p->state == P_ZOMBIE || p->state == P_DESTROYED) && !incl_zombie)
			nn--;
	}
	return nn;
}

int k64stack_overflowed(const struct proc64 *p)
{
	if (p == NULL)
		p = curproc;
	if (p == NULL) {
		fprintf(stderr, "k64stack_overflowed(): called without curproc - THIS SHOULD NOT HAPPEN\n");
		fflush(stderr);
		abort();
	}
	assert(p != NULL && p->magic == PROC_MAGIC_NUMBER);
	return k64stack_band_check(p);
}

#endif	/* KERNEL64_WORD_BITS */
