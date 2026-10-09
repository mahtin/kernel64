# Theory of Operation

##Overview

Kernel64 implements a lightweight cooperative multitasking environment inspired by many other user-level kernels.

Process scheduling is performed entirely in user space using:

- setjmp()
- longjmp()
- per-process stacks
- cooperative yielding

The design is portable across:

- Linux x86_64 & ARM64
- Linux i386 (via 32 bit toolset)
- Linux ARMv7 (on RPi)
- macOS Apple Silicon & Intel
- Windows x64 (work in progress - nearly working)

No operating-system threads are used for process scheduling.

## Process Control Blocks

Each process is represented by a `struct proc64`.

A process control block contains:

- process identification
- scheduling state
- entry function
- entry argument
- process stack information
- saved execution context

The scheduler operates entirely through these structures.

## Process States

Typical process states include:

State	Description
GO	Runnable but not started yet
RUNNING	Runnable and eligible for scheduling
WAITING	Waiting for a wakeup event
SLEEPING	Waiting for a timer expiry
DESTROYED	Kernel has destroyed the process (stack overflow?)
ZOMBIE	Process has exited

Processes transition between these states through the scheduler APIs.

## Cooperative Scheduling

Kernel64 uses cooperative scheduling.

A running process yields voluntarily by calling:

```C
	k64yield();
```

The scheduler then:

Saves the current process context with setjmp().
Selects the next READY process.
Restores that process context with longjmp().

Because scheduling is cooperative:

No timer interrupts are required.
No preemption occurs.
A process runs until it yields, sleeps, or exits.
Context Switching

The actual context switch is performed by:

```C
	setjmp();
	longjmp();
```

A process context (a setjmp context) contains:

- stack pointer
- instruction pointer
- preserved registers

Each processor, operating system, architecture will decide how information is stored and is external to this code.

When a process yields:

```C
	setjmp(curproc->context);
```

This stores the current execution state.

When another process is selected:

```C
	longjmp(next->context, 1);
```

This restores that state and resumes execution.

The scheduler therefore operates entirely in user space.

## Process Stacks

Each process owns its own stack.

Stacks are allocated dynamically and are independent of the scheduler's internal stack.

Typical stack sizes are (but not limited too):

Process type	Stack
Idle process	8 KB
Normal process	16 KB
Test process	16 KB+

To detect stack overflow, stacks are allocated with a guard page.
Size is processor, operating system, architecture dependent.

## POSIX systems

Linux and macOS use:

```C
	mmap();
	mprotect();
```

The first page of the allocation is marked:

```C
	PROT_NONE
```

Any stack overflow into that page immediately generates a fault.

## Windows

Windows uses:

```C
	VirtualAlloc();
	VirtualProtect();
```

with:

```C
	PAGE_NOACCESS
```

on the guard page.

This provides equivalent behavior.

## Stack Usage Tracking

Newly allocated stacks are initialized with a known fill byte:

```C
	STACK_GUARD_BYTE
```

This allows Kernel64 to calculate stack consumption by scanning the stack and locating the highest overwritten byte.

The result provides:

- current stack usage
- historical high-water mark
- overflow diagnostics

without platform-specific APIs.

## Temporary Stack Switching

Some process initialization operations execute code on a process stack while preserving the scheduler's stack.

This is performed by:

```C
	k64_switch_to_stack()
```

Architecture-specific implementations in assembly code exist for:

- x86_64 on Linux/macOS
- ARM64 on Linux/macOS and Raspberry PI's (64 bit versions)
- x86_64 on Windows (ABI is different)
- i386 (for 32 bit Intel devices)
- ARMv7 on Raspberry Pi's (32 bit versions)

The helper:

 - Saves the current stack pointer.
 - Switches to the process stack.
 - Executes the target routine.
 - Restores the original stack.
 - Returns normally.

This mechanism allows process code to execute using its own stack without disturbing the scheduler's execution environment.

## Idle Process

Kernel64 maintains an idle process.

The idle process:

-- never exits
-- guarantees a runnable process always exists
-- provides a safe execution context when no user process is runnable

Future timer and sleep management facilities may also execute from the idle process context.

## Process Exit

A process terminates by calling:

```C
	k64exit();
```

Exit processing:

- Marks the process as ZOMBIE.
- Removes it from scheduling.
- Releases associated resources.
- Transfers control back to the scheduler.

Exited processes are never scheduled again.

## Portability

Architecture-dependent functionality is intentionally isolated.

Per-architecture assembly code exists only for:

- temporary stack switching
- ABI-specific stack handling

All scheduling logic remains in portable C.

This allows the same scheduler implementation to operate across multiple CPU architectures and operating systems with minimal platform-specific code.

