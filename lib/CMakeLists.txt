add_library(LLVMlllvm MODULE
  PassRegistration.cpp)

add_subdirectory(Analysis)
add_subdirectory(Optimizations)
add_subdirectory(Support)
