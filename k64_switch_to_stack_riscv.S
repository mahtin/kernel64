/* k64_switch_to_stack_riscv64.S - RISC-V 64 bit and 32 bit stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg)
 *
 * RISC-V LP64 ABI & RISC-V ILP32 ABI
 *
 * a0 = stack_base
 * a1 = stack_size
 * a2 = entry
 * a3 = arg
 */

/*
 * Uses only caller-saved registers:
 *
 *      ra
 *      a0-a3
 *      t0-t2
 *
 * Plus, presently only s0 is preserved in the code below.
 * Do not use s1-s11 without explicitly preserving them.
 */

#if !defined(__riscv)
#error "__riscv not defined"
#endif
#if __riscv_xlen != 64 && __riscv_xlen != 32
#error "__riscv_xlen not defined"
#endif

	.text
	.globl	k64_switch_to_stack
	.type	k64_switch_to_stack, @function

k64_switch_to_stack:
	/* save caller return address */
	mv	t0, ra
	/* save original stack pointer */
	mv	t1, sp
	/* compute top of process stack */
	add	t2, a0, a1
	/* maintain 16-byte ABI alignment */
	andi	t2, t2, -16
	/* switch stack */
	mv	sp, t2
	/* save original stack and return address - also s0 in case we use it somewhere (in future code) */
#if __riscv_xlen == 64
	addi	sp, sp, -24
	sd	s0, 0(sp)
	sd	t1, 8(sp)
	sd	t0, 16(sp)
#endif
#if __riscv_xlen == 32
	addi	sp, sp, -12
	sw	s0, 0(sp)
	sw	t1, 4(sp)
	sw	t0, 8(sp)
#endif
	/* entry(arg) */
	mv	a0, a3
	jalr	a2
	/* restore return address and original stack - also s0 */
#if __riscv_xlen == 64
	ld	t0, 16(sp)
	ld	t1, 8(sp)
	ld	s0, 0(sp)
#endif
#if __riscv_xlen == 32
	lw	t0, 8(sp)
	lw	t1, 4(sp)
	lw	s0, 0(sp)
#endif
	mv	sp, t1
	mv	ra, t0
	ret

	.size	k64_switch_to_stack, .-k64_switch_to_stack
	.section .note.GNU-stack,"",@progbits
