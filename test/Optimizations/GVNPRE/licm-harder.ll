; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-gvnpre -S

; loop invariant code motion on a do-while loop
define void @licm(i128 %i_if) {
	%ifcmp = icmp ult i128 %i_if, 64
	br i1 %ifcmp, label %LOOP_PREHEADER, label %EXIT
LOOP_PREHEADER:
	br label %LOOP_BODY
LOOP_BODY:
	%phi = phi i128 [ %i_if, %LOOP_PREHEADER ], [ %i_loop, %LOOP_BODY ]
	%invariant = add i32 1, 4
	%i_loop = add i128 %phi, 1
	%loopcmp = icmp ult i128 %i_loop, 64
	br i1 %loopcmp, label %LOOP_BODY, label %EXIT
EXIT:
	ret void
}

