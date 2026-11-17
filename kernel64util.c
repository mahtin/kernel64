/*
 * kernel64util.c
 * for modern 64bit processor families: x86_64, aarch64/arm64.
 * plus some 32bit processor families: i386, armv6l/armv7l.
 */

#include "port.h"

#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
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
		fprintf(stderr, "k64stack_alloc(): VirtualAlloc() failed, error=%lu\n", (unsigned long)GetLastError());
		fflush(stderr);
		return;
	}

	DWORD oldprot;
	if (!VirtualProtect(base, pagesize, PAGE_NOACCESS, &oldprot)) {
		fprintf(stderr, "k64stack_alloc(): VirtualProtect() failed, error=%lu\n", (unsigned long)GetLastError());
		fflush(stderr);
		VirtualFree(base, 0, MEM_RELEASE);
		return;
	}
#else
#if defined(MAP_ANON) || defined(MAP_ANONYNOUS)
	base = mmap(NULL, total, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#else
	/* mmap of /dev/zero does not work on MacOS - which is fine because MAP_ANON is defined */
	int fd = open("/dev/zero", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "k64stack_alloc(): open(/dev/zero) failed, error=%d\n", errno);
		fflush(stderr);
		return;
	}
	base = mmap(NULL, total, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);
#endif /* MAP_ANONYNOUS */
	if (base == MAP_FAILED) {
		fprintf(stderr, "k64stack_alloc(): mmap() failed, error=%d\n", errno);
		fflush(stderr);
		return;
	}
	/* Make the lowest page (overflow direction) non-accessible */
	if (mprotect(base, pagesize, PROT_NONE) != 0) {
		fprintf(stderr, "k64stack_alloc(): mprotect() failed, error=%d\n", errno);
		fflush(stderr);
		munmap(base, total);
		return;
	}
#endif
	p->stack_real_base = base;
	p->stack_real_size = total;
	p->stack_base = (void *)((unsigned char *)base + pagesize);
	p->stack_size = usable;

	/* mark all stack area with guard */
	{
		const k64stack_guard *top, *bottom;
		k64stack_guard *s;

		bottom = (k64stack_guard *)p->stack_base;		/* lowest address */
		top = bottom + p->stack_size/sizeof(k64stack_guard);	/* highest address + 1 */
		s = bottom;
		while (s < top)
			*s++ = STACK_GUARD_NUMBER;
	}
}

void k64stack_free(struct proc64 *p)
{
	if (p->stack_real_base == NULL)
		return;
#ifdef _WIN32
	if (!VirtualFree(p->stack_real_base, 0, MEM_RELEASE)) {
		fprintf(stderr, "k64stack_alloc(): VirtualFree() failed, error=%lu\n", (unsigned long)GetLastError());
		fflush(stderr);
		/* fall thru to clear up values */
	}
#else
	if (munmap(p->stack_real_base, p->stack_real_size) != 0) {
		fprintf(stderr, "k64stack_alloc(): munmap() failed, error=%d\n", errno);
		fflush(stderr);
		/* fall thru to clear up values */
	}
#endif
	p->stack_real_base = NULL;
	p->stack_real_size = 0;
	p->stack_base = NULL;
	p->stack_size = 0;
}

size_t k64stack_used(const struct proc64 *p)
{
	const k64stack_guard *top, *bottom, *s;

	if (p->stack_base == NULL || p->stack_size == 0) {
		/* unallocated (i.e. zombie) */
		return 0;
	}
	bottom = (k64stack_guard *)p->stack_base;		/* lowest address */
	top = bottom + p->stack_size/sizeof(k64stack_guard);	/* highest address + 1 */
	s = bottom;
	while (s < top && *s == STACK_GUARD_NUMBER)
		s++;
	return (top - s) * sizeof(k64stack_guard);
}

int k64stack_back_check(const struct proc64 *p)
{
	const k64stack_guard *bottom;
	k64stack_guard *s;
	size_t ii, band = 256/sizeof(k64stack_guard);	/* Check a small band at the bottom (near guard page) or one page */

	if (band > p->stack_size)
		band = p->stack_size;

	bottom = (const k64stack_guard *)p->stack_base;

	for (ii=0,s=(k64stack_guard *)bottom;ii<band;ii++,s++) {
		if (*s != STACK_GUARD_NUMBER)
			return 1;		/* overflow into guard region */
	}
	return 0;
}

#endif	/* KERNEL64_WORD_BITS */
