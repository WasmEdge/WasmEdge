#!/bin/bash

# docker pull tinygo/tinygo:0.23.0
# docker run -it --rm -v $(pwd):/src tinygo/tinygo:0.23.0 /bin/bash

# build to wasm
docker run --rm -v $(pwd):/src tinygo/tinygo:0.23.0 \
    /bin/bash -c "
    cd /src
    tinygo build -o fib.wasm -target wasi main.go
    "

time wasmedge --reactor fib.wasm fibArray 10

# wasmedge optimize
# generate a Universal Wasm Binary Format file or .so file
wasmedgec fib.wasm fib.wasmedge
time wasmedge --reactor fib.wasmedge fibArray 10
