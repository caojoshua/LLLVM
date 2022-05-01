; This is a basic correctness check for common subexpression elimination.  The %Val2
; instruction should be eliminated.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-cse -S | not grep Val2

define i128 @test(i1 %B) {
	%Val = add i128 0, 1
	br i1 %B, label %BB1, label %BB2
BB1:
	%Val2 = add i128 0, 1
	br label %BB3
BB2:
	%Val3 = sub i128 0, 1
	br label %BB3
BB3:
	%Ret = phi i128 [%Val2, %BB1], [%Val3, %BB2]
	ret i128 %Ret
}

