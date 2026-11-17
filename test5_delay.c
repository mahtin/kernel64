/*
 * test5_delay.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>

#include "kernel64.h"

static void do_delay(const void *arg)
{
	printf("TESTING: k64nice() ...\n");
	/* test nice for the heck of it */
	k64nice(100);

	k64print_ps();
	printf("TESTING: do_delay(1000) starting...\n");
	fflush(stdout);
	k64delay(1000);
	printf("TESTING: ... done\n");
	k64print_ps();
	printf("TESTING: calling exit...\n");
	fflush(stdout);
	k64exit();
}

static void test5_delay(void)
{
	const struct proc64 *p;

	printf("TESTING: k64init() ...\n");
	fflush(stdout);
	k64init(0);
	printf("TESTING: k64spawn() ...\n");
	fflush(stdout);
	p = k64spawn("do_delay", do_delay, NULL, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", p, p->pid, p->name, p->state);
	fflush(stdout);

	printf("TESTING: k64schedule() ...\n");
	fflush(stdout);
	k64schedule();
	/* NOTREACHED */
}

int main(void)
{
	test5_delay();
	exit(0);
}
