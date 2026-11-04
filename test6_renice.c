/*
 * test6_renice.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "kernel64.h"

#define	N_PROCS	5

#define PRI_BASE	50
#define	PRI_SPREAD	20

#define DELAY_BASE	100
#define DELAY_SPREAD	100

static struct proc64 *processes[N_PROCS];

static void do_renice(void *arg)
{
	int ii, delay, proc;
	int my_proc = (int)(intptr_t)arg;
	unsigned int pri;

	printf("TESTING: do_renice(%d) started ...\n", my_proc);
	fflush(stdout);

	pri = PRI_BASE;
	printf("TESTING: k64nice(%u) ...\n", pri);
	fflush(stdout);
	/* test nice for the heck of it - set base amount */
	k64nice(pri);

	/* run N times */
	for (ii=0;ii<10;ii++) {
		proc = rand() % N_PROCS;
		if (proc == my_proc) {
			/* don't redo myself or a dead process */
			continue;
		}

		delay = DELAY_BASE - DELAY_SPREAD/2 + (rand() % DELAY_SPREAD);
		printf("TESTING: do_delay(%d) starting...\n", delay);
		fflush(stdout);
		k64delay(delay);

		if (processes[proc] == NULL) {
			/* a dead process */
			continue;
		}
		pri = PRI_BASE - PRI_SPREAD/2 + (rand() % PRI_SPREAD);
		printf("TESTING: k64renice(%d, %u) starting...\n", proc, pri);
		fflush(stdout);
		k64renice(processes[proc], pri);

		k64print_ps();
	}

	k64print_ps();
	/* remove my process - can't be acted upon anymore */
	processes[my_proc] = NULL;

	printf("TESTING: calling exit...\n");
	fflush(stdout);
	k64exit();
}

static void test6_renice(void)
{
	int ii;
	time_t t;
	struct proc64 *p;

	/* seed them random thingy */
	srand((unsigned int)time(NULL));

	printf("TESTING: k64init() ...\n");
	fflush(stdout);
	k64init(0);

	for (ii=0;ii<N_PROCS;ii++) {
		printf("TESTING: k64spawn(..., %d) ...\n", ii);
		fflush(stdout);
		p = k64spawn("do_renice", do_renice,  (void *)(intptr_t)ii, 0, NULL);
		printf("TESTING: p = %p %u \"%s\" %c\n", p, p->pid, p->name, p->state);
		fflush(stdout);
		/* save away for use later */
		processes[ii] = p;
	}

	k64print_ps();
	printf("TESTING: k64schedule() ...\n");
	fflush(stdout);
	k64schedule();
	/* NOTREACHED */
}

int main(void)
{
	test6_renice();
	exit(0);
}
