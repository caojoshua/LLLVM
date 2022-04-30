target_sources(LLVMlllvm PRIVATE
  DominatorTreeAnalysis.cpp
  )

target_include_directories(LLVMlllvm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
