/* k64_switch_to_stack_armv7l.S - ARM32 stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg);
 * r0 = stack_base
 * r1 = stack_size
 * r2 = entry
 * r3 = arg
 */

	.syntax	unified
	.arm

	.text
	.globl	k64_switch_to_stack
	.type	k64_switch_to_stack, %function

k64_switch_to_stack:
	add	r2, r0, r1		/* r2 = base + size */
	bic	r2, r2, #7		/* Align to 8 bytes (ARM EABI) */
	mov	sp, r2			/* Switch stack */
	mov	r0, r3			/* arg in 0 */
	blr	r2			/* call entry(arg) on new stack */
	bx	lr			/* Return to caller */

	.size k64_switch_to_stack, .-k64_switch_to_stack
