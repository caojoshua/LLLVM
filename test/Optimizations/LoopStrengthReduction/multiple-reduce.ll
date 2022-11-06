; Loop strength reduction test that multiple computations can be strength reduced

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes='lllvm-loop-strength-reduction' -S | FileCheck %s -check-prefix=CHECK

define i32 @multiple_reduce_single_var() {
	; Multiple strength reductions for a single induction variable

	; CHECK-LABEL: define i32 @multiple_reduce_single_var() {
	; CHECK: add i32 %2, 17
	; CHECK: add i32 %1, 71

	br label %Loop
Loop:		; preds = %Loop, %0
	%i = phi i32 [ 0, %0 ], [ %Next, %Loop ]
	%mul = mul i32 %i, 17
	%mul2 = mul i32 %i, 71
	%Next = add i32 %i, 1
	%cond = icmp ult i32 %Next, 1000
	br i1 %cond, label %Out, label %Loop
Out:		; preds = %Loop
	ret i32 %mul
}

define i32 @multiple_var() {
	; Strength reductions when there are multiple induction variables

	; CHECK-LABEL: define i32 @multiple_var() {
	; CHECK: add i32 %2, 23
	; CHECK: add i32 %1, 32

	br label %Loop
Loop:		; preds = %Loop, %0
	%i = phi i32 [ 0, %0 ], [ %NextI, %Loop ]
	%j = phi i32 [ 0, %0 ], [ %NextJ, %Loop ]
	%mulI = mul i32 %i, 23
	%mulJ = mul i32 %i, 32
	%NextI = add i32 %i, 1
	%NextJ = add i32 %j, 1
	%cond = icmp ult i32 %NextI, 1000
	br i1 %cond, label %Out, label %Loop
Out:		; preds = %Loop
	%ret = add i32 %mulI, %mulJ
	ret i32 %ret
}
