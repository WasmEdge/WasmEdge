# Host 函数

[Host 函数](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) 是在 WebAssembly 之外的函数，它们以导入的方式传递到 WASM 模块中。以下步骤是将 `host module` 注册到 WasmEdge runtime 的示例。

此示例适用于使用 C++ 编写的 WasmEdge 项目的源代码编译。如果开发者想使用C/C++ 和 WasmEdge C API 来实现 host 函数，而不用 WasmEdge 项目进行编译，请参阅 [C API 文档](../../embed/c/ref.md#host-functions)。

## Host 实例定义

WasmEdge 支持以导入的形式注册 `host function` 、`memory` 、`table` 和 `global` 等实例。
详情请参阅 `include/host/wasi/` 和 `test/core/spectest.h` 中的示例。

### 函数

可以按照如下方式声明一个简单的 host 函数类：

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

在上述示例中，返回类型 `Expect<T>` 表示该 host 函数的预期返回类型 `T`。 `Param1` 和 `Param2` 的类型表示此 host 函数的参数类型。 host 函数仅支持 WASM 的内置类型（如 `uint32_t`、 `uint64_t`、 `float` 以及 `double`）。在实例化时会生成 `vec(valtype) -> resulttype` 的函数签名，它能被 WASM 模块导入。

注意：目前 host 函数仅支持单个返回值。

另一种情况是传递需要由 `host function` 体访问的环境或信息。 以下示例展示了如何实现 host 函数集群：

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
} // namespace WasmEdge
```

### 表、 内存、 以及全局实例

要创建 `host table`、 `memory` 以及 `global` 实例，唯一的办法就是使用 `host module` 中它们各自的构造器来创建。以下关于 `host module` 的章节将提供更详细的示例。

## Host Modules/ host 模块

`Host module` 是一个可以注册到 WasmEdge runtime 的对象。`Host module` 包含了 `host functions`、 `tables`、 `memories`、 `globals` 以及其它用户自定义的数据。WasmEdge 提供 API 来注册 `host modules`。在注册后，`host modules` 中的这些 host 实例可以被 WASM 模块所导入。

### 声明

`Host module` 提供了导出的模块名称，并且可以包含自定义数据。 构建 `host modules` 时需要填写模块名称。

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"

namespace WasmEdge {
namespace Host {

class TestModule : public Runtime::ImportObject {
public:
  TestModule() : ImportObject("test");
  virtual ~TestModule() = default;
};

} // namespace Host
} // namespace WasmEdge
```

### 添加实例

`Host module` 提供了以下四种方法 `addHostFunc()`、 `addHostTable()`、 `addHostMemory()` 和 `addHostGlobal()` 来插入具有唯一名称的实例。插入操作可以在构造器中完成。下面的示例代码中还展示了如何创建 `host memories`、 `tables` 和 `globals`。

```cpp
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
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

    /// Add table instance with exporting name
    addHostTable("table", std::make_unique<Runtime::Instance::TableInstance>(
                              TableType(RefType::FuncRef, 10, 20)));

    /// Add memory instance with exporting name
    addHostMemory("memory", std::make_unique<Runtime::Instance::MemoryInstance>(
                                MemoryType(1, 2)));

    /// Add global instance with exporting name
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

`Host module` 提供了以下四种方法 `getFuncs()`、 `getTables()`、 `getMems()` 和 `getGlobals()` 用来通过唯一的导出名称搜索已注册的实例。更多细节和 APIs 请参阅 `include/runtime/importobj.h`。

### 在 WasmEdge 中注册 host module

用户可以通过 `WasmEdge::VM::registerModule()` API 来注册 `host module`。

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

### 在 CMakeFile 中链接库以及包含目录

为了从 WasmEdge 的包含目录中查找头文件以及链接静态库，CMakeFile 需要如下设置

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

## host 函数体实现

以下是一些实现 host 函数体的小技巧。

### 使用时检查 Memory 实例

Host 函数能够访问 WASM 内存，这些内存通常以 `MemoryInstance *` 参数的形式被传递。 当发生[函数调用](https://webassembly.github.io/spec/core/exec/instructions.html#function-calls) 时，被调用函数所属的带有模块的帧将被压入`堆栈`。 在 host 函数的例子中，堆栈顶部栈帧中的内存实例将作为 host 函数体的参数被传递。然而，一个 WASM 模块中可以不包含内存实例。因此，用户在访问时应检查内存实例指针是否为空指针。

### 预期返回

在我们的机制里， `include/common/errcode.h` 中声明的 `Expect<T>` 被用作函数体的结果类型。在 `Expect<void>` 的情况下，预期场景需要 `return {};` 。其它情况下， 预期场景需要 `return Value;` ，其中 `Value` 是 `T` 类型的一个变量。如果出现意外情况，用户可以调用 `return Unexpect(Code);` 来返回一个错误，其中 `Code` 是枚举 `ErrCode` 的一个元素。

### 强制终止

WasmEdge 提供了一种在 host 函数中终止 WASM 执行的方法。开发者可以返回 `ErrCode::Terminated`  来触发当前执行的强制终止，并将 `ErrCode::Terminated`  传递给 host 函数的调用者。
