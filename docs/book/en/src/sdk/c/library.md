# Use the WasmEdge Library

When programming with WasmEdge C API, developers should include the required headers and link with the WasmEdge Library.
Besides [install WasmEdge](../../quick_start/install.md) with the WasmEdge shared library, developers can also [build WasmEdge](../../contribute/build_from_src.md) to generate the WasmEdge static library.

Assume the example `test.c`:

```c
#include <stdio.h>
#include <wasmedge/wasmedge.h>

/* Host function body definition. */
WasmEdge_Result Add(void *Data,
                    const WasmEdge_CallingFrameContext *CallFrameCxt,
                    const WasmEdge_Value *In, WasmEdge_Value *Out) {
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  printf("Host function \"Add\": %d + %d\n", Val1, Val2);
  Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
  return WasmEdge_Result_Success;
}

int main() {
  /* Create the VM context. */
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

  /* The WASM module buffer. */
  uint8_t WASM[] = {/* WASM header */
                    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
                    /* Type section */
                    0x01, 0x07, 0x01,
                    /* function type {i32, i32} -> {i32} */
                    0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
                    /* Import section */
                    0x02, 0x13, 0x01,
                    /* module name: "extern" */
                    0x06, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E,
                    /* extern name: "func-add" */
                    0x08, 0x66, 0x75, 0x6E, 0x63, 0x2D, 0x61, 0x64, 0x64,
                    /* import desc: func 0 */
                    0x00, 0x00,
                    /* Function section */
                    0x03, 0x02, 0x01, 0x00,
                    /* Export section */
                    0x07, 0x0A, 0x01,
                    /* export name: "addTwo" */
                    0x06, 0x61, 0x64, 0x64, 0x54, 0x77, 0x6F,
                    /* export desc: func 0 */
                    0x00, 0x01,
                    /* Code section */
                    0x0A, 0x0A, 0x01,
                    /* code body */
                    0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B};

  /* Create the module instance. */
  WasmEdge_String ExportName = WasmEdge_StringCreateByCString("extern");
  WasmEdge_ModuleInstanceContext *HostModCxt =
      WasmEdge_ModuleInstanceCreate(ExportName);
  enum WasmEdge_ValType ParamList[2] = {WasmEdge_ValType_I32,
                                        WasmEdge_ValType_I32};
  enum WasmEdge_ValType ReturnList[1] = {WasmEdge_ValType_I32};
  WasmEdge_FunctionTypeContext *HostFType =
      WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
  WasmEdge_FunctionInstanceContext *HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, Add, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  WasmEdge_String HostFuncName = WasmEdge_StringCreateByCString("func-add");
  WasmEdge_ModuleInstanceAddFunction(HostModCxt, HostFuncName, HostFunc);
  WasmEdge_StringDelete(HostFuncName);

  WasmEdge_VMRegisterModuleFromImport(VMCxt, HostModCxt);

  /* The parameters and returns arrays. */
  WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(1234),
                              WasmEdge_ValueGenI32(5678)};
  WasmEdge_Value Returns[1];
  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("addTwo");
  /* Run the WASM function from file. */
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromBuffer(
      VMCxt, WASM, sizeof(WASM), FuncName, Params, 2, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }

  /* Resources deallocations. */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_ModuleInstanceDelete(HostModCxt);
  return 0;
}
```

This example will execute a WASM which call into a host function to add 2 numbers.

## Link with WasmEdge Shared Library

To link the executable with the WasmEdge shared library is easy. Just compile the example file after installation of WasmEdge.

```bash
$ gcc test.c -lwasmedge -o test
$ ./test
Host function "Add": 1234 + 5678
Get the result: 6912
```

## Link with WasmEdge Static Library

For preparing the WasmEdge static library, developers should [build WasmEdge from source](../../contribute/build_from_src.md#cmake-building-options) with the options:

```bash
# Recommend to use the `wasmedge/wasmedge:latest` docker image. This will provide the required packages.
# In the WasmEdge source directory
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_LINK_LLVM_STATIC=ON -DWASMEDGE_BUILD_SHARED_LIB=Off -DWASMEDGE_BUILD_STATIC_LIB=On -DWASMEDGE_LINK_TOOLS_STATIC=On -DWASMEDGE_BUILD_PLUGINS=Off
cmake --build build
cmake --install build
```

The cmake option `-DWASMEDGE_LINK_LLVM_STATIC=ON` will turn on the static library building, and the `-DWASMEDGE_BUILD_SHARED_LIB=Off` will turn off the shared library building.

After installation, developers can compile the example file:

```bash
# Note: only the Linux platforms need the `-lrt`. The MacOS platforms not need this linker flag.
$ gcc test.c -lwasmedge -lrt -ldl -pthread -lm -lstdc++ -o test
$ ./test
Host function "Add": 1234 + 5678
Get the result: 6912
```
