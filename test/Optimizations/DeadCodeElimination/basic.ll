; This is a basic correctness check for dead code elimination.  The sub
; instruction should be eliminated.

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-dce -S | not grep sub

define i128 @test(i128 %B) {
	%Unused = sub i128 %B, 1
	%Ret = add i128 %B, 1
	ret i128 %Ret
}

