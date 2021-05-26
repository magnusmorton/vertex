# Unnamed dataflow sanitizer
Automatically labels malloced regions

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

We need to use at least -O1. If you can figure out a way of using no
optimisation level while still using the "new" pass manager, please let me know.

then:
```shell
LD_LIBRARY_PATH=./build ./a.out
```
