/* k64_switch_to_stack_armv7l.S - armlv6/armlv7 (ARM32) stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg);
 * r0 = stack_base
 * r1 = stack_size
 * r2 = entry
 * r3 = arg
 */

/*
 * One thing to think about: this preserves r4 per the ARM EABI,
 * but it restores r4 from the new stack, not the old stack.
 */

	.syntax	unified
	.arch armv6zk				/* constrain to armv6 code (vs armv7) for Pi 1&2, Pi 3&4 (in 32 bit mode), Pi Zero */
	.arm

	.text
	.globl	k64_switch_to_stack
	.type	k64_switch_to_stack, %function

k64_switch_to_stack:
	/* preserve ABI-required callee-saved register */
	push	{r4}
	/* save caller's LR i.e. return address */
	mov	r4, lr
	/* compute top of new stack, 16-byte aligned */
	add	r0, r0, r1			/* r0 = base + size */
	bic	r0, r0, #0x7			/* align to 8 bytes (ARM EABI) */
	mov	sp, r0				/* switch stack */
	/* call entry(arg) on new stack */
	mov	r0, r3				/* arg in 0 */
	blx	r2				/* call entry(arg) on new stack */
	/* restore caller's LR i.e. return address */
	mov	lr, r4
	/* restore callee-saved register */
	pop	{r4}
	/* return to caller */
	bx	lr

	.size	k64_switch_to_stack, .-k64_switch_to_stack
	.section .note.GNU-stack,"",%progbits	/* Use percent vs at sign becuase at sign was used for comments on armv7 way-back-when */
