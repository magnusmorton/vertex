cmake_minimum_required(VERSION 3.13)

project(Vertex VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER "clang" )
set(CMAKE_CXX_COMPILER "clang++")

configure_file(VertexConfig.h.in VertexConfig.h)

find_package(PkgConfig REQUIRED)
find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

aux_source_directory(examples EXAMPLES)

add_library(MemPass MODULE
	MemPass.cc
)



add_library(runtime SHARED
  runtime.cc
  )
set_target_properties(runtime PROPERTIES
    COMPILE_FLAGS "-g -Wall"
)

target_compile_features(MemPass PRIVATE cxx_range_for cxx_auto_type)

foreach ( sourcefile ${EXAMPLES} )
  get_filename_component(exname ${sourcefile} NAME_WLE)
  add_executable( ${exname} ${sourcefile} )
  set_target_properties( ${exname} PROPERTIES
    COMPILE_FLAGS "-O1 -g -Wall -fexperimental-new-pass-manager -fpass-plugin=./libMemPass.so")
  target_link_libraries( ${exname} runtime )
  add_dependencies( ${exname} MemPass )
endforeach ( sourcefile ${EXAMPLES} )

#add_subdirectory(tests)
#
#add_custom_target(test
#  COMMAND ./tests/unittests)
#
#add_dependencies(test unittests)
