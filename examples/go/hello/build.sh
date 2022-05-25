#!/bin/bash

# docker pull tinygo/tinygo:0.23.0
# docker run -it --rm -v $(pwd):/src tinygo/tinygo:0.23.0 /bin/bash

docker run --rm -v $(pwd):/src tinygo/tinygo:0.23.0 \
    /bin/bash -c "
    cd /src
    tinygo build -o hello.wasm -target wasi main.go
    "

time wasmedge hello.wasm

# wasmedge optimize
wasmedgec hello.wasm hello.wasmedge
time wasmedge hello.wasmedge
