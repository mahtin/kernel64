/*
 * test7_suspend.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "kernel64.h"

#define	N_PROCS	2

static struct proc64 *processes[N_PROCS];

static void do_suspend(const void *arg)
{
	int ii;
	const void *no_event = 0;

	printf("TESTING: do_suspend() started ...\n");
	fflush(stdout);

	/* run N times */
	for (ii=0;ii<10;ii++) {
		printf("TESTING: do_suspend() ... k64suspend() ...\n");
		fflush(stdout);
		k64suspend(no_event);

		printf("TESTING: do_suspend() ... resumed\n");
		fflush(stdout);

		printf("TESTING: do_suspend() ... k64delay(200) ...\n");
		fflush(stdout);
		k64delay(200);

		k64print_ps();
	}

	k64print_ps();

	printf("TESTING: do_suspend() calling exit...\n");
	fflush(stdout);
	k64exit();
}

static void do_resume(const void *arg)
{
	int ii;

	printf("TESTING: do_resume() started ...\n");
	fflush(stdout);

	/* run N times */
	for (ii=0;ii<10;ii++) {
		printf("TESTING: do_resume() k64delay(1000) ...\n");
		fflush(stdout);
		k64delay(1000);

		k64print_ps();

		printf("TESTING: do_resume() k64resume() ...\n");
		fflush(stdout);
		k64resume(processes[0]);
	}

	k64print_ps();

	printf("TESTING: do_resume() calling exit...\n");
	fflush(stdout);
	k64exit();
}

static void test6_renice(void)
{
	int ii;
	struct proc64 *p;

	printf("TESTING: k64init() ...\n");
	fflush(stdout);
	k64init(0);

	ii = 0;
	printf("TESTING: k64spawn(..., %d) ...\n", ii);
	fflush(stdout);
	p = k64spawn("do_suspend", do_suspend, 0, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", (void *)p, p->pid, p->name, p->state);
	fflush(stdout);
	processes[ii] = p;

	ii = 1;
	printf("TESTING: k64spawn(..., %d) ...\n", ii);
	fflush(stdout);
	p = k64spawn("do_resume", do_resume, 0, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", (void *)p, p->pid, p->name, p->state);
	fflush(stdout);
	processes[ii] = p;

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
