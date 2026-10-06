/* k64_switch_to_stack_i386.S - Intel i386 stack switch helper */

/* void k64_switch_to_stack(void *stack_base, size_t stack_size, void (*entry)(void *), void *arg)
 *
 * i386 System V ABI:
 * [esp+4] = stack_base
 * [esp+8] = stack_size
 * [esp+12] = entry
 * [esp+16] = arg
 */

	.text
	.globl	k64_switch_to_stack
	.type	k64_switch_to_stack, @function

k64_switch_to_stack:
	/* Load arguments from caller stack */
	mov	4(%esp), %eax				/* eax = stack_base */
	mov	8(%esp), %ecx				/* ecx = stack_size */
	mov	12(%esp), %edx				/* edx = entry */
	mov	16(%esp), %ebx				/* ebx = arg */
	/* Save caller's return address */
	pop	%esi					/* esi = RA_bootstrap */
	/* Save original kernel stack pointer */
	mov	%esp, %edi				/* edi = old_sp */
	/* Compute new process stack top */
	add	%ecx, %eax				/* eax = base + size */
	and	$~0xF, %eax				/* align to 16 bytes */
	mov	%eax, %esp				/* switch to process stack */
	/* Save old_sp and RA_bootstrap on process stack */
	push	%edi					/* [old_sp] */
	push	%esi					/* [RA_bootstrap] */
	/* Call entry(arg) */
	mov	%ebx, %eax				/* arg → eax */
	push	%eax					/* push arg */
	call	*%edx					/* entry(arg) */
	add	$4, %esp				/* clean arg */
	/* Restore RA and kernel stack */
	pop	%esi					/* RA_bootstrap */
	pop	%esp					/* old_sp */
	/* Recreate caller frame and return */
	push	%esi					/* push RA_bootstrap */
	ret

	.size k64_switch_to_stack, .-k64_switch_to_stack
	.section .note.GNU-stack,"",@progbits
