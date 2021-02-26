cmake_minimum_required(VERSION 3.18)

project(Vertex VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 14)

configure_file(VertexConfig.h.in VertexConfig.h)


find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_library(GraphSan MODULE
	GraphSan.cc
)

target_compile_features(GraphSan PRIVATE cxx_range_for cxx_auto_type)
