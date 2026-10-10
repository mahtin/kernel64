/*
 * kernel64timer.c for modern 64 bit arch's (aarch64 & arm64) plus armv7l (32 bit)
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

static void (*timer_callback)(void) = NULL;	/* callback when timer goes off */

#ifdef _WIN32
static void CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
#else
static void k64timer_handler(int signum) {
#endif
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
	struct itimerval timer;

	sa.sa_handler = &k64timer_handler;
	sa.sa_flags = SA_RESTART;		/* Restart system calls if interrupted */
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		perror("Error registering signal handler");
		exit(EXIT_FAILURE);
	}
#endif

	/* Configure the timer to go off every 1/50'th of a second */
#ifdef _WIN32
	HANDLE hTimer = NULL;
	HANDLE hTimerQueue = NULL;
	int count = 0;

	/* Create a timer queue */
	hTimerQueue = CreateTimerQueue();
	if (hTimerQueue == NULL) {
		fprintf(stderr, "k64timer_init(): CreateTimerQueue() failed (%d)\n", GetLastError());
		return;
	}

	// Create a timer that starts after 1000 ms (1 second) 
	// and then repeats every 500 ms (0.5 seconds)
	if (!CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)k64timer_handler, NULL, 
			1000/50,	/* DueTime: 1/50'th second initial delay */
			1000/50,	/* Period: 1/50'th second repeat interval */
			0		/* Flags */
		)) {
		fprintf(stderr, "k64timer_init(): CreateTimerQueueTimer() failed (%d)\n", GetLastError());
		return;
	}
#else
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 1000 * 1000 / 50;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 1000 * 1000 / 50;

	/* Start the timer (ITIMER_REAL counts down in real-world time) */
	if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
		perror("Error setting timer");
		exit(EXIT_FAILURE);
	}
#endif

	timer_callback = callback;
	fprintf(stderr, "k64timer_init(): done!\n");
	fflush(stderr);
}

void k64timer_pause(void)
{
	pause();				/* use this to wait efficiently until a signal is received */
}

#endif	/* KERNEL64_WORD_BITS */
