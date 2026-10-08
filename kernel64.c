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
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "kernel64.h"

#define	OPTION_START_OF_LIST	0		/* 1 if process should go at begining of process list */
#define	OPTION_DELETE_ZOMBIE	0		/* 1 if process should be deleted vs being left as zombie in process list */

static int debug_flag = 0;			/* turn on for debug */

static struct proc64 *curproc = NULL;
static struct proc64 *proclist = NULL;

static unsigned int pid_number = 1;		/* start with process id 1 */

static void
print_ps(void)
{
	char **r = k64ps();
	for (size_t ii=0;r[ii];ii++) {
		printf("%s\n", r[ii]);
		fflush(stdout);
		free(r[ii]);
	}
	free(r);
	printf("\n");
	fflush(stdout);
}

static size_t k64pagesize(void)
{
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return (size_t)si.dwPageSize;
#else
	return (size_t)sysconf(_SC_PAGESIZE);
#endif
}

static void k64stack_alloc(struct proc64 *p, size_t stack_size)
{
	size_t pagesize = k64pagesize();
	/* round usable size up to page multiple */
	size_t usable = (stack_size + pagesize - 1) & ~(pagesize - 1);
	size_t total = usable + pagesize;		/* +1 guard page */
	void *base;

#ifdef _WIN32
	base = VirtualAlloc(NULL, total, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	if (base == NULL) {
		fprintf(stderr, "VirtualAlloc() failed, error=%lu\n", (unsigned long)GetLastError());
		fflush(stderr);
		return;
	}
	{
	DWORD oldprot;
	if (!VirtualProtect(base, pagesize, PAGE_NOACCESS, &oldprot)) {
		fprintf(stderr, "VirtualProtect() failed, error=%lu\n", (unsigned long)GetLastError());
		fflush(stderr);
		VirtualFree(base, 0, MEM_RELEASE);
		return;
	}
	}
#else
	base = mmap(NULL, total, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (base == MAP_FAILED) {
		p->stack_base = NULL;
		p->stack_size = 0;
		return;
	}
	/* Make the lowest page (overflow direction) non-accessible */
	if (mprotect(base, pagesize, PROT_NONE) != 0) {
		munmap(base, total);
		p->stack_base = NULL;
		p->stack_size = 0;
		return;
	}
#endif
	p->stack_real_base = base;
	p->stack_real_size = total;
	p->stack_base = (void *)((unsigned char *)base + pagesize);
	p->stack_size = usable;
}

static void k64stack_free(struct proc64 *p)
{
	/* never remove the stack from the current process! */
	if (p == curproc)
		return;
	if (!p->stack_real_base || !p->stack_real_size)
		return;
#ifdef _WIN32
	if (!VirtualFree(p->stack_real_base, 0, MEM_RELEASE)) {
		fprintf(stderr, "VirtualFree() failed, error=%lu\n", (unsigned long)GetLastError());
		fflush(stderr);
		return;
	}
#else
	if (munmap(p->stack_real_base, p->stack_real_size) != 0) {
		fprintf(stderr, "munmap() failed. %d\n", errno);
		fflush(stderr);
		return;
	}
#endif
	p->stack_real_base = NULL;
	p->stack_real_size = 0;
	p->stack_base = NULL;
	p->stack_size = 0;
}

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

static void k64zombies_reap(void)
{
	struct proc64 *p = proclist, *next;

	while (p) {
		next = p->next;
		if (p->pid > 1 && (p->state == P_ZOMBIE || p->state == P_KILLED)) {
#if OPTION_DELETE_ZOMBIE == 1
			/* unlink from list */
			if (p->prev)
				p->prev->next = p->next;
			else
				proclist = p->next;
			if (p->next)
				p->next->prev = p->prev;
			/* now safe to free stack + PCB */
			if (!p->stack_real_base || !p->stack_real_size)
				k64stack_free(p);
			if (p->name) {
				free(p->name);
				p->name = NULL;
			}
			free(p);
#else
			/* just remove stack as a zombie can never run anymore */
			if (p->stack_real_base && p->stack_real_size)
				k64stack_free(p);
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
		if (k64process_count(0) > 1) {
			/* still has processes running so lets quickly get them going again */
			k64yield();
		} else {
			print_ps();
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
	struct proc64 *idle = k64spawn("[idle]", k64idle_process, NULL, 8*1024, NULL);
	if (!idle) {
		fprintf(stderr, "k64init(): [init] process start failed - THIS SHOULD NOT HAPPEN\n");
		fflush(stderr);
		abort();	/* kernel cannot run without idle */
	}
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
	if (debug_flag) {
		fprintf(stderr, "k64enter(): doing longjmp:\n");
		fflush(stderr);
	}
	longjmp(next->context, 1);
	/* never returns */
}

static void k64switch(struct proc64 *next)
{
	struct proc64 *prev = curproc;

	assert(next->magic == PROC_MAGIC_NUMBER);
	if (debug_flag) {
		fprintf(stderr, "k64switch(%p) next->pid=%d next->name=%s prev->pid=%d\n", next, next->pid, next->name, prev?prev->pid:-1);
		fflush(stderr);
	}
	if (next == prev) {
		if (debug_flag) {
			fprintf(stderr, "k64switch(): next == prev - returning\n");
			fflush(stderr);
		}
		return;
	}

	/* Sanity check */
	assert(next->stack_base != NULL);
	assert(next->stack_size >= 4*1024);
	size_t used = k64stack_used(next);
	if (debug_flag) {
		fprintf(stderr, "k64switch(): STACK size: %zu used: %zu\n", next->stack_size, used);
		fflush(stderr);
	}
	assert(next->stack_size != used);

	curproc = next;
	if (prev == NULL || setjmp(prev->context) == 0) {
		fprintf(stderr, "k64switch(): k64enter ...\n");
		fflush(stderr);
		k64enter(next);
		/* never returns */
		fprintf(stderr, "k64switch(): k64enter returned - SHOULD NOT HAPPEN\n");
		fflush(stderr);
		abort();
	}
	if (debug_flag) {
		fprintf(stderr, "k64switch(): returning (at end)\n");
		fflush(stderr);
	}
}

/* Spawn */
struct proc64 *k64spawn(const char *name, void (*entry)(void *), void *arg, size_t stack_size, void *v)
{
	struct proc64 *p;

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
	p->priority = 0;
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
	strncpy(p->name, name, strlen(name));

	if (stack_size == 0)
		stack_size = 64 * 1024;
	k64stack_alloc(p, stack_size);
	if (p->stack_base == NULL || p->stack_size == 0) {
		free(p);
		return NULL;
	}
	memset(p->stack_base, STACK_GUARD_BYTE, p->stack_size);

	setjmp(p->context);			/* only save context, never run child branch */

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

/* Schedule - Simple round-robin scheduler */
void k64schedule(void)
{
	struct proc64 *p, *start;

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
	while (p != NULL) {
		assert(p->magic == PROC_MAGIC_NUMBER);
		if (debug_flag) {
			fprintf(stderr, "k64schedule: testing p->pid=%d p->state=%c\n", p->pid, p->state);
			fflush(stderr);
		}
		if (p->state == P_DELAY) {
			/* is it time to wake up */
			/* TODO */
			p->duration -= 1;
			if (p->duration <= 0)
				p->state = P_RUNNING;
		}
		if (p->state == P_GO || p->state == P_RUNNING) {
			break;
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
			return;		/* no READY/RUNNING processes */
		}
	}
	if (p == NULL)
		return;	/* no processes is ready to run - we have nothing to schedule */

	/* kick off processs p */
	if (k64stack_overflowed(p)) {
		/* stack corrupt on this process */
		size_t used = k64stack_used(p);
		fprintf(stderr, "k64schedule(%d): stack corrupt - dying name=%s stack_used=%zu / %zu\n", p->pid, p->name, used, p->stack_size);
		fflush(stderr);
		p->state = P_KILLED;
		if (p == curproc) {
			curproc = NULL;
		}
		return;	/* no processes */
	}
	if (debug_flag) {
		fprintf(stderr, "k64schedule: doing k64switch!\n");
		fflush(stderr);
	}
	k64switch(p);
}

/* Exit */
void k64exit(void)
{
	struct proc64 *p;

	if (curproc != NULL) {

		p = curproc;
		assert(p->magic == PROC_MAGIC_NUMBER);
		if (debug_flag) {
			fprintf(stderr, "k64exit(): p->pid=%d p->state=%c\n", p->pid, p->state);
			fprintf(stderr, "set state P_ZOMBIE\n");
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
	/* we cleanly exit becuase this only happens when someone kills the idle process */
	exit(0);
}

/* Delay */
void k64delay(unsigned long msecs)
{
	if (curproc == NULL)
		return;
	curproc->state = P_DELAY;
	curproc->duration = msecs;
	k64schedule();
}

/* Sleep */
void k64sleep(void * event)
{
	if (curproc == NULL)
		return;
	curproc->event = event;
	curproc->state = P_WAITING;
	k64schedule();
}

/* Wakeup */
void k64wakeup(struct proc64 *p)
{
	assert(p->magic == PROC_MAGIC_NUMBER);

	if (p && p->state == P_WAITING)
		p->state = P_RUNNING;
	k64schedule();
}

struct proc64 * k64pid_to_proc(int pid)
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


/* Kill */
void k64kill(struct proc64 *p)
{
	assert(p->magic == PROC_MAGIC_NUMBER);

	if (p->state == P_GO || p->state == P_RUNNING || p->state == P_DELAY || p->state == P_WAITING) {
		p->state = P_KILLED;
	}
	if (p == curproc) {
		curproc = NULL;
	}
	/* k64kyelid(); */
}

/* Yield */
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
		if (p->state == P_GO || p->state == P_RUNNING || p->state == P_DELAY || p->state == P_WAITING)
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
		if ((p->state == P_ZOMBIE || p->state == P_KILLED) && !incl_zombie)
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
		fprintf(stdout, "Top   : 0x%012lx %02x %02x %02x %02x %02x %02x %02x %02x\n",
			(unsigned long)top, *(top-0), *(top-1), *(top-2), *(top-3), *(top-4), *(top-5), *(top-6), *(top-7)
		);
		fprintf(stdout, "Bottom: 0x%012lx %02x %02x %02x %02x %02x %02x %02x %02x\n",
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

char **k64ps(void)
{
	char *buf;
	char **r;
	char *format_header = "%6s %1s %5s %10s %6s %6s %16s %16s";
	char *format_entry1 = "%6d %1c     %c %10s %6zu %6zu %16p %16p";
	char *format_entry2 = "%6d %1c     %c %10s %6s %6s %16p %16p";
	size_t nn, ii;

	buf = (char *)malloc(1024+1);
	assert(buf != NULL);
	ii = 0;
	nn = k64process_count(1);
	r = (char **)malloc(sizeof(char *) * (nn+1+1));
	assert(r != NULL);
	snprintf(buf, 1024, format_header,
		"PID",
		"*",
		"STATE",
		"NAME",
		"STACK",
		"USED",
		"ENTRY",
		"ARG"
	);
	r[ii] = malloc(strlen(buf)+1);
	strncpy(r[ii], buf, strlen(buf));
	for (struct proc64 *p=proclist;p!=NULL;p=p->next) {
		assert(p->magic == PROC_MAGIC_NUMBER);
		ii++;
		if (p->stack_base)
			snprintf(buf, 1024, format_entry1,
				p->pid,
				p == curproc? '*':' ',
				p->state,
				p->name?p->name:" ",
				p->stack_size,
				k64stack_used(p),
				p->entry,
				p->arg
			);
		else
			snprintf(buf, 1024, format_entry2,
				p->pid,
				p == curproc? '*':' ',
				p->state,
				p->name?p->name:" ",
				"-",
				"-",
				p->entry,
				p->arg
			);
		r[ii] = malloc(strlen(buf)+1);
		assert(r[ii] != NULL);
		strncpy(r[ii], buf, strlen(buf));
	}
	ii++;
	r[ii] = NULL;
	free(buf);
	return r;
}

#endif	/* KERNEL64_WORD_BITS */
