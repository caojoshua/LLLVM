; This is a basic correctness check for constant propagation.  BB2
; basic block should be eliminated.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=3lvm-sccp -S | not grep BB2

define i128 @test() {
	%Cond = add i1 100, -100
	br i1 %Cond, label %BB1, label %BB2
BB1:
	%Val = add i128 0, 1
	br label %BB3
BB2:
	br label %BB3
BB3:
	%Ret = phi i128 [%Val, %BB1], [2, %BB2]
	ret i128 %Ret
}

