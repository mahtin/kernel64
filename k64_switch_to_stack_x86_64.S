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
	/* compute new stack top */
	movq	%rdi, %rdx
	addq	%rsi, %rdx
	subq	$128, %rdx			/* reserve red zone */
	andq	$~0xf, %rdx			/* align to 16 bytes */
	movq	%rdx, %rsp			/* switch stack */
	pushq	%rax				/* push original return address for ret back to k64process_bootstrap */
	/* call entry(arg) on process stack */
	movq	%rcx, %rdi			/* arg → rdi */
	callq	*%rdx				/* entry(arg) */
	/* return to caller */
	ret

#ifndef __APPLE__
	.size	k64_switch_to_stack, .-k64_switch_to_stack
	.section .note.GNU-stack,"",@progbits
#endif
