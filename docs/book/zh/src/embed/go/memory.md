# 将复杂参数传递给 Wasm 函数

WebAssembly 规范的一个问题是它只支持非常有限的数据类型。如果要嵌入具有复杂调用参数或返回值的 WebAssembly 函数，则需要同时在 Go SDK 和 WebAssembly 函数端管理内存指针。此类复杂的调用参数和返回值包括字符串和字节数组等动态内存结构。在本节中，我们将讨论几个示例：

- [将字符串传递给 Rust 函数](#将字符串传递给-rust-函数)
- [将字符串传递给 TinyGo 函数](#将字符串传递给-tinygo-函数)
- [将字节传递给 Rust 函数](#将字节传递给-rust-函数)
- [将字节传递给 TinyGo 函数](#将字节传递给-tinygo-函数)

## 将字符串传递给 Rust 函数

在[此示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_MemoryGreet)中，我们将演示如何从 Go 应用程序调用[基于 Rust 的 WebAssembly 函数](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreet/rust_memory_greet/src/lib.rs)。特别是，我们将讨论如何传递字符串数据。

> 在 WebAssembly 中向 Rust 函数传递和返回复杂值的另一种方法是使用 `wasm-bindgen` 编译器工具。你可以在[这里](bindgen.md)了解更多。

Rust 函数获取字符串的内存指针，并自己构造 Rust 字符串。

```rust
use std::ffi::{CStr, CString};
use std::mem;
use std::os::raw::{c_char, c_void};

#[no_mangle]
pub extern fn allocate(size: usize) -> *mut c_void {
  let mut buffer = Vec::with_capacity(size);
  let pointer = buffer.as_mut_ptr();
  mem::forget(buffer);

  pointer as *mut c_void
}

#[no_mangle]
pub extern fn deallocate(pointer: *mut c_void, capacity: usize) {
  unsafe {
    let _ = Vec::from_raw_parts(pointer, 0, capacity);
  }
}

#[no_mangle]
pub extern fn greet(subject: *mut c_char) -> *mut c_char {
  let subject = unsafe { CStr::from_ptr(subject).to_bytes().to_vec() };
  let mut output = b"Hello, ".to_vec();
  output.extend(&subject);
  output.extend(&[b'!']);

  unsafe { CString::from_vec_unchecked(output) }.into_raw()
}
```

使用标准 Rust 编译器工具将 Rust 源代码编译成 WebAssembly 字节码应用程序：

```bash
cd rust_memory_greet
cargo build --target wasm32-wasi
# 输出的 WASM 将是 `target/wasm32-wasi/debug/rust_memory_greet_lib.wasm`.
```

[Go SDK 应用程序](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreet/greet_memory.go)必须从 WasmEdge 虚拟机中调用 `allocate` 以获得一个指向字符串参数的指针。然后它将用这个指针调用 Rust 中的 `greet` 函数。在该函数返回后，Go 应用程序将调用 `deallocate` 来释放内存空间。

```go
package main

import (
  "fmt"
  "os"
  "strings"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  wasmedge.SetLogErrorLevel()
  conf := wasmedge.NewConfigure(wasmedge.WASI)
  store := wasmedge.NewStore()
  vm := wasmedge.NewVMWithConfigAndStore(conf, store)

  wasi := vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],
    os.Environ(),
    []string{".:."},
  )

  err := vm.LoadWasmFile(os.Args[1])
  if err != nil {
    fmt.Println("failed to load wasm")
  }
  vm.Validate()
  vm.Instantiate()

  subject := "WasmEdge"
  lengthOfSubject := len(subject)

  // 为 subject 分配内存，并获得其指针
  // 包括一个字节，用于我们在下面添加的 NULL 结束符
  allocateResult, _ := vm.Execute("allocate", int32(lengthOfSubject+1))
  inputPointer := allocateResult[0].(int32)

  // 将 subject 写入内存
  mem := store.FindMemory("memory")
  memData, _ := mem.GetData(uint(inputPointer), uint(lengthOfSubject+1))
  copy(memData, subject)

  // C-字符串，以 NULL 结束
  memData[lengthOfSubject] = 0

  // 运行 `greet` 函数。给出指向 subject 的指针
  greetResult, _ := vm.Execute("greet", inputPointer)
  outputPointer := greetResult[0].(int32)

  pageSize := mem.GetPageSize()
  // 读取 `greet` 函数的结果
  memData, _ = mem.GetData(uint(0), uint(pageSize * 65536))
  nth := 0
  var output strings.Builder

  for {
    if memData[int(outputPointer) + nth] == 0 {
      break
    }

    output.WriteByte(memData[int(outputPointer) + nth])
    nth++
  }

  lengthOfOutput := nth

  fmt.Println(output.String())

  // 释放 subject 以及 output
  vm.Execute("deallocate", inputPointer, int32(lengthOfSubject+1))
  vm.Execute("deallocate", outputPointer, int32(lengthOfOutput+1))

  vm.Release()
  store.Release()
  conf.Release()
}
```

要构建 Go SDK 示例，请运行以下命令：

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0
go build greet_memory.go
```

现在你可以使用 Go 应用程序来运行从 Rust 编译的 WebAssembly 插件：

```bash
$ ./greet_memory rust_memory_greet_lib.wasm
Hello, WasmEdge!
```

## 将字符串传递给 TinyGo 函数

在[此示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_MemoryGreetTinyGo)中，我们将演示如何从 Go 应用程序调用[基于 TinyGo 的 WebAssembly 函数](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreetTinyGo/greet.go)。

TinyGo 函数获取字符串的内存指针，并自己构造 TinyGo 字符串。

> 编译 WebAssembly 程序需要 `main()` 为空，才能正确设置 WASI。

```go
package main

import (
  "strings"
  "unsafe"
)

func main() {}

// 导出 greet
func greet(subject *int32) *byte {
  nth := 0
  var subjectStr strings.Builder
  pointer := uintptr(unsafe.Pointer(subject))
  for {
    s := *(*int32)(unsafe.Pointer(pointer + uintptr(nth)))
    if s == 0 {
      break;
    }

    subjectStr.WriteByte(byte(s))
    nth++
  }

  output := subjectStr.String()
  output = "Hello, " + output + "!"

  return &(([]byte)(output)[0])
}
```

使用 TinyGo 编译器工具将 Go 源代码编译成 WebAssembly 字节码应用程序：

```bash
tinygo build -o greet.wasm -target wasi greet.go
```

[Go SDK 应用程序](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreetTinyGo/greet_memory.go)必须从 WasmEdge 虚拟机中调用 `malloc` 以获得一个指向字符串参数的指针。然后它将用这个指针调用 TinyGo 中的 `greet` 函数。在该函数返回后，Go 应用程序将调用 `free` 来释放内存空间。

```go
package main

import (
  "fmt"
  "os"
  "strings"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  wasmedge.SetLogErrorLevel()
  conf := wasmedge.NewConfigure(wasmedge.WASI)
  store := wasmedge.NewStore()
  vm := wasmedge.NewVMWithConfigAndStore(conf, store)

  wasi := vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],
    os.Environ(),
    []string{".:."},
  )

  err := vm.LoadWasmFile(os.Args[1])
  if err != nil {
    fmt.Println("failed to load wasm")
  }
  vm.Validate()
  vm.Instantiate()

  subject := "WasmEdge"
  lengthOfSubject := len(subject)

  // 为 subject 分配内存，并获得其指针
  // 包括一个字节，用于我们在下面添加的 NULL 结束符
  allocateResult, _ := vm.Execute("malloc", int32(lengthOfSubject+1))
  inputPointer := allocateResult[0].(int32)

  // 将 subject 写入内存
  mem := store.FindMemory("memory")
  memData, _ := mem.GetData(uint(inputPointer), uint(lengthOfSubject+1))
  copy(memData, subject)

  // C-字符串，以 NULL 结束
  memData[lengthOfSubject] = 0

  // 运行 `greet` 函数。给出指向 subject 的指针
  greetResult, _ := vm.Execute("greet", inputPointer)
  outputPointer := greetResult[0].(int32)

  pageSize := mem.GetPageSize()
  // 读取 `greet` 函数的结果
  memData, _ = mem.GetData(uint(0), uint(pageSize * 65536))
  nth := 0
  var output strings.Builder

  for {
    if memData[int(outputPointer) + nth] == 0 {
      break
    }

    output.WriteByte(memData[int(outputPointer) + nth])
    nth++
  }

  fmt.Println(output.String())

  // 释放 subject 以及 output
  vm.Execute("free", inputPointer)
  vm.Execute("free", outputPointer)

  vm.Release()
  store.Release()
  conf.Release()
}
```

要构建 Go SDK 示例，请运行以下命令：

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0
go build greet_memory.go
```

现在你可以使用 Go 应用程序运行从 TinyGo 编译的 WebAssembly 插件：

```bash
$ ./greet_memory greet.wasm
Hello, WasmEdge!
```

## 将字节传递给 Rust 函数

在[此示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_AccessMemory)中，我们将演示如何调用[基于 Rust 的 WebAssembly 函数](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemory/rust_access_memory/src/lib.rs)，并在 Go 应用程序中传入和传出数组。

> 在 WebAssembly 中向 Rust 函数传递和返回复杂值的另一种方法是使用 `wasm-bindgen` 编译器工具。你可以在[这里](bindgen.md)了解更多。

`fib_array()` 函数将一个数组作为调用参数，并用斐波那契数列填充它。或者，`fib_array_return_memory()` 函数返回一个斐波那契数列数组。

对于调用参数中的数组，Rust 函数 `fib_array()` 需要一个内存指针并使用 `from_raw_parts` 构造 Rust `Vec`。对于数组的返回值，Rust 函数 `fib_array_return_memory()` 只是返回指针。

```rust
use std::mem;
use std::os::raw::{c_void, c_int};

#[no_mangle]
pub extern fn allocate(size: usize) -> *mut c_void {
  let mut buffer = Vec::with_capacity(size);
  let pointer = buffer.as_mut_ptr();
  mem::forget(buffer);

  pointer as *mut c_void
}

#[no_mangle]
pub extern fn deallocate(pointer: *mut c_void, capacity: usize) {
  unsafe {
    let _ = Vec::from_raw_parts(pointer, 0, capacity);
  }
}

#[no_mangle]
pub extern fn fib_array(n: i32, p: *mut c_int) -> i32 {
  unsafe {
    let mut arr = Vec::<i32>::from_raw_parts(p, 0, (4*n) as usize);
    for i in 0..n {
      if i < 2 {
        arr.push(i);
      } else {
        arr.push(arr[(i - 1) as usize] + arr[(i - 2) as usize]);
      }
    }
    let r = arr[(n - 1) as usize];
    mem::forget(arr);
    r
  }
}

#[no_mangle]
pub extern fn fib_array_return_memory(n: i32) -> *mut c_int {
  let mut arr = Vec::with_capacity((4 * n) as usize);
  let pointer = arr.as_mut_ptr();
  for i in 0..n {
    if i < 2 {
      arr.push(i);
    } else {
      arr.push(arr[(i - 1) as usize] + arr[(i - 2) as usize]);
    }
  }
  mem::forget(arr);
  pointer
}
```

使用标准 Rust 编译器工具将 Rust 源代码编译成 WebAssembly 字节码应用程序：

```bash
cd rust_access_memory
cargo build --target wasm32-wasi
# 输出的 WASM 将是 target/wasm32-wasi/debug/rust_access_memory_lib.wasm.
```

[Go SDK 应用程序](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemory/run.go)必须从 WasmEdge 虚拟机中调用 `allocate` ，以获得一个指向数组的指针。然后它将调用 Rust 中的`fib_array()`函数并传入指针。在函数返回后，Go 应用程序将使用 WasmEdge 的 `store` API，从调用参数（ `fib_array()` ）或返回值（ `fib_array_return_memory()` ）中的指针构建一个数组。Go 应用最终会调用 `deallocate` 来释放内存空间。

```go
package main

import (
  "fmt"
  "os"
  "unsafe"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  wasmedge.SetLogErrorLevel()
  conf := wasmedge.NewConfigure(wasmedge.WASI)
  store := wasmedge.NewStore()
  vm := wasmedge.NewVMWithConfigAndStore(conf, store)

  wasi := vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],
    os.Environ(),
    []string{".:."},
  )

  err := vm.LoadWasmFile(os.Args[1])
  if err != nil {
    fmt.Println("failed to load wasm")
  }
  vm.Validate()
  vm.Instantiate()

  n := int32(10)

  p, err := vm.Execute("allocate", 4 * n)
  if err != nil {
    fmt.Println("allocate failed:", err)
  }

  fib, err := vm.Execute("fib_array", n, p[0])
  if err != nil {
    fmt.Println("fib_rray failed:", err)
  } else {
    fmt.Println("fib_array() returned:", fib[0])
    fmt.Printf("fib_array memory at: %p\n", unsafe.Pointer((uintptr)(p[0].(int32))))
    mem := store.FindMemory("memory")
    if mem != nil {
      // int32 占用 4 个字节
      fibArray, err := mem.GetData(uint(p[0].(int32)), uint(n * 4))
      if err == nil && fibArray != nil {
        fmt.Println("fibArray:", fibArray)
      }
    }
  }

  fibP, err := vm.Execute("fib_array_return_memory", n)
  if err != nil {
    fmt.Println("fib_array_return_memory failed:", err)
  } else {
    fmt.Printf("fib_array_return_memory memory at: %p\n", unsafe.Pointer((uintptr)(fibP[0].(int32))))
    mem := store.FindMemory("memory")
    if mem != nil {
      // int32 占用 4 个字节
      fibArrayReturnMemory, err := mem.GetData(uint(fibP[0].(int32)), uint(n * 4))
      if err == nil && fibArrayReturnMemory != nil {
        fmt.Println("fibArrayReturnMemory:", fibArrayReturnMemory)
      }
    }
  }

  _, err = vm.Execute("deallocate", p[0].(int32), 4 * n)
  if err != nil {
    fmt.Println("free failed:", err)
  }


  exitcode := wasi.WasiGetExitCode()
  if exitcode != 0 {
    fmt.Println("Go: Running wasm failed, exit code:", exitcode)
  }

  vm.Release()
  store.Release()
  conf.Release()
}
```

为了构建 Go SDK 示例，请运行以下命令：

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0
go build run.go
```

现在你可以使用 Go 应用程序运行从 Rust 编译的 WebAssembly 插件：

```bash
$ ./run rust_access_memory_lib.wasm
fib_array() returned: 34
fib_array memory at: 0x102d80
fibArray: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
fib_array_return_memory memory at: 0x105430
fibArrayReturnMemory: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
```

## 将字节传递给 TinyGo 函数

在[此示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_AccessMemoryTinyGo)中，我们将演示如何调用[基于 TinyGo 的 WebAssembly 函数](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemoryTinyGo/fib.go)以及将数组传入和传出 Go 应用程序。

函数 `fibArray` 接收一个数组作为调用参数，并将其填入一个斐波那契数列。或者，`fibArrayReturnMemory` 函数返回斐波那契数列的数组。

```go
package main

import (
  "fmt"
  "unsafe"
)

func main() {
  println("in main")
  n := int32(10)
  arr := make([]int32, n)
  arrP := &arr[0]
  fmt.Printf("call fibArray(%d, %p) = %d\n", n, arrP, fibArray(n, arrP))
  fmt.Printf("call fibArrayReturnMemory(%d) return %p\n", n, fibArrayReturnMemory(n))
}

// 导出 fibArray
func fibArray(n int32, p *int32) int32 {
  arr := unsafe.Slice(p, n)
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

// 导出 fibArrayReturnMemory
func fibArrayReturnMemory(n int32) *int32 {
  arr := make([]int32, n)
  for i := int32(0); i < n; i++ {
    switch {
    case i < 2:
      arr[i] = i
    default:
      arr[i] = arr[i-1] + arr[i-2]
    }
  }
  return &arr[0]
}
```

使用 TinyGo 编译器工具将 Go 源代码编译成 WebAssembly 字节码应用程序：

```bash
tinygo build -o fib.wasm -target wasi fib.go
```

[Go SDK 应用程序](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemoryTinyGo/run.go) 必须从 WasmEdge 虚拟机中调用 `malloc` 以获得指向数组的指针。然后它将用这个指针调用TinyGo中的 `fibArray()` 函数。在函数返回后，Go应用程序使用 WasmEdge SDK 的 `store` API，从调用参数（ `fibArray()` ）或返回值（ `fibArrayReturnMemory()` ）中的指针构建一个数组。Go应用程序最终会调用 `free` 来释放内存空间。

```go
package main

import (
  "fmt"
  "os"
  "unsafe"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  wasmedge.SetLogErrorLevel()
  conf := wasmedge.NewConfigure(wasmedge.WASI)
  store := wasmedge.NewStore()
  vm := wasmedge.NewVMWithConfigAndStore(conf, store)

  wasi := vm.GetImportModule(wasmedge.WASI)
  wasi.InitWasi(
    os.Args[1:],
    os.Environ(),
    []string{".:."},
  )

  err := vm.LoadWasmFile(os.Args[1])
  if err != nil {
    fmt.Println("failed to load wasm")
  }
  vm.Validate()
  vm.Instantiate()

  n := int32(10)

  p, err := vm.Execute("malloc", n)
  if err != nil {
    fmt.Println("malloc failed:", err)
  }

  fib, err := vm.Execute("fibArray", n, p[0])
  if err != nil {
    fmt.Println("fibArray failed:", err)
  } else {
    fmt.Println("fibArray() returned:", fib[0])
    fmt.Printf("fibArray memory at: %p\n", unsafe.Pointer((uintptr)(p[0].(int32))))
    mem := store.FindMemory("memory")
    if mem != nil {
      // int32 占用 4 个字节
      fibArray, err := mem.GetData(uint(p[0].(int32)), uint(n * 4))
      if err == nil && fibArray != nil {
        fmt.Println("fibArray:", fibArray)
      }
    }
  }

  fibP, err := vm.Execute("fibArrayReturnMemory", n)
  if err != nil {
    fmt.Println("fibArrayReturnMemory failed:", err)
  } else {
    fmt.Printf("fibArrayReturnMemory memory at: %p\n", unsafe.Pointer((uintptr)(fibP[0].(int32))))
    mem := store.FindMemory("memory")
    if mem != nil {
      // int32 占用 4 个字节
      fibArrayReturnMemory, err := mem.GetData(uint(fibP[0].(int32)), uint(n * 4))
      if err == nil && fibArrayReturnMemory != nil {
        fmt.Println("fibArrayReturnMemory:", fibArrayReturnMemory)
      }
    }
  }

  _, err = vm.Execute("free", p...)
  if err != nil {
    fmt.Println("free failed:", err)
  }

  exitcode := wasi.WasiGetExitCode()
  if exitcode != 0 {
    fmt.Println("Go: Running wasm failed, exit code:", exitcode)
  }

  vm.Release()
  store.Release()
  conf.Release()
}
```

要构建 Go SDK 示例，请运行以下命令：

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0
go build run.go
```

现在你可以使用 Go 应用程序运行从 TinyGo 编译的 WebAssembly 插件：

```bash
$ ./run fib.wasm
fibArray() returned: 34
fibArray memory at: 0x14d3c
fibArray: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
fibArrayReturnMemory memory at: 0x14d4c
fibArrayReturnMemory: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
```
