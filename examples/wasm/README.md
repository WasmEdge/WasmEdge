# Examples with the WasmEdge binary tools

This folder contains the example WASM files which can be executed by the WasmEdge binary tools.

The `hello.wasm` example is compiled from [Rust source code](https://github.com/second-state/wasm-learning/tree/master/cli/hello).

```bash
# Run it in the interpreter mode
$ wasmedge hello.wasm 1 2 3
hello
1
2
3

# Run it in the AOT mode
$ wasmedgec hello.wasm hello.so
$ wasmedge hello.so 1 2 3
hello
1
2
3
```

The `add.wat` is a handwritten WebAssembly script to add two given numbers. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes the result of (1+2).

```bash
$ wasmedge --reactor add.wasm add 1 2
3
```

The `fibonacci.wat` is a handwritten WebAssembly script to compute the Fibonacci sequence. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes the 8th Fibonacci number.

```bash
$ wasmedge --reactor fibonacci.wasm fib 8
34
```

The `factorial.wat` is a handwritten WebAssembly script to compute factorial numbers. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes `12!`

```bash
$ wasmedge --reactor factorial.wasm fac 12
479001600
```
