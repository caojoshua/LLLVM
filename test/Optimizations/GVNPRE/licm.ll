; Test GVNPRE for basic LICM

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-gvnpre -S

; loop invariant code motion on a do-while loop
define void @licm(i1 %cond) {
	br label %BODY
BODY:
	%invariant = add i32 1, 4
	%cmp = icmp eq i1 %cond, 0
	br i1 %cmp, label %BODY, label %EXIT
EXIT:
	ret void
}

