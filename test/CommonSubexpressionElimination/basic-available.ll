; This is a basic correctness check for common subexpression elimination that only
; we only replace Instructions with available values.  The %Val3 instruction should
; not be eliminated, and the entire function should be unmodified. Incorrect CSE would
; replace %Val3 with %Val1.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-cse -S | grep Val3

define i128 @test(i1 %B) {
	br i1 %B, label %BB1, label %BB2
BB1:
	%Val1 = add i128 0, 1
	br label %BB3
BB2:
	%Val2 = sub i128 0, 1
	br label %BB3
BB3:
	%Ret = phi i128 [%Val1, %BB1], [%Val2, %BB2]
	%Val3 = add i128 0, 1
	ret i128 %Val3
}

