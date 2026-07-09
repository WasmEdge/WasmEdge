# Fibonacci example with WasmEdge C API

Load `fibonacci.wasm` and call the exported `fib` function.

The WASM module is built from [`../../wasm/fibonacci.wat`](../../wasm/fibonacci.wat). See [`../../wasm/README.md`](../../wasm/README.md) for `wat2wasm` instructions.

## Prerequisites

- WasmEdge installed (tested with 0.17.x)
- `g++` and the WasmEdge C API headers/library

## Build and run

```bash
cd examples/capi/fibonacci
make
./run_fib 8
```

Expected output:

```text
fib(8) = 34
```

If WasmEdge is not in the default library path:

```bash
export LD_LIBRARY_PATH="$HOME/.local/wasmedge/lib64:$LD_LIBRARY_PATH"
make WASMEDGE=$HOME/.local/wasmedge
```
