/* k64_switch_to_stack_x86_64.S - x86_64 stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg);
 * rdi = stack_base
 * rsi = stack_size
 * rdx = entry
 * rcx = arg
 */

	.text
	.globl	k64_switch_to_stack
	.globl	_k64_switch_to_stack

#ifndef __APPLE__
	/* Linux (ELF + GNU assembler) requires this */
	.type	k64_switch_to_stack, @function
#endif

k64_switch_to_stack:
_k64_switch_to_stack:
	/* pop original return address from old stack */
	popq	%rax				/* RAX = return address */
	/* Save original kernel stack pointer */
	movq	%rsp, %rbx			/* RBX = old_sp */
	/* Compute new stack top in a scratch register (r8), keep rdx = entry */
	movq	%rdi, %r8
	addq	%rsi, %r8
	subq	$128, %r8			/* Reserve red zone */
	andq	$~0xf, %r8			/* Align to 16 bytes */
	movq	%r8, %rsp			/* Switch stack */
	/* Save old_sp and original RA on the process stack */
	pushq	%rbx				/* [old_sp] */
	pushq	%rax				/* [RA_bootstrap] */
	/* Call entry(arg) on process stack */
	movq	%rcx, %rdi			/* arg → rdi */
	callq	*%rdx				/* entry(arg) */
	/* After entry() returns, restore RA and kernel stack */
	popq	%rax				/* RA_bootstrap */
	popq	%rsp				/* old_sp (kernel stack) */
	/* Recreate caller's stack frame and return to k64process_bootstrap */
	pushq	%rax
	ret

#ifndef __APPLE__
	.size	k64_switch_to_stack, .-k64_switch_to_stack
	.section .note.GNU-stack,"",@progbits
#endif
