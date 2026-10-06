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
	/* save caller's LR */
	mov	r4, lr
	/* compute top of new stack, 16-byte aligned */
	add	r0, r0, r1			/* r0 = base + size */
	bic	r0, r0, #0x7			/* align to 8 bytes (ARM EABI) */
	mov	sp, r0				/* switch stack */
	/* call entry(arg) on new stack */
	mov	r0, r3				/* arg in 0 */
	blx	r2				/* call entry(arg) on new stack */
	/* restore caller's LR */
	mov	lr, r4
	/* return to caller */
	bx	lr

	.size	k64_switch_to_stack, .-k64_switch_to_stack
