# Customized External References

[External References](https://webassembly.github.io/spec/core/syntax/types.html#syntax-reftype) denotes an opaque and unforgettable reference to a host object. A new `externref` type can be passed into a Wasm module or returned from it. The Wasm module cannot reveal an `externref` value's bit pattern, nor create a fake host reference by an integer value.

## Tutorial

The following tutorial is the summary of the `externref` example in WasmEdge.

### Prepare Your Wasm File

The Wasm file should contain importing host functions that would take the `externref`.
Take [the test WASM file](https://github.com/WasmEdge/WasmEdge/raw/master/test/externref/externrefTestData/funcs.wasm) ([this WAT](https://github.com/WasmEdge/WasmEdge/blob/master/test/externref/externrefTestData/funcs.wat) is the corresponding text format) as an example:

```wasm
(module
  (type $t0 (func (param externref i32) (result i32)))
  (type $t1 (func (param externref i32 i32) (result i32)))
  (type $t2 (func (param externref externref i32 i32) (result i32)))
  (import "extern_module" "functor_square" (func $functor_square (type $t0)))
  (import "extern_module" "class_add" (func $class_add (type $t1)))
  (import "extern_module" "func_mul" (func $func_mul (type $t1)))
  (func $call_add (export "call_add") (type $t1) (param $p0 externref) (param $p1 i32) (param $p2 i32) (result i32)
    (call $class_add
      (local.get $p0)
      (local.get $p1)
      (local.get $p2)))
  (func $call_mul (export "call_mul") (type $t1) (param $p0 externref) (param $p1 i32) (param $p2 i32) (result i32)
    (call $func_mul
      (local.get $p0)
      (local.get $p1)
      (local.get $p2)))
  (func $call_square (export "call_square") (type $t0) (param $p0 externref) (param $p1 i32) (result i32)
    (call $functor_square
      (local.get $p0)
      (local.get $p1)))
  (func $call_add_square (export "call_add_square") (type $t2) (param $p0 externref) (param $p1 externref) (param $p2 i32) (param $p3 i32) (result i32)
    (call $functor_square
      (local.get $p1)
      (call $class_add
        (local.get $p0)
        (local.get $p2)
        (local.get $p3))))
  (memory $memory (export "memory") 1))
```

Users can convert `wat` to `wasm` through [wat2wasm](https://webassembly.github.io/wabt/demo/wat2wasm/) live tool. Noted that `reference types` checkbox should be checked on this page.

### Implement Host Module and Register into WasmEdge

The host module should be implemented and registered into WasmEdge before executing Wasm. Assume that the following code is saved as `main.c`:

```c
#include <wasmedge/wasmedge.h>

#include <stdio.h>

uint32_t SquareFunc(uint32_t A) { return A * A; }
uint32_t AddFunc(uint32_t A, uint32_t B) { return A + B; }
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

// Host function to call `SquareFunc` by external reference
WasmEdge_Result ExternSquare(void *Data,
                             const WasmEdge_CallingFrameContext *CallFrameCxt,
                             const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // Function type: {externref, i32} -> {i32}
  uint32_t (*Func)(uint32_t) = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t C = Func(WasmEdge_ValueGetI32(In[1]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}

// Host function to call `AddFunc` by external reference
WasmEdge_Result ExternAdd(void *Data,
                          const WasmEdge_CallingFrameContext *CallFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // Function type: {externref, i32, i32} -> {i32}
  uint32_t (*Func)(uint32_t, uint32_t) = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t C = Func(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}

// Host function to call `ExternMul` by external reference
WasmEdge_Result ExternMul(void *Data,
                          const WasmEdge_CallingFrameContext *CallFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // Function type: {externref, i32, i32} -> {i32}
  uint32_t (*Func)(uint32_t, uint32_t) = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t C = Func(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}

// Helper function to create the "extern_module" module instance.
WasmEdge_ModuleInstanceContext *CreateExternModule() {
  WasmEdge_String HostName;
  WasmEdge_FunctionTypeContext *HostFType = NULL;
  WasmEdge_FunctionInstanceContext *HostFunc = NULL;
  enum WasmEdge_ValType P[3], R[1];

  HostName = WasmEdge_StringCreateByCString("extern_module");
  WasmEdge_ModuleInstanceContext *HostMod =
      WasmEdge_ModuleInstanceCreate(HostName);
  WasmEdge_StringDelete(HostName);

  // Add host function "functor_square": {externref, i32} -> {i32}
  P[0] = WasmEdge_ValType_ExternRef;
  P[1] = WasmEdge_ValType_I32;
  R[0] = WasmEdge_ValType_I32;
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSquare, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("functor_square");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "class_add": {externref, i32, i32} -> {i32}
  P[2] = WasmEdge_ValType_I32;
  HostFType = WasmEdge_FunctionTypeCreate(P, 3, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternAdd, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("class_add");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "func_mul": {externref, i32, i32} -> {i32}
  HostFType = WasmEdge_FunctionTypeCreate(P, 3, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternMul, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("func_mul");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  return HostMod;
}

int main() {
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
  WasmEdge_ModuleInstanceContext *HostMod = CreateExternModule();
  WasmEdge_Value P[3], R[1];
  WasmEdge_String FuncName;
  WasmEdge_Result Res;

  Res = WasmEdge_VMRegisterModuleFromImport(VMCxt, HostMod);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Host module instance registration failed\n");
    return EXIT_FAILURE;
  }
  Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "funcs.wasm");
  if (!WasmEdge_ResultOK(Res)) {
    printf("WASM file loading failed\n");
    return EXIT_FAILURE;
  }
  Res = WasmEdge_VMValidate(VMCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("WASM validation failed\n");
    return EXIT_FAILURE;
  }
  Res = WasmEdge_VMInstantiate(VMCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("WASM instantiation failed\n");
    return EXIT_FAILURE;
  }

  // Test 1: call add -- 1234 + 5678
  P[0] = WasmEdge_ValueGenExternRef(AddFunc);
  P[1] = WasmEdge_ValueGenI32(1234);
  P[2] = WasmEdge_ValueGenI32(5678);
  FuncName = WasmEdge_StringCreateByCString("call_add");
  Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 3, R, 1);
  WasmEdge_StringDelete(FuncName);
  if (WasmEdge_ResultOK(Res)) {
    printf("Test 1 -- `call_add` -- 1234 + 5678 = %d\n",
           WasmEdge_ValueGetI32(R[0]));
  } else {
    printf("Test 1 -- `call_add` -- 1234 + 5678 -- failed\n");
    return EXIT_FAILURE;
  }

  // Test 2: call mul -- 789 * 4321
  P[0] = WasmEdge_ValueGenExternRef(MulFunc);
  P[1] = WasmEdge_ValueGenI32(789);
  P[2] = WasmEdge_ValueGenI32(4321);
  FuncName = WasmEdge_StringCreateByCString("call_mul");
  Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 3, R, 1);
  WasmEdge_StringDelete(FuncName);
  if (WasmEdge_ResultOK(Res)) {
    printf("Test 2 -- `call_mul` -- 789 * 4321 = %d\n",
           WasmEdge_ValueGetI32(R[0]));
  } else {
    printf("Test 2 -- `call_mul` -- 789 * 4321 -- failed\n");
    return EXIT_FAILURE;
  }

  // Test 3: call square -- 8256^2
  P[0] = WasmEdge_ValueGenExternRef(SquareFunc);
  P[1] = WasmEdge_ValueGenI32(8256);
  FuncName = WasmEdge_StringCreateByCString("call_square");
  Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 2, R, 1);
  if (WasmEdge_ResultOK(Res)) {
    printf("Test 3 -- `call_mul` -- 8256 ^ 2 = %d\n",
           WasmEdge_ValueGetI32(R[0]));
  } else {
    printf("Test 3 -- `call_mul` -- 8256 ^ 2 -- failed\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
```

### Setup the Environment And Compile

1. Install the WasmEdge shared library.

    Please refer to the [Installation](../../quick_start/install.md) for details.

2. Prepare the WASM file and the `main.c` source file as above.

3. Compile

    ```bash
    gcc main.c -lwasmedge
    # Or you can use g++ for the C++ case, or use the clang.
    ```

4. Run the Test

    ```bash
    $ ./a.out
    Test 1 -- `call_add` -- 1234 + 5678 = 6912
    Test 2 -- `call_mul` -- 789 * 4321 = 3409269
    Test 3 -- `call_mul` -- 8256 ^ 2 = 68161536
    ```

## Wasm module with External References

Take the following `wat` for example:

```wasm
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

## WasmEdge ExternRef Example

The following examples are how to use `externref` in Wasm with WasmEdge C API.

### Wasm Code

The Wasm code must pass the `externref` to host functions that want to access it.
Take the following `wat` for example, which is a part of [the test WASM file](https://github.com/WasmEdge/WasmEdge/blob/master/test/externref/externrefTestData/funcs.wat):

```wasm
(module
  (type $t0 (func (param externref i32 i32) (result i32)))
  (import "extern_module" "func_mul" (func $func_mul (type $t0)))
  (func $call_mul (export "call_mul") (type $t0) (param $p0 externref) (param $p1 i32) (param $p2 i32) (result i32)
    (call $func_mul (local.get $p0) (local.get $p1) (local.get $p2))
  )
  (memory $memory (export "memory") 1))
```

The host function "`extern_module::func_mul`" takes `externref` as a function pointer to multiply parameters 1 and 2 and then returns the result. The exported Wasm function "`call_mul`" calls "`func_mul`" and passes the `externref` and 2 numbers as arguments.

### Host Functions

To instantiate the above example Wasm, the host functions must be registered into WasmEdge. See [Host Functions](ref.md#host-functions) for more details.
The host functions which take `externref`s must know the original objects' types. We take the function pointer case for example.

```c
/* Function to pass as function pointer. */
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

/* Host function to call the function by external reference as a function pointer */
WasmEdge_Result ExternMul(void *, const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* Function type: {externref, i32, i32} -> {i32} */
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t (*Obj)(uint32_t, uint32_t) = Ptr;
  /*
   * For C++, the `reinterpret_cast` is needed:
   * uint32_t (*Obj)(uint32_t, uint32_t) = 
   *   *reinterpret_cast<uint32_t (*)(uint32_t, uint32_t)>(Ptr);
   */
  uint32_t C = Obj(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}
```

"`MulFunc`" is a function that will be passed into Wasm as `externref`. In the "`func_mul`" host function, users can use "`WasmEdge_ValueGetExternRef`" API to get the pointer from the `WasmEdge_Value` which contains a `externref`.

Developers can add the host functions with names into a module instance.

```c
/* Create a module instance. */
WasmEdge_String HostName = WasmEdge_StringCreateByCString("extern_module");
WasmEdge_ModuleInstanceContext *HostMod =
    WasmEdge_ModuleInstanceCreate(HostName);
WasmEdge_StringDelete(HostName);

/* Create a function instance and add into the module instance. */
enum WasmEdge_ValType P[3], R[1];
P[0] = WasmEdge_ValType_ExternRef;
P[1] = WasmEdge_ValType_I32;
P[2] = WasmEdge_ValType_I32;
R[0] = WasmEdge_ValType_I32;
WasmEdge_FunctionTypeContext *HostFType =
    WasmEdge_FunctionTypeCreate(P, 3, R, 1);
WasmEdge_FunctionInstanceContext *HostFunc =
    WasmEdge_FunctionInstanceCreate(HostFType, ExternFuncMul, NULL, 0);
WasmEdge_FunctionTypeDelete(HostFType);
HostName = WasmEdge_StringCreateByCString("func_mul");
WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
WasmEdge_StringDelete(HostName);

...
```

### Execution

Take [the test WASM file](https://github.com/WasmEdge/WasmEdge/raw/master/test/externref/externrefTestData/funcs.wasm) ([this WAT](https://github.com/WasmEdge/WasmEdge/blob/master/test/externref/externrefTestData/funcs.wat) is the corresponding text format) for example.
Assume that the `funcs.wasm` is copied into the current directory.
The following is the example to execute WASM with `externref` through the WasmEdge C API.

```c
/* Create the VM context. */
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
/* Create the module instance context that contains the host functions. */
WasmEdge_ModuleInstanceContext *HostMod = /* Ignored ... */;
/* Assume that the host functions are added to the module instance above. */
WasmEdge_Value P[3], R[1];
WasmEdge_String FuncName;
WasmEdge_Result Res;

/* Register the module instance into VM. */
Res = WasmEdge_VMRegisterModuleFromImport(VMCxt, HostMod);
if (!WasmEdge_ResultOK(Res)) {
  printf("Import object registration failed\n");
  return EXIT_FAILURE;
}
/* Load WASM from the file. */
Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "funcs.wasm");
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM file loading failed\n");
  return EXIT_FAILURE;
}
/* Validate WASM. */
Res = WasmEdge_VMValidate(VMCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM validation failed\n");
  return EXIT_FAILURE;
}
/* Instantiate the WASM module. */
Res = WasmEdge_VMInstantiate(VMCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM instantiation failed\n");
  return EXIT_FAILURE;
}

/* Run a WASM function. */
P[0] = WasmEdge_ValueGenExternRef(AddFunc);
P[1] = WasmEdge_ValueGenI32(1234);
P[2] = WasmEdge_ValueGenI32(5678);
/* Run the `call_add` function. */
FuncName = WasmEdge_StringCreateByCString("call_add");
Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 3, R, 1);
WasmEdge_StringDelete(FuncName);
if (WasmEdge_ResultOK(Res)) {
  printf("Run -- `call_add` -- 1234 + 5678 = %d\n",
          WasmEdge_ValueGetI32(R[0]));
} else {
  printf("Run -- `call_add` -- 1234 + 5678 -- failed\n");
  return EXIT_FAILURE;
}
```

## Passing Objects

The above example is passing a function reference as `externref`. The following examples are about how to pass an object reference into WASM as `externref` in C++.

### Passing a Class

To pass a class as `externref`, the object instance is needed.

```cpp
class AddClass {
public:
  uint32_t add(uint32_t A, uint32_t B) const { return A + B; }
};

AddClass AC;
```

Then users can pass the object into WasmEdge by using `WasmEdge_ValueGenExternRef()` API.

```cpp
WasmEdge_Value P[3], R[1];
P[0] = WasmEdge_ValueGenExternRef(&AC);
P[1] = WasmEdge_ValueGenI32(1234);
P[2] = WasmEdge_ValueGenI32(5678);
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_add");
WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 3, R, 1);
WasmEdge_StringDelete(FuncName);
if (WasmEdge_ResultOK(Res)) {
  std::cout << "Result : " << WasmEdge_ValueGetI32(R[0]) std::endl;
  // Will print `6912`.
} else {
  return EXIT_FAILURE;
}
```

In the host function which would access the object by reference, users can use the `WasmEdge_ValueGetExternRef()` API to retrieve the reference to the object.

```cpp
// Modify the `ExternAdd` in the above tutorial.
WasmEdge_Result ExternAdd(void *, const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // Function type: {externref, i32, i32} -> {i32}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  AddClass &Obj = *reinterpret_cast<AddClass *>(Ptr);
  uint32_t C =
      Obj.add(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}
```

### Passing an Object As Functor

As the same as passing a class instance, the functor object instance is needed.

```cpp
struct SquareStruct {
  uint32_t operator()(uint32_t Val) const { return Val * Val; }
};

SquareStruct SS;
```

Then users can pass the object into WasmEdge by using the `WasmEdge_ValueGenExternRef()` API.

```cpp
WasmEdge_Value P[2], R[1];
P[0] = WasmEdge_ValueGenExternRef(&SS);
P[1] = WasmEdge_ValueGenI32(1024);
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_square");
WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 2, R, 1);
WasmEdge_StringDelete(FuncName);
if (WasmEdge_ResultOK(Res)) {
  std::cout << "Result : " << WasmEdge_ValueGetI32(R[0]) std::endl;
  // Will print `1048576`.
} else {
  return EXIT_FAILURE;
}
```

In the host function which would access the object by reference, users can use the `WasmEdge_ValueGetExternRef` API to retrieve the reference to the object, and the reference is a functor.

```cpp
// Modify the `ExternSquare` in the above tutorial.
WasmEdge_Result ExternSquare(void *, const WasmEdge_CallingFrameContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // Function type: {externref, i32, i32} -> {i32}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  SquareStruct &Obj = *reinterpret_cast<SquareStruct *>(Ptr);
  uint32_t C = Obj(WasmEdge_ValueGetI32(In[1]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}
```

### Passing STL Objects

The [example Wasm binary](https://github.com/WasmEdge/WasmEdge/raw/master/test/externref/externrefTestData/stl.wasm) ([this WAT](https://github.com/WasmEdge/WasmEdge/blob/master/test/externref/externrefTestData/stl.wat) is the corresponding text format) provides functions to interact with host functions which can access C++ STL objects.
Assume that the WASM file `stl.wasm` is copied into the current directory.

Take the `std::ostream` and `std::string` objects for example. Assume that there's a host function accesses to a `std::ostream` and a `std::string` through `externref`s:

```cpp
// Host function to output std::string through std::ostream
WasmEdge_Result ExternSTLOStreamStr(void *,
                                    const WasmEdge_CallingFrameContext *,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *) {
  // Function type: {externref, externref} -> {}
  void *Ptr0 = WasmEdge_ValueGetExternRef(In[0]);
  void *Ptr1 = WasmEdge_ValueGetExternRef(In[1]);
  std::ostream &RefOS = *reinterpret_cast<std::ostream *>(Ptr0);
  std::string &RefStr = *reinterpret_cast<std::string *>(Ptr1);
  RefOS << RefStr;
  return WasmEdge_Result_Success;
}
```

Assume that the above host function is added to the module instance `HostMod`, and the `HostMod` is registered into a VM context `VMCxt`.
Then users can instantiate the Wasm module:

```cpp
WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "stl.wasm");
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM file loading failed\n");
  return EXIT_FAILURE;
}
Res = WasmEdge_VMValidate(VMCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM validation failed\n");
  return EXIT_FAILURE;
}
Res = WasmEdge_VMInstantiate(VMCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM instantiation failed\n");
  return EXIT_FAILURE;
}
```

Last, pass the `std::cout` and a `std::string` object by external references.

```cpp
std::string PrintStr("Hello world!");
WasmEdge_Value P[2], R[1];
P[0] = WasmEdge_ValueGenExternRef(&std::cout);
P[1] = WasmEdge_ValueGenExternRef(&PrintStr);
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_ostream_str");
WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 2, R, 1);
// Will print "Hello world!" to stdout.
WasmEdge_StringDelete(FuncName);
if (!WasmEdge_ResultOK(Res)) {
  return EXIT_FAILURE;
}
```

For other C++ STL objects cases, such as `std::vector<T>`, `std::map<T, U>`, or `std::set<T>`, the object can be accessed correctly in host functions if the type in `reinterpret_cast` is correct.
