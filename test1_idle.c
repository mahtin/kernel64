/*
 * test1_idle.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel64.h"

static void
print_ps(void)
{
	char **r = k64ps();
	for (int ii=0;r[ii];ii++) {
		printf("%s\n", r[ii]);
		fflush(stdout);
		free(r[ii]);
	}
	free(r);
	printf("\n");
	fflush(stdout);
}

static void test_killing(void *arg)
{
	printf("TESTING: killing idle starting...\n");
	fflush(stdout);
	for (int ii=0;ii<3;ii++) {
		printf("LOOP ... %d\n", ii);
		k64yield();
		print_ps();
	}
	printf("TESTING: killing idle process...\n");
	fflush(stdout);
	k64kill(k64pid_to_proc(1));
	printf("TESTING: killing idle continuing...\n");
	fflush(stdout);
	for (int ii=0;ii<3;ii++) {
		printf("LOOP ... %d\n", ii);
		fflush(stdout);
		k64yield();
		print_ps();
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

	p = k64spawn("killer", test_killing, NULL, 0, NULL);
	printf("TESTING: p = %p %d \"%s\" %c\n", p, p->pid, p->name, p->state);
	fflush(stdout);

	printf("TESTING: about to loop on schedule ...\n");
	fflush(stdout);

	while (k64has_a_process_to_run()) {
		print_ps();
		printf("TESTING: k64schedule being called ...\n");
		fflush(stdout);
		k64schedule();
		printf("TESTING: k64schedule returned\n");
		fflush(stdout);
	}
	print_ps();
	printf("TESTING: scheduler returned - hence no more processes\n");
	fflush(stdout);
}

int
main(void)
{
	test1_idle();
	exit(0);
}
