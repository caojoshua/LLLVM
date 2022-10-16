; basic GVN test

; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-gvnpre -S | FileCheck %s

define i128 @local() {
	; CHECK-LABEL: define i128 @local() {
	; CHECK-NOT: Dupe
	; CHECK: ret i128 %Val

	%Val = add i128 0, 1
	%Dupe = add i128 0, 1
	ret i128 %Dupe
}

define i128 @singlepath() {
	; CHECK-LABEL: define i128 @singlepath() {
	; CHECK-NOT: Dupe
	; CHECK: ret i128 %Val

	%Val = add i128 0, 1
	br label %BB1
BB1:
	br label %BB2
BB2:
	%Dupe = add i128 0, 1
	ret i128 %Dupe
}

define i128 @ifelse(i1 %B) {
	; CHECK-LABEL: define i128 @ifelse(i1 %B) {
	; CHECK-NOT: Val2

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

