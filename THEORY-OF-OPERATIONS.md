# Kernel Design / Theory of Operation

## Overview

Kernel64 is a portable cooperative multitasking kernel inspired by many other schedulers found online.
The implementation is written primarily in C and uses:

- `setjmp()`
- `longjmp()`
- per-process stacks
- architecture-specific stack-switch primitives

to provide lightweight processes without requiring operating system threads.
Kernel64 currently supports:

- Linux x86_64
- Linux i386
- Linux ARM64 (AArch64)
- Linux ARMv7
- macOS Apple Silicon
- macOS Intel
- Windows x64 (work in progress)

The scheduler is entirely cooperative. Processes execute until they voluntarily yield, sleep, exit, or are destroyed.

## Process Architecture

Each process is represented by a `struct proc64`.
A process contains:

- Process identifier (`pid`)
- Process name
- Scheduler state
- Priority
- Execution context (`jmp_buf`)
- Process stack
- Entry function
- Entry argument
- Sleep/wakeup information

Processes are linked together in a doubly-linked process list.

The currently executing process is tracked by:

```c
	struct proc64 *curproc;
```

and the complete process list is tracked by:

```c
	struct proc64 *proclist;
```

## Process States

Kernel64 represents process state as a single character value.

| State | Character | Description |
|---------|---------|---------|
| `P_GO` | `G` | Process created but never executed |
| `P_RUNNING` | `R` | Runnable process |
| `P_WAITING` | `W` | Waiting for wakeup event |
| `P_SLEEPING` | `S` | Sleeping for a timed delay |
| `P_DESTROYED` | `D` | Process destroyed because of error |
| `P_ZOMBIE` | `Z` | Process exited and awaiting cleanup |

The scheduler considers the following states runnable:

```c
	P_GO
	P_RUNNING
```

A process in any other state will not be selected for execution.

## Process Creation

Processes are created with:

```c
	k64spawn(name, entry, arg, stack_size, v);
```

During creation Kernel64:

1. Allocates a process control block.
2. Assigns a process identifier.
3. Allocates a stack.
4. Initializes the stack guard pattern.
5. Stores the process entry function and argument.
6. Captures an initial scheduler context using `setjmp()`.
7. Adds the process to the process list.
8. Marks the process state as `P_GO`.

A newly created process does not begin execution immediately.
Execution starts when the scheduler first selects the process.

## Stack Allocation

Each process owns its own private stack.
Stacks are allocated using virtual memory rather than standard heap allocation.

## POSIX Platforms

Linux and macOS use:

```c
	mmap();
	mprotect();
```

The lowest page of each stack allocation is marked:

```c
	PROT_NONE
```

to act as a guard page.

## Windows

Windows uses:

```c
	VirtualAlloc();
	VirtualProtect();
```

The lowest page of the allocation is marked:

```c
	PAGE_NOACCESS
```

to provide the same protection.

## Guard Pages

All process stacks contain an inaccessible guard page at the bottom of the stack.
Normal stack growth proceeds downward toward the guard page.
If a process overflows its stack, the CPU immediately generates a fault upon entering the guard page.
This provides deterministic stack overflow detection.

## Stack Usage Measurement

After allocation, every usable byte of stack memory is initialized with:

```c
	STACK_GUARD_BYTE
```

currently:

```c
	0xAA
```

The function:

```c
	k64stack_used()
```

measures stack usage by scanning upward through the stack until the first modified byte is located.
This permits:

- stack utilization reporting
- high-water mark measurement
- overflow diagnostics

without platform-specific APIs.

---

# Idle Process

Kernel64 always creates an idle process during initialization.
The idle process becomes PID 1.
Initialization is performed by:

```c
	k64init();
```

which internally creates:

```text
	[idle]
```

The idle process serves several purposes:

- Provides a runnable process when no others are available.
- Reaps zombie process resources.
- Performs scheduler housekeeping.
- Prevents the scheduler from running out of executable processes.

The idle process runs at the lowest priority value used by the scheduler.

## Context Switching

Kernel64 performs scheduling through:

```c
	setjmp();
	longjmp();
```

A context switch occurs in two stages.
This code does not work fully on Windows. i.e. work in progress.

## Saving State

When a running process yields:

```c
	setjmp(curproc->context);
```

stores:

- stack pointer
- instruction pointer
- preserved registers

inside the process control block.
Each processor, operating system, architecture will decide how information is stored and is external to this code.

## Restoring State

When the scheduler selects a process:

```c
	longjmp(next->context, 1);
```

restores the saved execution state and resumes execution exactly where that process last yielded.
No operating system thread context switching occurs.
All scheduling remains inside user space.

## First Process Execution

A newly created process begins in state:

```c
	P_GO
```

The first time the scheduler selects such a process:

```c
	k64process_bootstrap();
```

is invoked.

The bootstrap routine:

1. Switches to the process stack using an architecture-specific assembly helper.
2. Executes the process entry routine.
3. Calls `k64exit()` when the process entry routine returns.

The bootstrap function never returns.

## Stack Switching

Kernel64 contains architecture-specific assembly routines named:

```c
	k64_switch_to_stack();
```

Implementations exist for:

- x86_64
- i386
- ARM64
- ARMv7
- x86_64 specific to Windows (ABI is different)

The purpose of the routine is:

1. Save the current kernel stack.
2. Switch to the target process stack.
3. Execute code on that process stack.
4. Restore the original stack.
5. Return to the caller.

Only this small assembly component is architecture-specific.
All scheduling logic remains portable C code.

## Scheduling

Scheduling is performed by:

```c
	k64schedule();
```

Kernel64 performs a circular scan of the process list.

During the scan it:

1. Wakes expired sleeping processes.
2. Examines runnable processes.
3. Selects the runnable process with the highest scheduling priority.

Priority values are interpreted as:

```text
	Lower value = Higher priority
```

For example:

```text
	Priority 10  > Priority 50
	Priority 50  > Priority 100
```

where ">" means "runs before".

The selected process is activated using:

```c
	k64switch();
```

## Yielding

A process voluntarily relinquishes execution by calling:

```c
	k64yield();
```

This keep the process in a runnable state:

```c
	P_RUNNING
```

and then invokes the scheduler.

Because Kernel64 is cooperative:

- No timer interrupt is required.
- No preemption occurs.
- A process runs until it explicitly yields.

## Sleeping

Timed delays are implemented with:

```c
	k64delay(milliseconds);
```

This places the process into:

```c
	P_SLEEPING
```

and stores a countdown value.
The scheduler decrements the remaining duration on each scheduling pass.
When the countdown reaches zero, the process becomes runnable again.

## Waiting and Wakeup

Processes may block waiting for an event.
Waiting is performed with:

```c
	k64sleep(event);
```

The process enters state:

```c
	P_WAITING
```

Another process may resume it by calling:

```c
	k64wakeup(process);
```

which returns the process to:

```c
	P_RUNNING
```

and makes it eligible for scheduling again.

## Process Exit

A process terminates itself by calling:

```c
	k64exit();
```

or by returning from its entry function.

Exit processing:

1. Marks the process as `P_ZOMBIE`.
2. Clears the current process pointer.
3. Invokes the scheduler.
4. Never returns.

Zombie processes remain in the process list until the idle process reaps their resources.

## Process Destruction

Kernel64 can force the removal of a process by calling:

```c
	k64destroy(process);
```

A destroyed process enters state:

```c
	P_DESTROYED
```

Destroyed processes are eventually cleaned up by the idle process.
This mechanism is used primarily when stack corruption or internal errors are detected.

## Stack Corruption Detection

Prior to scheduling a process, Kernel64 performs:

```c
	k64stack_sanity_check();
```

and

```c
	k64stack_overflowed();
```

checks.

If corruption is detected:

1. Diagnostic information is printed.
2. The process state becomes:

```c
	P_DESTROYED
```

3. The scheduler selects another runnable process.

This prevents a corrupted process from continuing execution.

## Process Listing

Kernel64 provides:

```c
	k64ps();
```

which generates a textual snapshot of all processes.

Displayed information includes:

- PID
- Current process indicator
- State
- Name
- Priority
- Stack size
- Stack usage
- Entry function
- Entry argument

This facility serves as the primary scheduler debugging and monitoring tool.

## Design Goals

Kernel64 attempts to preserve the simplicity of classic cooperative process systems while remaining portable across modern architectures.
Key goals include:

- Minimal architecture-specific code.
- Portable scheduler implementation.
- Deterministic process behavior.
- Detectable stack overflow conditions.
- No dependency on operating system threads.
- Easy integration into existing applications.

The resulting design provides a lightweight process environment suitable for experimentation, embedded systems, network services, and educational operating system development.
