/* k64_switch_to_stack_x86_64.S - x86_64 stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg);
 * rdi = stack_base
 * rsi = stack_size
 * TODO !!!!
 */

	.text
	.globl _k64_switch_to_stack
	.globl k64_switch_to_stack

#ifndef __APPLE__
	/* Linux (ELF + GNU assembler) requires this */
	.type k64_switch_to_stack, @function
#endif

_k64_switch_to_stack:
k64_switch_to_stack:
	/* Pop original return address from old stack */
	popq	%rax				/* RAX = return address */
	movq	%rdi, %rdx			/* Compute new stack top */
	addq	%rsi, %rdx
	subq	$128, %rdx			/* Reserve red zone (128 bytes) */
	andq	$~0xF, %rdx			/* Align to 16 bytes */
	movq	%rdx, %rsp			/* Switch stack */
	pushq	%rax				/* Push original return address */
	ret					/* Return */

#ifndef __APPLE__
	.size k64_switch_to_stack, .-k64_switch_to_stack
	.section .note.GNU-stack,"",@progbits
#endif
