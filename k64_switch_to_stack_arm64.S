/* k64_switch_to_stack_arm64.S - ARM64 stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg);
 * X0 = stack_base
 * X1 = stack_size
 * X2 = entry
 * X3 = arg
 */

	.text
	.globl	k64_switch_to_stack
	.globl	_k64_switch_to_stack

k64_switch_to_stack:
_k64_switch_to_stack:
	/* compute top of new stack, 16-byte aligned */
	add	x0, x0, x1			/* x0 = base + size */
	bic	x0, x0, #0xf			/* SP = (base + size) & ~0xf */
	mov	sp, x0
	/* call entry(arg) on new stack */
	mov	x0, x3				/* arg in x0 */
	blr	x2				/* call entry(arg) on new stack */
	/* return to caller */
	ret
#ifndef __APPLE__
	.size	k64_switch_to_stack,.-k64_switch_to_stack
#endif
