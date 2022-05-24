# Go

在 WasmEdge 中运行 Go 程序的最佳方式是使用 [TinyGo](https://tinygo.org/) 将 Go 源代码编译为 WebAssembly。在本文中，我们将向你展示如何操作。

## 安装 TinyGo

在安装 TinyGo 之前，你的机器上必须已经安装了 [Go](https://go.dev/doc/install)。建议使用 Go v1.17 或更高版本。
对于 x86 处理器上的 Ubuntu 或其他基于 Debian 的 Linux 系统，你可以使用以下命令行来安装 TinyGo。其他平台请参考 [TinyGo 文档](https://tinygo.org/getting-started/install/)。

```bash
wget https://github.com/tinygo-org/tinygo/releases/download/v0.21.0/tinygo_0.21.0_amd64.deb
sudo dpkg -i tinygo_0.21.0_amd64.deb`
```

接下来，运行以下命令行检查是否安装成功。

```bash
$ tinygo version
tinygo version 0.21.0 linux/amd64 (using go version go1.16.7 and LLVM version 11.0.0)
```

## Hello world

以下是个简单的 Go 应用程序，`main()` 函数主要用来向控制台打印一条消息。
`main.go` 文件中的源代码如下：

```go
package main

func main() {
  println("Hello TinyGo from WasmEdge!")
}
```

> 在 `main()` 函数中，你可以使用 Go 标准 API 读取/写入文件，以及访问命令行参数和 `env` 变量。

### Hello world：编译和构建

接下来，使用 TinyGo 将 `main.go` 程序编译为 WebAssembly。

```bash
tinygo build -o hello.wasm -target wasi main.go
```

你将在同一目录中看到一个名为 `hello.wasm` 的文件。这是一个 WebAssembly 字节码文件。

### Hello world：运行

你可以使用 [WasmEdge命令行](../start/cli.md) 运行。

```bash
$ wasmedge hello.wasm
Hello TinyGo from WasmEdge!
```

## 一个简单的函数

第二个例子是一个 Go 函数，输入一个参数，计算一个斐波那契数。但是，为了让 Go 应用程序设置对操作系统的正确权限（例如，获得命令行参数的权限），你必须在源代码中包含一个空的 `main()` 函数。

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

### 一个简单的函数：编译和构建

接下来，使用 TinyGo 将 `main.go` 程序编译为 WebAssembly。

```bash
tinygo build -o fib.wasm -target wasi main.go
```

你将在同一目录中看到一个名为 `fib.wasm` 的文件。这是一个 WebAssembly 字节码文件。

### 一个简单的函数：运行

你可以使用 [WasmEdge命令行](../start/cli.md) 在其 `--reactor` 模式下运行它。
`wasm` 文件后面的命令行参数是函数名及其调用参数。

```bash
$ wasmedge --reactor fib.wasm fibArray 10
34
```

## 性能提升

要为这些应用程序达到原生 Go 性能，你可以使用 `wasmedgec` 命令来 AOT 编译 `wasm` 程序，然后使用 `wasmedge` 命令运行它。

```bash
$ wasmedgec hello.wasm hello.wasm

$ wasmedge hello.wasm
Hello TinyGo from WasmEdge!
```

对于 `--reactor` 模式，

```bash
$ wasmedgec fib.wasm fib.wasm

$ wasmedge --reactor fib.wasm fibArray 10
34
```
