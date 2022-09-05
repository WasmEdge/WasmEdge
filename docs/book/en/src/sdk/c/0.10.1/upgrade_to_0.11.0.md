# Upgrade to WasmEdge 0.11.0

Due to the WasmEdge C API breaking changes, this document shows the guideline for programming with WasmEdge C API to upgrade from the `0.10.1` to the `0.11.0` version.

## Concepts

1. Supported the user-defined error code in host functions.

    Developers can use the new API `WasmEdge_ResultGen()` to generate a `WasmEdge_Result` struct with `WasmEdge_ErrCategory_UserLevelError` and the error code.
    With this support, developers can specify the host function error code when failed by themselves.
    For the examples to specify the user-defined error code, please refer to [the example below](#user-defined-error-code-in-host-functions).

2. Calling frame for the host function extension

    In the previous versions, host functions only pass the memory instance into the function body.
    For supporting the WASM multiple memories proposal and providing the recursive invocation in host functions, the new context `WasmEdge_CallingFrameContext` replaced the memory instance in the second argument of the host function definition.
    For the examples of the new host function definition, please refer to [the example below](#calling-frame-in-host-functions).

3. Apply the SONAME and SOVERSION.

    When linking the WasmEdge shared library, please notice that `libwasmedge_c.so` is renamed to `libwasmedge.so` after the 0.11.0 release.
    Please use `-lwasmedge` instead of `-lwasmedge_c` for the linker option.

## User Defined Error Code In Host Functions

Assume that we want to specify that the host function failed in the versions before `0.10.1`:

```c
/* Host function body definition. */
WasmEdge_Result FaildFunc(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* This will create a trap in WASM. */
  return WasmEdge_Result_Fail;
}
```

When the execution is finished, developers will get the `WasmEdge_Result`.
If developers call the `WasmEdge_ResultOK()` with the returned result, they will get `false`.
If developers call the `WasmEdge_ResultGetCode()` with the returned result, they will always get `2`.

For the versions after `0.11.0`, developers can specify the error code within 24-bit (smaller than `16777216`) size.

```c
/* Host function body definition. */
WasmEdge_Result FaildFunc(void *Data,
                          const WasmEdge_CallingFrameContext *CallFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* This will create a trap in WASM with the error code. */
  return WasmEdge_ResultGen(WasmEdge_ErrCategory_UserLevelError, 12345678);
}
```

Therefore when developers call the `WasmEdge_ResultGetCode()` with the returned result, they will get the error code `12345678`.
Noticed that if developers call the `WasmEdge_ResultGetMessage()`, they will always get the C string `"user defined error code"`.

## Calling Frame In Host Functions

When implementing the host functions, developers usually use the input memory instance to load or store data.
In the WasmEdge versions before `0.10.1`, the argument before the input and return value list of the host function definition is the memory instance context, so that developers can access the data in the memory instance.

```c
/* Host function body definition. */
WasmEdge_Result LoadOffset(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                           const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* Function type: {i32} -> {} */
  uint32_t Offset = (uint32_t)WasmEdge_ValueGetI32(In[0]);
  uint32_t Num = 0;
  WasmEdge_Result Res =
      WasmEdge_MemoryInstanceGetData(MemCxt, (uint8_t *)(&Num), Offset, 4);
  if (WasmEdge_ResultOK(Res)) {
    printf("u32 at memory[%u]: %u\n", Offset, Num);
  } else {
    return Res;
  }
  return WasmEdge_Result_Success;
}
```

The input memory instance is the one that belongs to the module instance on the top calling frame of the stack.
However, after applying the WASM multiple memories proposal, there may be more than 1 memory instance in a WASM module.
Furthermore, there may be requests for accessing the module instance on the top frame of the stack to get the exported WASM functions, such as recursive invocation in host functions.
To support these, the `WasmEdge_CallingFrameContext` is designed to replace the memory instance input of the host function.

In the WasmEdge versions after `0.11.0`, the host function definitions are changed:

```c
typedef WasmEdge_Result (*WasmEdge_HostFunc_t)(
    void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
    const WasmEdge_Value *Params, WasmEdge_Value *Returns);

typedef WasmEdge_Result (*WasmEdge_WrapFunc_t)(
    void *This, void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
    const WasmEdge_Value *Params, const uint32_t ParamLen,
    WasmEdge_Value *Returns, const uint32_t ReturnLen);
```

Developers need to change to use the `WasmEdge_CallingFrameContext` related APIs to access the memory instance:

```c
/* Host function body definition. */
WasmEdge_Result LoadOffset(void *Data,
                           const WasmEdge_CallingFrameContext *CallFrameCxt,
                           const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* Function type: {i32} -> {} */
  uint32_t Offset = (uint32_t)WasmEdge_ValueGetI32(In[0]);
  uint32_t Num = 0;

  /* Get the 0th memory instance of the module of the top frame on the stack. */
  WasmEdge_MemoryInstanceContext *MemCxt =
      WasmEdge_CallingFrameGetMemoryInstance(CallFrameCxt, 0);
  WasmEdge_Result Res =
      WasmEdge_MemoryInstanceGetData(MemCxt, (uint8_t *)(&Num), Offset, 4);
  if (WasmEdge_ResultOK(Res)) {
    printf("u32 at memory[%u]: %u\n", Offset, Num);
  } else {
    return Res;
  }
  return WasmEdge_Result_Success;
}
```

The `WasmEdge_CallingFrameGetModuleInstance()` API can help developers to get the module instance of the top frame on the stack.
With the module instance context, developers can use the module instance-related APIs to get its contents.

The `WasmEdge_CallingFrameGetExecutor()` API can help developers to get the currently used executor context.
Therefore developers can use the executor to recursively invoke other WASM functions without creating a new executor context.
