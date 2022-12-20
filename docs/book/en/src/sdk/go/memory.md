# Pass complex parameters to Wasm functions

An issue with the WebAssembly spec is that it only supports a very limited number of data types. If you want to embed a WebAssembly function with complex call parameters or return values, you will need to manage memory pointers both on Go SDK and WebAssembly function sides.
Such complex call parameters and return values include dynamic memory structures such as strings and byte arrays.
In this section, we will discuss several examples.

* [Pass strings to Rust functions](#pass-strings-to-rust-functions)
* [Pass strings to TinyGo functions](#pass-strings-to-tinygo-functions)
* [Pass bytes to Rust functions](#pass-bytes-to-rust-functions)
* [Pass bytes to TinyGo functions](#pass-bytes-to-tinygo-functions)

## Pass strings to Rust functions

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_MemoryGreet), we will demonstrate how to call [a Rust-based WebAssembly function](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreet/rust_memory_greet/src/lib.rs) from a Go app.
Specially, we will discuss how to pass string data.

> An alternative approach to pass and return complex values to Rust functions in WebAssembly is to use the `wasmedge_bindgen` compiler tool. You can [learn more here](bindgen.md).

The Rust function takes a memory pointer for the string, and constructs the Rust string itself.

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

Use standard Rust compiler tools to compile the Rust source code into a WebAssembly bytecode application.

```bash
cd rust_memory_greet
cargo build --target wasm32-wasi
# The output WASM will be `target/wasm32-wasi/debug/rust_memory_greet_lib.wasm`.
```

The [Go SDK application](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreet/greet_memory.go) must call `allocate` from the WasmEdge VM to get a pointer to the string parameter.
It will then call the `greet` function in Rust with the pointer.
After the function returns, the Go application will call `deallocate` to free the memory space.

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
  vm := wasmedge.NewVMWithConfig(conf)

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

  // Allocate memory for the subject, and get a pointer to it.
  // Include a byte for the NULL terminator we add below.
  allocateResult, _ := vm.Execute("allocate", int32(lengthOfSubject + 1))
  inputPointer := allocateResult[0].(int32)

  // Write the subject into the memory.
  mod := vm.GetActiveModule()
  mem := mod.FindMemory("memory")
  memData, _ := mem.GetData(uint(inputPointer), uint(lengthOfSubject+1))
  copy(memData, subject)

  // C-string terminates by NULL.
  memData[lengthOfSubject] = 0

  // Run the `greet` function. Given the pointer to the subject.
  greetResult, _ := vm.Execute("greet", inputPointer)
  outputPointer := greetResult[0].(int32)

  pageSize := mem.GetPageSize()
  // Read the result of the `greet` function.
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

  // Deallocate the subject, and the output.
  vm.Execute("deallocate", inputPointer, int32(lengthOfSubject+1))
  vm.Execute("deallocate", outputPointer, int32(lengthOfOutput+1))

  vm.Release()
  conf.Release()
}
```

To build the Go SDK example, run the following commands.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v{{ wasmedge_version }}
go build greet_memory.go
```

Now you can use the Go application to run the WebAssembly plug-in compiled from Rust.

```bash
$ ./greet_memory rust_memory_greet_lib.wasm
Hello, WasmEdge!
```

## Pass strings to TinyGo functions

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_MemoryGreetTinyGo), we will demonstrate how to call [a TinyGo-based WebAssembly function](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreetTinyGo/greet.go) from a Go app.

The TinyGo function takes a memory pointer for the string, and constructs the TinyGo string itself.

> The empty `main()` is needed to the compiled WebAssembly program to set up WASI properly.

```go
package main

import (
  "strings"
  "unsafe"
)

func main() {}

//export greet
func greet(subject *int32) *int32 {
  nth := 0
  var subjectStr strings.Builder
  pointer := uintptr(unsafe.Pointer(subject))
  for {
    s := *(*int32)(unsafe.Pointer(pointer + uintptr(nth)))
    if s == 0 {
      break
    }

    subjectStr.WriteByte(byte(s))
    nth++
  }

  output := []byte("Hello, " + subjectStr.String() + "!")

  r := make([]int32, 2)
  r[0] = int32(uintptr(unsafe.Pointer(&(output[0]))))
  r[1] = int32(len(output))

  return &r[0]
}
```

Use the TinyGo compiler tools to compile the Go source code into a WebAssembly bytecode application.

```bash
tinygo build -o greet.wasm -target wasi greet.go
```

The [Go SDK application](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_MemoryGreetTinyGo/greet_memory.go) must call `malloc` from the WasmEdge VM to get a pointer to the string parameter.
It will then call the `greet` function in TinyGo with the pointer.
After the function returns, the Go application will call `free` to free the memory space.

```go
package main

import (
  "fmt"
  "os"
  "encoding/binary"

  "github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
  wasmedge.SetLogErrorLevel()
  conf := wasmedge.NewConfigure(wasmedge.WASI)
  vm := wasmedge.NewVMWithConfig(conf)

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

  // Allocate memory for the subject, and get a pointer to it.
  // Include a byte for the NULL terminator we add below.
  allocateResult, _ := vm.Execute("malloc", int32(lengthOfSubject+1))
  inputPointer := allocateResult[0].(int32)

  // Write the subject into the memory.
  mod := vm.GetActiveModule()
  mem := mod.FindMemory("memory")
  memData, _ := mem.GetData(uint(inputPointer), uint(lengthOfSubject+1))
  copy(memData, subject)

  // C-string terminates by NULL.
  memData[lengthOfSubject] = 0

  // Run the `greet` function. Given the pointer to the subject.
  greetResult, _ := vm.Execute("greet", inputPointer)
  outputPointer := greetResult[0].(int32)

  memData, _ = mem.GetData(uint(outputPointer), 8)
  resultPointer := binary.LittleEndian.Uint32(memData[:4])
  resultLength := binary.LittleEndian.Uint32(memData[4:])

  // Read the result of the `greet` function.
  memData, _ = mem.GetData(uint(resultPointer), uint(resultLength))
  fmt.Println(string(memData))

  // Deallocate the subject, and the output.
  vm.Execute("free", inputPointer)

  vm.Release()
  conf.Release()
}
```

To build the Go SDK example, run the following commands.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v{{ wasmedge_version }}
go build greet_memory.go
```

Now you can use the Go application to run the WebAssembly plug-in compiled from TinyGo.

```bash
$ ./greet_memory greet.wasm
Hello, WasmEdge!
```

## Pass bytes to Rust functions

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_AccessMemory), we will demonstrate how to call [Rust-based WebAssembly functions](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemory/rust_access_memory/src/lib.rs) and pass arrays to and from a Go app.

> An alternative approach to pass and return complex values to Rust functions in WebAssembly is to use the `wasmedge_bindgen` compiler tool. You can [learn more here](bindgen.md).

The `fib_array()` function takes a array as a call parameter and fill it with a fibonacci sequence of numbers. Alternatively, the `fib_array_return_memory()` function returns a array of fibonacci sequence of numbers.

For the array in the call parameter, the Rust function `fib_array()` takes a memory pointer and constructs the Rust `Vec` using `from_raw_parts`. For the array return value, the Rust function `fib_array_return_memory()` simply returns the pointer.

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

Use standard Rust compiler tools to compile the Rust source code into a WebAssembly bytecode application.

```bash
cd rust_access_memory
cargo build --target wasm32-wasi
# The output WASM will be target/wasm32-wasi/debug/rust_access_memory_lib.wasm.
```

The [Go SDK application](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemory/run.go) must call `allocate` from the WasmEdge VM to get a pointer to the array.
It will then call the `fib_array()` function in Rust and pass in the pointer.
After the functions return, the Go application will use the WasmEdge `store` API to construct an array from the pointer in the call parameter (`fib_array()`) or in the return value (`fib_array_return_memory()`).
The Go app will eventually call `deallocate` to free the memory space.

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
  vm := wasmedge.NewVMWithConfig(conf)

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
    mod := vm.GetActiveModule()
    mem := mod.FindMemory("memory")
    if mem != nil {
      // int32 occupies 4 bytes
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
    mod := vm.GetActiveModule()
    mem := mod.FindMemory("memory")
    if mem != nil {
      // int32 occupies 4 bytes
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
  conf.Release()
}
```

To build the Go SDK example, run the following commands.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v{{ wasmedge_version }}
go build run.go
```

Now you can use the Go application to run the WebAssembly plug-in compiled from Rust.

```bash
$ ./run rust_access_memory_lib.wasm
fib_array() returned: 34
fib_array memory at: 0x102d80
fibArray: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
fib_array_return_memory memory at: 0x105430
fibArrayReturnMemory: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
```

## Pass bytes to TinyGo functions

In [this example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_AccessMemoryTinyGo), we will demonstrate how to call [TinyGo-based WebAssembly functions](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemoryTinyGo/fib.go) and pass arrays to and from a Go app.

The `fibArray` function takes a array as a call parameter and fill it with
a fibonacci sequence of numbers. Alternatively, the `fibArrayReturnMemory` function returns
a array of fibonacci sequence of numbers.

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

// export fibArray
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

// export fibArrayReturnMemory
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

Use the TinyGo compiler tools to compile the Go source code into a WebAssembly bytecode application.

```bash
tinygo build -o fib.wasm -target wasi fib.go
```

The [Go SDK application](https://github.com/second-state/WasmEdge-go-examples/blob/master/go_AccessMemoryTinyGo/run.go) must call `malloc` from the WasmEdge VM to get a pointer to the array.
It will then call the `fibArray()` function in TinyGo with the pointer.
After the functions return, the Go app uses the WasmEdge SDK's `store` API to construct an array from the pointer in the call parameter (`fibArray()`) or in the return value (`fibArrayReturnMemory()`).
The Go application will eventually call `free` to free the memory space.

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
  vm := wasmedge.NewVMWithConfig(conf)

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
    mod := vm.GetActiveModule()
    mem := mod.FindMemory("memory")
    if mem != nil {
      // int32 occupies 4 bytes
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
    mod := vm.GetActiveModule()
    mem := mod.FindMemory("memory")
    if mem != nil {
      // int32 occupies 4 bytes
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
  conf.Release()
}
```

To build the Go SDK example, run the following commands.

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v{{ wasmedge_version }}
go build run.go
```

Now you can use the Go application to run the WebAssembly plug-in compiled from TinyGo.

```bash
$ ./run fib.wasm
fibArray() returned: 34
fibArray memory at: 0x14d3c
fibArray: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
fibArrayReturnMemory memory at: 0x14d4c
fibArrayReturnMemory: [0 0 0 0 1 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 5 0 0 0 8 0 0 0 13 0 0 0 21 0 0 0 34 0 0 0]
```
