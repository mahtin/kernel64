/* k64_switch_to_stack_win64.S - Windows x86_64 stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg)
 *
 * Win64 ABI
 *
 * rcx = stack_base
 * rdx = stack_size
 * r8 = entry
 * r9 = arg
 */

	.text
	.globl	k64_switch_to_stack

k64_switch_to_stack:
	/* Save caller's return address */
	popq	%rax				/* RA_bootstrap */
	/* Save original kernel stack pointer */
	movq	%rsp, %r11			/* old_sp */
	/* Compute top of process stack */
	movq	%rcx, %r10
	addq	%rdx, %r10
	andq	$~0xF, %r10			/* 16-byte align */
	/* Switch to process stack */
	movq	%r10, %rsp
	/* Save old_sp and return address on process stack */
	pushq	%r11				/* old_sp */
	pushq	%rax				/* RA_bootstrap */
	/* 32-byte Win64 shadow space */
	subq	$32, %rsp
	/* entry(arg) */
	movq	%r9, %rcx			/* arg -> RCX */
	callq	*%r8
	addq	$32, %rsp
	/* Restore return address and original kernel stack */
	popq	%rax				/* RA_bootstrap */
	popq	%rsp				/* old_sp */
	/* Recreate original call frame */
	pushq	%rax
	ret
