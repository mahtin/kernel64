/*
 * test4_pause.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "kernel64.h"

struct _a {
	char *name;
};

static void print_time(void)
{
	const struct tm *local;
	char buf[sizeof("9999-12-31T23:59:59.999+00:00")];	/* 29 chars + '\0' */
	size_t bufsize = sizeof(buf);
	int off = 0;

#ifdef CLOCK_REALTIME
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	local = localtime(&tp.tv_sec);
#else
	time_t raw_time;
	local = localtime(&raw_time);
#endif

	off = strftime(buf, bufsize, "%Y-%m-%dT%H:%M:%S", local);		/* same as "%Y-%m-%dT%H:%M:%S"  */
#ifdef CLOCK_REALTIME
	off += snprintf(buf+off, bufsize-off, ".%03ld", tp.tv_nsec/1000000);
#endif
#ifdef _WIN32
	snprintf(buf+off, bufsize-off, "%s", "");
#else
#ifdef CLOCK_REALTIME
	snprintf(buf+off, bufsize-off,
		"%c%02ld:%02ld",
		local->tm_gmtoff >= 0 ? '+' : '-',
		labs(local->tm_gmtoff)/3600,
		labs(local->tm_gmtoff)%3600/60
	);
#endif
#endif

	printf("%s\n", buf);
	fflush(stdout);
}

static void do_pause(const void *arg)
{
	int count;
	const char *name = ((struct _a *)arg)->name;

	printf("TESTING: do_pause(%s) starting...\n", name);
	fflush(stdout);
	k64print_ps();
	for (count=0;count<=400;count++) {
		if ((count % 100) == 0) {
			printf("%s %3d ", name, count);
			print_time();
			k64print_ps();
		}
		if (name[7] == 'A')
			k64delay(3);
		if (name[7] == 'B')
			k64delay(4);
		if (name[7] == 'C')
			k64delay(5);
		if (name[7] == 'D')
			k64pause();
	}
	k64print_ps();
	k64exit();
}

static void test4_pause(void)
{
	const struct proc64 *p;
	struct _a argA, argB, argC, argD;

	k64init(0);
	printf("k64init() done\n");
	fflush(stdout);

	argA.name = "pausingA";
	p = k64spawn("pausingA", do_pause, (void *)&argA, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", (void *)p, p->pid, p->name, p->state);
	fflush(stdout);

	argB.name = "pausingB";
	p = k64spawn("pausingB", do_pause, (void *)&argB, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", (void *)p, p->pid, p->name, p->state);
	fflush(stdout);

	argC.name = "pausingC";
	p = k64spawn("pausingC", do_pause, (void *)&argC, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", (void *)p, p->pid, p->name, p->state);
	fflush(stdout);

	argD.name = "pausingD";
	p = k64spawn("pausingD", do_pause, (void *)&argD, 0, NULL);
	printf("TESTING: p = %p %u \"%s\" %c\n", (void *)p, p->pid, p->name, p->state);
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

int main(void)
{
	test4_pause();
	exit(0);
}
