; Test GVNPRE when a critical edge needs to be removed. BB2 -> BB3 is a critical edge because BB2
; has two successors and BB3 has two predecessors. A basic block should be inserted in between
; BB2 and BB3.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-gvnpre -S

; CHECK-LABEL: define i128 @test(i1 %B, i1 %B2) {

; CHECK: gvnpre.inbetween.0
; CHECK-NEXT: add i128 0, BB1
; CHECK-NEXT: br lable %BB3

; CHECK: BB3
; CHECK-NEXT: phi i128 [ %2, %gvnpre.inbetween.0 ], [ %Val2, %BB1]

define i128 @test(i1 %B, i1 %B2) {
	br i1 %B, label %BB1, label %BB2
BB1:
	%Val2 = add i128 0, 1
	br label %BB3
BB2:
	%Val3 = sub i128 0, 1
	br i1 %B2, label %BB3, label %BB4
BB3:
	%Ret = add i128 0, 1
	ret i128 %Ret
BB4:
	ret i128 %Val3
}

