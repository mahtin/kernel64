/* k64_switch_to_stack_x86_64.S - x86_64 stack switch helper */

	.text

/* void k64_switch_to_stack(void *stack_base, size_t stack_size)
 * rdi = stack_base
 * rsi = stack_size
 */

#ifdef __APPLE__
	.globl _k64_switch_to_stack
_k64_switch_to_stack:
	/* macOS / Darwin (Mach-O) */
	/* Pop original return address from old stack */
	popq	%rax				/* RAX = return address */
	movq	%rdi, %rdx			/* Compute new stack top */
	addq	%rsi, %rdx
	subq	$128, %rdx			/* Reserve red zone (128 bytes) */
	andq	$~0xF, %rdx			/* Align to 16 bytes */
	movq	%rdx, %rsp			/* Switch stack */
	pushq	%rax				/* Push original return address */
	/* Return */
	ret
#else
	.globl k64_switch_to_stack
k64_switch_to_stack:
	/* Linux (ELF + GNU assembler) */
	.type k64_switch_to_stack, @function
	/* Pop original return address from old stack */
	popq	%rax				/* RAX = return address */
	movq	%rdi, %rdx			/* Compute new stack top */
	addq	%rsi, %rdx
	subq	$128, %rdx			/* Reserve red zone (128 bytes) */
	andq	$~0xF, %rdx			/* Align to 16 bytes */
	movq	%rdx, %rsp			/* Switch stack */
	pushq	%rax				/* Push original return address */
	ret					/* Return */
	.size k64_switch_to_stack, .-k64_switch_to_stack
	.section .note.GNU-stack,"",@progbits
#endif
