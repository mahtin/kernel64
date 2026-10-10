/*
 * test1_idle.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel64.h"

static void test_destroy_idle_processs(void *arg)
{
	printf("TESTING: destroy_idle_processs starting...\n");
	fflush(stdout);
	for (int ii=0;ii<3;ii++) {
		printf("LOOP ... %d\n", ii);
		k64yield();
		k64print_ps();
	}
	printf("TESTING: destroy_idle_processs process...\n");
	fflush(stdout);
	k64destroy(k64pid_to_proc(1));
	printf("TESTING: destroy_idle_processs continuing...\n");
	fflush(stdout);
	for (int ii=0;ii<3;ii++) {
		printf("LOOP ... %d\n", ii);
		fflush(stdout);
		k64yield();
		k64print_ps();
	}

	k64exit();
}

void
test1_idle(void)
{
	struct proc64 *p;

	k64init();
	printf("k64init() done\n");
	fflush(stdout);

	p = k64spawn("testD", test_destroy_idle_processs, NULL, 0, NULL);
	printf("TESTING: p = %p %d \"%s\" %c\n", p, p->pid, p->name, p->state);
	fflush(stdout);

	printf("TESTING: about to loop on schedule ...\n");
	fflush(stdout);

	while (k64has_a_process_to_run()) {
		k64print_ps();
		printf("TESTING: k64schedule being called ...\n");
		fflush(stdout);
		k64schedule();
		printf("TESTING: k64schedule returned\n");
		fflush(stdout);
	}
	k64print_ps();
	printf("TESTING: scheduler returned - hence no more processes\n");
	fflush(stdout);
}

int
main(void)
{
	test1_idle();
	exit(0);
}
