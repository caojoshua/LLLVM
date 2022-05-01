; Test for Dead Code Elimination that unused instructions and its operands are all removed.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-dce -S | not grep -E "%Val1|%Val2"

define i128 @test(i1 %OP1, i128 %OP2) {
	br i1 %OP1, label %BB1, label %BB2
BB1:
	%Val1 = add i128 %OP2, 1
	br label %BB3
BB2:
	%Val2 = sub i128 %OP2, 1
	br label %BB3
BB3:
	%Unused = phi i128 [%Val1, %BB1], [%Val2, %BB2]
	%Ret = sdiv i128 %OP2, 2
	ret i128 %Ret
}

