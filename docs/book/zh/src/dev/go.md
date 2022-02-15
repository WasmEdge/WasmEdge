# Go

在 WasmEdge 中运行 Go 程序最好的方式是使用 [TinyGo](https://tinygo.org/) 编译 Go 源码到 WebAssembly。 在本篇中，我们将向你介绍这种做法。

## 安装 TinyGo

你需要在安装 TinyGo 之前已经在你的机器上安装过 [Go](https://go.dev/doc/install) 。

对于 x86 处理器上的 Ubuntu 或者其他 Debian 系 Linux 系统，你可以使用接下来的命令去安装 TinyGo。对于其他的 Linux 发行版，可以去阅读一下 [TinyGo docs](https://tlinygo.org/getting-started/instal/) 。Go 安装版本最好在 1.17 及以上。

```
wget https://github.com/tinygo-org/tinygo/releases/download/v0.21.0/tinygo_0.21.0_amd64.deb
sudo dpkg -i tinygo_0.21.0_amd64.deb`
```

接着，运行下一条的命令来检测安装是否成功。

```
$ tinygo version
tinygo version 0.21.0 linux/amd64 (using go version go1.16.7 and LLVM version 11.0.0)
```

## Hello world 

下面这个简单的 Go 应用有一个打印信息到控制台的 `main()` 函数。 `main.go` 文件源代码如下所示。

```go
package main

func main() {
    println("Hello TinyGo from WasmEdge!")
}
``` 

> 在 `main()` 函数中，你可以使用 GO 标准 API 读写文件，并访问命令行参数和 `env` 变量。

### 编译和构建

之后，使用 TinyGO 编译 `main.go` 程序到 WebAssembly 。

```bash
tinygo build -o hello.wasm -target wasi main.go
```

你将在同一目录下看见一个名为 `hello.wasm` 文件。这是一个 WebAssembly 字节码文件。

### 运行

你可以使用 [WasmEdge CLI](../start/cli.md) 运行它。

```bash
$ wasmedge hello.wasm
Hello TinyGo from WasmEdge!
```

## 一个简单的函数

第二个例子是接收一个参数来计算斐波那契数的 Go 函数。但是为了确保 Go 应用设置对操作系统正确的访问(例如访问命令行参数)，你需要在源代码中包含一个空的 `main()` 函数。

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

### 编译和构建

然后，使用 TinyGo 编译 `main.go` 程序到 WebAssembly。

```bash
tinygo build -o fib.wasm -target wasi main.go
```

你将在相同目录下看到一个名为 `fib.wasm` 的文件。这是一个 WebAssembly 字节码文件。

### 运行

你能使用 [WasmEdge CLI](../start/cli.md) 在其 `--reator` 模式下运行它。wasm 文件后面的命令行参数为函数名和参数。

```bash
$ wasmedge --reactor fib.wasm fibArray 10
34
```

## 提升性能

为了让这些应用达到本地 Go 性能，你可以使用 `wasmedgec` 命令对 `wasm` 程序进行 AOT 编译，然后使用 `wasmedge` 命令运行它。

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
