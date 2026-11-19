/*
 * test3_nothing.c - simple kernel64 scheduler test harness
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "kernel64.h"

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

static void do_nothing(const void *arg)
{
	int count;

	printf("TESTING: do_nothing() starting...\n");
	fflush(stdout);
	k64print_ps();
	for (count=0;count<5;count++) {
		printf("%3d ", count);
		print_time();
		k64delay(1250);
		k64print_ps();
	}
	k64exit();
}

static void test3_nothing(void)
{
	const struct proc64 *p;

	k64init(0);
	printf("k64init() done\n");
	fflush(stdout);

	p = k64spawn("nothing", do_nothing, NULL, 0, NULL);
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
	test3_nothing();
	exit(0);
}
