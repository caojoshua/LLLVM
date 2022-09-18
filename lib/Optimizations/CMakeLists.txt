target_sources(LLVMlllvm PRIVATE
  CommonSubexpressionElimination.cpp
  DeadCodeElimination.cpp
  LICM.cpp
  SparseConditionalConstantPropagation.cpp
  TailRecursionElimination.cpp
  )

target_include_directories(LLVMlllvm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


