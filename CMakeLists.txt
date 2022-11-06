cmake_minimum_required(VERSION 3.13)
project(MyProj)

find_package(LLVM REQUIRED CONFIG)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS}, include)

add_subdirectory(lib)
add_subdirectory(test)

include(CTest)
add_test(NAME litTests COMMAND lit ${CMAKE_BINARY_DIR}/test)
