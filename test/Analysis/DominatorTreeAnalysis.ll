; RUN: opt < %s -disable-output -load-pass-plugin %shlibdir/libLLVMlllvm.so -passes='lllvm-print-dom-tree' 2>&1 | FileCheck %s -check-prefix=CHECK -check-prefix=CHECK-NEWPM

; Mostly copy pasted from LLVM. Changed the order of nodes that are printed, but they are still logically equivalent.

define void @test1() {
; CHECK-OLDPM-LABEL: 'Dominator Tree Construction' for function 'test1':
; CHECK-NEWPM-LABEL: DominatorTree for function: test1
; CHECK:      [1] %entry
; CHECK-NEXT:   [2] %a
; CHECK-NEXT:   [2] %b
; CHECK-NEXT:   [2] %c
; CHECK-NEXT:     [3] %d
; CHECK-NEXT:     [3] %e

entry:
  br i1 undef, label %a, label %b

a:
  br label %c

b:
  br label %c

c:
  br i1 undef, label %d, label %e

d:
  ret void

e:
  ret void
}

define void @test2() {
; CHECK-OLDPM-LABEL: 'Dominator Tree Construction' for function 'test2':
; CHECK-NEWPM-LABEL: DominatorTree for function: test2
; CHECK:      [1] %entry
; CHECK-NEXT:   [2] %a
; CHECK-NEXT:     [3] %b
; CHECK-NEXT:       [4] %c
; CHECK-NEXT:         [5] %d
; CHECK-NEXT:         [5] %ret

entry:
  br label %a

a:
  br label %b

b:
  br i1 undef, label %a, label %c

c:
  br i1 undef, label %d, label %ret

d:
  br i1 undef, label %a, label %ret

ret:
  ret void
}
