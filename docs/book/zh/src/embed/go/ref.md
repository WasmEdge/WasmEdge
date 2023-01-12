# WasmEdge Go API 参考

以下是使用 WasmEdge-Go SDK 的指南。

## 目录

* [开始](#开始)
  * [WasmEdge 安装](#WasmEdge-安装)
  * [获取 WasmEdge-go](#获取-WasmEdge-go)
  * [WasmEdge-go 扩展](#WasmEdge-go-扩展)
  * [使用 wasm-bindgen 嵌入函数的示例](#使用-wasm-bindgen-嵌入函数的示例)
  * [完整的 WASI 应用示例](#完整的-WASI-应用示例)
* [WasmEdge-go 基础知识](#WasmEdge-go-基础知识)
  * [版本](#版本)
  * [日志设置](#日志设置)
  * [值类型](#值类型)
  * [结果](#结果)
  * [上下文及其生命周期](#上下文及其生命周期)
  * [WASM 数据结构](#WASM-数据结构)
  * [配置](#配置)
* [WasmEdge VM](#WasmEdge-VM)
  * [带有 VM 对象的 WASM 执行示例](#带有-VM-对象的-WASM-执行示例)
  * [创建虚拟机](#创建虚拟机)
  * [预注册](#预注册)
  * [Host 模块注册](#Host-模块注册)
  * [WASM 注册和执行](#WASM-注册和执行)
  * [实例跟踪](#实例跟踪)
* [WasmEdge Runtime](#WasmEdge-Runtime)
  * [WASM 执行示例手把手教程](#WASM-执行示例手把手教程)
  * [加载器](#加载器)
  * [验证器](#验证器)
  * [执行者](#执行者)
  * [AST 模块](#AST-模块)
  * [Store](#Store)
  * [实例](#实例)
  * [Host 函数](#Host-函数)
* [WasmEdge AOT 编译器](#WasmEdge-AOT-编译器)
  * [编译示例](#编译示例)
  * [编译器选项](编译器选项)

## 开始

WasmEdge-go 需要 golang 版本 >= 1.15。请在安装前检查你的 golang 版本。你可以 [在这里下载 golang](https://golang.org/dl/) 。

```bash
go version
go version go1.16.5 linux/amd64
```

### WasmEdge 安装

你必须安装相同版本的 [WasmEdge 共享库](start/install.md) 与 `WasmEdge-go` 发行版或预发行版。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.1
```

如果你需要使用 `WasmEdge-go` 的 `TensorFlow` 或 `Image` 扩展，请安装带有扩展的 `WasmEdge`：

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e tf,image -v 0.9.1
```

请注意，`TensorFlow` 和 `Image` 扩展仅适用于 `Linux` 平台。 安装后，你可以使用 `source` 命令更新包含和链接搜索路径。

### 获取 WasmEdge-go

安装 WasmEdge 后，你可以获取 `WasmEdge-go` 包并在你的 Go 项目目录中构建它。

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
go build
```

### WasmEdge-go 扩展

默认情况下，`WasmEdge-go` 只开启基本 Runtime 。

`WasmEdge-go` 有以下扩展（仅在 Linux 平台上）：

* Tensorflow
  * 此扩展支持 [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow) 中的 Host 函数。
  * 如果需要 `TensorFlow` 扩展。请使用 `-e tensorflow` 命令安装 `WasmEdge`。
  * 要使用此扩展，构建时需要标签 `tensorflow`：

    ```bash
    go build -tags tensorflow
    ```

* Image
  * 此扩展支持 [WasmEdge-image](https://github.com/second-state/WasmEdge-image) 中的 Host 函数。
  * 如果需要 `Image` 扩展。请使用 `-e image` 命令安装 `WasmEdge`。
  * 为了使用这个扩展，构建时需要标签 `image`：

    ```bash
    go build -tags image
    ```

你还可以在构建时开启多个扩展：

```bash
go build -tags image,tensorflow
```

### 使用 wasm-bindgen 嵌入函数的示例

在[这个示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_BindgenFuncs) 中，我们将演示如何从 Golang 应用程序中使用 wasm-bindgen 调用一些简单的 WebAssembly 函数. [函数](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/rust_bindgen_funcs/src/lib.rs) 是用 Rust 编写的，需要复杂的调用参数和返回值。 编译器工具需要 `#[wasm_bindgen]` 宏来自动生成正确的代码，以将调用参数从 Golang 传递到 WebAssembly。

注意：目前，我们需要 Rust 编译器版本 1.50 或更低版本才能使 WebAssembly 函数与 WasmEdge 的 Golang API 一起使用。一旦接口类型规范最终确定并得到支持，我们将[赶上最新的 Rust](https://github.com/WasmEdge/WasmEdge/issues/264) 编译器版本。

注意：WebAssembly 仅支持一些开箱即用的简单数据类型。它[不支持](https://medium.com/wasm/strings-in-webassembly-wasm-57a05c1ea333) 字符串和数组等类型。为了将 Golang 中的丰富类型传递给 WebAssembly，编译器需要将它们转换为简单的整数。例如，它将字符串转换为整数内存地址和整数长度。嵌入在 rustwasmc 中的 `wasm-bindgen` 工具会自动执行此转换。

```rust
use wasm_bindgen::prelude::*;
use num_integer::lcm;
use sha3::{Digest, Sha3_256, Keccak256};

#[wasm_bindgen]
pub fn say(s: &str) -> String {
  let r = String::from("hello ");
  return r + s;
}

#[wasm_bindgen]
pub fn obfusticate(s: String) -> String {
  (&s).chars().map(|c| {
    match c {
      'A' ..= 'M' | 'a' ..= 'm' => ((c as u8) + 13) as char,
      'N' ..= 'Z' | 'n' ..= 'z' => ((c as u8) - 13) as char,
      _ => c
    }
  }).collect()
}

#[wasm_bindgen]
pub fn lowest_common_multiple(a: i32, b: i32) -> i32 {
  let r = lcm(a, b);
  return r;
}

#[wasm_bindgen]
pub fn sha3_digest(v: Vec<u8>) -> Vec<u8> {
  return Sha3_256::digest(&v).as_slice().to_vec();
}

#[wasm_bindgen]
pub fn keccak_digest(s: &[u8]) -> Vec<u8> {
  return Keccak256::digest(s).as_slice().to_vec();
}
```

首先，我们使用 [`rustwasmc` 工具](dev/rust/bindgen.md) 将 Rust 源代码编译为 WebAssembly 字节码函数。注意这里 Rust 版本要用 1.50 或更低版本的。

```bash
rustup default 1.50.0
cd rust_bindgen_funcs
rustwasmc build
# 生成的 WASM 在 pkg/rust_bindgen_funcs_lib_bg.wasm
```

在 WasmEdge 中运行 WebAssembly 函数的 [Golang 源码](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_BindgenFuncs/bindgen_funcs.go) 如下。`ExecuteBindgen()` 函数调用 WebAssembly 函数并在 `wasm-bindgen` 支持下传递参数。

```go
package main

import (
    "fmt"
    "os"

    "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
    // Expected Args[0]: program name (./bindgen_funcs)
    // Expected Args[1]: wasm or wasm-so file (rust_bindgen_funcs_lib_bg.wasm)
    wasmedge.SetLogErrorLevel()

    var conf = wasmedge.NewConfigure(wasmedge.WASI)
    var vm = wasmedge.NewVMWithConfig(conf)
    var wasi = vm.GetImportObject(wasmedge.WASI)
    wasi.InitWasi(
        os.Args[1:],     // The args
        os.Environ(),    // The envs
        []string{".:."}, // The mapping directories
    )

    // Instantiate wasm
    vm.LoadWasmFile(os.Args[1])
    vm.Validate()
    vm.Instantiate()

    // Run bindgen functions
    var res interface{}
    var err error
    
    res, err = vm.ExecuteBindgen("say", wasmedge.Bindgen_return_array, []byte("bindgen funcs test"))
    if err == nil {
        fmt.Println("Run bindgen -- say:", string(res.([]byte)))
    } 
    res, err = vm.ExecuteBindgen("obfusticate", wasmedge.Bindgen_return_array, []byte("A quick brown fox jumps over the lazy dog"))
    if err == nil {
        fmt.Println("Run bindgen -- obfusticate:", string(res.([]byte)))
    } 
    res, err = vm.ExecuteBindgen("lowest_common_multiple", wasmedge.Bindgen_return_i32, int32(123), int32(2))
    if err == nil {
        fmt.Println("Run bindgen -- lowest_common_multiple:", res.(int32))
    } 
    res, err = vm.ExecuteBindgen("sha3_digest", wasmedge.Bindgen_return_array, []byte("This is an important message"))
    if err == nil {
        fmt.Println("Run bindgen -- sha3_digest:", res.([]byte))
    } 
    res, err = vm.ExecuteBindgen("keccak_digest", wasmedge.Bindgen_return_array, []byte("This is an important message"))
    if err == nil {
        fmt.Println("Run bindgen -- keccak_digest:", res.([]byte))
    } 

    vm.Release()
    conf.Release()
}
```

接下来，使用 WasmEdge Golang SDK 构建 Golang 应用程序。

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
go build
```

运行 Golang 应用程序，WebAssembly 函数将在 WasmEdge Runtime 中执行 。

```bash
$ ./bindgen_funcs rust_bindgen_funcs/pkg/rust_bindgen_funcs_lib_bg.wasm
Run bindgen -- say: hello bindgen funcs test
Run bindgen -- obfusticate: N dhvpx oebja sbk whzcf bire gur ynml qbt
Run bindgen -- lowest_common_multiple: 246
Run bindgen -- sha3_digest: [87 27 231 209 189 105 251 49 159 10 211 250 15 159 154 181 43 218 26 141 56 199 25 45 60 10 20 163 54 211 195 203]
Run bindgen -- keccak_digest: [126 194 241 200 151 116 227 33 216 99 159 22 107 3 177 169 216 191 114 156 174 193 32 159 246 228 245 133 52 75 55 27]
```

### 完整的 WASI 应用示例

注意：你可以使用最新的 Rust 编译器创建一个带有 `main.rs` 函数的独立 WasmEdge 应用程序，然后将其嵌入到 Golang 应用程序中。

除函数之外，WasmEdge Golang SDK 还可以[嵌入独立的 WebAssembly 应用程序](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile) — 比如：带有 `main( )` 的 Rust 函数编译成 WebAssembly。

我们用一个读取文件内容的 [Rust 应用](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_ReadFile/rust_readfile) 作为 demo。注意这里不需要 `#[wasm_bindgen]`，因为 WebAssembly 程序的 WASI 支持 `main()` 函数的 `argv` 输入和 `exit code` 输出。

```rust
use std::env;
use std::fs::File;
use std::io::{self, BufRead};

fn main() {
    // Get the argv.
    let args: Vec<String> = env::args().collect();
    if args.len() <= 1 {
        println!("Rust: ERROR - No input file name.");
        return;
    }

    // Open the file.
    println!("Rust: Opening input file \"{}\"...", args[1]);
    let file = match File::open(&args[1]) {
        Err(why) => {
            println!("Rust: ERROR - Open file \"{}\" failed: {}", args[1], why);
            return;
        },
        Ok(file) => file,
    };

    // Read lines.
    let reader = io::BufReader::new(file);
    let mut texts:Vec<String> = Vec::new();
    for line in reader.lines() {
        if let Ok(text) = line {
            texts.push(text);
        }
    }
    println!("Rust: Read input file \"{}\" succeeded.", args[1]);

    // Get stdin to print lines.
    println!("Rust: Please input the line number to print the line of file.");
    let stdin = io::stdin();
    for line in stdin.lock().lines() {
        let input = line.unwrap();
        match input.parse::<usize>() {
            Ok(n) => if n > 0 && n <= texts.len() {
                println!("{}", texts[n - 1]);
            } else {
                println!("Rust: ERROR - Line \"{}\" is out of range.", n);
            },
            Err(e) => println!("Rust: ERROR - Input \"{}\" is not an integer: {}", input, e),
        }
    }
    println!("Rust: Process end.");
}
```

使用 rustwasmc 工具将应用程序编译成 WebAssembly。

```bash
cd rust_readfile
rustwasmc build
# 输出文件将位于 `pkg/rust_readfile.wasm`。
```

或者你可以直接通过 `cargo` 将应用程序编译成 WebAssembly：

```bash
cd rust_readfile
# 需要添加 `wasm32-wasi` 目标。
rustup target add wasm32-wasi
cargo build --release --target=wasm32-wasi
#  输出的 wasm 将位于 `target/wasm32-wasi/release/rust_readfile.wasm`。
```

在 Go 程序里面嵌入 WasmEdge 运行 WebAssembly 函数，Go 程序代码如下。

```go
package main

import (
    "os"

    "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
    wasmedge.SetLogErrorLevel()

    var conf = wasmedge.NewConfigure(wasmedge.REFERENCE_TYPES)
    conf.AddConfig(wasmedge.WASI)
    var vm = wasmedge.NewVMWithConfig(conf)
    var wasi = vm.GetImportObject(wasmedge.WASI)
    wasi.InitWasi(
        os.Args[1:],     // The args
        os.Environ(),    // The envs
        []string{".:."}, // The mapping directories
    )

    // Instantiate and run WASM "_start" function, which refers to the main() function
    vm.RunWasmFile(os.Args[1], "_start")

    vm.Release()
    conf.Release()
}
```

接下来，使用 WasmEdge Golang SDK 构建 Golang 应用程序。

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
go build
```

运行 Golang 应用程序。

```bash
$ ./read_file rust_readfile/pkg/rust_readfile.wasm file.txt
Rust: Opening input file "file.txt"...
Rust: Read input file "file.txt" succeeded.
Rust: Please input the line number to print the line of file.
# 输入“5”并回车。
5
# 将打印 `file.txt` 的第 5 行：
abcDEF___!@#$%^
# To terminate the program, send the EOF (Ctrl + D).
^D
# 输出将打印终止消息：
Rust: Process end.
```

更多示例请参考[示例仓库](https://github.com/second-state/WasmEdge-go-examples/) 。

## WasmEdge-go 基础知识

在本部分中，我们将介绍 WasmEdge-go API 和数据结构的实用程序和概念。

### 版本

`Version` 相关的 API 可以检查安装的 WasmEdge 共享库版本。

```go
import "github.com/second-state/WasmEdge-go/wasmedge"

verstr := wasmedge.GetVersion() // Will be `string` of WasmEdge version.
vermajor := wasmedge.GetVersionMajor() // Will be `uint` of WasmEdge major version number.
verminor := wasmedge.GetVersionMinor() // Will be `uint` of WasmEdge minor version number.
verpatch := wasmedge.GetVersionPatch() // Will be `uint` of WasmEdge patch version number.
```

### 日志设置

`wasmedge.SetLogErrorLevel()` 和 `wasmedge.SetLogDebugLevel()` API 可以将日志系统设置为调试级别或错误级别。默认情况下，是错误级别，调试信息会被隐藏。

### 值类型

在 WasmEdge-go 中，API 会自动对内置类型进行转换，并为引用类型实现数据结构。

1. 数字类型：`i32`、`i64`、`f32`和`f64`

    * 将值传递给 WASM 时，自动将 `uint32` 和 `int32` 转换为 `i32`。
    * 将值传递给 WASM 时，自动将 `uint64` 和 `int64` 转换为 `i64`。
    * 在 32 位系统中将值传递给 WASM 时，会自动将 `uint` 和 `int` 转换为 `i32`。
    * 在 64 位系统中将值传递给 WASM 时，自动将 `uint` 和 `int` 转换为 `i64`。
    * 将值传递给 WASM 时自动将 `float32` 转换为 `f32`。
    * 将值传递给 WASM 时自动将 `float64` 转换为 `f64`。
    * 得到结果时，将 WASM 的 `i32` 转换为 `int32`。
    * 得到结果时将 WASM 的 `i64` 转换为 `int64`。
    * 获取结果时将 WASM 中的 `f32` 转换为 `float32`。
    * 获取结果时将 WASM 中的 `f64` 转换为 `float64`。

2. 数字类型：为 `SIMD` 提案实现的 `v128`

    你应该使用 `wasmedge.NewV128()` 生成 `v128` 值，并使用 `wasmedge.GetV128()` 获取值。

    ```go
    val := wasmedge.NewV128(uint64(1234), uint64(5678))
    high, low := val.GetVal()
    // `high` 为 uint64(1234)，`low` 为 uint64(5678)
    ```

3. 引用类型：为 `Reference-Types` 提案实现的 `FuncRef` 和 `ExternRef`

    ```go
    funcref := wasmedge.NewFuncRef(10)
    // 创建一个函数索引为 10 的 `FuncRef`。

    num := 1234
    // `num` 是一个 `int`。
    externref := wasmedge.NewExternRef(&num)
    // 创建一个引用 `num` 的 `ExternRef`。
    num = 5678
    // 修改 `num` 为 5678。
    numref := externref.GetRef().(*int)
    // 从 `ExternRef` 获取原始引用。
    fmt.Println(*numref)
    // 将打印 `5678`。
    numref.Release()
    // 应该调用 `Release` 方法。
    ```

### 结果

`Result` 对象指定执行状态。你可以使用 `Error()` 函数来获取错误信息。

```go
// 假设 `vm` 是一个 `wasmedge.VM` 对象。
res, err = vm.Execute(...) // 忽略参数的细节。
// 假设 `res, err` 是使用 `vm` 执行函数的返回值。
if err != nil {
    fmt.Println("Error message:", err.Error())
}
```

### 上下文及其生命周期

`VM`、`Store`、`Function` 等对象是由 WasmEdge 共享库中的 `Context` 组成的。
所有的上下文都可以通过调用对应的 `New` API 来创建，你也应该调用上下文对应的 `Release` 函数来释放资源。
请注意，对于从其他上下文检索但不是从 `New` API 创建的上下文，不需要调用 `Release` 函数。

```go
// 创建一个配置。
conf := wasmedge.NewConfigure()
// 立即释放 `conf`。
conf.Release()
```

其他上下文的细节将在后面介绍。

### WASM 数据结构

WASM 数据结构用于创建实例或可以从实例上下文中查询。
实例创建的细节将在 [Instances](#Instances) 中介绍。

1. 限制

    `Limit` 结构表示最小值和最大值数据结构。

    ```go
    lim1 := wasmedge.NewLimit(12)
    fmt.Println(lim1.HasMax())
    // 将打印 `false`。
    fmt.Println(lim1.GetMin())
    // 将打印 `12`。

    lim2 := wasmedge.NewLimitWithMax(15, 50)
    fmt.Println(lim2.HasMax())
    // 将打印 `true`。
    fmt.Println(lim2.GetMin())
    // 将打印 `15`。
    fmt.Println(lim2.GetMax())
    // 将打印 `50`。
    ```

2. 函数类型上下文

    `FunctionType` 是一个持有函数类型上下文的对象，用于创建 `Function`，检查 `Function` 实例的值类型，或者从 VM 中获取具有函数名称的函数类型。
    你可以使用 `FunctionType` API 来获取参数或返回值类型信息。

    ```go
    functype := wasmedge.NewFunctionType(
        []wasmedge.ValType{
            wasmedge.ValType_ExternRef,
            wasmedge.ValType_I32,
            wasmedge.ValType_I64,
        }, []wasmedge.ValType{
            wasmedge.ValType_F32,
            wasmedge.ValType_F64,
        })

    plen := functype.GetParametersLength()
    // `plen` 将是 3。
    rlen := functype.GetReturnsLength()
    // `rlen` 将是 2。
    plist := functype.GetParameters()
    // `plist` 将是 `[]wasmedge.ValType{wasmedge.ValType_ExternRef, wasmedge.ValType_I32, wasmedge.ValType_I64}`。
    rlist := functype.GetReturns()
    // `rlist` 将是 `[]wasmedge.ValType{wasmedge.ValType_F32, wasmedge.ValType_F64}`。

    functype.Release()
    ```

3. 表类型上下文

    `TableType` 是一个持有表类型上下文的对象，用于创建 `Table` 实例或从 `Table` 实例获取信息。

    ```go
    lim := wasmedge.NewLimit(12)
    tabtype := wasmedge.NewTableType(wasmedge.RefType_ExternRef, lim)

    rtype := tabtype.GetRefType()
    // `rtype` 将是 `wasmedge.RefType_ExternRef`。
    getlim := tabtype.GetLimit()
    // `getlim` 与 `lim` 的值相同。

    tabtype.Release()
    ```

4. 内存类型上下文

    `MemoryType` 是一个保存内存类型上下文的对象，用于创建 `Memory` 实例或从 `Memory` 实例获取信息。

    ```go
    lim := wasmedge.NewLimit(1)
    memtype := wasmedge.NewMemoryType(lim)

    getlim := memtype.GetLimit()
    // `getlim` 与 `lim` 的值相同。

    memtype.Release()
    ```

5. 全局类型上下文

   `GlobalType` 是一个持有全局类型上下文的对象，用于创建 `Global` 实例或从 `Global` 实例获取信息。

    ```go
    globtype := wasmedge.NewGlobalType(wasmedge.ValType_F64, wasmedge.ValMut_Var)

    vtype := globtype.GetValType()
    // `vtype` 将是 `wasmedge.ValType_F64`。
    vmut := globtype.GetMutability()
    // `vmut` 将是 `wasmedge.ValMut_Var`。

    globtype.Release()
    ```

6. 引入类型上下文

    `ImportType` 是一个包含导入类型上下文的对象，用于从 [AST 模块](#AST-Module) 获取导入信息。你可以从 `ImportType` 对象中获取外部类型（`function`、`table`、`memory` 或 `global`）、导入模块名称和外部名称。查询 `ImportType` 对象的详细信息将在 [AST 模块](#AST-Module) 中介绍。

    ```go
    var ast *wasmedge.AST = ...
    // 假设 `Loader` 返回 `ast` 作为 WASM 文件的加载结果。
    imptypelist := ast.ListImports()
    // 假设 `imptypelist` 是从 `ast` 中列出的用于导入的数组。

    for i, imptype := range imptypelist {
        exttype := imptype.GetExternalType()
        // `exttype` 必须是 `wasmedge.ExternType_Function`、`wasmedge.ExternType_Table` 之一，
        // wasmedge.ExternType_Memory`，或 `wasmedge.ExternType_Global`。

        modname := imptype.GetModuleName()
        extname := imptype.GetExternalName()
        // 获取导入的模块名称和外部名称。

        extval := imptype.GetExternalValue()
        // `extval` 是 `interface{}` 的类型，表示 `*wasmedge.FunctionType` 之一，
        // `*wasmedge.TableType`、`*wasmedge.MemoryType` 或 `*wasmedge.GlobalType`。
    }
    ```

7. 导出类型上下文

    `ExportType` 是一个持有导出类型上下文的对象，用于从 [AST 模块](#AST-Module) 获取导出信息。你可以从 `Export Type` 上下文中获取外部类型（`function`、`table`、`memory` 或 `global`）和外部名称。关于查询 `ExportType` 对象的详细信息将在 [AST 模块](#AST-Module) 中介绍。

    ```go
    var ast *wasmedge.AST = ...
    // 假设 `Loader` 返回 `ast` 作为 WASM 文件的加载结果。
    exptypelist := ast.ListExports()
    // 假设 `exptypelist` 是从 `ast` 中列出的用于导出的数组。

    for i, exptype := range exptypelist {
        exttype := exptype.GetExternalType()
        // `exttype` 必须是 `wasmedge.ExternType_Function`、`wasmedge.ExternType_Table` 之一，
        // wasmedge.ExternType_Memory`，或 `wasmedge.ExternType_Global`。

        extname := exptype.GetExternalName()
        // 获取导出的外部名称。

        extval := exptype.GetExternalValue()
        // `extval` 是 `interface{}` 的类型，表示 `*wasmedge.FunctionType` 之一，
        // `*wasmedge.TableType`、`*wasmedge.MemoryType` 或 `*wasmedge.GlobalType`。
    }
    ```

### 配置

配置对象 `wasmedge.Configure` 管理 `Loader`、`Validator`、`Executor`、`VM` 和 `Compiler` 的配置。你可以调整 proposals 配置、VM Host 预注册（比如 `WASI`）和 AOT 编译器选项的设置，然后应用 `Configure` 对象来创建其他 Runtime 对象。

1. 配置

    WasmEdge 支持打开或关闭 WebAssembly proposals。此配置在使用 `Configure` 对象创建的任何上下文中都有效。

    ```go
    const (
        IMPORT_EXPORT_MUT_GLOBALS         = Proposal(C.WasmEdge_Proposal_ImportExportMutGlobals)
        NON_TRAP_FLOAT_TO_INT_CONVERSIONS = Proposal(C.WasmEdge_Proposal_NonTrapFloatToIntConversions)
        SIGN_EXTENSION_OPERATORS          = Proposal(C.WasmEdge_Proposal_SignExtensionOperators)
        MULTI_VALUE                       = Proposal(C.WasmEdge_Proposal_MultiValue)
        BULK_MEMORY_OPERATIONS            = Proposal(C.WasmEdge_Proposal_BulkMemoryOperations)
        REFERENCE_TYPES                   = Proposal(C.WasmEdge_Proposal_ReferenceTypes)
        SIMD                              = Proposal(C.WasmEdge_Proposal_SIMD)
        TAIL_CALL                         = Proposal(C.WasmEdge_Proposal_TailCall)
        ANNOTATIONS                       = Proposal(C.WasmEdge_Proposal_Annotations)
        MEMORY64                          = Proposal(C.WasmEdge_Proposal_Memory64)
        THREADS                           = Proposal(C.WasmEdge_Proposal_Threads)
        EXCEPTION_HANDLING                = Proposal(C.WasmEdge_Proposal_ExceptionHandling)
        FUNCTION_REFERENCES               = Proposal(C.WasmEdge_Proposal_FunctionReferences)
    )
    ```

   你可以在 `Configure` 对象中添加或删除提案。

    ```go
    // 默认情况下，以下提案最初已开启：
    // * IMPORT_EXPORT_MUT_GLOBALS
    // * NON_TRAP_FLOAT_TO_INT_CONVERSIONS
    // * SIGN_EXTENSION_OPERATORS
    // * MULTI_VALUE
    // * BULK_MEMORY_OPERATIONS
    // * REFERENCE_TYPES
    // * SIMD
    conf := wasmedge.NewConfigure()
    // 你也可以将提案作为参数传递：
    // conf := wasmedge.NewConfigure(wasmedge.SIMD, wasmedge.BULK_MEMORY_OPERATIONS)
    conf.AddConfig(wasmedge.SIMD)
    conf.RemoveConfig(wasmedge.REFERENCE_TYPES)
    is_bulkmem := conf.HasConfig(wasmedge.BULK_MEMORY_OPERATIONS)
    // `is_bulkmem` 将是 `true`。
    conf.Release()
    ```

2. Host 注册

   此配置用于 `VM` 上下文以打开 `WASI` 或 `wasmedge_process` 支持，并且仅在 `VM` 对象中有效。

    ```go
    const (
        WASI = HostRegistration(C.WasmEdge_HostRegistration_Wasi)
        WasmEdge_PROCESS = HostRegistration(C.WasmEdge_HostRegistration_WasmEdge_Process)
    )
    ```

   详情将在 [VM上下文的预注册](###预注册) 中介绍。

    ```go
    conf := wasmedge.NewConfigure()
    // 你也可以将提案作为参数传递：
    // conf := wasmedge.NewConfigure(wasmedge.WASI)
    conf.AddConfig(wasmedge.WASI)
    conf.Release()
    ```

3. 最大内存页数

   你可以通过这个配置来限制内存实例的页面大小。当在 WASM 执行中增加内存实例的页面大小并超过限制大小时，会失败。此配置仅在 `Executor` 和 `VM` 对象中有效。

    ```go
    conf := wasmedge.NewConfigure()
    
    pagesize := conf.GetMaxMemoryPage()

    // 默认情况下，每个内存实例的最大内存页大小为 65536。
    conf.SetMaxMemoryPage(1234)
    pagesize := conf.GetMaxMemoryPage()
    // `pagesize` 将是 1234。

    conf.Release()
    ```

4. AOT 编译器选项

   AOT 编译器选项配置有关优化级别、输出格式、转储 IR 和通用二进制文件的行为。

    ```go
    const (
        // 禁用尽可能多的优化。
        CompilerOptLevel_O0 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O0)
        // 在不破坏可调试性的情况下快速优化。
        CompilerOptLevel_O1 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O1)
        // 在不触发显着增量编译时间或代码大小增长的情况下尽可能地优化快速执行。
        CompilerOptLevel_O2 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O2)
        // 尽可能优化快速执行。
        CompilerOptLevel_O3 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O3)
        // 代码越小越好，而不会触发显着的编译时间增量或执行时间减慢。
        CompilerOptLevel_Os = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Os)
        // 代码越小越好。
        CompilerOptLevel_Oz = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Oz)
    )

    const (
        // 原生动态库格式。
        CompilerOutputFormat_Native = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Native)
        // WebAssembly 与自定义部分中的 AOT 编译代码。
        CompilerOutputFormat_Wasm = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Wasm)
    )
    ```

   这些配置仅在 `Compiler` 上下文中有效。

    ```go
    conf := wasmedge.NewConfigure()

    // 默认优化级别为O3。
    conf.SetCompilerOptimizationLevel(wasmedge.CompilerOptLevel_O2)
    // 默认情况下，输出格式为通用 WASM。
    conf.SetCompilerOutputFormat(wasmedge.CompilerOutputFormat_Native)
    // 默认情况下，转储 IR 为 `false`。
    conf.SetCompilerDumpIR(true)
    // 默认情况下，通用二进制文件是 `false`。
    conf.SetCompilerGenericBinary(true)

    conf.Release()
    ```

5. 统计选项

   统计选项配置有关 Runtime 和 AOT 编译器中的指令计数、成本测量和时间测量的行为。
   这些配置在 `Compiler`、`VM` 和 `Executor` 对象中有效。

    ```go
    conf := wasmedge.NewConfigure()

    // 默认情况下，在运行已编译的 WASM 或纯 WASM 时，指令计数为 `false`。
    conf.SetStatisticsInstructionCounting(true)
    // 默认情况下，运行已编译的 WASM 或纯 WASM 时，成本度量为 `false`。
    conf.SetStatisticsTimeMeasuring(true)
    // 默认情况下，运行已编译的 WASM 或纯 WASM 时，时间测量为 `false`。
    conf.SetStatisticsCostMeasuring(true)

    conf.Release()
    ```

## WasmEdge VM

在本部分中，我们将介绍 `wasmedge.VM` 对象的功能，并展示执行 WASM 功能的示例。

### 带有 VM 对象的 WASM 执行示例

下面是使用 WASM 运行斐波那契的示例。
这个例子使用了 [fibonacci.wasm](../tools/wasmedge/examples/fibonacci.wasm)，对应的 WAT 文件在 [fibonacci.wat](../tools/wasmedge/examples/fibonacci.wat)。

```wasm
(module
 (export "fib" (func $fib))
 (func $fib (param $n i32) (result i32)
  (if
   (i32.lt_s (get_local $n)(i32.const 2))
   (return (i32.const 1))
  )
  (return
   (i32.add
    (call $fib (i32.sub (get_local $n)(i32.const 2)))
    (call $fib (i32.sub (get_local $n)(i32.const 1)))
   )
  )
 )
)
```

1. 快速运行 WASM 函数

   首先新建一个 Go 项目：

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

   假设将 WASM 文件 [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) 复制到当前的 `wasmedge_test` 目录中，然后创建并编辑 Go 文件 `main.go`，如下所示：

    ```go
    package main

    import (
        "fmt"

        "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
        // 设置日志级别。
        wasmedge.SetLogErrorLevel()

        // 创建配置上下文并添加 WASI 支持。
        // 除非你需要 WASI 支持，否则此步骤不是必需的。
        conf := wasmedge.NewConfigure(wasmedge.WASI)
        // 使用配置创建虚拟机。
        vm := wasmedge.NewVMWithConfig(conf)

        res, err := vm.RunWasmFile("fibonacci.wasm", "fib", uint32(21))
        if err == nil {
            fmt.Println("Get fibonacci[21]:", res[0].(int32))
        } else {
            fmt.Println("Run failed:", err.Error())
        }

        vm.Release()
        conf.Release()
    }
    ```

   然后，你可以使用 WasmEdge Golang SDK 构建和运行 Golang 应用程序：（第 21 个斐波那契数在从 0 开始的索引中为 17711）

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
    $ go build
    $ ./wasmedge_test
    Get fibonacci[21]: 17711
    ```

2. 手动实例化和运行 WASM 函数

    除了上面的示例，你还可以使用 `VM` 对象 API 逐步运行 WASM 函数：

    ```go
    package main

    import (
        "fmt"

        "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
        // 设置日志级别。
        wasmedge.SetLogErrorLevel()

        // 创建虚拟机。
        vm := wasmedge.NewVM()
        var err error
        var res []interface{}

        // 第 1 步：加载 WASM 文件。
        err = vm.LoadWasmFile("fibonacci.wasm")
        if err != nil {
            fmt.Println("Load WASM from file FAILED:", err.Error())
            return
        }

        // 第 2 步：验证 WASM 模块。
        err = vm.Validate()
        if err != nil {
            fmt.Println("Validation FAILED:", err.Error())
            return
        }

        // 第 3 步：实例化 WASM 模块。
        err = vm.Instantiate()
        // 你可以加载、验证和实例化另一个 WASM 模块替换实例化的。在这种情况下，旧模块将被清除，但注册的模块仍然保留。
        if err != nil {
            fmt.Println("Instantiation FAILED:", err.Error())
            return
        }

        // 第 4 步：执行 WASM 函数。参数：(funcname, args...)
        res, err = vm.Execute("fib", uint32(25))
        // 你可以在实例化后重复执行函数。
        if err == nil {
            fmt.Println("Get fibonacci[25]:", res[0].(int32))
        } else {
            fmt.Println("Run failed:", err.Error())
        }

        vm.Release()
    }
    ```

    然后你可以构建并运行：（第 25 个斐波那契数是 121393，基于 0 的索引）

    ```bash
    $ go build
    $ ./wasmedge_test
    Get fibonacci[25]: 121393
    ```

    下图解释了 `VM` 对象的状态。

    ```text
                           |========================|
                  |------->|      虚拟机: 已初始化     |
                  |        |========================|
                  |                    |
                  |                 LoadWasm
                  |                    |
                  |                    v
                  |        |========================|
                  |--------|       虚拟机: 已加载     |<-------|
                  |        |========================|        |
                  |              |            ^              |
                  |              验证          |              |
                 清理             |          加载 WASM        |
                  |              v            |            加载WASM
                  |        |========================|        |
                  |--------|      虚拟机: 已验证      |        |
                  |        |========================|        |
                  |              |            ^              |
                  |            实例化          |              |
                  |              |         RegisterMoulde    |
                  |              v            |              |
                  |        |========================|        |
                  |--------|    虚拟机: 实例化        |--------|
                           |========================|
                                 |            ^
                                 |            |
                                 --------------
                    Instantiate, Execute, ExecuteRegistered,
                    ExecuteBindgen, ExecuteBindgenRegistered
    ```

    `VM` 上下文的状态在创建时会是 `Inited`。
    成功加载 WASM 后，状态将为 `Loaded`。
    成功验证 WASM 后，状态将为 `Validated`。
    WASM实例化成功后，状态为 `Instantiated`，你可以调用函数了。
    你可以在任何状态下注册 WASM 或导入对象，但需要再次实例化 WASM。
    你也可以在任何状态下加载 WASM，并且在函数调用之前应该验证和实例化 WASM 模块。
    当处于 `Instantiated` 状态时，你可以再次实例化 WASM 模块以重置旧的 WASM Runtime 结构。

3. Wasm-bindgen 支持

   `(*VM).ExecuteBindgen` 和 `(*VM).ExecuteBindgenRegistered` 是支持 `wasm-bindgen` 函数执行的特殊函数。在 WasmEdge-go 中，我们支持以下 `wasm-bindgen` 函数的返回类型：

    ```go
    type bindgen int

    const (
        Bindgen_return_void bindgen = iota
        Bindgen_return_i32 bindgen = iota
        Bindgen_return_i64 bindgen = iota
        Bindgen_return_array bindgen = iota
    )
    ```

   并且只接受 `int32`、`uint32`、`int64`、`uint64` 和 `[]byte` 参数。每个 `wasm-bindgen` 函数最多只有 1 个返回值。

    ```go
    // 以 wasm-bindgen 为例。
    var res interface{}
    var err error
    res, err = vm.ExecuteBindgen(
        "lowest_common_multiple",       // Function name
        wasmedge.Bindgen_return_i32,    // Return type: int32
        int32(123), int32(2)            // Parameters: int32, int32
    )
    if err == nil {
        fmt.Println("Run bindgen -- lowest_common_multiple:", res.(int32))
    } 
    res, err = vm.ExecuteBindgen(
        "sha3_digest",                          // Function name
        wasmedge.Bindgen_return_array,          // Return type: []byte
        []byte("This is an important message")  // Parameter: []byte
    )
    if err == nil {
        fmt.Println("Run bindgen -- sha3_digest:", res.([]byte))
    } 
    ```

   有关完整示例，请参阅 [上面的示例](#使用-wasm-bindgen-嵌入函数的示例)。

### 创建 VM

`VM` 的创建 API 接收 `Configure` 对象和 `Store` 对象。
请注意，如果 `VM` 使用外部 `Store` 对象创建，则 `VM` 将在该 `Store` 对象上执行 WASM。如果将`Store`对象设置成多个`VM`对象，在执行时可能会造成数据冲突。
`Store` 对象的详细信息将在 [Store](#Store) 中介绍。

```go
conf := wasmedge.NewConfigure()
store := wasmedge.NewStore()

// 使用默认配置和存储创建 VM。
vm := wasmedge.NewVM()
vm.Release()

// 使用指定的配置和默认存储创建 VM。
vm = wasmedge.NewVMWithConfig(conf)
vm.Release()

// 使用默认配置和指定存储创建 VM。
vm = wasmedge.NewVMWithStore(store)
vm.Release()

// 使用指定的配置和存储创建一个 VM。
vm = wasmedge.NewVMWithConfigAndStore(conf, store)
vm.Release()

conf.Release()
store.Release()
```

### 预注册

WasmEdge 提供以下内置预注册。

1. [WASI（WebAssembly 系统接口）](https://github.com/WebAssembly/WASI)

    你可以在 `Configure` 对象中开启对 VM 的 WASI 支持。

    ```go
    conf := wasmedge.NewConfigure(wasmedge.WASI)
    // 或者你可以通过 `(*Configure).AddConfig` 将 `wasmedge.WASI` 设置到配置对象中。
    vm := wasmedge.NewVMWithConfig(conf)
    vm.Release()

    // 以下 API 可以从 VM 对象中检索预注册导入对象。
    // 如果未在配置中设置相应的预注册，此 API 将返回 `nil`。
    wasiconf := conf.GetImportObject(wasmedge.WASI)
    // 初始化 WASI。
    wasiconf.InitWasi(/* ... 忽略 */)

    conf.Release()
    ```

    也可以从 API 创建 WASI 导入对象。详细内容将在 [Host 函数](#Host-函数) 和 [Host 模块注册](#Host-模块注册) 中介绍。

2. [WasmEdge_Process](https://crates.io/crates/wasmedge_process_interface)

   此预注册适用于 WasmEdge 在 `Rust` 源上的流程接口。
   开启此预注册后，VM 将支持 `wasmedge_process` Host 函数。

    ```go
    conf := wasmedge.NewConfigure(wasmedge.WasmEdge_PROCESS)
    vm := wasmedge.NewVMWithConfig(conf)
    vm.Release()
    
    // 以下 API 可以从 VM 对象中检索预注册导入对象。
    // 如果未在配置中设置相应的预注册，此 API 将返回 `nil`。
    procconf := conf.GetImportObject(wasmedge.WasmEdge_PROCESS)
    // 初始化 WasmEdge_Process。
    procconf.InitWasmEdgeProcess(/* ... 忽略 */)

    conf.Release()
    ```

   也可以从 API 创建 WasmEdge_Process 导入对象。详细内容将在 [Host 函数](#Host-函数) 和 [Host 模块预注册](#Host-模块预注册) 中介绍。

### Host 模块注册

[Host 函数](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) 是 WebAssembly 之外的函数，并作为导入传递给 WASM 模块。 在 WasmEdge-go 中，Host 函数被组合成 Host 模块作为带有模块名称的 `ImportObject` 对象。详情请参考 [WasmEdge Runtime 中的 Host 函数](#Host-函数)。在本章中，我们展示了将 host 函数注册到 `VM` 对象中的示例。

```go
vm := wasmedge.NewVM()
// 你也可以通过此 API 创建和注册 WASI Host 模块。
wasiobj := wasmedge.NewWasiImportObject(/* ... 忽略 ... */)

res := vm.RegisterImport(wasiobj)
// 应该检查结果状态。

vm.Release()
// 应该释放创建的导入对象。
wasiobj.Release()
```

### WASM 注册和执行

在 WebAssembly 中，WASM 模块中的实例可以被导出，也可以被其他 WASM 模块导入。WasmEdge VM 提供 API 供注册和导出任何 WASM 模块，并执行注册的 WASM 模块中的函数或 host 函数。

1. 使用导出的模块名称注册 WASM 模块

   除非导入对象已经包含模块名称，否则每个 WASM 模块在注册时都应该唯一命名。下面展示了示例。

   首先新建一个 Go 项目：

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

   假设 WASM 文件 [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) 被复制到当前目录。
   然后创建并编辑 Go 文件 `main.go`，如下所示：

    ```go
    package main

    import "github.com/second-state/WasmEdge-go/wasmedge"

    func main() {
        // 创建虚拟机。
        vm := wasmedge.NewVM()

        var err error
        err = vm.RegisterWasmFile("module_name", "fibonacci.wasm")
        // 你可以通过 `[]byte` 注册 WASM 模块 `(*VM).RegisterWasmBuffer` 函数，或来自 `AST` 对象 `(*VM).RegisterAST` 函数。应该检查结果状态。如果 WASM 模块实例化失败或模块名称冲突。
        vm.Release()
    }
    ```

2. 执行注册的 WASM 模块中的功能

    编辑 Go 文件 `main.go`，如下所示：

    ```go
    package main

    import (
        "fmt"

        "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
        // 创建虚拟机。
        vm := wasmedge.NewVM()

       var res []interface{}
        var err error
        // 使用模块名称 mod 将 WASM 模块从文件注册到 VM。
        err = vm.RegisterWasmFile("mod", "fibonacci.wasm")
        // 你可以通过 `[]byte` 注册 WASM 模块 `(*VM).RegisterWasmBuffer` 函数，或来自 `AST` 对象 `(*VM).RegisterAST` 函数。
        if err != nil {
            fmt.Println("WASM registration failed:", err.Error())
            return
        }
        // `fibonacci.wasm` 中的函数 `fib` 与模块一起导出名称 `mod`。与 host 函数一样，其他模块可以导入函数 `mod fib`.

        // 在注册模块中执行 WASM 函数。
        // 与函数的执行不同，注册的函数可以在没有 `(*VM).Instantiate` 的情况下调用，因为 WASM 模块是注册时实例化。
        // 你也可以通过这个 API 直接调用 host 函数。
        res, err = vm.ExecuteRegistered("mod", "fib", int32(25))
        if err == nil {
            fmt.Println("Get fibonacci[25]:", res[0].(int32))
        } else {
            fmt.Println("Run failed:", err.Error())
        }

        vm.Release()
    }
    ```

    然后你可以构建并运行：（第 25 个斐波那契数是 121393，基于 0 的索引）

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
    $ go build
    $ ./wasmedge_test
    Get fibonacci[25]: 121393
    ```

### 实例跟踪

有时你可能需要获取 WASM runtime 的实例。`VM` 对象提供 API 来检索实例。

1. Store

   如果在创建 `VM` 对象时没有分配 `Store` 对象，则 `VM` 上下文将分配并拥有一个 `Store`。

    ```go
    vm := wasmedge.NewVM()
    store := vm.GetStore()
    // 对象应该 __NOT__ 通过调用 `(*Store).Release` 被删除。
    vm.Release()
    ```

   你还可以使用 `Store` 对象创建 `VM` 对象。在这种情况下，你应该保证 `Store` 对象不能在 `VM` 对象之前被释放。有关 `Store` API 的详细信息，请参阅 [Store Objects](#Store)。

    ```go
    store := wasmedge.NewStore()
    vm := wasmedge.NewVMWithStore(store)
 
    storemock := vm.GetStore()
    // `store` 和 `storemock` 的内部存储上下文是相同的。

    vm.Release()
    store.Release()
    ```

2. 列出导出的函数

    WASM 模块实例化后，你可以使用 `(*VM).Execute` 函数调用导出的 WASM 函数。为此，你可能需要有关导出的 WASM 函数列表的信息。
    有关函数类型的详细信息，请参阅 [Runtime 实例](#Instances)。

    假设创建了一个新的 Go 项目，如下所示：

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

    然后假设将 WASM 文件 [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) 复制到当前目录，并创建并编辑 Go 文件 `main.go`：

    ```go
    package main

    import (
        "fmt"

        "github.com/second-state/WasmEdge-go/wasmedge"
    )

    func main() {
        // 创建虚拟机。
        vm := wasmedge.NewVM()

        // 第 1 步：加载 WASM 文件。
        err := vm.LoadWasmFile("fibonacci.wasm")
        if err != nil {
            fmt.Println("Load WASM from file FAILED:", err.Error())
            return
        }

        // 第 2 步：验证 WASM 模块。
        err = vm.Validate()
        if err != nil {
            fmt.Println("Validation FAILED:", err.Error())
            return
        }

        // 第 3 步：实例化 WASM 模块。
        err = vm.Instantiate()
        if err != nil {
            fmt.Println("Instantiation FAILED:", err.Error())
            return
        }

        // 列出导出函数的名称和函数类型。
        funcnames, functypes := vm.GetFunctionList()
        for _, fname := range funcnames {
            fmt.Println("Exported function name:", fname)
        }
        for _, ftype := range functypes {
            // `ftype` 是 `funcnames` 数组中相同索引的 `FunctionType` 对象。
            // 你应该 __NOT__ 调用 `ftype.Release()`。
        }

        vm.Release()
    }
    ```

    然后你可以构建并运行：（ `fibonacci.wasm` 中唯一导出的函数是 `fib` ）

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
    $ go build
    $ ./wasmedge_test
    Exported function name: fib
    ```

    如果你想在注册的 WASM 模块中获取导出的函数名，请从 `VM` 对象中获取 `Store` 对象，参考 [Store Contexts](#Store) 的 API 按模块列出注册的函数名称。

3. 获取函数类型

    `VM` 对象提供 API 以通过函数名称查找函数类型。
    有关函数类型的详细信息，请参阅 [Runtime 实例](#Instances)。

    ```go
    // 假设一个 WASM 模块在 `vm` 中实例化，它是一个 `wasmedge.VM` 对象。
    functype := vm.GetFunctionType("fib")
    // 你可以通过
    // `(*VM).GetFunctionTypeRegistered` API，带有函数名和模块名。
    // 如果未找到该函数，这些 API 将返回 `nil`。
    // 你应该__NOT__调用返回对象的`(*FunctionType).Release`函数。
    ```

## WasmEdge Runtime

接下来，我们将一步步的介绍 WasmEdge Runtime。

### WASM 执行示例手把手教程

除了通过 [`VM` 对象](#WasmEdge-VM) 快速执行 WASM 之外，你还可以通过 `Loader`、`Validator`、`Executor` 和 `Store` 执行、初始化 WASM。

假设创建了一个新的 Go 项目，如下所示：

```bash
mkdir wasmedge_test && cd wasmedge_test
go mod init wasmedge_test
```

然后将 WASM 文件 [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) 复制到当前目录，创建并编辑 Go 文件 `main.go`：

```go
package main

import (
    "fmt"

    "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
    // 将日志级别设置为调试以打印统计信息。
    wasmedge.SetLogDebugLevel()
    // 创建配置对象。如果你使用默认配置，则不需要这样做。
    conf := wasmedge.NewConfigure()
    // 打开 runtime 指令计数和时间测量。
    conf.SetStatisticsInstructionCounting(true)
    conf.SetStatisticsTimeMeasuring(true)
    // 创建统计对象。如果不需要 runtime 的统计信息，这不是必需的。
    stat := wasmedge.NewStatistics()
    // 创建存储对象。store 对象是 WASM runtime 结构的核心。
    store := wasmedge.NewStore()

    var err error
    var res []interface{}
    var ast *wasmedge.AST
    // 创建加载器对象。
    // 对于使用默认配置创建加载器，你可以使用 `wasmedge.NewLoader()` 代替。
    loader := wasmedge.NewLoaderWithConfig(conf)
    // 创建验证器对象。
    // 对于使用默认配置创建验证器，你可以使用 `wasmedge.NewValidator()` 代替。
    validator := wasmedge.NewValidatorWithConfig(conf)
    // 创建执行器对象。
    // 对于使用默认配置且没有统计信息的执行器创建，你可以使用 `wasmedge.NewExecutor()` 代替。
    executor := wasmedge.NewExecutorWithConfigAndStatistics(conf, stat)

    // 加载 WASM 文件或编译后的 WASM 文件并转换为 AST 模块对象。
    ast, err = loader.LoadFile("fibonacci.wasm")
    if err != nil {
        fmt.Println("Load WASM from file FAILED:", err.Error())
        return
    }
    // 验证 WASM 模块。
    err = validator.Validate(ast)
    if err != nil {
        fmt.Println("Validation FAILED:", err.Error())
        return
    }
    // 将 WASM 模块实例化为 Store 对象。
    err = executor.Instantiate(store, ast)
    if err != nil {
        fmt.Println("Instantiation FAILED:", err.Error())
        return
    }

    // 尝试列出实例化的 WASM 模块的导出函数。
    funcnames := store.ListFunction()
    for _, fname := range funcnames {
        fmt.Println("Exported function name:", fname)
    }

    // 调用 WASM 函数。
    res, err = executor.Invoke(store, "fib", int32(30))
    if err == nil {
        fmt.Println("Get fibonacci[30]:", res[0].(int32))
    } else {
        fmt.Println("Run failed:", err.Error())
    }

    // 资源释放。
    conf.Release()
    stat.Release()
    ast.Release()
    loader.Release()
    validator.Release()
    executor.Release()
    store.Release()
}
```

然后你可以构建并运行：（从 30 开始的第 18 个斐波那契数是 1346269 ）

```bash
$ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
$ go build
$ ./wasmedge_test
Exported function name: fib
[2021-11-24 18:53:01.451] [debug]  Execution succeeded.
[2021-11-24 18:53:01.452] [debug]
 ====================  Statistics  ====================
 Total execution time: 556372295 ns
 Wasm instructions execution time: 556372295 ns
 Host functions execution time: 0 ns
 Executed wasm instructions count: 28271634
 Gas costs: 0
 Instructions per second: 50814237
Get fibonacci[30]: 1346269
```

### 加载器

`Loader` 对象从文件或缓冲区加载 WASM 二进制文件。
支持 [WasmEdge AOT Compiler](#WasmEdge-AOT-Compiler) 中的 WASM 和已编译的 WASM。

```go
var buf []byte
// ... 将 WASM 代码读取到 `buf`。

// 你可以在配置对象中调整设置。
conf := wasmedge.NewConfigure()
// 创建加载器对象。
// 对于使用默认配置创建加载器，你可以使用 `wasmedge.NewLoader()` 代替。
loader := wasmedge.NewLoaderWithConfig(conf)
conf.Release()

// 从文件中加载 WASM 或已编译的 WASM。
ast, err := loader.LoadFile("fibonacci.wasm")
if err != nil {
    fmt.Println("Load WASM from file FAILED:", err.Error())
} else {
    // 应该释放输出的 AST 对象。
    ast.Release()
}

// 从缓冲区加载 WASM 或编译后的 WASM
ast, err = loader.LoadBuffer(buf)
if err != nil {
    fmt.Println("Load WASM from buffer FAILED:", err.Error())
} else {
    // 应该释放输出的 AST 对象。
    ast.Release()   
}

loader.Release()
```

### 验证器

`Validator` 对象可以验证 WASM 模块。  
每个 WASM 模块都应该在实例化之前进行验证。

```go
// ...
// 假设 `ast` 是加载器上下文的输出 `*wasmedge.AST` 对象。
// 假设 `conf` 是 `*wasmedge.Configure` 对象。

// 创建验证器上下文。
// 对于使用默认配置创建验证器，你可以使用 `wasmedge.NewValidator()` 代替。
validator := wasmedge.NewValidatorWithConfig(conf)

err := validator.Validate(ast)
if err != nil {
    fmt.Println("Validation FAILED:", err.Error())
}

validator.Release()
```

### 执行器

`Executor` 对象是 WASM 和编译型 WASM 的执行器。
这个对象应该基于 `Store` 对象工作。`Store` 对象的详细信息，请参考 [下一章](#Store)。

1. 注册模块

    与 `VM` 对象中的[Host 模块注册](#Host-模块预注册)或[导入 WASM 模块](#WASM-Registrations-And-Executions)相同，你可以注册 `ImportObject` 或 `AST` 对象通过 `Executor` API 进入 `Store` 对象。
    有关导入对象的详细信息，请参阅[Host 函数](#Host-函数)。

    ```go
    // ...
    // 假设 `ast` 是加载器的输出 `*wasmedge.AST` 对象并且通过了验证。
    // 假设 `conf` 是 `*wasmedge.Configure` 对象。

    // 创建统计对象。如果不需要统计数据，则无需此步骤。
    stat := wasmedge.NewStatistics()
    // 创建执行器对象。
    // 使用默认配置创建执行器，没有统计信息，你可以使用 `wasmedge.NewExecutor()` 代替。
    executor := wasmedge.NewExecutorWithConfigAndStatistics(conf, stat)
    // 创建存储对象。store 是 WASM runtime 结构的核心。
    store := wasmedge.NewStore()

    // 使用导出模块名称 mod 将加载的 WASM `ast` 注册到 store 中。
    res := executor.RegisterModule(store, ast, "mod")
    if err != nil {
        fmt.Println("WASM registration FAILED:", err.Error())
        return
    }

    // 假设 `impobj` 是 Host 函数的 `*wasmedge.ImportObject`。
    impobj := ...
    err = executor.RegisterImport(store, impobj)
    if err != nil {
        fmt.Println("Import object registration FAILED:", err.Error())
        return
    }

    executor.Release()
    stat.Release()
    store.Release()
    impobj.Release()
    ```

2. 实例化模块

   WASM 或已编译的 WASM 模块应在函数调用之前实例化。请注意，你只能将一个模块实例化到 `Store` 对象中，在这种情况下，旧的实例化模块将被清除。在实例化 WASM 模块之前，请检查 [import section](https://webassembly.github.io/spec/core/syntax/modules.html#syntax-import) 以确保将导入注册到 `Store` 对象中.

    ```go
    // ...
    // 假设 `ast` 是加载器的输出 `*wasmedge.AST` 对象并且通过了验证。
    // 假设 `conf` 是 `*wasmedge.Configure` 对象。

    // 创建统计对象。如果统计数据，则无需此步骤不需要。
    stat := wasmedge.NewStatistics()
    // 创建执行器对象。
    // 使用默认配置创建执行器，没有统计信息，
    // 你可以使用 `wasmedge.NewExecutor()` 代替。
    executor := wasmedge.NewExecutorWithConfigAndStatistics(conf, stat)
    // 创建存储对象。store 是 WASM Runtime 结构的核心。
    store := wasmedge.NewStore()

    // 实例化 WASM 模块。
    err := executor.Instantiate(stpre, ast)
    if err != nil {
        fmt.Println("WASM instantiation FAILED:", err.Error())
        return
    }

    executor.Release()
    stat.Release()
    store.Release()
    ```

3. 调用函数

    与通过 `VM` 对象调用函数一样，你可以调用实例化或注册的模块的函数。API `(*Executor).Invoke` 和 `(*Executor).InvokeRegistered` 与 `VM` 对象的 API 类似。有关详细信息，请参阅 [VM 上下文工作流](#WASM-Execution-Example-With-VM-Object)。

### AST 模块

`AST` 对象表示从 WASM 文件或缓冲区加载的结构。从 [Loader](#Loader) 加载一个 WASM 文件或缓冲区后会得到这个对象。在实例化之前，你还可以查询 `AST` 对象的导入和导出。

```go
ast := ...
// 假设 WASM 从 loader 加载到 `*wasmedge.AST` 对象中。

// 列出导入。
imports := ast.ListImports()
for _, import := range imports {
    fmt.Println("Import:", import.GetModuleName(), import.GetExternalName())
}

// 列出导出。
exports := ast.ListExports()
for _, export := range exports {
    fmt.Println("Export:", export.GetExternalName())
}


ast.Release()
```

### Store

[Store](https://webassembly.github.io/spec/core/exec/runtime.html#store) 是 runtime 结构，用于表示在抽象机（Abstract Machine）的生命周期分配的所有 `Function`、 `Table`、`Memory` 和 `Global` 实例。
WasmEdge-go 中的 Store 对象提供了 API 来列出导出的实例及其名称或通过导出的名称查找实例。要将实例添加到 `Store` 对象中，请通过 `Executor` API 实例化或注册 WASM 模块或 `ImportObject` 对象。

1. 列出实例

    ```go
    store := wasmedge.NewStore()
    // ...
    // 通过 `*wasmedge.Executor` 对象实例化一个 WASM 模块。
    // ...

    // 尝试列出实例化的 WASM 模块的导出函数。
    // 这里以函数实例为例。
    funcnames := store.ListFunction()
    for _, name := range funcnames {
        fmt.Println("Exported function name:", name)
    }

    store.Release()
    ```

   你可以通过 `(*Store).ListFunctionRegistered()` API 列出已注册模块的函数实例导出名称和模块名称。

2. 查找实例

    ```go
    store := wasmedge.NewStore()
    // ...
    // 通过 `*wasmedge.Executor` 对象实例化一个 WASM 模块。
    // ...

    // 尝试查找实例化的 WASM 模块的导出函数。
    // 这里以函数实例为例。
    funcobj := store.FindFunction("fib")
    // 如果找不到函数，`funcobj` 将为 `nil`。

    store.Release()
    ```

   你可以通过 `(*Store).FindFunctionRegistered` API 获取已注册模块的导出函数实例，并带有模块名称。

3. 列出已注册的模块

    使用模块名称，你可以列出已注册模块的导出实例及其名称。

    ```go
    store := wasmedge.NewStore()
    // ...
    // 通过 `*wasmedge.Executor` 对象实例化一个 WASM 模块。
    // ...

    // 尝试列出已注册的 WASM 模块。
    modnames := store.ListModule()
    for _, name := range modnames {
        fmt.Println("Registered module names:", name)
    }

    store.Release()
    ```

### 实例

实例是 WASM 的 Runtime 结构。你可以从 `Store` 对象中检索实例。当通过 `Executor` 注册或实例化 WASM 模块或 `ImportObject` 时，`Store` 对象将分配实例。单个实例可以通过它的创建函数来分配。你可以将实例构造成一个 `ImportObject` 进行注册。详情请参阅[host 函数](#Host-函数)。由它们的创建函数创建的实例应该被销毁，除非它们被添加到一个 `ImportObject` 对象中。

1. 函数实例

    [host 函数](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) 是 WebAssembly 之外的函数，并作为导入传递给 WASM 模块。在 WasmEdge 中，你可以为 Host 函数创建 `Function` 对象，并将它们添加到 `ImportObject` 对象中，以便注册到 `VM` 或 `Store` 中。对于 Host 函数和从 `Store` 获取的函数，你可以从 `Function` 对象中检索 `FunctionType`。`Host Function` 指南的详细内容请参考 [下一章](#Host-Functions)。

    ```go
    funcinst := ...
    // `funcobj` 是从 store 对象中检索到的 `*wasmedge.Function`。
    functype := funcobj.GetFunctionType()
    // 从 store 对象中检索到的 `funcobj` 应该 __NOT__ 被释放。
    // 从 `funcobj` 检索到的 `functype` 应该 __NOT__ 被释放。
    ```

   1. 表实例

       在 WasmEdge 中，你可以创建 `Table` 对象并将它们添加到 `ImportObject` 对象中，以便注册到 `VM` 或 `Store` 中。
       `Table` 对象提供 API 来控制表实例中的数据。

       ```go
       lim := wasmedge.NewLimitWithMax(10, 20)
       // 创建具有限制和 `FuncRef` 元素类型的表类型。
       tabtype := wasmedge.NewTableType(wasmedge.RefType_FuncRef, lim)
       // 创建具有表类型的表实例。
       tabinst := wasmedge.NewTable(tabtype)
       // 删除表类型。
       tabtype.Release()

       gottabtype := tabinst.GetTableType()
       // 从表实例中得到的 `gottabtype` 归 `tabinst` 所有
       // 并且应该 __NOT__ 被释放。
       reftype := gottabtype.GetRefType()
       // `reftype` 将是 `wasmedge.RefType_FuncRef`。

       var gotdata 接口{}
       data := wasmedge.NewFuncRef(5)
       err := tabinst.SetData(data, 3)
       // 将函数索引 5 设置为 table[3]。

       // 下面的行会得到一个“out of bounds table access”错误，因为位置 (13) 超出了表大小 (10)：
       // err = tabinst.SetData(data, 13)

       gotdata, err = tabinst.GetData(3)
       // 获取 table[3] 的 FuncRef 值。

       // 下面的行会得到一个“out of bounds table access”错误，因为位置 (13) 超出了表大小 (10)：
       // gotdata, err = tabinst.GetData(13)

       tabsize := tabinst.GetSize()
       // `tabsize` 将是 10。
       err = tabinst.Grow(6)
       // 将表大小增加到 6，表大小将是 16。

       // 下面的行会得到一个“out of bounds table access”错误
       // 因为大小 (16 + 6) 将达到表限制 (20):
       // err = tabinst.Grow(6)

       tabinst.Release()
       ```

2. 内存实例

    在 WasmEdge 中，你可以创建 `Memory` 对象并将它们添加到 `ImportObject` 对象中，以便注册到 `VM` 或 `Store` 中。`Memory` 对象提供 API 来控制内存实例中的数据。

    ```go
    lim := wasmedge.NewLimitWithMax(1, 5)
    // 创建有限制的内存类型。内存页大小为 64KiB。
    memtype := wasmedge.NewMemoryType(lim)
    // 创建内存类型的内存实例。
    meminst := wasmedge.NewMemory(memtype)
    // 删除内存类型。
    memtype.Release()

    data := []byte("A quick brown fox jumps over the lazy dog")
    err := meminst.SetData(data, 0x1000, 10)
    // 将数据[0:9]设置为内存[4096:4105]。

    // 下面的行会得到一个“内存访问越界”的错误
    // 因为 [65535:65544] 超出 1 个页面大小 (65536)：
    // err = meminst.SetData(data, 0xFFFF, 10)

    var gotdata []byte
    gotdata, err = meminst.GetData(0x1000, 10)
    // 获取内存[4096:4105]。`gotdata` 将是 `[]byte("A quick br")。
    // 下面的行会得到一个“内存访问越界”的错误，因为 [65535:65544] 超出 1 个页面大小 (65536)：gotdata, err = meminst.Getdata(0xFFFF, 10)

    pagesize := meminst.GetPageSize()
    // `pagesize` 将为 1。
    err = meminst.GrowPage(2)
    // 增加 2 的页面大小，内存实例的页面大小将是 3。

    // 下面的行会得到一个“内存访问越界”的错误，因为大小 (3 + 3) 将达到内存限制 (5): err = meminst.GetPageSize(3)

    meminst.Release()
    ```

3. 全局实例

   在 WasmEdge 中，你可以创建 `Global` 对象并将它们添加到 `ImportObject` 对象中，以便注册到 `VM` 或 `Store` 中。
   `Global` 对象提供 API 来控制全局实例中的值。

    ```go
    // 创建具有值类型和突变的全局类型。
    globtype := wasmedge.NewGlobalType(wasmedge.ValType_I64, wasmedge.ValMut_Var)
    // 创建具有值和全局类型的全局实例。
    globinst := wasmedge.NewGlobal(globtype, uint64(1000))
    // 删除全局类型。
    globtype.Release()

    gotglobtype := globinst.GetGlobalType()
    // 从全局实例获取的 `gotglobtype` 归 `globinst` 所有
    // 并且应该 __NOT__ 被释放。
    valtype := gotglobtype.GetValType()
    // `valtype` 将是 `wasmedge.ValType_I64`。
    valmut := gotglobtype.GetMutability()
    // `valmut` 将是 `wasmedge.ValMut_Var`。

    globinst.SetValue(uint64(888))
    // 将值 u64(888) 设置为全局。
    // 如果值类型不匹配或
    // 全局可变性是 `wasmedge.ValMut_Const`。
    gotval := globinst.GetValue()
    // `gotbal` 将是 `interface{}` 类型为 `uint64` 和
    // 值为 888。

    globinst.Release()
    ```

### Host 函数

[Host 函数](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) 是 WebAssembly 之外的函数，并作为导入传递给 WASM 模块。
在 WasmEdge-go 中，你可以创建 `Function`、`Memory`、`Table` 和 `Global` 对象并将它们添加到 `ImportObject` 对象中，以便注册到 `VM` 或 `Store` 中。

1. Host 功能分配

    你可以定义具有以下函数签名的 Go 函数作为 Host 函数体：

    ```go
    type hostFunctionSignature func(
        data interface{}, mem *Memory, params []interface{}) ([]interface{}, Result)
    ```

    添加 2 个 `i32` 值的 `add` Host 函数示例：

    ```c
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
        // 添加：i32, i32 -> i32
        res := params[0].(int32) + params[1].(int32)

        // 设置返回
        returns := make([]interface{}, 1)
        returns[0] = res

        // 返回
        return returns, wasmedge.Result_Success
    }
    ```

    然后你可以创建带有 Host 函数体和函数类型的 `Function` 对象：

    ```go
    // 创建一个函数类型：{i32, i32} -> {i32}。
    functype := wasmedge.NewFunctionType(
        []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
        []wasmedge.ValType{wasmedge.ValType_I32},
    )

    // 使用函数类型和 Host 函数体创建函数上下文。
    // 第三个参数是指向附加数据的指针。
    // 你要保证数据的生命周期，可以
    // 如果不需要外部数据，则为 `nil`。
    // 如果你不需要成本计量，最后一个参数可以为0。
    func_add := wasmedge.NewFunction(functype, host_add, nil, 0)

    // 如果函数对象未添加到导入对象对象中，则应将其释放。
    func_add.Release()
    functype.Release()
    ```

2. 导入对象

    `ImportObject` 对象包含一个导出模块名称和实例。你可以添加 `Function`、`Memory`、`Table` 和 `Global` 实例及其导出名称。

    ```go
    // Host 函数体定义。
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
        // 添加：i32, i32 -> i32
        res := params[0].(int32) + params[1].(int32)

        // 设置返回
        returns := make([]interface{}, 1)
        returns[0] = res

        // 返回
        return returns, wasmedge.Result_Success
    }

    // 创建模块名称为 `module` 的导入对象。
    impobj := wasmedge.NewImportObject("module")

    // 创建一个函数实例并将其添加到导出名称为 `add` 的导入对象中。
    functype := wasmedge.NewFunctionType(
        []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
        []wasmedge.ValType{wasmedge.ValType_I32},
    )
    hostfunc := wasmedge.NewFunction(functype, host_add, nil, 0)
    functype.Release()
    impobj.AddFunction("add", hostfunc)

    // 创建一个表实例并将其添加到导出名称为 `table` 的导入对象中。
    tabtype := wasmedge.NewTableType(wasmedge.RefType_FuncRef ,wasmedge.NewLimitWithMax(10, 20))
    hosttab := wasmedge.NewTable(tabtype)
    tabtype.Release()
    impobj.AddTable("table", hosttab)

    // 创建一个内存实例并将其添加到导出名称为 `memory` 的导入对象中。
    memtype := wasmedge.NewMemoryType(wasmedge.NewLimitWithMax(1, 2))
    hostmem := wasmedge.NewMemory(memtype)
    memtype.Release()
    impobj.AddMemory("memory", hostmem)

    // 创建一个全局实例并将其添加到导出名称为 `global` 的导入对象中。
    globtype := wasmedge.NewGlobalType(wasmedge.ValType_I32, wasmedge.ValMut_Var)
    hostglob := wasmedge.NewGlobal(globtype, uint32(666))
    globtype.Release()
    impobj.AddGlobal("global", hostglob)

    // 导入对象应该被释放。
    // 你应该 __NOT__ 释放添加到导入对象对象中的实例。
    impobj.Release()
    ```

3. 指定导入对象

    `wasmedge.NewWasiImportObject()` API 可以创建和初始化 `WASI` 导入对象。
    `wasmedge.NewWasmEdgeProcessImportObject()` API 可以创建和初始化 `wasmedge_process` 导入对象。
    你可以创建这些导入对象对象并将它们注册到 `Store` 或 `VM` 对象中，而不是调整 `Configure` 对象中的设置。

    ```go
    wasiobj := wasmedge.NewWasiImportObject(
        os.Args[1:], // 参数
        os.Environ(), // 环境
        []string{".:."}, // 映射预打开
    )
    procobj := wasmedge.NewWasmEdgeProcessImportObject(
        []string{"ls", "echo"}, // 允许的命令
        false, // 不允许所有命令
    )

    // 将 WASI 和 WasmEdge_Process 注册到 VM 对象中。
    vm := wasmedge.NewVM()
    vm.RegisterImport(wasiobj)
    vm.RegisterImport(procobj)

    // ... 执行一些 WASM 函数。

    // 获取 WASI 退出代码。
    exitcode := wasiobj.WasiGetExitCode()
    // 如果 WASI 函数“_start”执行没有错误，则 `exitcode` 将为 0。
    // 否则，它将返回相关的退出代码。

    vm.Release()
    // 应该删除导入对象。
    wasiobj.Release()
    procobj.Release()
    ```

4. 例子

   首先新建一个 Go 项目：

    ```bash
    mkdir wasmedge_test && cd wasmedge_test
    go mod init wasmedge_test
    ```

   假设 WAT 中有一个简单的 WASM，如下所示：

    ```wasm
    (module
      (type $t0 (func (param i32 i32) (result i32)))
      (import "extern" "func-add" (func $f-add (type $t0)))
      (func (export "addTwo") (param i32 i32) (result i32)
        local.get 0
        local.get 1
        call $f-add)
    )
    ```

   创建并编辑 Go 文件 `main.go`，如下所示：

    ```go
    package main

    import (
        "fmt"

        "github.com/second-state/WasmEdge-go/wasmedge"
    )

    // Host 函数体定义。
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
        // 添加：i32, i32 -> i32
        res := params[0].(int32) + params[1].(int32)

        // 设置返回
      returns := make([]interface{}, 1)
        returns[0] = res

        // 返回
        return returns, wasmedge.Result_Success
    }

    func main() {
        // 创建虚拟机对象。
        vm := wasmedge.NewVM()

        // WASM 模块缓冲区。
        wasmbuf := []字节{
            /* WASM 头文件 */
            0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
            /* 类型部分 */
            0x01, 0x07, 0x01,
            /* 函数类型 {i32, i32} -> {i32} */
            0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
            /* 导入部分 */
            0x02, 0x13, 0x01, 
            /* 模块名称：“extern” */
            0x06, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E, 
            /* 外部名称：“func-add” */
            0x08, 0x66, 0x75, 0x6E, 0x63, 0x2D, 0x61, 0x64, 0x64,
            /* 导入描述：函数 0 */
            0x00, 0x00,
            /* 函数部分 */
            0x03, 0x02, 0x01, 0x00,
            /* 导出部分 */
            0x07, 0x0A, 0x01,
            /* 导出名称：`addTwo` */
            0x06, 0x61, 0x64, 0x64, 0x54, 0x77, 0x6F, 
            /* 导出描述：函数 0 */
            0x00, 0x01,
            /* 代码部分 */
            0x0A, 0x0A, 0x01, 
            /* 代码主体 */
            0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B,
        }

        // 创建模块名称为 `extern` 的导入对象。
        impobj := wasmedge.NewImportObject("extern")

        // 创建一个函数实例并将其添加到导出名称为 `func-add` 的导入对象中。
        functype := wasmedge.NewFunctionType(
            []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
            []wasmedge.ValType{wasmedge.ValType_I32},
        )
        hostfunc := wasmedge.NewFunction(functype, host_add, nil, 0)
        functype.Release()
        impobj.AddFunction("func-add", hostfunc)

        // 将导入对象注册到 VM 中。
        vm.RegisterImport(impobj)

        res, err := vm.RunWasmBuffer(wasmbuf, "addTwo", uint32(1234), uint32(5678))
        if err == nil {
            fmt.Println("Get the result:", res[0].(int32))
        } else {
            fmt.Println("Error message:", err.Error())
        }

        impobj.Release()
        vm.Release()
    }
    ```

   然后，你可以使用 WasmEdge Golang SDK 构建和运行 Golang 应用程序：

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
    $ go build
    $ ./wasmedge_test
    得到结果：6912
    ```

5. Host 数据示例

   你可以给函数对象设置一个外部数据对象，并在函数体中访问该对象。假设编辑上面的 Go 文件 `main.go`：

    ```go
    package main

    import (
        "fmt"

        "github.com/second-state/WasmEdge-go/wasmedge"
    )

    // Host 函数体定义。
    func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
        // 添加：i32, i32 -> i32
        res := params[0].(int32) + params[1].(int32)

        // 设置返回
        returns := make([]interface{}, 1)
        returns[0] = res

        // 同时将结果设置为数据。
        *data.(*int32) = res

        // 返回
        return returns, wasmedge.Result_Success
    }

    func main() {
        // 创建虚拟机对象。
        vm := wasmedge.NewVM()

        // WASM 模块缓冲区。
        wasmbuf := []字节{
            /* WASM 头文件 */
            0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
            /* 类型部分 */
            0x01, 0x07, 0x01,
            /* 函数类型 {i32, i32} -> {i32} */
            0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F, 
            /* 导入部分 */
            0x02, 0x13, 0x01, 
            /* 模块名称：“extern” */
            0x06, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E, 
            /* 外部名称：“func-add” */
            0x08, 0x66, 0x75, 0x6E, 0x63, 0x2D, 0x61, 0x64, 0x64,
            /* 导入描述：函数 0 */
            0x00, 0x00,
            /* 函数部分 */
            0x03, 0x02, 0x01, 0x00,
            /* 导出部分 */
            0x07, 0x0A, 0x01,
            /* 导出名称：“addTwo” */
            0x06, 0x61, 0x64, 0x64, 0x54, 0x77, 0x6F, 
            /* 导出描述：函数 0 */
            0x00, 0x01,
            /* 代码部分 */
            0x0A, 0x0A, 0x01, 
            /* 代码主体 */
            0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B,
        }

        // 要设置到 Host 函数中的附加数据。
        var data int32 = 0

        // 创建模块名称为 `extern` 的导入对象。
        impobj := wasmedge.NewImportObject("extern")

        // 创建一个函数实例并将其添加到导出名称为 `func-add` 的导入对象中。
        functype := wasmedge.NewFunctionType(
            []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
            []wasmedge.ValType{wasmedge.ValType_I32},
        )
        hostfunc := wasmedge.NewFunction(functype, host_add, &data, 0)
        functype.Release()
        impobj.AddFunction("func-add", hostfunc)

        // 将导入对象注册到 VM 中。
        res, err := vm.RunWasmBuffer(wasmbuf, "addTwo", uint32(1234), uint32(5678))
        if err == nil {
            fmt.Println("Get the result:", res[0].(int32))
        } else {
            fmt.Println("Error message:", err.Error())
        }
        fmt.Println("Data value:", data)

        impobj.Release()
        vm.Release()
    }
    ```

   然后，你可以使用 WasmEdge Golang SDK 构建和运行 Golang 应用程序：

    ```bash
    $ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
    $ go build
    $ ./wasmedge_test
    Get the result: 6912
    Data value: 6912
    ```

## WasmEdge AOT 编译器

在本部分中，我们将介绍 WasmEdge AOT 编译器和 Go 中的选项。WasmEdge 以解释器模式运行 WASM 文件，并且 WasmEdge 还支持在不修改任何代码的情况下运行 AOT (ahead-of-time) 模式。WasmEdge AOT（提前）编译器编译 WASM 文件以在 AOT 模式下运行，这比解释器模式快得多。你可以将 WASM 文件编译成共享库格式的已编译 WASM 文件，用于 AOT 模式执行的通用 WASM 格式。

### 编译示例

[go_WasmAOT 示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_WasmAOT) 提供了一个编译 WASM 文件的工具。

### 编译器选项

你可以为 AOT 编译器设置优化级别和输出格式等选项：

```go
const (
    // 禁用尽可能多的优化。
    CompilerOptLevel_O0 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O0)
    // 在不破坏可调试性的情况下快速优化。
    CompilerOptLevel_O1 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O1)
    // 在不触发显着增量编译时间或代码大小增长的情况下尽可能地优化快速执行。
    CompilerOptLevel_O2 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O2)
    // 尽可能优化快速执行。
    CompilerOptLevel_O3 = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_O3)
    // 代码越小越好，而不会触发显着的编译时间增量或执行时间减慢。
    CompilerOptLevel_Os = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Os)
    // 代码越小越好。
    CompilerOptLevel_Oz = CompilerOptimizationLevel(C.WasmEdge_CompilerOptimizationLevel_Oz)
)

const (
    // 原生动态库格式。
    CompilerOutputFormat_Native = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Native)
    // WebAssembly 与自定义部分中的 AOT 编译代码。
    CompilerOutputFormat_Wasm = CompilerOutputFormat(C.WasmEdge_CompilerOutputFormat_Wasm)
)
```

详情请参考 [AOT 编译器选项配置](#配置)。
