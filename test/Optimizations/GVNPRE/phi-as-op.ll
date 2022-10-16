; Test that we can determine partially redundant expressions with a phi as an
; operand. %Redundant should be determined partially redudandant with %First

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-gvnpre -S | FileCheck %s

define i128 @test(i1 %B) {
; CHECK-LABEL: define i128 @test(i1 %B) {
; CHECK-NOT: %redudandant

	br i1 %B, label %BB1, label %BB2
BB1:
	%Left = add i128 0, 1
	%First = add i128 %Left, 2
	br label %BB3
BB2:
	%Right = sub i128 0, 1
	br label %BB3
BB3:
	%Center = phi i128 [ %Left, %BB1 ], [ %Right, %BB2 ]
	%Redundant = add i128 %Center, 2
	ret i128 %Center
}

