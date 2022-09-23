# Host Functions

[Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are the functions outside WebAssembly and passed to WASM modules as imports.
The following steps give an example of implementing host functions and registering a `host module` into the WasmEdge runtime.

## Host Instances

WasmEdge supports registering `host function`, `memory`, `table`, and `global` instances as imports.

### Functions

The host function body definition in WasmEdge is defined as follows:

```c
typedef WasmEdge_Result (*WasmEdge_HostFunc_t)(
    void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
    const WasmEdge_Value *Params, WasmEdge_Value *Returns);
```

A simple host function can be defined as follows:

```c
#include <wasmedge/wasmedge.h>

/* This function can add 2 i32 values and return the result. */
WasmEdge_Result Add(void *, const WasmEdge_CallingFrameContext *,
                    const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /*
  * Params: {i32, i32}
  * Returns: {i32}
  */
 
  /* Retrieve the value 1. */
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  /* Retrieve the value 2. */
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  /* Output value 1 is Val1 + Val2. */
  Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
  /* Return the status of success. */
  return WasmEdge_Result_Success;
}
```

For adding the host function into a host module instance, developers should create the function instance with the function type context first.

```c
enum WasmEdge_ValType ParamList[2] = {WasmEdge_ValType_I32,
                                      WasmEdge_ValType_I32};
enum WasmEdge_ValType ReturnList[1] = {WasmEdge_ValType_I32};
/* Create a function type: {i32, i32} -> {i32}. */
WasmEdge_FunctionTypeContext *HostFType =
    WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
/*
  * Create a function context with the function type and host function body.
  * The `Cost` parameter can be 0 if developers do not need the cost
  * measuring.
  */
WasmEdge_FunctionInstanceContext *HostFunc =
    WasmEdge_FunctionInstanceCreate(HostFType, Add, NULL, 0);
/*
  * The third parameter is the pointer to the additional data.
  * Developers should guarantee the life cycle of the data, and it can be NULL if the external data is not needed.
  */
WasmEdge_FunctionTypeDelete(HostType);
```

### Tables, Memories, and Globals

To create a `host table`, `memory`, and `global` instance, developers can use similar APIs.

```c
/* Create a host table exported as "table". */
WasmEdge_Limit TabLimit = {
    .HasMax = true, .Shared = false, .Min = 10, .Max = 20};
WasmEdge_TableTypeContext *HostTType =
    WasmEdge_TableTypeCreate(WasmEdge_RefType_FuncRef, TabLimit);
WasmEdge_TableInstanceContext *HostTable =
    WasmEdge_TableInstanceCreate(HostTType);
WasmEdge_TableTypeDelete(HostTType);

/* Create a host memory exported as "memory". */
WasmEdge_Limit MemLimit = {.HasMax = true, .Shared = false, .Min = 1, .Max = 2};
WasmEdge_MemoryTypeContext *HostMType = WasmEdge_MemoryTypeCreate(MemLimit);
WasmEdge_MemoryInstanceContext *HostMemory =
    WasmEdge_MemoryInstanceCreate(HostMType);
WasmEdge_MemoryTypeDelete(HostMType);

/* Create a host global exported as "global_i32" and initialized as `666`. */
WasmEdge_GlobalTypeContext *HostGType =
    WasmEdge_GlobalTypeCreate(WasmEdge_ValType_I32, WasmEdge_Mutability_Const);
WasmEdge_GlobalInstanceContext *HostGlobal =
    WasmEdge_GlobalInstanceCreate(HostGType, WasmEdge_ValueGenI32(666));
WasmEdge_GlobalTypeDelete(HostGType);
```

## Host Modules

The host module is a module instance that contains `host functions`, `tables`, `memories`, and `globals`, the same as the WASM modules. Developers can use APIs to add the instances into a host module.
After registering the host modules into a `VM` or `Store` context, the exported instances in that modules can be imported by WASM modules when instantiating.

### Module Instance Creation

Module instance supplies exported module name.

```c
WasmEdge_String HostName = WasmEdge_StringCreateByCString("test");
WasmEdge_ModuleInstanceContext *HostMod =
    WasmEdge_ModuleInstanceCreate(HostName);
WasmEdge_StringDelete(HostName);
```

### Add Instances

Developers can add the `host functions`, `tables`, `memories`, and `globals` into the module instance with the export name.
After adding to the module, the ownership of the instances is moved into the module. Developers should __NOT__ access or destroy them.

```c
/* Add the host function created above with the export name "add". */
HostName = WasmEdge_StringCreateByCString("add");
WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
WasmEdge_StringDelete(HostName);

/* Add the table created above with the export name "table". */
HostName = WasmEdge_StringCreateByCString("table");
WasmEdge_ModuleInstanceAddTable(HostMod, HostName, HostTable);
WasmEdge_StringDelete(HostName);

/* Add the memory created above with the export name "memory". */
HostName = WasmEdge_StringCreateByCString("memory");
WasmEdge_ModuleInstanceAddMemory(HostMod, HostName, HostMemory);
WasmEdge_StringDelete(HostName);

/* Add the global created above with the export name "global_i32". */
HostName = WasmEdge_StringCreateByCString("global_i32");
WasmEdge_ModuleInstanceAddGlobal(HostMod, HostName, HostGlobal);
WasmEdge_StringDelete(HostName);
```

### Register Host Modules to WasmEdge

For importing the host functions in WASM, developers can register the host modules into a `VM` or `Store` context.

```c
WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(NULL, NULL);

/* Register the module instance into the store. */
WasmEdge_Result Res =
    WasmEdge_ExecutorRegisterImport(ExecCxt, StoreCxt, HostModCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("Host module registration failed: %s\n",
         WasmEdge_ResultGetMessage(Res));
  return -1;
}
/*
 * Developers can register the host module into a VM context by the
 * `WasmEdge_VMRegisterModuleFromImport()` API.
 */
/*
 * The owner of the host module will not be changed. Developers can register
 * the host module into several VMs or stores.
 */

/* Although being registered, the host module should be destroyed. */
WasmEdge_StoreDelete(StoreCxt);
WasmEdge_ExecutorDelete(ExecCxt);
WasmEdge_ModuleInstanceDelete(HostModCxt);
```

## Host Function Body Implementation Tips

There are some tips about implementing the host functions.

### Calling Frame Context

The `WasmEdge_CallingFrameContext` is the context to provide developers to access the module instance of the [frame on the top of the calling stack](https://webassembly.github.io/spec/core/exec/runtime.html#activations-and-frames).
According to the [WASM spec](https://webassembly.github.io/spec/core/exec/instructions.html#function-calls), a frame with the module instance to which the caller function belonging is pushed into the stack when invoking a function.
Therefore, the host functions can access the module instance of the top frame to retrieve the memory instances to read/write data.

```c
/* Host function body definition. */
WasmEdge_Result LoadOffset(void *Data,
                           const WasmEdge_CallingFrameContext *CallFrameCxt,
                           const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* Function type: {i32} -> {} */
  uint32_t Offset = (uint32_t)WasmEdge_ValueGetI32(In[0]);
  uint32_t Num = 0;

  /* Get the 0th memory instance of the module of the top frame on the stack. */
  /*
   * Noticed that the `MemCxt` will be `NULL` if there's no memory instance in
   * the module instance on the top frame.
   */
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

### Return Error Codes

Usually, the host function in WasmEdge can return the `WasmEdge_Result_Success` to present the successful execution.
For presenting the host function execution failed, one way is to return a trap with the error code.
Then the WasmEdge runtime will cause the trap in WASM and return that error.

*Note: We don't recommend using system calls such as `exit()`. That will shut down the whole WasmEdge runtime.*

For simply generating the trap, developers can return the `WasmEdge_Result_Fail`. If developers call the `WasmEdge_ResultOK()` with the returned result, they will get `false`. If developers call the `WasmEdge_ResultGetCode()` with the returned result, they will always get `2`.

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

### Host Data

The third parameter of the `WasmEdge_FunctionInstanceCreate()` API is for the host data as the type `void *`.
Developers can pass the data into the host functions when creating. Then in the host function body, developers can access the data from the first argument.
Developers should guarantee that the availability of the host data should be longer than the host functions.

```c
/* Host function body definition. */
WasmEdge_Result PrintData(void *Data,
                          const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* Function type: {} -> {} */
  printf("Data: %lf\n", *(double *)Data);
  return WasmEdge_Result_Success;
}

/* The host data. */
double Number = 0.0f;

/* Create a function type: {} -> {}. */
WasmEdge_FunctionTypeContext *HostFType =
    WasmEdge_FunctionTypeCreate(NULL, 0, NULL, 0);
/* Create a function context with the function type and host function body. */
WasmEdge_FunctionInstanceContext *HostFunc =
    WasmEdge_FunctionInstanceCreate(HostFType, (void *)(&Number), NULL, 0);
WasmEdge_FunctionTypeDelete(HostType);
```

### Forcing Termination

Sometimes developers may want to terminate the WASM execution with the success status.
WasmEdge provides a method for terminating WASM execution in host functions.
Developers can return `WasmEdge_Result_Terminate` to trigger the forcing termination of the current execution.
If developers call the `WasmEdge_ResultOK()` with the returned result, they will get `true`. If developers call the `WasmEdge_ResultGetCode()` with the returned result, they will always get `1`.
