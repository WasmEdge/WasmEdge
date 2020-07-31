# Customized Host Functions

[Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports. The following steps give an example of registering a `host module` into SSVM runtime.

## Definitions of Host Instances

SSVM supports registering `host function`, `memory`, `table`, and `global` instances as imports.
For more details, samples can be found in `include/host/wasi/` and `test/core/spectest.h`. 

### Functions

A simple host function class can be declared as follows:

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace SSVM {
namespace Host {

class TestHost : public Runtime::HostFunction<TestHost> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Param1, float Param2);
};

} // namespace Host
} // namespace SSVM
```

According to example, return type `Expect<T>` presents the expected return number type `T` of this host function. Types of `Param1` and `Param2` presents argument types of this `host function`. Only WASM built-in types (aka. `uint32_t`, `uint64_t`, `float`, and `double`) are supported in `host functions`. When instantiating, the function signature of `vec(valtype) -> resulttype` is generated and can be imported by WASM modules.

Note: In the current state, only a single value returning is supported.

Another situation is passing environments or information which need to be accessed by `host function` body. The following sample shows how to implement host function clusters:

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include <vector>

namespace SSVM {
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
    /// Operations to `Data` ...
    return {};
  }
};

class TestHost2 : public TestCluster<TestHost2> {
public:
  TestHost2(std::vector<uint8_t> &Vec) : TestCluster(Vec) {}
  Expect<uint64_t> body(Runtime::Instance::MemoryInstance *MemInst, uint64_t Param1, double Param2) {
    /// Operations to `Data` ...
    return {};
  }
};

} // namespace Host
} // namespace SSVM
```

### Tables, Memories, and Globals

To create a `host table`, `memory`, and `global` instance, the only way is to create them with their constructor in the `host module`. The following chapter about the `host module` will provide examples.

## Host Modules

`Host module` is an object which can be registered into SSVM runtime. `Host module` contains `host functions`, `tables`, `memories`, `globals`, and other user-customized data. SSVM provides API to register `host modules`. After registering, these host instances in the `host module` can be imported by WASM modules.

### Declaration

`Host module` supplies exported module name and can contain customized data. A module name is needed when constructing `host modules`.

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"

namespace SSVM {
namespace Host {

class TestModule : public Runtime::ImportObject {
public:
  TestModule() : ImportObject("test");
  virtual ~TestModule() = default;
};

} // namespace Host
} // namespace SSVM
```

### Add Instances

`Host module` provides `addHostFunc()`, `addHostTable()`, `addHostMemory()`, and `addHostGlobal()` to insert instances with their unique names. Insertion can be done in constructor. The following example also shows how to create `host memories`, `tables`, and `globals`.

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
#include <memory>
#include <vector>

namespace SSVM {
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
    /// Operations to `Data` ...
    return {};
  }
};

class TestHost2 : public TestCluster<TestHost2> {
public:
  TestHost2(std::vector<uint8_t> &Vec) : TestCluster(Vec) {}
  Expect<uint64_t> body(Runtime::Instance::MemoryInstance *MemInst, uint64_t Param1, double Param2) {
    /// Operations to `Data` ...
    return {};
  }
};

class TestModule : public Runtime::ImportObject {
public:
  TestModule(std::vector<uint8_t> &Vec) : ImportObject("test"), Data(Vec) {
    /// Add function instances with exporting name
    addHostFunc("test_func1", std::make_unique<TestHost1>(Data));
    addHostFunc("test_func2", std::make_unique<TestHost2>(Data));

    AST::Limit TabLimit(10, 20); /// Create a limit object of table instance
    /// Add table instance with exporting name
    addHostTable("table", std::make_unique<Runtime::Instance::TableInstance>(RefType::FuncRef, TabLimit));

    AST::Limit MemLimit(1, 2); /// Create a limit object of memory instance
    /// Add memory instance with exporting name
    addHostMemory("memory", std::make_unique<Runtime::Instance::MemoryInstance>(MemLimit));

    /// Add global instance with exporting name
    addHostGlobal("global_i32", std::make_unique<Runtime::Instance::GlobalInstance>(ValType::I32, ValMut::Const, uint32_t(666)));
    addHostGlobal("global_i64", std::make_unique<Runtime::Instance::GlobalInstance>(ValType::I64, ValMut::Const, uint64_t(666)));
    addHostGlobal("global_f32", std::make_unique<Runtime::Instance::GlobalInstance>(ValType::F32, ValMut::Const, float(666)));
    addHostGlobal("global_f64", std::make_unique<Runtime::Instance::GlobalInstance>(ValType::F64, ValMut::Const, double(666)));
  }
  virtual ~TestModule() = default;

private:
  std::vector<uint8_t> &Data;
};

} // namespace Host
} // namespace SSVM
```

`Host module` supplies `getFuncs()`, `getTables()`, `getMems()`, and `getGlobals()` to search registered instances by unique exporting name. For more details, APIs can be found in `include/runtime/importobj.h`.

### Register Host Modules to SSVM

Users can register host modules via `SSVM::VM::registerModule()` API.

```cpp
#include "vm/configure.h"
#include "vm/vm.h"
#include <vector>

SSVM::VM::Configure Conf;
SSVM::VM::VM VM(Conf);
std::vector<uint8_t> Data;
SSVM::Host::TestModule TestMod(Data);
VM.registerModule(TestMod);
```

### Link Libraries And Include Directories in CMakeFile

For finding headers from SSVM include directories and linking static libraries, some settings are necessary for CMakeFile:

```
add_library(ssvmHostModuleTest  # Static library name of host modules
  test.cpp  # Path to host modules cpp files
)

target_include_directories(ssvmHostModuleTest
  PUBLIC
  ${Boost_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
```

## Implementation of Host Function Body

There are some tips about implementing host function bodies.

### Checking Memory Instance When Using

Host function can access WASM memory, which passed as `MemoryInstance *` argument. When a [function call occurs](https://webassembly.github.io/spec/core/exec/instructions.html#function-calls), a frame with module which the called function belonging to will be pushed onto the `stack`. In the `host function` case, the `memory` instance of the module of the top frame on the `stack` will be passed as the `host function` body's argument. But there can be no `memory` instance in a WASM module. Therefore, users should check if the memory instance pointer is a `nullptr` or not when accessing.

### Returning Expectation

From our mechanism, `Expect<T>` declared in `include/common/errcode.h` is used as the result type of function body. In `Expect<void>` case, `return {};` is needed for an expected situation. In other cases, `return Value;` is needed, where `Value` is a variable of type `T`. If an unexpected situation occurs, users can call `return Unexpect(Code);` to return an error, which `Code` is an element of enumeration `ErrCode`.

### Forcing Termination

SSVM provides a method for terminating WASM execution in host functions. Developers can return `ErrCode::Terminated` to trigger the forcing termination of the current execution and the execution result will be set as successful.
