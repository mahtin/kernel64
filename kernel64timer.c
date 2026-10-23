/*
 * kernel64timer.c
 * for modern 64bit processor families: x86_64, aarch64/arm64.
 * plus some 32bit processor families: i386, armv6l/armv7l.
 */

#include "port.h"

#if KERNEL64_WORD_BITS == 64 || KERNEL64_WORD_BITS == 32

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "kernel64.h"
#include "kernel64timer.h"

static void (*timer_callback)(void) = NULL;	/* callback when timer goes off */

#ifdef _WIN32
static HANDLE g_hTimerEvent = NULL;
static HANDLE hTimer = NULL;
static HANDLE hTimerQueue = NULL;
#else
static struct itimerval timer;
#endif

static size_t ticks = 0;			/* in msec's - jumps by MSECS_PER_TICK */

#ifdef _WIN32
static void CALLBACK k64timer_handler(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
#else
static void k64timer_handler(int signum) {
#endif
	ticks += 1000/MSECS_PER_TICK;		/* keep track of time */

	// fprintf(stderr, "k64timer_handler(): Timer expired\n");
	// fflush(stderr);
	if (timer_callback)
		timer_callback();
}

void k64timer_init(void (*callback)(void))
{
#ifdef _WIN32
	/* Windows ignores signal blocking entirely, so you can safely leave this blank */
#else
	struct sigaction sa;

	sa.sa_handler = &k64timer_handler;
	sa.sa_flags = SA_RESTART;		/* Restart system calls if interrupted */
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		perror("Error registering signal handler");
		exit(EXIT_FAILURE);
	}
#endif

	/* Configure the timer to go off every MSECS_PER_TICK */
#ifdef _WIN32
	/* Create a manual-reset event, initially unsignaled */
	g_hTimerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_hTimerEvent == NULL) {
		fprintf(stderr, "k64timer_init(): CreateEvent() failed (%d)\n", GetLastError());
		return;
	}
	/* Create a timer queue */
	hTimerQueue = CreateTimerQueue();
	if (hTimerQueue == NULL) {
		fprintf(stderr, "k64timer_init(): CreateTimerQueue() failed (%d)\n", GetLastError());
		CloseHandle(g_hTimerEvent);
		return;
	}

	/* Create and start a timer */
	if (!CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)k64timer_handler, NULL, 
			1000/MSECS_PER_TICK,	/* DueTime: MSECS_PER_TICK initial delay */
			1000/MSECS_PER_TICK,	/* Period: MSECS_PER_TICK repeat interval */
			0			/* Flags */
		)) {
		fprintf(stderr, "k64timer_init(): CreateTimerQueueTimer() failed (%d)\n", GetLastError());
		DeleteTimerQueue(hTimerQueue);
		CloseHandle(g_hTimerEvent);
		return;
	}
#else
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 1000 * 1000 / MSECS_PER_TICK;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 1000 * 1000 / MSECS_PER_TICK;

	/* Start and start a timer (ITIMER_REAL counts down in real-world time) */
	if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
		perror("Error setting timer");
		exit(EXIT_FAILURE);
	}
#endif

	timer_callback = callback;
	// fprintf(stderr, "k64timer_init(): done!\n");
	// fflush(stderr);
}

size_t k64timer_ticks(void)
{
	return ticks;
}

void k64timer_stop(void)
{
#ifdef _WIN32
	DeleteTimerQueueTimer(hTimerQueue, hTimer, INVALID_HANDLE_VALUE);
	DeleteTimerQueue(hTimerQueue);
	CloseHandle(g_hTimerEvent);
#else
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
#endif
}

void k64timer_pause(void)
{
#ifdef _WIN32
	DWORD waitResult = WaitForSingleObject(g_hTimerEvent, INFINITE);

	if (waitResult != WAIT_OBJECT_0) {
		fprintf(stderr, "k64timer_init(): waitResult() failed (%d)\n", GetLastError());
		return;
	}
#else
	pause();				/* use this to wait efficiently until a signal is received */
#endif
}

#endif	/* KERNEL64_WORD_BITS */
