; test for loop unswitching that preheader phis are moved to the new if-else header

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-loop-unswitching -S | FileCheck %s -check-prefix=CHECK

define void @test(i1 %cond, i128 %N) {
	; CHECK-LABEL: define void @test(i1 %cond, i128 %N) {

	; CHECK: IF_ELSE_HEAD:
	; CHECK-NEXT: %induct2 = add i128 %induct, 1
	; CHECK-NEXT: br label %LOOP_BODY_LEFT

	; CHECK: IF_ELSE_HEAD.split:
	; CHECK-NEXT: %PRED_PHI = phi i32 [ %LEFT, %PRED_IF ], [ %RIGHT, %PRED_ELSE ]
	; CHECK-NEXT: %if_else_cmp = icmp eq i1 %cond, false
	; CHECK-NEXT: br i1 %if_else_cmp, label %PREHEADER, label %PREHEADER.clone

	; CHECK: IF_ELSE_HEAD.clone:
	; CHECK-NEXT: %induct2.clone = add i128 %induct.clone, 1
	; CHECK-NEXT: br label %LOOP_BODY_RIGHT.clone

	br label %PRED_IF_ELSE
PRED_IF_ELSE:
	%A = icmp eq i1 %cond, 0
	br i1 %A, label %PRED_IF, label %PRED_ELSE
PRED_IF:
	%LEFT = add i32 1, 2
	br label %PREHEADER
PRED_ELSE:
	%RIGHT = add i32 3, 4
	br label %PREHEADER
PREHEADER:
	%PRED_PHI = phi i32 [ %LEFT, %PRED_IF ], [ %RIGHT, %PRED_ELSE ]
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
