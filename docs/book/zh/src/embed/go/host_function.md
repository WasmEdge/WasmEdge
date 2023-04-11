# 如何用 Go 编写 Host Function

**This file is needed to update to 0.10.0 and translate into English.**

## 什么是 Host Function

顾名思义, Host Function 就是定义在 Host 程序中的函数. 对于 Wasm 来说, Host Function 可以做为导入段 `import` 被注册到一个模块 `module` 中, 之后便可以在 Wasm 运行时被调用.

Wasm 目前的能力有限，但那些 Wasm 本身做不了的事情, 都可以依靠 Host Function 来解决, 这极大地扩展了 Wasm 的能力范围.

[WasmEdge](https://github.com/WasmEdge/WasmEdge) 在标准之外做的扩展基本都是依赖 Host Function 做的的，比如，[WasmEdge](https://wasmedge.org/book/en/dev/rust/tensorflow.html) 提供的 [Tensorflow API](https://github.com/second-state/wasmedge_tensorflow_interface), 是使用 Host Function 实现的，也因此实现了以原生速度运行 AI 推理的目标。

Networking socket 也是使用 host function 实现的，因此我们可以在 [WasmEdge 运行异步 HTTP 客户端和服务器](https://wasmedge.org/book/en/dev/rust/networking-nonblocking.html)，弥补了 WebAssembly 在网络上的不足。

再比如 [Fastly](https://www.fastly.com/blog/edge-programming-rust-web-assembly) 使用 Host Function 为 Wasm 增加了 Http Request 和 Key-value store 等接口, 进而增添了扩展功能。

## 如何编写简单的 Host Function

让我们从一个最简单的例子入手, 来看看如何在一个 Go 程序里编写 Host function。

先来编写一个简单的 rust 程序。国际惯例，`Cargo.toml` 不能少。

`Cargo.toml`:

```toml
[package]
name = "rust_host_func"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib", "rlib"]

[dependencies]
```

再来看看 Rust 代码是什么样的。

`lib.rs`:

```rust
extern "C" {
  fn add(a: i32, b: i32) -> i32;
}

#[no_mangle]
pub unsafe extern fn run() -> i32 {
  add(1, 2)
}
```

上述程序中的 `add` 函数被声明在 `extern "C"` 中, 这就是一个 Host Function。我们使用如下命令将这段 Rust 程序编译为 wasm:

```bash
cargo build --target wasm32-wasi --release
```

然后我们使用 `wasm2wat` 来查看 wasm 文件的导入段:

```bash
wasm2wat target/wasm32-wasi/release/rust_host_func.wasm | grep import
```

输出如下:

```bash
  (import "env" "add" (func $add (type 0)))
```

可以看到 `add` 函数被放到了默认名称为 `env` 的模块的导入段中.

接下来我们来看如何使用 [WasmEdge-go](https://github.com/second-state/WasmEdge-go) SDK 来执行这段 wasm 程序.

`hostfunc.go`:

```go
package main

import (
  "fmt"
  "os"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

func add(_ interface{}, _ *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
  // 将从 wasm 传过来的两个参数做加法运算
  return []interface{}{params[0].(int32) + params[1].(int32)}, wasmedge.Result_Success
}

func main() {
  vm := wasmedge.NewVM()
 
  // 使用默认名称 env 构建导入段对象
  obj := wasmedge.NewImportObject("env")

  // 构建 Host Function 的参数和返回值类型
  funcAddType := wasmedge.NewFunctionType(
    []wasmedge.ValType{
      wasmedge.ValType_I32,
      wasmedge.ValType_I32,
    },
    []wasmedge.ValType{
      wasmedge.ValType_I32,
    })
  hostAdd := wasmedge.NewFunction(funcAddType, add, nil, 0)
 
  // 将 Host Function 加入到导入段对象中
  // 注意第一个参数 `add` 是 rust 中定义的外部函数的名称
  obj.AddFunction("add", hostAdd)

  // 注册导入段对象
  vm.RegisterImport(obj)

  // 加载, 验证并实例化 wasm 程序
  vm.LoadWasmFile(os.Args[1])
  vm.Validate()
  vm.Instantiate()

  // 执行 wasm 导出的函数并取得返回值
  r, _ := vm.Execute("run")
  fmt.Printf("%d", r[0].(int32))

  obj.Release()
  vm.Release()
}

```

编译并执行:

```bash
go build
./hostfunc rust_host_func.wasm
```

程序输出 `3` 。

这样我们就完成了一个最简单的在 Host 中定义 Function, 并在 wasm 中调用的例子。

下面让我们尝试用 Host Function 做一些更有趣的事情.

## 传递复杂类型

受 Wasm 里数据类型的制约, Host Function 只能传递如 int32 等少数几种基本类型的数据, 这就会大大限制 Host Function 的应用范围. 那有没有什么办法能让我们传递如 string 等复杂数据类型的数据呢？答案是当然可以， 下面我们就通过一个例子看看是如何做到的。

在这个例子中, 我们要统计 `https://www.google.com` 的网页源代码中 `google` 出现的次数。
例子的源代码在[这里](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_HostFunc).

还是先上 Rust 代码。`Cargo.toml` 是必不可少的，只是我在这里省略了。

`lib.rs`:

```rust
extern "C" {
  fn fetch(url_pointer: *const u8, url_length: i32) -> i32;
  fn write_mem(pointer: *const u8);
}

#[no_mangle]
pub unsafe extern fn run() -> i32 {
  let url = "https://www.google.com";
  let pointer = url.as_bytes().as_ptr();

  // call host function to fetch the source code, return the result length
  let res_len = fetch(pointer, url.len() as i32) as usize;

  // malloc memory
  let mut buffer = Vec::with_capacity(res_len);
  let pointer = buffer.as_mut_ptr();

  // call host function to write source code to the memory
  write_mem(pointer);

  // find occurrences from source code
  buffer.set_len(res_len);
  let str = std::str::from_utf8(&buffer).unwrap();
  str.matches("google").count() as i32
}
```

在这段代码中, 引入了两个 Host Function:

* `fetch` 用于发送 http 请求以获取网页源代码
* `write_mem` 用于把网页源代码写到 wasm 的内存

你可能已经看出来了, 要在 Host Function 里传递 string, 实际是通过传递这段 string 所在内存指针和长度来实现的. `fetch` 接收两个参数, 他们就分别是字符串 `https://www.google.com` 的指针和字节长度.

`fetch` 在获取到源代码后, 将源码的字节长度做为返回值返回。Rust 在分配了此长度的内存后, 将内存指针传递给 `write_mem`, host 将源码写入到这段内存, 进而达到了返回 string 的目的.

编译的过程同上不再赘述, 接下来展示如何使用 [WasmEdge-go](https://github.com/second-state/WasmEdge-go) SDK 来执行这段 Wasm 程序。

`hostfun.go`:

```go
package main

import (
  "fmt"
  "io"
  "os"
  "net/http"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

type host struct {
  fetchResult []byte
}

// do the http fetch
func fetch(url string) []byte {
  resp, err := http.Get(string(url))
  if err != nil {
    return nil
  }
  defer resp.Body.Close()
  body, err := io.ReadAll(resp.Body)
  if err != nil {
    return nil
  }

  return body
}

// Host function for fetching
func (h *host) fetch(_ interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
  // get url from memory
  pointer := params[0].(int32)
  size := params[1].(int32)
  data, _ := mem.GetData(uint(pointer), uint(size))
  url := make([]byte, size)

  copy(url, data)

  respBody := fetch(string(url))

  if respBody == nil {
    return nil, wasmedge.Result_Fail
  }

  // store the source code
  h.fetchResult = respBody

  return []interface{}{len(respBody)}, wasmedge.Result_Success
}

// Host function for writing memory
func (h *host) writeMem(_ interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
  // write source code to memory
  pointer := params[0].(int32)
  mem.SetData(h.fetchResult, uint(pointer), uint(len(h.fetchResult)))

  return nil, wasmedge.Result_Success
}

func main() {
  conf := wasmedge.NewConfigure(wasmedge.WASI)
  vm := wasmedge.NewVMWithConfig(conf)
  obj := wasmedge.NewImportObject("env")

  h := host{}
  // Add host functions into the import object
  funcFetchType := wasmedge.NewFunctionType(
    []wasmedge.ValType{
      wasmedge.ValType_I32,
      wasmedge.ValType_I32,
    },
    []wasmedge.ValType{
      wasmedge.ValType_I32,
    })

  hostFetch := wasmedge.NewFunction(funcFetchType, h.fetch, nil, 0)
  obj.AddFunction("fetch", hostFetch)

  funcWriteType := wasmedge.NewFunctionType(
    []wasmedge.ValType{
      wasmedge.ValType_I32
    },
    []wasmedge.ValType{})
  hostWrite := wasmedge.NewFunction(funcWriteType, h.writeMem, nil, 0)
  obj.AddFunction("write_mem", hostWrite)

  vm.RegisterImport(obj)

  vm.LoadWasmFile(os.Args[1])
  vm.Validate()
  vm.Instantiate()

  r, _ := vm.Execute("run")
  fmt.Printf("There are %d 'google' in source code of google.com\n", r[0])

  obj.Release()
  vm.Release()
  conf.Release()
}

```

有了对 Rust 代码的理解, 这段 go 代码其实就很容易理解了。 比较关键的就是对 Wasm 内存的存取:

* `mem.GetData(uint(pointer), uint(size))` 取得 Wasm 中网页的 url
* `mem.SetData(h.fetchResult, uint(pointer), uint(len(h.fetchResult)))` 将网页源码写入 wasm 内存

这个例子的编译执行步骤和前一个例子一模一样, 最后执行的结果是:

`There are 79 'google' in source code of google.com`

## 结语

通过以上两个例子的抛砖引玉, 相信你已经对 Host Function 有了一个初步印象。
虽然因为 Wasm 的诸多限制, 在开发体验上还不太理想, 但随着我们对工具及库的不断完善, 将会为 Wasm 的应用场景带来无尽可能。

欢迎持续关注 [WasmEdge](https://wasmedge.org) 项目, 谢谢。
