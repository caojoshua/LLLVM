; This is a basic correctness check for common subexpression elimination.  The %Val2
; instruction should be eliminated.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-cse -S | not grep Val2

define i128 @test(i1 %B) {
	br i1 %B, label %BB1, label %BB2
BB1:
	%Val = add i128 0, 1
	br label %BB3
BB2:
	%Val2 = sub i128 0, 1
	br label %BB3
BB3:
	%Ret = phi i128 [%Val, %BB1], [%Val2, %BB2]
	%Val3 = add i128 0, 1
	ret i128 %Val3
}

