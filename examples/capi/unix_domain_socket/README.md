# Unix Domain Socket With WasmEdge C API

## Introduction

+ WasmEdge
  + Wasi Socket in UNIX Domain Socket with address V2

## Tutorial

### WasmEdge Installation

Please follows the [installation step](https://wasmedge.org/book/en/quick_start/install.html) to install WasmEdge.

### Emscripten Installation

Please follow the [installation step](https://emscripten.org/docs/getting_started/index.html) to install Emscripten Compiler Frontend (emcc)

### The Socket C++ Program to WASM

A example signal thread server and a simple client are provided. The header file `wrapper.h` provided the `sock_*_v2` function signature let emcc to generate the api import.

The Unix Domain Socket use file path as input address, therefore the address format V2 are required. Unlike the address V1 has only 4 or 8 bytes. The address has fixed 128 bytes storage and make it large enough to store the unix path.

```bash
address V2 format
|01             |23456789...127|
|address family |address buffer|
```

#### Compile C++ into WASM

Use `emcc` to compile a c++ program to WASM. Add option `ERROR_ON_UNDEFINED_SYMBOLS=0` to generate the customize module import.

```bash
emcc server.cpp -o server.wasm -sERROR_ON_UNDEFINED_SYMBOLS=0 -sSTANDALONE_WASM
emcc client.cpp -o client.wasm -sERROR_ON_UNDEFINED_SYMBOLS=0 -sSTANDALONE_WASM
```

## Results and Evaluation

Try to input an string in client. The example server will return a reversed string to client.

### Client

```bash
$ Wasmedge
Server: egdemsaW
$ egdemsaW
Server: Wasmedge
$ Was it a car or a cat I saw?
Server: ?was I tac a ro rac a ti saW
```

### Server

```bash
Client: Wasmedge
Client: egdemsaW
Client: Was it a car or a cat I saw?
```
