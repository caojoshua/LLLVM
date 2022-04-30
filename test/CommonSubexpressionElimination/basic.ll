; This is a basic correctness check for common subexpression elimination.  The %Val2
; instruction should be eliminated.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-cse -S | not grep Val2

define i128 @test(i1 %B) {
	%Val = add i128 0, 1
	%Val2 = add i128 0, 1
	ret i128 %Val2
}

