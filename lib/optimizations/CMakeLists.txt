target_sources(LLVMlllvm PRIVATE
  SparseConditionalConstantPropagation.cpp
  )

target_include_directories(LLVMlllvm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


