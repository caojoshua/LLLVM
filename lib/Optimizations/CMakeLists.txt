target_sources(LLVMlllvm PRIVATE
  CommonSubexpressionElimination.cpp
  DeadCodeElimination.cpp
  GVNPRE.cpp
  LICM.cpp
  LoopStrengthReduction.cpp
  LoopUnswitching.cpp
  SparseConditionalConstantPropagation.cpp
  TailRecursionElimination.cpp
  )

target_include_directories(LLVMlllvm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


