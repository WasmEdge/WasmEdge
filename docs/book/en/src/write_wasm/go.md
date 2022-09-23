# Go

The best way to run Go programs in WasmEdge is to compile Go source code to WebAssembly using [TinyGo](https://tinygo.org/). In this article, we will show you how.

## Install TinyGo

You must have [Go already installed](https://go.dev/doc/install) on your machine before installing TinyGo. Go v1.17 or above is recommended.
For Ubuntu or other Debian-based Linux systems on x86 processors, you could use the following command line to install TinyGo. For other platforms, please refer to [TinyGo docs](https://tinygo.org/getting-started/install/).

```bash
wget https://github.com/tinygo-org/tinygo/releases/download/v0.21.0/tinygo_0.21.0_amd64.deb
sudo dpkg -i tinygo_0.21.0_amd64.deb`
```

Next, run the following command line to check out if the installation is successful.

```bash
$ tinygo version
tinygo version 0.21.0 linux/amd64 (using go version go1.16.7 and LLVM version 11.0.0)
```

## Hello world

The simple Go app has a `main()` function to print a message to the console.
The source code in `main.go` file is as follows.

```go
package main

func main() {
  println("Hello TinyGo from WasmEdge!")
}
```

> Inside the `main()` function, you can use Go standard API to read / write files, and access command line arguments and `env` variables.

### Hello world: Compile and build

Next, compile the `main.go` program to WebAssembly using TinyGo.

```bash
tinygo build -o hello.wasm -target wasi main.go
```

You will see a file named `hello.wasm` in the same directory. This is a WebAssembly bytecode file.

### Hello world: Run

You can run it with the [WasmEdge CLI](../cli/wasmedge.md).

```bash
$ wasmedge hello.wasm
Hello TinyGo from WasmEdge!
```

## A simple function

The second example is a Go function that takes a call parameter to compute a fibonacci number. However, in order for the Go application to set up proper access to the OS (e.g., to access the command line arguments), you must include an empty `main()` function in the source code.

```go
package main

func main(){
}

//export fibArray
func fibArray(n int32) int32{
  arr := make([]int32, n)
  for i := int32(0); i < n; i++ {
    switch {
    case i < 2:
      arr[i] = i
    default:
      arr[i] = arr[i-1] + arr[i-2]
    }
  }
  return arr[n-1]
}
```

### A simple function: Compile and build

Next, compile the `main.go` program to WebAssembly using TinyGo.

```bash
tinygo build -o fib.wasm -target wasi main.go
```

You will see a file named `fib.wasm` in the same directory. This is a WebAssembly bytecode file.

### A simple function: Run

You can run it with the [WasmEdge CLI](../cli/wasmedge.md) in its `--reactor` mode.
The command line arguments that follow the `wasm` file are the function name and its call parameters.

```bash
$ wasmedge --reactor fib.wasm fibArray 10
34
```

## Improve performance

To achieve native Go performance for those applications, you could use the `wasmedgec` command to AOT compile the `wasm` program, and then run it with the `wasmedge` command.

```bash
$ wasmedgec hello.wasm hello.wasm

$ wasmedge hello.wasm
Hello TinyGo from WasmEdge!
```

For the `--reactor` mode,

```bash
$ wasmedgec fib.wasm fib.wasm

$ wasmedge --reactor fib.wasm fibArray 10
34
```
