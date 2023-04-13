# Develop WasmEdge Plug-in in C API

## Prerequisites

For developing the WasmEdge plug-in in C API, please [install WasmEdge](../quick_start/install.md) first.

## Example

Assume that the plug-in example is in the file `testplugin.c`.

### Host Functions

The goal of the plug-in is to provide the host functions which can be imported when instantiating WASM.
Therefore, developers can implement their plug-in host functions first, as the same as the [host functions in WasmEdge C API](../sdk/c/ref.md#host-functions).

> For the more details about the [external data](../sdk/c/hostfunction.md#host-data) and [calling frame context](../sdk/c/hostfunction.md#calling-frame-context), please refer to the host function guide.

```c
#include <wasmedge/wasmedge.h>

/* The host function definitions. */

/* The host function to add 2 int32_t numbers. */
WasmEdge_Result HostFuncAdd(void *Data,
                            const WasmEdge_CallingFrameContext *CallFrameCxt,
                            const WasmEdge_Value *In, WasmEdge_Value *Out) {
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
  return WasmEdge_Result_Success;
}

/* The host function to sub 2 int32_t numbers. */
WasmEdge_Result HostFuncSub(void *Data,
                            const WasmEdge_CallingFrameContext *CallFrameCxt,
                            const WasmEdge_Value *In, WasmEdge_Value *Out) {
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(Val1 - Val2);
  return WasmEdge_Result_Success;
}
```

### Host Modules

Then developers should implement the module creation functions.

Noticed that there can be several module instances in a plug-in shared library.
Here take a module named as `wasmedge_plugintest_c_module` for the example.

```c
/* The creation function of creating the module instance. */
WasmEdge_ModuleInstanceContext *
CreateTestModule(const struct WasmEdge_ModuleDescriptor *Desc) {
  /*
   * The `Desc` is the const pointer to the module descriptor struct:
   *
   *   typedef struct WasmEdge_ModuleDescriptor {
   *     const char *Name;
   *     const char *Description;
   *     WasmEdge_ModuleInstanceContext *(*Create)(
   *         const struct WasmEdge_ModuleDescriptor *);
   *   } WasmEdge_ModuleDescriptor;
   * 
   * Developers can get the name and the description from this descriptor.
   */

  /* Exported module name of this module instance. */
  WasmEdge_String ModuleName =
      WasmEdge_StringCreateByCString("wasmedge_plugintest_c_module");
  WasmEdge_ModuleInstanceContext *Mod =
      WasmEdge_ModuleInstanceCreate(ModuleName);
  WasmEdge_StringDelete(ModuleName);

  WasmEdge_String FuncName;
  WasmEdge_FunctionTypeContext *FType;
  WasmEdge_FunctionInstanceContext *FuncCxt;
  enum WasmEdge_ValType ParamTypes[2], ReturnTypes[1];
  ParamTypes[0] = WasmEdge_ValType_I32;
  ParamTypes[1] = WasmEdge_ValType_I32;
  ReturnTypes[0] = WasmEdge_ValType_I32;

  /* Create and add the host function instances into the module instance. */
  FType = WasmEdge_FunctionTypeCreate(ParamTypes, 2, ReturnTypes, 1);
  FuncName = WasmEdge_StringCreateByCString("add");
  FuncCxt = WasmEdge_FunctionInstanceCreate(FType, HostFuncAdd, NULL, 0);
  WasmEdge_ModuleInstanceAddFunction(Mod, FuncName, FuncCxt);
  WasmEdge_StringDelete(FuncName);
  FuncName = WasmEdge_StringCreateByCString("sub");
  FuncCxt = WasmEdge_FunctionInstanceCreate(FType, HostFuncSub, NULL, 0);
  WasmEdge_ModuleInstanceAddFunction(Mod, FuncName, FuncCxt);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_FunctionTypeDelete(FType);

  return Mod;
}
```

### Plug-in Descriptions

For constructing the plug-in, developers should supply the descriptions of this plug-in and the modules.

```c
/* The module descriptor array. There can be multiple modules in a plug-in. */
static WasmEdge_ModuleDescriptor ModuleDesc[] = {{
    /*
     * Module name. This is the name for searching and creating the module
     * instance context by the `WasmEdge_PluginCreateModule()` API.
     */
    .Name = "wasmedge_plugintest_c_module",
    /* Module description. */
    .Description = "This is for the plugin tests in WasmEdge C API.",
    /* Creation function pointer. */
    .Create = CreateTestModule,
}};

/* The plug-in descriptor */
static WasmEdge_PluginDescriptor Desc[] = {{
    /*
     * Plug-in name. This is the name for searching the plug-in context by the
     * `WasmEdge_PluginFind()` API.
     */
    .Name = "wasmedge_plugintest_c",
    /* Plug-in description. */
    .Description = "",
    /* Plug-in API version. */
    .APIVersion = WasmEdge_Plugin_CurrentAPIVersion,
    /* Plug-in version. Developers can define the version of this plug-in. */
    .Version =
        {
            .Major = 0,
            .Minor = 1,
            .Patch = 0,
            .Build = 0,
        },
    /* Module count in this plug-in. */
    .ModuleCount = 1,
    /* Plug-in option description count in this plug-in (Work in progress). */
    .ProgramOptionCount = 0,
    /* Pointer to the module description array. */
    .ModuleDescriptions = ModuleDesc,
    /* Pointer to the plug-in option description array (Work in progress). */
    .ProgramOptions = NULL,
}};
```

### Plug-in Options

WORK IN PROGRESS. This section is reserved for the feature in the future.

### Implement the Get Descriptor API

The final step is to implement the `WasmEdge_Plugin_GetDescriptor()` API to return the plug-in descriptor.

```c
WASMEDGE_CAPI_PLUGIN_EXPORT const WasmEdge_PluginDescriptor *
WasmEdge_Plugin_GetDescriptor(void) {
  return &Desc;
}
```

## Build

To build the plug-in shared library, developers can choose to build stand-alone by the compiler or use cmake.

### Build with Command

```bash
clang -shared -std=c11 -DWASMEDGE_PLUGIN testplugin.c -lwasmedge -o libwasmedgePluginTest.so
```

### Build in CMake

```cmake
add_library(wasmedgePluginTest
  SHARED
  testplugin.c
)

set_target_properties(wasmedgePluginTest PROPERTIES
  C_STANDARD 11
)

target_compile_options(wasmedgePluginTest
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_link_libraries(wasmedgePluginTest
  PRIVATE
  wasmedge
)
```
