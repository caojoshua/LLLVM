; Basic loop strength reduction test. %mul should be replaced with an add instruction

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes='lllvm-loop-strength-reduction' -S | grep "add i32 %1, 17"

define i32 @testfunc() {
	br label %Loop
Loop:		; preds = %Loop, %0
	%i = phi i32 [ 0, %0 ], [ %Next, %Loop ]
	%mul = mul i32 %i, 17
	%Next = add i32 %i, 1
	%cond = icmp ult i32 %Next, 1000
	br i1 %cond, label %Out, label %Loop
Out:		; preds = %Loop
	ret i32 %mul
}
