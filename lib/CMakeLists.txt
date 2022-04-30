add_library(LLVMlllvm MODULE
  PassRegistration.cpp)

add_subdirectory(analysis)
add_subdirectory(optimizations)
add_subdirectory(support)
