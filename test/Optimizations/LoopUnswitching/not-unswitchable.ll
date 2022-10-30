; Test that we don't unswitch on non-invariant branches

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-loop-unswitching -S | FileCheck %s -check-prefix=CHECK

define void @test(i128 %N) {
	; CHECK-LABEL: define void @test(i128 %N) {
	; CHECK-NOT: clone

	br label %PREHEADER
PREHEADER:
	br label %LOOP_COND
LOOP_COND:
	%induct = phi i128 [ 0, %PREHEADER ], [ %induct2, %LOOP_BODY_END ]
	%loop_cmp = icmp ult i128 %induct, %N
	br i1 %loop_cmp, label %IF_ELSE_HEAD, label %EXIT
IF_ELSE_HEAD:
	%induct2 = add i128 %induct, 1
	%if_else_cmp = icmp ult i128 %induct2, 100
	br i1 %if_else_cmp, label %LOOP_BODY_LEFT, label %LOOP_BODY_RIGHT
LOOP_BODY_LEFT:
	br label %LOOP_BODY_END
LOOP_BODY_RIGHT:
	br label %LOOP_BODY_END
LOOP_BODY_END:
	br label %LOOP_COND
EXIT:
	ret void
}
