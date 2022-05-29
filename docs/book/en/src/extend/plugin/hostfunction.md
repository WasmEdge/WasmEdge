# Host Functions

[Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports. The following steps give an example of registering a `host module` into WasmEdge runtime.

This example is for the sources compile with the WasmEdge project in C++.
If developers want to implement the host functions in C/C++ with WasmEdge C API and without compiling with the WasmEdge project, please refer to the [C API Documentation](../../embed/c/ref.md#host-functions).

## Definitions of Host Instances

WasmEdge supports registering `host function`, `memory`, `table`, and `global` instances as imports.
For more details, samples can be found in `include/host/wasi/` and `test/core/spectest.h`.

### Functions

A simple host function class can be declared as follows:

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

class TestHost : public Runtime::HostFunction<TestHost> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Param1, float Param2);
};

} // namespace Host
} // namespace WasmEdge
```

According to example, return type `Expect<T>` presents the expected return number type `T` of this host function. Types of `Param1` and `Param2` presents argument types of this `host function`. Only WASM built-in types (aka. `uint32_t`, `uint64_t`, `float`, and `double`) are supported in `host functions`. When instantiating, the function signature of `vec(valtype) -> resulttype` is generated and can be imported by WASM modules.

Note: In the current state, only a single value returning is supported.

Another situation is passing environments or information which need to be accessed by `host function` body. The following sample shows how to implement host function clusters:

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include <vector>

namespace WasmEdge {
namespace Host {

template <typename T> class TestCluster : public Runtime::HostFunction<T> {
public:
  TestCluster(std::vector<uint8_t> &Vec) : Data(Vec) {}

protected:
  std::vector<uint8_t> &Data;
};

class TestHost1 : public TestCluster<TestHost1> {
public:
  TestHost1(std::vector<uint8_t> &Vec) : TestCluster(Vec) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Param1, float Param2) {
    // Operations to `Data` ...
    return {};
  }
};

class TestHost2 : public TestCluster<TestHost2> {
public:
  TestHost2(std::vector<uint8_t> &Vec) : TestCluster(Vec) {}
  Expect<uint64_t> body(Runtime::Instance::MemoryInstance *MemInst, uint64_t Param1, double Param2) {
    // Operations to `Data` ...
    return {};
  }
};

} // namespace Host
} // namespace WasmEdge
```

### Tables, Memories, and Globals

To create a `host table`, `memory`, and `global` instance, the only way is to create them with their constructor in the `host module`. The following chapter about the `host module` will provide examples.

## Host Modules

The host module is a module instance which can be registered into WasmEdge runtime. A module instance contains `host functions`, `tables`, `memories`, `globals`, and other user-customized data. WasmEdge provides API to register a module instance into a `VM` or `Store`. After registering, these host instances in the module instance can be imported by WASM modules.

### Declaration

Module instance supplies exported module name and can contain customized data. A module name is needed when constructing module instances.

```cpp
#include "common/errcode.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class TestModule : public Runtime::Instance::ModuleInstance {
public:
  TestModule() : ModuleInstance("test");
  virtual ~TestModule() = default;
};

} // namespace Host
} // namespace WasmEdge
```

### Add Instances

Module instance provides `addHostFunc()`, `addHostTable()`, `addHostMemory()`, and `addHostGlobal()` to insert instances with their unique names. Insertion can be done in constructor. The following example also shows how to create `host memories`, `tables`, and `globals`.

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/module.h"
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {

template <typename T> class TestCluster : public Runtime::HostFunction<T> {
public:
  TestCluster(std::vector<uint8_t> &Vec) : Data(Vec) {}

protected:
  std::vector<uint8_t> &Data;
};

class TestHost1 : public TestCluster<TestHost1> {
public:
  TestHost1(std::vector<uint8_t> &Vec) : TestCluster(Vec) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Param1, float Param2) {
    // Operations to `Data` ...
    return {};
  }
};

class TestHost2 : public TestCluster<TestHost2> {
public:
  TestHost2(std::vector<uint8_t> &Vec) : TestCluster(Vec) {}
  Expect<uint64_t> body(Runtime::Instance::MemoryInstance *MemInst, uint64_t Param1, double Param2) {
    // Operations to `Data` ...
    return {};
  }
};


class TestModule : public Runtime::Instance::ModuleInstance {
public:
  TestModule(std::vector<uint8_t> &Vec) : ModuleInstance("test"), Data(Vec) {
    // Add function instances with exporting name
    addHostFunc("test_func1", std::make_unique<TestHost1>(Data));
    addHostFunc("test_func2", std::make_unique<TestHost2>(Data));

    // Add table instance with exporting name
    addHostTable("table", std::make_unique<Runtime::Instance::TableInstance>(
                              TableType(RefType::FuncRef, 10, 20)));

    // Add memory instance with exporting name
    addHostMemory("memory", std::make_unique<Runtime::Instance::MemoryInstance>(
                                MemoryType(1, 2)));

    // Add global instance with exporting name
    addHostGlobal("global_i32",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      GlobalType(ValType::I32, ValMut::Const), uint32_t(666)));
    addHostGlobal("global_i64",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      GlobalType(ValType::I64, ValMut::Const), uint64_t(666)));
    addHostGlobal("global_f32",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      GlobalType(ValType::F32, ValMut::Const), float(666)));
    addHostGlobal("global_f64",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      GlobalType(ValType::F64, ValMut::Const), double(666)));
  }
  virtual ~TestModule() = default;

private:
  std::vector<uint8_t> &Data;
};

} // namespace Host
} // namespace WasmEdge
```

Module instance supplies `getFuncs()`, `getTables()`, `getMems()`, and `getGlobals()` to search registered instances by unique exporting name. For more details, APIs can be found in `include/runtime/importobj.h`.

### Register Host Modules to WasmEdge

Users can register host modules via `WasmEdge::VM::registerModule()` API.

```cpp
#include "common/configure.h"
#include "vm/vm.h"
#include <vector>

WasmEdge::Configure Conf;
WasmEdge::VM::VM VM(Conf);
std::vector<uint8_t> Data;
WasmEdge::Host::TestModule TestMod(Data);
VM.registerModule(TestMod);
```

### Link Libraries And Include Directories in CMakeFile

For finding headers from WasmEdge include directories and linking static libraries, some settings are necessary for CMakeFile:

```cmake
add_library(wasmedgeHostModuleTest  # Static library name of host modules
  test.cpp  # Path to host modules cpp files
)

target_include_directories(wasmedgeHostModuleTest
  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
)
```

## Implementation of Host Function Body

There are some tips about implementing host function bodies.

### Checking Memory Instance When Using

Host function can access WASM memory, which passed as `MemoryInstance *` argument. When a [function call occurs](https://webassembly.github.io/spec/core/exec/instructions.html#function-calls), a frame with module which the called function belonging to will be pushed onto the `stack`. In the `host function` case, the `memory` instance of the module of the top frame on the `stack` will be passed as the `host function` body's argument. But there can be no `memory` instance in a WASM module. Therefore, users should check if the memory instance pointer is a `nullptr` or not when accessing.

### Returning Expectation

From our mechanism, `Expect<T>` declared in `include/common/errcode.h` is used as the result type of function body. In `Expect<void>` case, `return {};` is needed for an expected situation. In other cases, `return Value;` is needed, where `Value` is a variable of type `T`. If an unexpected situation occurs, users can call `return Unexpect(Code);` to return an error, which `Code` is an element of enumeration `ErrCode`.

### Forcing Termination

WasmEdge provides a method for terminating WASM execution in host functions. Developers can return `ErrCode::Terminated` to trigger the forcing termination of the current execution and pass the `ErrCode::Terminated` to the caller of the host functions.
