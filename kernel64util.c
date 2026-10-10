/*
 * kernel64util.c for modern 64 bit arch's (aarch64 & arm64) plus armv7l (32 bit)
 */

#include "port.h"

#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "kernel64.h"
#include "kernel64util.h"

static size_t k64pagesize(void)
{
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return (size_t)si.dwPageSize;
#else
	return (size_t)sysconf(_SC_PAGESIZE);
#endif
}

void k64stack_alloc(struct proc64 *p, size_t stack_size)
{
	size_t pagesize = k64pagesize();
	/* round usable size up to page multiple */
	size_t usable = (stack_size + pagesize - 1) & ~(pagesize - 1);
	size_t total = usable + pagesize;		/* +1 guard page */
	void *base;

	p->stack_base = NULL;
	p->stack_size = 0;
	p->stack_base = NULL;
	p->stack_size = 0;

#ifdef _WIN32
	base = VirtualAlloc(NULL, total, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	if (base == NULL) {
		fprintf(stderr, "VirtualAlloc() failed, error=%lu\n", (unsigned long)GetLastError());
		return;
	}

	DWORD oldprot;
	if (!VirtualProtect(base, pagesize, PAGE_NOACCESS, &oldprot)) {
		fprintf(stderr, "VirtualProtect() failed, error=%lu\n", (unsigned long)GetLastError());
		VirtualFree(base, 0, MEM_RELEASE);
		return;
	}
#else
	base = mmap(NULL, total, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (base == MAP_FAILED) {
		fprintf(stderr, "mmap() failed, error=%d\n", errno);
		return;
	}
	/* Make the lowest page (overflow direction) non-accessible */
	if (mprotect(base, pagesize, PROT_NONE) != 0) {
		fprintf(stderr, "mprotect() failed, error=%d\n", errno);
		munmap(base, total);
		return;
	}
#endif
	p->stack_real_base = base;
	p->stack_real_size = total;
	p->stack_base = (void *)((unsigned char *)base + pagesize);
	p->stack_size = usable;
}

void k64stack_free(struct proc64 *p)
{
	if (p->stack_real_base == NULL)
		return;
#ifdef _WIN32
	if (!VirtualFree(p->stack_real_base, 0, MEM_RELEASE)) {
		fprintf(stderr, "VirtualFree() failed, error=%lu\n", (unsigned long)GetLastError());
		fflush(stderr);
		/* fall thru to clear up values */
	}
#else
	if (munmap(p->stack_real_base, p->stack_real_size) != 0) {
		fprintf(stderr, "munmap() failed, error=%d\n", errno);
		fflush(stderr);
		/* fall thru to clear up values */
	}
#endif
	p->stack_real_base = NULL;
	p->stack_real_size = 0;
	p->stack_base = NULL;
	p->stack_size = 0;
}

#endif	/* KERNEL64_WORD_BITS */
