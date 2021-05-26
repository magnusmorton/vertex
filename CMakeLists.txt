cmake_minimum_required(VERSION 3.13)

project(Vertex VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 14)

configure_file(VertexConfig.h.in VertexConfig.h)


find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_library(MemPass MODULE
	MemPass.cc
)

add_library(runtime SHARED
  runtime.c
  util.c
  )

set_target_properties(runtime PROPERTIES
    COMPILE_FLAGS "-g -std=c11"
)

target_compile_features(MemPass PRIVATE cxx_range_for cxx_auto_type)
