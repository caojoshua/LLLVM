; Dead code elimination test on a loop. Mostly concerned there is no infinite loop.
; %i3 should be eliminated.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes='lllvm-dce' -S | not grep "%i3"

define void @testfunc(i32 %i) {
; <label>:0
	br label %Loop
Loop:		; preds = %Loop, %0
	%j = phi i32 [ 0, %0 ], [ %Next, %Loop ]		; <i32> [#uses=1]
	%i2 = mul i32 %i, 17		; <i32> [#uses=1]
	%i3 = mul i32 %i, %j
	%Next = add i32 %j, %i2		; <i32> [#uses=2]
	%cond = icmp eq i32 %Next, 0		; <i1> [#uses=1]
	br i1 %cond, label %Out, label %Loop
Out:		; preds = %Loop
	ret void
}
