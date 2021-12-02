# Go


The best way to run Go programs is to create the application in [TinyGo](https://tinygo.org/) and then compile it to WebAssembly. In this article, we will show you how to develop and run pure wasm from Go.

## Install TinyGo

You must have Go already installed on your machine to install TinyGo. The Go v1.17 or above is recommended.

For Ubuntu or another Debian-based Linux on an Intel processor, you could use the following command line to install TinyGo. For other platforms, please refer to the TinyGo docs.  https://tinygo.org/getting-started/install/

```
wget https://github.com/tinygo-org/tinygo/releases/download/v0.21.0/tinygo_0.21.0_amd64.deb
sudo dpkg -i tinygo_0.21.0_amd64.deb`
```

Next, run the following command line to check out if the installation is successful.

```
$ tinygo version
tinygo version 0.21.0 linux/amd64 (using go version go1.16.7 and LLVM version 11.0.0)
```

## Compile a Go program to WebAssembly

There are two ways to write Go functions. One is with `mian()` function, the other one is with empty `main()` function. You could use the following command to compile both Go codes. We will discuss how to run different codes in WasmEdge later.

```
tinygo build -o fib.wasm -target wasi main.go
```

You will see a file named `fib.wasm` in the same directory. This is a pure wasm bytecode file.

## Run the bytecode in WasmEdge

As we mentioned before, the way you write Go functions decides how to run the compiled wasm bytecode in WasmEdge. We will show the differences with examples.

### With `main()` function

We use the fibArray function as an example like this.

```
package main

func main() {
    println("in main")
    println("call fibArray(10) = ", fibArray(10))
}

//export fibArray
func fibArray(n int32) int32 {
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


After we compiled the go code to wasm bytecode using Tinygo, we could run the wasm file in WasmEdge via the following command.


> Make sure you have [installed WasmEdge](https://wasmedge.org/book/en/start/install.html) before.

```
$ wasmedge lib.wasm fibArray 10

in main
call fibArray(10) = 34
```

### With empty  `main()` function


We will use the fibArray function again but with an empty `main() `function.


```
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

> If the WebAssembly program contains one or more public functions, `wasmedge` could execute individual functions in the reactor model. More details here: https://wasmedge.org/book/en/start/cli.html


We use the reactor model here to run the compiled go code in WasmEdge.


```
$ wasmedge --reactor wasm.wasm fibArray 10
34
```


To achieve higher performance, you could also compile the `.wasm` file to the `.so` file.  Check out [the install docs](https://wasmedge.org/book/en/start/install.html#whats-installed) for more details.


