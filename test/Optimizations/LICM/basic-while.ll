; basic while loop test.
; Checks that the loop invariant computation of %i2 is moved to before the loop

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes='lllvm-licm' -S | FileCheck %s -check-prefix=CHECK

define void @testfunc(i32 %i) {
; CHECK-LABEL:  @testfunc(
; CHECK-NEXT:     %i2 = mul i32 %i, 17
; CHECK-NEXT:     br label %Header

; CHECK-LABEL:  Body:
; CHECK-NOT:      %i2 = mul i32 %i, 17

; <label>:0
	br label %Header
Header:		; preds = %Body, %0
	%j = phi i32 [ 0, %0 ], [ %Next, %Body ]		; <i32> [#uses=1]
	%cond = icmp eq i32 %j, 0		; <i1> [#uses=1]
	br i1 %cond, label %Out, label %Body
Body:
	%i2 = mul i32 %i, 17		; <i32> [#uses=1]
	%Next = add i32 %j, %i2		; <i32> [#uses=2]
	br label %Header
Out:
	ret void
}
