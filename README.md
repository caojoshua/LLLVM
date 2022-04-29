# LLLVM
learn-LLVM. A project to learn about LLVM and compiler optimizations.

## Introduction
LLVM is an educational project with the goal of learning about LLVM libraries and compiler optmizations. LLLVM will implement many optimizations that are similar to that in LLVM, but will be kept much simpler so that inexperienced LLVM developers can better understand the source code. This project is build separately from LLVM source (following [this tutorial](https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project)) to keep LLLVM smaller.

LLLVM will be kept to only middle end optimizations on LLVM IR. This means that register allocation, instruction scheduling, and codegen are out of scope.

One interesting future idea with LLLVM would be to benchmark against LLVM. We can compare compile time and compiled binary runtime.
* implement suite of example C programs i.e. Fibonacci and compile to LLVM bitcode
* run bitcodes through `opt` using LLVM and LLLVM default passes, recording the run time
* compile optimized bitcodes with LLVM. This probably means that LLLVM will get to take advantage of any LLVM backend optmizations
* run the binaries and record the run time

With this, we could see exactly how slow/fast LLLVM runs optimization algorithms and how optimal the compiled binaries are compared to LLVM. LLLVM should implement many passes before trying this.

## Requirements
* CMake >= 3.13
* LLVM 13 libraries
* [lit](https://llvm.org/docs/TestingGuide.html) for testing

## Building
From project root:
```
mkdir build
cd build
cmake ..
cmake --build .
```

## Testing
From project root after building:
```
lit build/test
```

## llvm-tutor
Much of the build and test infrastructure is copied from [llvm-tutor](https://github.com/banach-space/llvm-tutor). It will probably stay this way until I feel like taking a deeper look into it.
