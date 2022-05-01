target_sources(LLVMlllvm PRIVATE
  CommonSubexpressionElimination.cpp
  DeadCodeElimination.cpp
  SparseConditionalConstantPropagation.cpp
  )

target_include_directories(LLVMlllvm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


