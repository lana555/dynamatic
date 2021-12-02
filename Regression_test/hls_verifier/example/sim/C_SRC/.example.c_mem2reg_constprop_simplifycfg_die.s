	.text
	.file	"example.c"
	.globl	example                 # -- Begin function example
	.p2align	4, 0x90
	.type	example,@function
example:                                # @example
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	xorl	%eax, %eax
	movq	%rsi, -8(%rbp)          # 8-byte Spill
	movq	%rdi, -16(%rbp)         # 8-byte Spill
	movq	%rdx, -24(%rbp)         # 8-byte Spill
	movl	%eax, -28(%rbp)         # 4-byte Spill
	jmp	.LBB0_1
.LBB0_1:                                # %for.body
                                        # =>This Inner Loop Header: Depth=1
	movl	-28(%rbp), %eax         # 4-byte Reload
	movl	%eax, %ecx
	movl	%ecx, %edx
	movq	-16(%rbp), %rsi         # 8-byte Reload
	movl	(%rsi,%rdx,4), %ecx
	movslq	%ecx, %rdx
	movq	-24(%rbp), %rdi         # 8-byte Reload
	movl	(%rdi,%rdx,4), %r8d
	movl	%eax, %r9d
	movl	%r9d, %edx
	movq	-8(%rbp), %r10          # 8-byte Reload
	imull	(%r10,%rdx,4), %r8d
	movslq	%ecx, %rdx
	movl	%r8d, (%rdi,%rdx,4)
	addl	$1, %eax
	cmpl	$100, %eax
	movl	%eax, -28(%rbp)         # 4-byte Spill
	jb	.LBB0_1
# %bb.2:                                # %for.end
	popq	%rbp
	retq
.Lfunc_end0:
	.size	example, .Lfunc_end0-example
	.cfi_endproc
                                        # -- End function
	.globl	main                    # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$1216, %rsp             # imm = 0x4C0
.LBB1_1:                                # %for.body
                                        # =>This Loop Header: Depth=1
                                        #     Child Loop BB1_2 Depth 2
	xorl	%eax, %eax
	movl	%eax, -1204(%rbp)       # 4-byte Spill
	jmp	.LBB1_2
.LBB1_2:                                # %for.body3
                                        #   Parent Loop BB1_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	-1204(%rbp), %eax       # 4-byte Reload
	movl	%eax, -1208(%rbp)       # 4-byte Spill
	callq	rand
	movl	$50, %ecx
	cltd
	idivl	%ecx
	movl	-1208(%rbp), %ecx       # 4-byte Reload
	movl	%ecx, %esi
	movl	%esi, %edi
	movl	%edx, -400(%rbp,%rdi,4)
	callq	rand
	movl	$10, %ecx
	cltd
	idivl	%ecx
	movl	-1208(%rbp), %ecx       # 4-byte Reload
	movl	%ecx, %esi
	movl	%esi, %edi
	movl	%edx, -800(%rbp,%rdi,4)
	callq	rand
	movl	$50, %ecx
	cltd
	idivl	%ecx
	movl	-1208(%rbp), %ecx       # 4-byte Reload
	movl	%ecx, %esi
	movl	%esi, %edi
	movl	%edx, -1200(%rbp,%rdi,4)
	addl	$1, %ecx
	cmpl	$100, %ecx
	movl	%ecx, -1204(%rbp)       # 4-byte Spill
	jb	.LBB1_2
# %bb.3:                                # %for.end
                                        #   in Loop: Header=BB1_1 Depth=1
	xorl	%eax, %eax
	movb	%al, %cl
	movl	$5, -388(%rbp)
	movl	$5, -384(%rbp)
	movl	$5, -280(%rbp)
	movl	$5, -276(%rbp)
	testb	$1, %cl
	jne	.LBB1_1
# %bb.4:                                # %for.end30
	xorl	%eax, %eax
	movl	%eax, -1212(%rbp)       # 4-byte Spill
	jmp	.LBB1_5
.LBB1_5:                                # %for.body34
                                        # =>This Inner Loop Header: Depth=1
	movl	-1212(%rbp), %eax       # 4-byte Reload
	leaq	-1200(%rbp), %rcx
	leaq	-800(%rbp), %rdx
	leaq	-400(%rbp), %rsi
	movl	%eax, %edi
	movl	%edi, %r8d
	imulq	$400, %r8, %r8          # imm = 0x190
	addq	%r8, %rsi
	movl	%eax, %edi
	movl	%edi, %r8d
	imulq	$400, %r8, %r8          # imm = 0x190
	addq	%r8, %rdx
	movl	%eax, %edi
	movl	%edi, %r8d
	imulq	$400, %r8, %r8          # imm = 0x190
	addq	%r8, %rcx
	movq	%rsi, %rdi
	movq	%rdx, %rsi
	movq	%rcx, %rdx
	movl	%eax, -1216(%rbp)       # 4-byte Spill
	callq	example
	xorl	%eax, %eax
	movb	%al, %r9b
	movl	-1216(%rbp), %eax       # 4-byte Reload
	addl	$1, %eax
	testb	$1, %r9b
	movl	%eax, -1212(%rbp)       # 4-byte Spill
	jne	.LBB1_5
# %bb.6:                                # %for.end45
	xorl	%eax, %eax
	addq	$1216, %rsp             # imm = 0x4C0
	popq	%rbp
	retq
.Lfunc_end1:
	.size	main, .Lfunc_end1-main
	.cfi_endproc
                                        # -- End function

	.ident	"clang version 6.0.1 (http://llvm.org/git/clang.git 2f27999df400d17b33cdd412fdd606a88208dfcc) (http://llvm.org/git/llvm.git 5136df4d089a086b70d452160ad5451861269498)"
	.section	".note.GNU-stack","",@progbits
