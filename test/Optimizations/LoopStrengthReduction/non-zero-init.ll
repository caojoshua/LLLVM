; Loop strength reduction test where the initial value is not 0

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes='lllvm-loop-strength-reduction' -S | FileCheck %s -check-prefix=CHECK

define i32 @init1() {
	; Induction variable's initial value is 1

	; CHECK-LABEL: define i32 @init1() {
	; CHECK: phi i32 [ 6, %0 ], [ %2, %Loop ]
	; CHECK: %2 = add i32 %1, 6

	br label %Loop
Loop:		; preds = %Loop, %0
	%i = phi i32 [ 1, %0 ], [ %Next, %Loop ]
	%mul = mul i32 %i, 6
	%Next = add i32 %i, 1
	%cond = icmp ult i32 %Next, 1000
	br i1 %cond, label %Out, label %Loop
Out:		; preds = %Loop
	ret i32 %mul
}

define i32 @init5() {
	; Induction variable's initial value is 5

	; CHECK-LABEL: define i32 @init5() {
	; CHECK: phi i32 [ 70, %0 ], [ %2, %Loop ]
	; CHECK: %2 = add i32 %1, 14

	br label %Loop
Loop:		; preds = %Loop, %0
	%i = phi i32 [ 5, %0 ], [ %Next, %Loop ]
	%mul = mul i32 %i, 14
	%Next = add i32 %i, 1
	%cond = icmp ult i32 %Next, 1000
	br i1 %cond, label %Out, label %Loop
Out:		; preds = %Loop
	ret i32 %mul
}
