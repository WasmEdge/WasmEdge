# Examples

The `hello.wasm` example is compiled from [Rust source code](https://github.com/second-state/wasm-learning/tree/master/cli/hello).

```
# Run it in the interpreter mode
$ wasmedge hello.wasm 1 2 3
hello.wasm
1
2
3

# Run it in the AOT mode
$ wasmedgec hello.wasm hello.so
$ wasmedge hello.so 1 2 3
hello.wasm
1
2
3
```

The `add.wasm` example shows how a [Rust function](https://github.com/second-state/wasm-learning/tree/master/cli/add) can be invoked directly in the reactor mode.

```
$ wasmedge --reactor add.wasm add 2 2
4
```

The `fibonacci.wat` is a handwritten WebAssembly script to compute the Fibonacci sequence. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes the 8th Fibonacci number.

```
$ wasmedge --reactor fibonacci.wasm fib 8
34
```

The `factorial.wat` is a handwritten WebAssembly script to compute factorial numbers. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt). The following example computes `12!`

```
$ wasmedge --reactor factorial.wasm fac 12
479001600
```

The `js` folder contains JavaScript examples. You can use WasmEdge as a managed JavaScript application runtime / sandbox. [Learn more](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)

