# Parse JSON example with WasmEdge C API

The `parse-json` example shows how to use a host function with a shared library `json-c`. The `parseJson` function accepts a string as a key, parses the JSON file `test.json` and returns the corresponding value.

The `parse-json.wat` is a WebAssembly script. It is compiled into WebAssembly in the form of `parse-json.wasm` using the [WABT tool](https://github.com/WebAssembly/wabt).

## Installation

The version of `json-c` installed is `0.13.1` in `Ubuntu 20.04.3`.

Install json-c using the command below.

```bash
sudo apt install -y libjson-c-dev
```

## Instructions

```bash
# Compilation
$ gcc host_function.c -ljson-c -lwasmedge -o host_function
$ ./host_function
Got the result: Success
```
