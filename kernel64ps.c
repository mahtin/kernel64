/*
 * kernel64ps.c
 * for modern 64bit processor families: x86_64, aarch64/arm64.
 * plus some 32bit processor families: i386, armv6l/armv7l.
 */

#include "port.h"

#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kernel64.h"
#include "kernel64util.h"

static char *format_msec_elapsed(size_t msec_elapsed)
{
	static char buf[2+1+2+1+2+1];
	int sec_elapsed = msec_elapsed / 1000;

	unsigned int hours = sec_elapsed / 3600;
	unsigned int minutes = (sec_elapsed % 3600) / 60;
	unsigned int seconds = sec_elapsed % 60;

	/* %02d ensures at least 2 digits are printed, padded with a leading zero if needed */
	snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours % 100, minutes % 60, seconds % 60);
	return (buf);
}

static char **k64ps(void)
{
	const static char *format_header = "%6s %1s %5s %10s %3s %10s %6s %6s %16s %16s";
	const static char *format_entry1 = "%6u %1c     %c %10s %3u %10s %6zu %6zu %16p %16p";
	const static char *format_entry2 = "%6u %1c     %c %10s %3s %10s %6s %6s %16p %16p";
	struct proc64 *p;
	char **r;
	char *buf;
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
		"PRI",
		"TIME",
		"STACK",
		"USED",
		"ENTRY",
		"ARG"
	);
	r[ii] = malloc(strlen(buf)+1);
	strncpy(r[ii], buf, strlen(buf)+1);
	for (p=proclist;p!=NULL;p=p->next) {
		assert(p->magic == PROC_MAGIC_NUMBER);
		ii++;
		if (p->stack_base)
			snprintf(buf, 1024, format_entry1,
				p->pid,
				p == curproc? '*':' ',
				p->state,
				p->name?p->name:" ",
				p->priority,
				format_msec_elapsed(p->msec_elapsed),
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
				format_msec_elapsed(p->msec_elapsed),
				"-",
				"-",
				p->entry,
				p->arg
			);
		r[ii] = malloc(strlen(buf)+1);
		assert(r[ii] != NULL);
		strncpy(r[ii], buf, strlen(buf)+1);
	}
	ii++;
	r[ii] = NULL;
	free(buf);
	return r;
}

void k64print_ps(void)
{
	size_t ii;
	char **r = k64ps();

	for (ii=0;r[ii];ii++) {
		printf("%s\n", r[ii]);
		fflush(stdout);
		free(r[ii]);
	}
	free(r);
	printf("\n");
	fflush(stdout);
}

#endif	/* KERNEL64_WORD_BITS */
