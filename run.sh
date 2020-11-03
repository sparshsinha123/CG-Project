#!/bin/bash
make clean
make
dir=$(pwd)
LD_LIBRARY_PATH=$dir"/src/libtorch/lib" ./cd
make clean