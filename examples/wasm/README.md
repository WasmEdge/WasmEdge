# Examples with the WasmEdge binary tools

This folder contains the example WASM files which can be executed by the WasmEdge binary tools.

## Compile WAT to WASM

Most of the following examples are handwritten in WAT format. You can use [WABT tool](https://github.com/WebAssembly/wabt) to compile it to the WASM format.

```bash
wat2wasm add.wat # This will generate add.wasm
wat2wasm fibonacci.wat # This will generate fibonacci.wasm
wat2wasm factorial.wat # This will generate factorial.wasm
```

## Add two numbers

The `add.wat` is a handwritten WebAssembly script to add two given numbers. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes the result of (1+2).

```bash
$ wasmedge --reactor add.wasm add 1 2
3
```

## Calculate Fibonacci Number

The `fibonacci.wat` is a handwritten WebAssembly script to compute the Fibonacci sequence. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes the 8th Fibonacci number.

```bash
$ wasmedge --reactor fibonacci.wasm fib 8
34
```

## Calculate the N factorial

The `factorial.wat` is a handwritten WebAssembly script to compute factorial numbers. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes `12!`

```bash
$ wasmedge --reactor factorial.wasm fac 12
479001600
```

## Hello World

The `hello.wasm` example is a WebAssembly which compiled from a Rust application called `hello`, you can find it under the `hello` folder..

### Build from source

If you want to compile it, please [install Rust toolchain](https://www.rust-lang.org/tools/install). And then use the following commands in the `hello` folder:

```bash
cargo build --offline --release --target=wasm32-wasi
# The hello.wasm will be located at `target/wasm32-wasi/release/hello.wasm`
```

```bash
# Run it in the interpreter mode
$ wasmedge hello.wasm WasmEdge 1 2 3
hello
WasmEdge
1
2
3

# Run it in the AOT mode
$ wasmedge compile hello.wasm hello.aot.wasm
$ wasmedge hello.aot.wasm WasmEdge 1 2 3
hello
WasmEdge
1
2
3
```
