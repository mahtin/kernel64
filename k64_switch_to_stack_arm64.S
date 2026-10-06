/* k64_switch_to_stack_arm64.S - ARM64 stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg);
 * X0 = stack_base
 * X1 = stack_size
 * X2 = entry
 * X3 = arg
 */

	.text
	.globl k64_switch_to_stack
	.globl _k64_switch_to_stack

k64_switch_to_stack:
_k64_switch_to_stack:
	add	x0, x0, x1	/* x0 = base + size */
	bic	sp, x0, #0xf	/* SP = (base + size) & ~0xF */
	mov	x0, x3		/* arg in x0 */
	blr	x2		/* call entry(arg) on new stack */
	ret			/* never returns to old frame; caller should not expect to resume */
