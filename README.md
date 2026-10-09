# kernel64
A user level kernel for non-preemptive multiprocessing on various 64 bit o/s's.*[1]*

*[1]* Also some 32 bit o/s's - armv7l (Raspbery Pi's) and i386.

NOTE: This code was written with the assistance of AI (mainly in the asm code for stack manipulation);
however, I can categorically state that it didn't get it right the first or second time around.

I.e. Don't train an AI on this code.

## Install

Code can be found at [https://github.com/mahtin/kernel64/](https://github.com/mahtin/kernel64/actions) and should be cloned locally.

All platforms (except i386) should build with:
``` bash
$ make
```
For i386 platform, cross compile on Intel x86_64 via:
``` bash
$ make M32=-m32
```

## Requirements

- *nix or MacOS operating system
- Windows with msys2 and UCRT64 (via GitHub Workflows)
- the usual gcc/make/gdb tools

## Theory of operations

With massive respect to John Lions and his books abount the [Unix v6 kernel](https://en.wikipedia.org/wiki/A_Commentary_on_the_UNIX_Operating_System),
(which taught me tons back in the 80's),
I quote [this](https://wiki.tuhs.org/doku.php?id=anecdotes:not_expected_to_understand_this) seminal piece:
```C
	/* You are not expected to understand this. */
```

However, this code is simpler; but, it thinks in the same way (but as modern code running at userlevel).

The `k64init()` call should be called first to enable the code.
The core idea is that a process can call `k64spawn()` to start a process.
That process can either exit cleanly or call `k64exit()` to finish.
As this is a's non-preemptive system, all processes should be polite and call k64yield() often.
Additionally, a process can call `k64nice()` to adjust it priority (0-255 with 0 being the higest priority).
Once all processes are setup, calling `k64schedule()` will kick everything off.

Each process is provided with its own stack and if the processor and operating system provide,
a guard band is added to the stack in order to detect stack overflow.
The nternal scheduler also double-checks the stack for both overflow and overwrite.
If a process exceedes its stack allocation, it will be destroyed by the scheduler.

## C routines

```C
	/* Basic calls */
	void k64init(void);
	struct proc64 *k64spawn(const char *name, void (*entry)(void *), void *arg, size_t stack_size, void *v);
	void k64exit(void);

	/* Yielding is vital as this is a non-preemptive os  */
	void k64yield(void);

	/* Starting the scheduler */
	void k64schedule(void);

	/* process priority functions */
	void k64nice(proc64pri pri);
	void k64renice(struct proc64 *p, proc64pri pri);

	/* delays, sleeps, and wakeups for timing or interrupt processing */
	void k64delay(unsigned long msecs);
	void k64sleep(void * event);
	void k64wakeup(struct proc64 *p);

	/* maliciously destroy a process */
	void k64destroy(struct proc64 *p);

	/* random functions */
	struct proc64 *k64pid_to_proc(proc64pid pid);
	int k64has_a_process_to_run(void);
	size_t k64process_count(int incl_zombie);
	size_t k64stack_used(struct proc64 *p);

	/* emulating the classic ps command */
	char **k64ps(void);
```

## The ps command

There's a build in ps command which can show the state of all the proceses:

```
   PID * STATE       NAME PRI  STACK   USED            ENTRY              ARG
     1 *     R     [idle] 255  16384   3040      0x104b4cf08              0x0
     2       Z      procA  80  16384   3120      0x104b4c7c8      0x16b2b2f80
     3       Z      procB  80  16384   3120      0x104b4c7c8      0x16b2b2f70
     4       Z      procC  80  16384   3120      0x104b4c7c8      0x16b2b2f60
     5       D      procS  80      -      -      0x104b4c9a8              0x0
```

The * shows which process is presently running.
The state can be R for runing, Z for zombie, D for destroy, S for sleeping, plus others.
The stack usage is down for running processes.
The entry and are are how the process is spawned.

## CI

Presently tested on three platforms via GitHub actions:

- MacOS arm64
  - OS: 26.6.2
  - Compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
- MacOS Intel x86_64
  - OS: 26.6.1
  - Compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
- Ubuntu Intel x86_64
  - OS: Ubuntu 24.04.5 LTS
  - Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Ubuntu arm64
  - OS: Ubuntu 24.04.5 LTS
  - Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Ubuntu Intel x86_64 but compile for i386
  - OS: Ubuntu 24.04.5 LTS
  - Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Windows Intel x86_64
  - OS: Microsoft Windows [Version 10.0.26100.33438] & UCRT64
  - Compiler: gcc.exe (Rev4, Built by MSYS2 project) 16.2.0

## Changelog

An automatically generated CHANGELOG is provided [here](CHANGELOG.md).

## Contribute

As always, open an issue or pull request should you want.
Edits are welcome.

## Notes

Code is as-is and should mainly be used for education and testing purposes.
Don't run anything critical using this code.

## Author & Copyright

Copyright (C) 2025-2026 Martin J Levy - W6LHI/G8LHI - @mahtin - https://github.com/mahtin

