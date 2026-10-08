/*
 * test_kernel.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel64.h"

struct _a {
	char *name;
	int count;
};

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

static void
test_proc(void *arg)
{
	const char *name = ((struct _a *)arg)->name;
	const int count = ((struct _a *)arg)->count;
	int ii;

	printf("TESTING: [%s] starting\n", name);
	fflush(stdout);
	for (ii=0;ii<count;ii++) {
		printf("TESTING: [%s] iteration %d\n", name, ii);
		fflush(stdout);
		if (ii == 0)
			print_ps();
		/* be nice - yield to other processes */
		k64yield();
	}

	print_ps();
	printf("TESTING: [%s] now exiting...\n", name);
	fflush(stdout);
	k64exit();
	printf("TESTING: [%s] WHY ARE WE HERE?\n", name);
	fflush(stdout);
}

static void
test_proc_bad(void *arg)
{
	char on_stack_allocation[128];

	/* FORCEFULLY destroy the stack via recursive calls */

	if ((uintptr_t)arg < 1024) {
		printf("TESTING: [%s] test_proc_bad(%lu)\n", "bad", (long)(uintptr_t)arg);
		fflush(stdout);
		memset(on_stack_allocation, 0x00, sizeof(on_stack_allocation));
		print_ps();
		k64yield();
		arg += 1;
		test_proc_bad(arg);
	}
	k64exit();
}

void
test_kernel(void)
{
	struct _a argA, argB, argC;
	size_t stack_size = 16 * 1024;		/* one day we work out what size is really needed */
	struct proc64 *p;

	k64init();
	printf("k64init() done\n");
	fflush(stdout);

	argA.name = "procA"; argA.count=5;
	p = k64spawn(argA.name, test_proc, (void *)&argA, stack_size, NULL);
	print_ps();
	printf("TESTING: p = %p %d \"%s\" %c\n", p, p->pid, p->name, p->state);
	fflush(stdout);

	argB.name = "procB"; argB.count=7;
	p = k64spawn(argB.name, test_proc, (void *)&argB, stack_size, NULL);
	print_ps();
	printf("TESTING: p = %p %d \"%s\" %c\n", p, p->pid, p->name, p->state);
	fflush(stdout);

	argC.name = "procC"; argC.count=11;
	p = k64spawn(argC.name, test_proc, (void *)&argC, stack_size, NULL);
	print_ps();
	printf("TESTING: p = %p %d \"%s\" %c\n", p, p->pid, p->name, p->state);
	fflush(stdout);

	p = k64spawn("bad", test_proc_bad, (void *)0, 0, NULL);

	printf("TESTING: about to loop on schedule ...\n");
	fflush(stdout);

	while (k64has_a_process_to_run()) {
		print_ps();
		k64schedule();
		printf("TESTING: k64schedule returned\n");
		fflush(stdout);
	}
	print_ps();

	printf("TESTING: scheduler returned\n");
	fflush(stdout);
}

int
main(void)
{
	test_kernel();
	exit(0);
}
