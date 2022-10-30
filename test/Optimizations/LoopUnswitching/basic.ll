; Basic test for loop unswitching. There is a loop with a invariant if-else check. The if-else
; should be hoisted out of the loop. The loop should be duplicated without an if-else.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-loop-unswitching -S | FileCheck %s -check-prefix=CHECK

define void @test(i1 %cond, i128 %N) {
	; CHECK-LABEL: define void @test(i1 %cond, i128 %N) {
	; CHECK-NEXT: br label %IF_ELSE_HEAD.split

	; CHECK: IF_ELSE_HEAD:
	; CHECK-NEXT: %induct2 = add i128 %induct, 1
	; CHECK-NEXT: br label %LOOP_BODY_LEFT

	; CHECK: IF_ELSE_HEAD.split:
	; CHECK-NEXT: %if_else_cmp = icmp eq i1 %cond, false
	; CHECK-NEXT: br i1 %if_else_cmp, label %PREHEADER, label %PREHEADER.clone

	; CHECK: IF_ELSE_HEAD.clone:
	; CHECK-NEXT: %induct2.clone = add i128 %induct.clone, 1
	; CHECK-NEXT: br label %LOOP_BODY_RIGHT.clone

	br label %PREHEADER
PREHEADER:
	br label %LOOP_COND
LOOP_COND:
	%induct = phi i128 [ 0, %PREHEADER ], [ %induct2, %LOOP_BODY_END ]
	%loop_cmp = icmp ult i128 %induct, %N
	br i1 %loop_cmp, label %IF_ELSE_HEAD, label %EXIT
IF_ELSE_HEAD:
	%induct2 = add i128 %induct, 1
	%if_else_cmp = icmp eq i1 %cond, 0
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
