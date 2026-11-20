/*
 * test7_suspend.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>

#include "kernel64.h"

static struct proc64 *process_that_suspends;

#define	N_TIMES	5

static void do_suspend(const void *arg)
{
	int ii;
	const void *no_event = 0;

	(void)arg;
	printf("TESTING: do_suspend() started ...\n");
	fflush(stdout);

	/* run N times */
	for (ii=0;ii<(N_TIMES+0);ii++) {
		printf("TESTING: do_suspend() k64suspend() ...\n");
		fflush(stdout);
		k64suspend(no_event);

		printf("TESTING: do_suspend() ... resumed\n");
		fflush(stdout);

		k64print_ps();

		printf("TESTING: do_suspend() k64delay(200) ...\n");
		fflush(stdout);
		k64delay(200);
	}

	k64print_ps();

	printf("TESTING: do_suspend() calling exit...\n");
	fflush(stdout);
	k64exit();
}

static void do_resume(const void *arg)
{
	int ii;

	(void)arg;
	printf("TESTING: do_resume() started ...\n");
	fflush(stdout);

	/* run N times */
	for (ii=0;ii<(N_TIMES+2);ii++) {
		printf("TESTING: do_resume() k64delay(1000) ...\n");
		fflush(stdout);
		k64delay(1000);

		k64print_ps();

		printf("TESTING: do_resume() k64resume() ...\n");
		fflush(stdout);
		k64resume(process_that_suspends);
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

	process_that_suspends = p;

	ii = 1;
	printf("TESTING: k64spawn(..., %d) ...\n", ii);
	fflush(stdout);
	p = k64spawn("do_resume", do_resume, 0, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", (void *)p, p->pid, p->name, p->state);
	fflush(stdout);

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
