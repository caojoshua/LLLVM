; Constant propagagation with a loop to make sure we don't infinite loop.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-sccp -S | not grep add

define i128 @test(i1 %B) {
	%C1 = add i128 0, 1
	br label %BB1
BB1:
	br i1 %B, label %BB2, label %BB3
BB2:
	; C2 should be entirely removed. It will replace all its uses with a constant, but
	; does not have any uses.
	%C2 = add i128 %C1, 2
	br label %BB1
BB3:
	ret i128 %C1
}

