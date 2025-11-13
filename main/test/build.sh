#!/bin/zsh

mkdir -p build
cd build
cmake -DRUN_MY_TESTS=ON ../.. || exit 1
make || exit 1
./tests