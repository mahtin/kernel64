/* k64_switch_to_stack_arm64.S - ARM64 stack switch helper */

	.text
	.globl k64_switch_to_stack
	.globl _k64_switch_to_stack

/* void k64_switch_to_stack(void *stack_base, size_t stack_size)
 * X0 = stack_base
 * X1 = stack_size
 */

k64_switch_to_stack:
_k64_switch_to_stack:
	add	x0, x0, x1		/* x0 = base + size */
	bic	sp, x0, #0xf		/* SP = (base + size) & ~0xF */
	ret
