; basic PRE test

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-gvnpre -S | FileCheck %s

define i128 @test(i1 %B) {
	; CHECK-LABEL: define i128 @test(i1 %B) {

	; CHECK: BB2:
	; CHECK-NEXT: %Val3 = sub i128 0, 1
	; CHECK-NEXT: %1 = add i128 0, 1

	; CHECK: BB3:
	; CHECK-NEXT: phi i128 [ %1, %BB2 ], [ %Val2, %BB1 ]

	br i1 %B, label %BB1, label %BB2
BB1:
	%Val2 = add i128 0, 1
	br label %BB3
BB2:
	%Val3 = sub i128 0, 1
	br label %BB3
BB3:
	%Ret = add i128 0, 1
	ret i128 %Ret
}

