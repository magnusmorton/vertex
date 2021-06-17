cmake_minimum_required(VERSION 3.13)

project(Vertex VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

configure_file(VertexConfig.h.in VertexConfig.h)

find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(IGRAPH REQUIRED)
find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${IGRAPH_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
link_directories(${IGRAPH_LIBRARY_DIRS})

aux_source_directory(examples EXAMPLES)

add_library(MemPass MODULE
	MemPass.cc
)

add_library(runtime SHARED
  runtime.cc
  )

set_target_properties(runtime PROPERTIES
    COMPILE_FLAGS "-g"
)

target_compile_features(MemPass PRIVATE cxx_range_for cxx_auto_type)

foreach ( sourcefile ${EXAMPLES} )
  get_filename_component(exname ${sourcefile} NAME_WLE)
  add_executable( ${exname} ${sourcefile} )
  target_link_libraries( ${exname} runtime )
  set_target_properties( ${exname} PROPERTIES
    COMPILE_FLAGS "-O1 -g -fexperimental-new-pass-manager  -fpass-plugin=./libMemPass.so")
  add_dependencies( ${exname} MemPass )
endforeach ( sourcefile ${EXAMPLES} )
