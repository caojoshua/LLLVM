; RUN: opt < %s -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes=lllvm-tre -S | FileCheck %s

define i32 @factorial1(i32 %0, i32 %1) {
; CHECK-LABEL: define i32 @factorial1(i32 %0, i32 %1) {
; CHECK-NEXT: %A = alloca i32

; CHECK: 3:
; CHECK-NEXT:	%4 = phi i32 [ %0, %2 ], [ %8, %7 ]
; CHECK-NEXT:	%5 = phi i32 [ %1, %2 ], [ %9, %7 ]

; CHECK-NOT: call

	%A = alloca i32
  %3 = icmp eq i32 %0, 0
  br i1 %3, label %8, label %4

4:                                                ; preds = %2
  %5 = add i32 %0, -1
  %6 = mul i32 %1, %0
  %7 = call i32 @factorial1(i32 %5, i32 %6)
	ret i32 %7

8:                                                ; preds = %2, %4
	ret i32 %1
}

; Check that we do not perform tail recursion elimination for calls with pointer arguments.
define i32 @ignore_pointer(i32* %param) {
; CHECK-LABEL: define i32 @ignore_pointer(i32* %param) {
; CHECK: call
	%A = alloca i32
	store i32 5, i32* %A
	%X = call i32 @ignore_pointer(i32* %A)
	ret i32 %X
}


