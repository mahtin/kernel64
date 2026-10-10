/*
 * kernel64ps.c for modern 64 bit arch's (aarch64 & arm64) plus armv7l (32 bit)
 */

#include "port.h"

#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kernel64.h"

void k64print_ps(void)
{
	char **r = k64ps();
	for (size_t ii=0;r[ii];ii++) {
		printf("%s\n", r[ii]);
		fflush(stdout);
		free(r[ii]);
	}
	free(r);
	printf("\n");
	fflush(stdout);
}

char **k64ps(void)
{
	const static char *format_header = "%6s %1s %5s %10s %3s %6s %6s %16s %16s";
	const static char *format_entry1 = "%6d %1c     %c %10s %3d %6zu %6zu %16p %16p";
	const static char *format_entry2 = "%6d %1c     %c %10s %3s %6s %6s %16p %16p";
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
		"STACK",
		"USED",
		"ENTRY",
		"ARG"
	);
	r[ii] = malloc(strlen(buf)+1);
	strncpy(r[ii], buf, strlen(buf)+1);
	for (struct proc64 *p=proclist;p!=NULL;p=p->next) {
		assert(p->magic == PROC_MAGIC_NUMBER);
		ii++;
		if (p->stack_base)
			snprintf(buf, 1024, format_entry1,
				p->pid,
				p == curproc? '*':' ',
				p->state,
				p->name?p->name:" ",
				p->priority,
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

#endif	/* KERNEL64_WORD_BITS */
