# Customized External References

[External References](https://webassembly.github.io/reference-types/core/syntax/types.html#syntax-reftype) denotes an opaque and unforgeable reference to a host object. A new `externref` type can be passed into a Wasm module or return from it. The Wasm module cannot reveal an `externref` value's bit pattern, nor create a fake host reference by an integer value.

## Wasm module with External References

Take the following `wat` for example:

```
(module
  (type $t0 (func (param externref i32) (result i32)))
  ;; Import a host function which type is {externref i32} -> {i32}
  (import "extern_module" "functor_square" (func $functor_square (type $t0)))
  ;; Wasm function which type is {externref i32} -> {i32} and exported as "call_square"
  (func $call_square (export "call_square") (type $t0) (param $p0 externref) (param $p1 i32) (result i32)
    (call $functor_square (local.get $p0) (local.get $p1))
  )
  (memory $memory (export "memory") 1))
```

The Wasm function "`call_square`" takes an `externref` parameter, and calls the imported host function `functor_square` with that `externref`.
Therefore, the `functor_square` host function can get the object reference when users call "`call_square`" Wasm function and pass the object's reference.

## SSVM ExternRef Example

The following examples are how to use `externref` in Wasm with SSVM in C++.
Example C++ code can be found in `test/externref/ExternRefTest.h` and `test/externref/ExternRefTest.cpp`, and the example Wasm can be found in `test/externref/externrefTestData/`.

### Wasm Code

The Wasm code must pass the `externref` to host functions that want to access it.
Take the following `wat` for example, which is a part of `test/externref/externrefTestData/funcs.wat`:

```
(module
  (type $t0 (func (param externref i32 i32) (result i32)))
  (import "extern_module" "func_mul" (func $func_mul (type $t0)))
  (func $call_mul (export "call_mul") (type $t0) (param $p0 externref) (param $p1 i32) (param $p2 i32) (result i32)
    (call $func_mul (local.get $p0) (local.get $p1) (local.get $p2))
  )
  (memory $memory (export "memory") 1))
```

The host function "`extern_module::func_mul`" takes `externref` as a function pointer to multiply parameters 1 and 2 and then return the result. The exported Wasm function "`call_mul`" calls "`func_mul`" and pass the `externref` and 2 numbers as arguments.

### Host Functions

To instantiate the above example Wasm, the host functions must be registered into SSVM. See [Host Functions](https://github.com/second-state/SSVM/blob/master/doc/host_function.md) for more details.
The host functions which take `externref`s must know the original objects' types. We take the function pointer case for example.

```cpp
/// Function to pass as function pointer
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

namespace SSVM {
/// Host function to call function as function pointer
class ExternFuncMul : public Runtime::HostFunction<ExternFuncMul> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef Ref, uint32_t A, uint32_t B) {
    auto &Obj = retrieveExternRef<decltype(MulFunc)>(Ref);
    return Obj(A, B);
  }
};

/// Host module to register
class ExternMod : public Runtime::ImportObject {
public:
  ExternMod() : ImportObject("extern_module") {
    addHostFunc("func_mul", std::make_unique<ExternFuncMul>());
  }
  virtual ~ExternMod() = default;
};
} // namespace SSVM
```

"`MulFunc`" is a function that will be passed into Wasm as `externref`. In the "`func_mul`" host function, users can use "`retrieveExternRef`" template API to cast the `externref` to a reference of an object.

### SSVM Usage

Assume that the above `wat` is translated and stored into `test.wasm` binary file.
First, the host functions are needed to be registered into SSVM.

```cpp
#include "vm/configure.h"
#include "vm/vm.h"
#include <vector>

SSVM::VM::Configure Conf;
SSVM::VM::VM VM(Conf);
SSVM::ExternMod ExtMod;
VM.registerModule(ExtMod);
```

Second, load the `wasm` file and instantiate it.

```cpp
VM.loadWasm("test.wasm");
VM.validate();
VM.instantiate();
```

Last, prepare the `externref` as an argument and call the wasm function.
Noted that `MulFunc` is a function definition in above, which is: `uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }`.
`SSVM::genExternRef()` is a helper function to convert object into `externref` and pass it into SSVM.

```cpp
std::vector<SSVM::ValVariant> FuncArgs = {SSVM::genExternRef(MulFunc), 789U, 4321U};
std::vector<SSVM::ValVariant> Returns = *(VM.execute("call_mul", FuncArgs));
std::cout << std::get<uint32_t>(Returns[0]); // will print 3409269
```

## Passing Objects

The above example is passing a function reference as `externref`. The following examples are about how to pass an object reference into Wasm as `externref`.

### Passing a Class

To pass a class as `externref`, the object instance is needed.

```cpp
class AddClass {
public:
  uint32_t add(uint32_t A, uint32_t B) const { return A + B; }
};

AddClass AC;
```

Then users can pass the object into SSVM by using `SSVM::genExternRef()` API.

```cpp
std::vector<SSVM::ValVariant> FuncArgs = {SSVM::genExternRef(&AC), 1234U, 5678U};
std::vector<SSVM::ValVariant> Returns = *(VM.execute("call_add", FuncArgs));
std::cout << std::get<uint32_t>(Returns[0]); // will print 6912
```

In the host function which would access the object by reference, users can use "`retrieveExternRef`" API to retrieve the reference to the object.

```cpp
namespace SSVM {
/// Host function to access class as reference
class ExternClassAdd : public Runtime::HostFunction<ExternClassAdd> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef Ref, uint32_t A, uint32_t B) {
    auto &Obj = retrieveExternRef<AddClass>(Ref);
    return Obj.add(A, B);
  }
};
} // namespace SSVM
```

### Passing an Object As Functor

As the same as passing a class instance, the functor object instance is needed.

```cpp
struct SquareStruct {
  uint32_t operator()(uint32_t Val) const { return Val * Val; }
};

SquareStruct SS;
```

Then users can pass the object into SSVM by using `SSVM::genExternRef()` API.

```cpp
std::vector<SSVM::ValVariant> FuncArgs = {SSVM::genExternRef(&SS), 1024U};
std::vector<SSVM::ValVariant> Returns = *(VM.execute("call_square", FuncArgs));
std::cout << std::get<uint32_t>(Returns[0]); // will print 1048576
```

In the host function which would access the object by reference, users can use "`retrieveExternRef`" API to retrieve the reference to the object, and the reference is a functor.

```cpp
namespace SSVM {
/// Host function to call functor as reference
class ExternFunctorSquare : public Runtime::HostFunction<ExternFunctorSquare> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef Ref, uint32_t Val) {
    auto &Obj = retrieveExternRef<SquareStruct>(Ref);
    return Obj(Val);
  }
};
} // namespace SSVM
```

### Passing STL Objects

The example Wasm binary "`test/externref/externrefTestData/stl.wasm`" provides functions to interact with host functions which can access C++ STL objects.
Take the `std::ostream` and `std::string` objects for example. Assume that there's a host function accesses to a `std::ostream` and a `std::string` through `externref`s:

```cpp
// Part of host functions.
namespace SSVM {
/// Host function to output std::string through std::ostream
class ExternSTLOStreamStr : public Runtime::HostFunction<ExternSTLOStreamStr> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef RefOS, ExternRef RefStr) {
    /// operator<<(std::ostream &, std::string &)
    retrieveExternRef<std::ostream>(RefOS) << retrieveExternRef<std::string>(RefStr);
    return {};
  }
};
} // namespace SSVM
```

Assume that the above host function is in a host module `SSVM::ExternMod`.
Then users can allocate a VM and register this host module instance and instantiate the Wasm module.

```cpp
SSVM::VM::Configure Conf;
SSVM::VM::VM VM(Conf);
SSVM::ExternMod ExtMod;
VM.registerModule(ExtMod);
VM.loadWasm("externrefTestData/stl.wasm");
VM.validate();
VM.instantiate();
```

Last, pass the `std::cout` and a `std::string` object by external references.

```cpp
std::string PrintStr("Hello world!");
std::vector<SSVM::ValVariant> FuncArgs = {SSVM::genExternRef(&std::cout), SSVM::genExternRef(&PrintStr)};
VM.execute("call_ostream_str", FuncArgs);
/// Will get "Hello world!" in stdout.
```

For other C++ STL objects cases, such as `std::vector<T>`, `std::map<T, U>`, or `std::set<T>`, the object can be accessed correctly in host functions if the type in "`retrieveExternRef<>`" is correct.

## Tutorial

The following tutorial is the summary of how to use `externref` in SSVM.

### Prepare Your Wasm File

The Wasm file should contain importing host functions that would take the `externref`. Assume the Wasm file `test.wasm` is the binary format of following `wat`:

```
(module
  (type $t0 (func (param externref i32)))
  (type $t1 (func (param externref i32 i32) (result i32)))
  (type $t2 (func (param externref externref i32 i32)))

  (import "extern_module" "stl_ostream_u32" (func $stl_ostream_u32 (type $t0)))
    ;; Host function : std::ostream << uint32_t
  (import "extern_module" "func_mul" (func $func_mul (type $t1)))
    ;; Host function : function pointer to calculate uint32_t * uint32_t

  (func $print_mul (export "print_mul") (type $t2) (param $p0 externref) (param $p1 externref) (param $p2 i32) (param $p3 i32)
    ;; Wasm function : p0 = reference to std::ostream, p1 = function pointer to multiply function, p2 and p3 are values to multiply.
    (call $stl_ostream_u32
      (local.get $p0) (call $func_mul (local.get $p1) (local.get $p2) (local.get $p3))
    )
  )

  (memory $memory (export "memory") 1))
```

Users can convert `wat` to `wasm` through [wat2wasm](https://webassembly.github.io/wabt/demo/wat2wasm/) live tool. Noted that `reference types` checkbox should be checked on this page.

### Implement Host Module and Register into SSVM

The host module should be implemented and registered into SSVM before executing Wasm. Assume that the following code is saved as `main.cpp`:

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
#include "runtime/instance/memory.h"
#include "vm/configure.h"
#include "vm/vm.h"

#include <iostream>
#include <vector>

/// Multiply function to pass as function pointer
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

namespace SSVM {
/// Host function to call function as function pointer
class ExternFuncMul : public Runtime::HostFunction<ExternFuncMul> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef Ref, uint32_t A, uint32_t B) {
    auto &Obj = retrieveExternRef<decltype(MulFunc)>(Ref);
    return Obj(A, B);
  }
};

/// Host function to output uint32_t through std::ostream
class ExternSTLOStreamU32 : public Runtime::HostFunction<ExternSTLOStreamU32> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef RefOS, uint32_t Val) {
    retrieveExternRef<std::ostream>(RefOS) << Val;
    return {};
  }
};

/// Host module to register
class ExternMod : public Runtime::ImportObject {
public:
  ExternMod() : ImportObject("extern_module") {
    addHostFunc("func_mul", std::make_unique<ExternFuncMul>());
    addHostFunc("stl_ostream_u32", std::make_unique<ExternSTLOStreamU32>());
  }
  virtual ~ExternMod() = default;
};
} // namespace SSVM

int main() {
  SSVM::VM::Configure Conf;
  SSVM::VM::VM VM(Conf);
  SSVM::Log::setErrorLoggingLevel();

  /// Register the external module
  SSVM::ExternMod ExtMod;
  VM.registerModule(ExtMod);

  /// Load and instantiate Wasm file
  VM.loadWasm("test.wasm");
  VM.validate();
  VM.instantiate();

  /// Arguments are: reference to std::cout, reference to multiply function, and 123 and 456 to multiply
  std::vector<SSVM::ValVariant> FuncArgs = {SSVM::genExternRef(&std::cout), SSVM::genExternRef(MulFunc), 123U, 456U};

  /// Call Wasm function
  if (VM.execute("print_mul", FuncArgs)) {
    /// Success case, we will get "56088" in stdout.
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
```

### Setup and Build the Project

1. Create a cmake project

For example, create the `ref-test` project:

```bash
$ mkdir ref-test
$ cd ref-test
```

2. Clone the SSVM repository

```bash
$ git clone git@github.com:second-state/SSVM.git
```

3. Edit the CMakeLists

For finding headers from SSVM include directories and linking static libraries, some settings are necessary for CMakeFile:

```
cmake_minimum_required(VERSION 3.11)
project(Ref-test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(SSVM)

add_executable(reftest  # executable name of this example
  main.cpp  # Path to cpp file of this example
)

target_include_directories(reftest
  PUBLIC
  SSVM/include
  SSVM/thirdparty
)

target_link_libraries(reftest
  PRIVATE
  ssvmVM
)
```

4. Put the source files into this project directory

```bash
.
├── CMakeLists.txt
├── main.cpp
├── SSVM
└── test.wasm
```

5. Build the Project

For more details about building a project with SSVM, please read this [SSVM building tutorial](https://github.com/second-state/SSVM).

```bash
$ mkdir build
$ cd build
$ cmake -DSSVM_DISABLE_AOT_RUNTIME=On .. # Only interpreter supports reference types now.
$ make -j
```

6. Run the Test

```bash
$ cp ../test.wasm .
$ ./reftest
56088
```
