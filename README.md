# Vertex
Detects data structures based on graph structured of memory regions.


## Prerequisites
You need llvm (tested with versions 12 and 13; does not work on 11 or lower) +
the headers and clang >= version 12. You also need the boost libaries and CMake
installed.

## Building library and runtime
the usual:
```shell
cd build
CC=clang CXX=clang++ cmake ..
make
```

## Using library and runtime
```shell
clang -O1 -g -fexperimental-new-pass-manager -fpass-plugin=build/libMemPass.so -L./build/ -lruntime test_dfauto.c 
```

I'm reasonably sure there's a new flag for the pass manager in recent clang, but
the above still works.

We need to use at least -O1. If you can figure out a way of using no
optimisation level while still using the "new" pass manager, please let me know.

then:
```shell
LD_LIBRARY_PATH=./build ./a.out
```

There are some examples in the examples directory. Compiled versions are found
in the build directory after a normal build.

## Important files
`MemPass.cc` - the llvm compiler pass. Inserts calls to `libruntime.so` into LLVMIR.
`runtime.cc` - the runtime. It does everything everything else.

## How it works
Vertex marks all allocations and store their addresses and sizes. We also mark
all stores, check if the target and source are addresses we know about, and then
build a graph of the relationship between these addresses. The properties of the
graph are used to determine the data structures used


## Future directions
- I always planned on using this in a pure binary analysis framework. I don't
  think this should be too difficult.
- This was always planned to be targeted at detecting graph structures, hence the name
- Eventually, we'll want to detect properties of the data structure. Also
  functions that traverse, update, delete, create etc.
- There are no satisfactory graph libraries for C or C++, so why not make another one?
- If we detect, say, an adjacency list graph (Can't right now!), that has
  betweenness centrality calculated on it, can we synthesise new code that does
  the same thing using a new graph library and perhaps a different
  representation?