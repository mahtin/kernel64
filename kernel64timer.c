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

static void (*timer_callback)(size_t msecs) = NULL;	/* callback when timer goes off */

#ifdef _WIN32
static HANDLE g_hTimerEvent = NULL;
static HANDLE hTimer = NULL;
static HANDLE hTimerQueue = NULL;
#else
static struct itimerval timer;
#endif

/*
 * size_t is (on 64 bit machines) an eight byte long - a 64 bit unsigned int.
 * Hence this implenmtation of time_msec_since_start (meansured in msecs) will overflow in 584 million years.
 * I've set a calendar reminder to change this it around 250  million years.
 * size_t is (on 32 bit machines) much smaller and we are looking at 49.7 days for overflow. TODO This is a real issue.
 */
static size_t time_msec_since_start = 0;	/* in msecs - jumps by 1000 / TICKS_PER_SECOND msecs - time is modeled as monotonically increasing (always moving forward) */
static size_t time_msec_last_sleep = 0;		/* in msecs */

static void k64timer_set_msecs(size_t msecs);

#ifdef _WIN32
static void CALLBACK k64timer_handler(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
#else
static void k64timer_handler(int signum) {
#endif
	time_msec_since_start += time_msec_last_sleep;

	// fprintf(stderr, "k64timer_handler(): Timer expired\n");
	// fflush(stderr);
	if (timer_callback)
		timer_callback(time_msec_last_sleep);

	/* now the repeat timer kicks in */
	time_msec_last_sleep = 1000 / TICKS_PER_SECOND;
}

void k64timer_init(void (*callback)(size_t msecs))
{
	/* set interrupts */
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
	k64timer_set_msecs(1000 / TICKS_PER_SECOND);
	timer_callback = callback;
	// fprintf(stderr, "k64timer_init(): done!\n");
	// fflush(stderr);
}

static void k64timer_set_msecs(size_t msecs)
{
	/* setup the timer */
#ifdef _WIN32
	if (g_hTimerEvent == NULL) {
		/* only run once */
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
	}
#endif /* WIN32 */

	/* configure the timer */
#ifdef _WIN32
	/* do we need to cancel the previous timer? */
	if (hTimer != NULL) {
		DeleteTimerQueueTimer(hTimerQueue, hTimer, INVALID_HANDLE_VALUE);
		hTimer = NULL;
	}
	/* create and start a new timer */
	if (!CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)k64timer_handler, NULL,
			msecs,				/* DueTime: initial delay */
			1000 / TICKS_PER_SECOND,	/* Period: repeat interval */
			0				/* Flags */
		)) {
		fprintf(stderr, "k64timer_init(): CreateTimerQueueTimer() failed (%d)\n", GetLastError());
		DeleteTimerQueue(hTimerQueue);
		CloseHandle(g_hTimerEvent);
		return;
	}
#else
	/* Unix/Linux just overwrites this one timer - easy easy */
	timer.it_value.tv_sec = msecs / 1000;
	timer.it_value.tv_usec = 1000 * (msecs % 1000);
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 1000 * 1000 / TICKS_PER_SECOND;

	/* Start and start a timer (ITIMER_REAL counts down in real-world time) */
	if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
		perror("Error setting timer");
		exit(EXIT_FAILURE);
	}
#endif /* WIN32 */

	time_msec_last_sleep = msecs;
}

size_t k64timer_time_in_msecs(void)
{
	return time_msec_since_start;
}

void k64timer_stop(void)
{
#ifdef _WIN32
	DeleteTimerQueueTimer(hTimerQueue, hTimer, INVALID_HANDLE_VALUE);
	hTimer = NULL;
	DeleteTimerQueue(hTimerQueue);
	hTimerQueue = NULL;
	CloseHandle(g_hTimerEvent);
	g_hTimerEvent = NULL;
#else
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
#endif
}

void k64timer_pause(size_t msecs)
{
	/* we can use the msecs or not - the idle code will handle it */
	if (msecs > 0) {
		/* change timers */
		k64timer_set_msecs(msecs);
	}
	// fprintf(stderr, "k64timer_pause(%zu)\n", msecs);
	// fflush(stderr);

	/* now we wait */
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
