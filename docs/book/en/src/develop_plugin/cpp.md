# Develop WasmEdge Plug-in in C++ API

> We recommend developers to [develop plug-ins in WasmEdge C API](c.md).

## Prerequisites

For developing the WasmEdge plug-in in internal C++, developers should [build WasmEdge from source](../contribute/build_from_src.md).

## Example

Assume that the plug-in example is in the file `testplugin.h` and `testplugin.cpp`.

### Host Functions and Modules

The goal of the plug-in is to provide the host functions which can be imported when instantiating WASM.
Therefore, developers should implement their plug-in host functions in WasmEdge internal C++ first.
Assume that the host function implementations are in the `testplugin.h`.

```cpp
#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace Host {

// The environment class. For the register object.
class WasmEdgePluginTestEnv {
public:
  WasmEdgePluginTestEnv() noexcept = default;

  static Plugin::PluginRegister Register;
};

// The host function base template class. For inheriting the environment class
// reference.
template <typename T>
class WasmEdgePluginTestFunc : public Runtime::HostFunction<T> {
public:
  WasmEdgePluginTestFunc(WasmEdgePluginTestEnv &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgePluginTestEnv &Env;
};

// The host function to add 2 int32_t numbers.
class WasmEdgePluginTestFuncAdd
    : public WasmEdgePluginTestFunc<WasmEdgePluginTestFuncAdd> {
public:
  WasmEdgePluginTestFuncAdd(WasmEdgePluginTestEnv &HostEnv)
      : WasmEdgePluginTestFunc(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t A, uint32_t B) {
    return A + B;
  }
};

// The host function to sub 2 int32_t numbers.
class WasmEdgePluginTestFuncSub
    : public WasmEdgePluginTestFunc<WasmEdgePluginTestFuncSub> {
public:
  WasmEdgePluginTestFuncSub(WasmEdgePluginTestEnv &HostEnv)
      : WasmEdgePluginTestFunc(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t A, uint32_t B) {
    return A - B;
  }
};

// The host module class. There can be several modules in a plug-in.
class WasmEdgePluginTestModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgePluginTestModule()
      : Runtime::Instance::ModuleInstance("wasmedge_plugintest_cpp_module") {
    addHostFunc("add", std::make_unique<WasmEdgePluginTestFuncAdd>(Env));
    addHostFunc("sub", std::make_unique<WasmEdgePluginTestFuncSub>(Env));
  }

  WasmEdgePluginTestEnv &getEnv() { return Env; }

private:
  WasmEdgePluginTestEnv Env;
};

} // namespace Host
} // namespace WasmEdge
```

### Creation Functions for Modules

Then developers should implement the module creation functions.
Assume that the following implementations are all in the `testplugin.cpp`.

```cpp
#include "testplugin.h"

namespace WasmEdge {
namespace Host {
namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  // There can be several modules in a plug-in. For that, developers should
  // implement several `create` functions for each module.
  return new WasmEdgePluginTestModule;
}

} // namespace
} // namespace Host
} // namespace WasmEdge
```

### Plug-in Descriptions

For constructing the plug-in, developers should supply the descriptions of this plug-in and the modules.

```cpp
namespace WasmEdge {
namespace Host {
namespace {

Plugin::Plugin::PluginDescriptor Descriptor{
    // Plug-in name. This is the name for searching the plug-in context by the
    // `WasmEdge_PluginFind()` C API.
    .Name = "wasmedge_plugintest_cpp",
    // Plug-in description.
    .Description = "",
    // Plug-in API version.
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    // Plug-in version.
    .Version = {0, 10, 0, 0},
    // Module count in this plug-in.
    .ModuleCount = 1,
    // Pointer to module description array.
    .ModuleDescriptions =
        // The module descriptor array.
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                // Module name. This is the name for searching and creating the
                // module instance context by the
                // `WasmEdge_PluginCreateModule()` C API.
                .Name = "wasmedge_plugintest_cpp_module",
                // Module description.
                .Description = "This is for the plugin tests in WasmEdge.",
                // Creation function pointer.
                .Create = create,
            },
        },
    // Plug-in options (Work in progress).
    .AddOptions = nullptr,
};

} // namespace
} // namespace Host
} // namespace WasmEdge
```

### Plug-in Options

WORK IN PROGRESS. This section is reserved for the feature in the future.

### Implement the Plug-in Descriptor Registration

The final step is to implement the `Plugin::PluginRegister` initialization with the plug-in descriptor.

```cpp
namespace WasmEdge {
namespace Host {

Plugin::PluginRegister WasmEdgePluginTestEnv::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
```

## Build

To build the plug-in shared library, developers should build in cmake with the WasmEdge source.

Assume that the folder `test` is created under the `<PATH_TO_WASMEDGE_SOURCE>/plugins`.

Add this line in the `<PATH_TO_WASMEDGE_SOURCE>/plugins/CMakeLists.txt`:

```cmake
add_subdirectory(test)
```

Copy the `testplugin.h` and `testplugin.cpp` into the `<PATH_TO_WASMEDGE_SOURCE>/plugins/test` directory.

And then edit the file `<PATH_TO_WASMEDGE_SOURCE>/plugins/test/CMakeLists.txt`:

```cmake
wasmedge_add_library(wasmedgePluginTest
  SHARED
  testplugin.cpp
)

target_compile_options(wasmedgePluginTest
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginTest
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginTest
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginTest
    PRIVATE
    wasmedge_shared
  )
endif()
```

Then you can [follow the guide to build from source](../contribute/build_from_src/linux.md).
