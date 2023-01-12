# 自定义外部引用

[外部引用](https://webassembly.github.io/reference-types/core/syntax/types.html#syntax-reftype)指示对 host 对象的不透明且不可伪造的引用。新的 `externref` 类型可以被传递到 Wasm 模块或从 Wasm 模块返回。Wasm 模块不能显示 `externref` 值的位模式，也不能通过整数值创建一个假的 host 引用。

## 教程

下面的教程是对 WasmEdge 中的 `externref` 示例的总结。

### 准备 Wasm 文件

Wasm 文件应该包含接受 `externref` 的 host 函数导入。以[这个 WASM 测试用例](../test/externref/externrefTestData/funcs.wasm)为例（[WAT](../test/externref/externrefTestData/funcs.wat)是相对应的文本格式）。

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

你可以通过 [wat2wasm](https://webassembly.github.io/wabt/demo/wat2wasm/) 在线实时工具将 `wat` 转换为 `wasm`。注意页面上的 `reference types` 复选框必须勾选。

### 实现 Host 模块以及在 WasmEdge 中注册

在执行 Wasm 执行前，host 模块必须实现以及在 WasmEdge 中注册。假设下面的代码保存为 `main.c`：

```cpp
#include <wasmedge/wasmedge.h>

#include <stdio.h>

uint32_t SquareFunc(uint32_t A) { return A * A; }
uint32_t AddFunc(uint32_t A, uint32_t B) { return A + B; }
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

// Host 函数通过外部引用调用 `SquareFunc`
WasmEdge_Result ExternSquare(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                             const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // 函数类型: {externref, i32} -> {i32}
  uint32_t (*Func)(uint32_t) = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t C = Func(WasmEdge_ValueGetI32(In[1]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}

// Host 函数通过外部引用调用 `AddFunc`
WasmEdge_Result ExternAdd(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // 函数类型: {externref, i32, i32} -> {i32}
  uint32_t (*Func)(uint32_t, uint32_t) = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t C = Func(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}

// host 函数通过外部引用调用 `ExternMul`
WasmEdge_Result ExternMul(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // 函数类型: {externref, i32, i32} -> {i32}
  uint32_t (*Func)(uint32_t, uint32_t) = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t C = Func(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}

// 创建“extern_module”引入对象的辅助函数。
WasmEdge_ImportObjectContext *CreateExternModule() {
  WasmEdge_String HostName;
  WasmEdge_FunctionTypeContext *HostFType = NULL;
  WasmEdge_FunctionInstanceContext *HostFunc = NULL;
  enum WasmEdge_ValType P[3], R[1];

  HostName = WasmEdge_StringCreateByCString("extern_module");
  WasmEdge_ImportObjectContext *ImpObj = WasmEdge_ImportObjectCreate(HostName);
  WasmEdge_StringDelete(HostName);

  // 添加 host 函数 "functor_square": {externref, i32} -> {i32}
  P[0] = WasmEdge_ValType_ExternRef;
  P[1] = WasmEdge_ValType_I32;
  R[0] = WasmEdge_ValType_I32;
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSquare, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("functor_square");
  WasmEdge_ImportObjectAddFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // 添加 host 函数 "class_add": {externref, i32, i32} -> {i32}
  P[2] = WasmEdge_ValType_I32;
  HostFType = WasmEdge_FunctionTypeCreate(P, 3, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternAdd, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("class_add");
  WasmEdge_ImportObjectAddFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // 添加 host 函数 "func_mul": {externref, i32, i32} -> {i32}
  HostFType = WasmEdge_FunctionTypeCreate(P, 3, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternMul, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("func_mul");
  WasmEdge_ImportObjectAddFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  return ImpObj;
}

int main() {
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
  WasmEdge_ImportObjectContext *ImpObj = CreateExternModule();
  WasmEdge_Value P[3], R[1];
  WasmEdge_String FuncName;
  WasmEdge_Result Res;

  Res = WasmEdge_VMRegisterModuleFromImport(VMCxt, ImpObj);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Import object registration failed\n");
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

  // 测试用例 1：调用 add -- 1234 + 5678
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

  // 测试用例 2：调用 mul -- 789 * 4321
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

  // 测试用例 3：调用 square -- 8256^2
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

### 设置环境并编译

1. 安装 WasmEdge 共享库。

    详细信息请参阅[安装](install.md)。

2. 如上所示准备 WASM 文件和 `main.c` 源文件。

3. 编译

    ```bash
    gcc main.c -lwasmedge
    # 或者你可以在 C++ 场景下使用 g++，或者使用 clang。
    ```

4. 运行测试用例

    ```bash
    $ ./a.out
    Test 1 -- `call_add` -- 1234 + 5678 = 6912
    Test 2 -- `call_mul` -- 789 * 4321 = 3409269
    Test 3 -- `call_mul` -- 8256 ^ 2 = 68161536
    ```

## 使用外部引用的  Wasm

以下文的 `wat` 为例：

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

Wasm 函数 "`call_square`" 包含一个 `externref` 参数，并用这个 `externref` 调用导入的 host 函数 `functor_square`。所以，当你调用 Wasm 函数 `call_square` 并传递对象的引用时，host 函数 `functor_square` 就能获得该对应引用。

## WasmEdge ExternRef 示例

下面的例子展示了如何在 Wasm 里通过 WasmEdge C API 使用 `externref`。

### Wasm 代码

Wasm 代码必须将 `externref` 传递给想要访问它的 host 函数。以下文的 `wat` 为例，这是 [WASM 测试用例](../test/externref/externrefTestData/funcs.wat)的其中一部分：

```wasm
(module
  (type $t0 (func (param externref i32 i32) (result i32)))
  (import "extern_module" "func_mul" (func $func_mul (type $t0)))
  (func $call_mul (export "call_mul") (type $t0) (param $p0 externref) (param $p1 i32) (param $p2 i32) (result i32)
    (call $func_mul (local.get $p0) (local.get $p1) (local.get $p2))
  )
  (memory $memory (export "memory") 1))
```

host 函数 `extern_module::func_mul` 将 `externref` 作为一个函数指针用来将参数 1 和参数 2 相乘并返回其结果。输出的 Wasm 函数 `call_mul` 调用 `func_mul` 并传递 `externref` 和 2 个数字作为参数。

### Host 函数

要实例化上面的 Wasm 示例，host 函数必须注册到 WasmEdge 中。详见 [Host 函数](../../embed/c/ref.md#host-functions)。接受 `externref` 的 host 函数必须知道原始对象的类型。我们以函数指针为例。

```c
/* 作为函数指针传递的函数。 */
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

/* Host 函数通过外部引用作为函数指针来调用函数 */
WasmEdge_Result ExternMul(void *, WasmEdge_MemoryInstanceContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /* 函数类型: {externref, i32, i32} -> {i32} */
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t (*Obj)(uint32_t, uint32_t) = Ptr;
  /*
   * 对于 C++ 来说，`reinterpret_cast` 是必需的：
   * uint32_t (*Obj)(uint32_t, uint32_t) = 
   *   *reinterpret_cast<uint32_t (*)(uint32_t, uint32_t)>(Ptr);
   */
  uint32_t C = Obj(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}
```

"`MulFunc`" 是一个函数，将作为 `externref` 传递给 Wasm。在 host 函数 "`func_mul`" 中，你可以使用 "`WasmEdge_ValueGetExternRef`" API 从包含 `externref` 的 `WasmEdge_Value` 中获取指针。

开发人员可以将带有名称的 host 函数添加到导入对象中。

```c
/* 创建一个导入对象。 */
WasmEdge_String HostName = WasmEdge_StringCreateByCString("extern_module");
WasmEdge_ImportObjectContext *ImpObj = WasmEdge_ImportObjectCreate(HostName);
WasmEdge_StringDelete(HostName);

/* 创建一个函数实例并添加到一个导入对象中。 */
enum WasmEdge_ValType P[3], R[1];
P[0] = WasmEdge_ValType_ExternRef;
P[1] = WasmEdge_ValType_I32;
P[2] = WasmEdge_ValType_I32;
R[0] = WasmEdge_ValType_I32;
WasmEdge_FunctionTypeContext *HostFType = WasmEdge_FunctionTypeCreate(P, 3, R, 1);
WasmEdge_FunctionInstanceContext *HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternFuncMul, NULL, 0);
WasmEdge_FunctionTypeDelete(HostFType);
HostName = WasmEdge_StringCreateByCString("func_mul");
WasmEdge_ImportObjectAddFunction(ImpObj, HostName, HostFunc);
WasmEdge_StringDelete(HostName);

...
```

### 执行

以[这个 WASM 测试用例](../test/externref/externrefTestData/funcs.wasm)为例（[WAT](../test/externref/externrefTestData/funcs.wat)是相对应的文本格式）。假设函数 `funcs.wasm` 被复制到了当前目录。下面的例子展示了如何在 Wasm 里用 WasmEdge C API 使用 `externref`。

```c
/* 创建 VM 上下文。 */
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
/* 创建包含 host 函数的导入对象上下文。 */
WasmEdge_ImportObjectContext *ImpObj = /* Ignored ... */;
/* 假设 host 函数被添加到上面的导入对象中。 */
WasmEdge_Value P[3], R[1];
WasmEdge_String FuncName;
WasmEdge_Result Res;

/* 将导入对象注册到 VM。 */
Res = WasmEdge_VMRegisterModuleFromImport(VMCxt, ImpObj);
if (!WasmEdge_ResultOK(Res)) {
  printf("Import object registration failed\n");
  return EXIT_FAILURE;
}
/* 从文件中加载 WASM。 */
Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "funcs.wasm");
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM file loading failed\n");
  return EXIT_FAILURE;
}
/* 验证 WASM。 */
Res = WasmEdge_VMValidate(VMCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM validation failed\n");
  return EXIT_FAILURE;
}
/* 实例化 WASM 模块。 */
Res = WasmEdge_VMInstantiate(VMCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("WASM instantiation failed\n");
  return EXIT_FAILURE;
}

/* 运行 WASM 函数。 */
P[0] = WasmEdge_ValueGenExternRef(AddFunc);
P[1] = WasmEdge_ValueGenI32(1234);
P[2] = WasmEdge_ValueGenI32(5678);
/* 运行 `call_add` 函数。 */
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

## 传递对象

上面的例子是传递一个函数引用 `externref`。下面的例子是关于如何在 C++ 中将对象引用作为 `externref` 传递给 WASM 的。

### 传递一个类

将一个类通过 `externref` 传递，对象实例是必须的。

```cpp
class AddClass {
public:
  uint32_t add(uint32_t A, uint32_t B) const { return A + B; }
};

AddClass AC;
```

然后你可以通过使用 `WasmEdge_ValueGenExternRef()` API 将对象传入 WasmEdge。

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
  // 将会打印 `6912`.
} else {
  return EXIT_FAILURE;
}
```

在通过引用访问对象的 host 函数中，你可以使用 `WasmEdge_ValueGetExternRef()` API 来检索对对象的引用。

```cpp
// 修改上面的教程中的 `ExternAdd`。
WasmEdge_Result ExternAdd(void *, WasmEdge_MemoryInstanceContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // 函数类型：{externref, i32, i32} -> {i32}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  AddClass &Obj = *reinterpret_cast<AddClass *>(Ptr);
  uint32_t C =
      Obj.add(WasmEdge_ValueGetI32(In[1]), WasmEdge_ValueGetI32(In[2]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}
```

### 传递一个对象作为仿函数

与传递类实例一样，需要 functor 对象实例。

```cpp
struct SquareStruct {
  uint32_t operator()(uint32_t Val) const { return Val * Val; }
};

SquareStruct SS;
```

然后你可以通过使用 `WasmEdge_ValueGenExternRef()` API 将对象传递到 WasmEdge。

```cpp
WasmEdge_Value P[2], R[1];
P[0] = WasmEdge_ValueGenExternRef(&SS);
P[1] = WasmEdge_ValueGenI32(1024);
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_square");
WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 2, R, 1);
WasmEdge_StringDelete(FuncName);
if (WasmEdge_ResultOK(Res)) {
  std::cout << "Result : " << WasmEdge_ValueGetI32(R[0]) std::endl;
  // 将会打印 `1048576`.
} else {
  return EXIT_FAILURE;
}
```

在通过引用访问对象的 host 函数中，你可以使用 `WasmEdge_ValueGetExternRef` API 来检索对该对象的引用，而该引用是一个 functor。

```cpp
// 修改上文教程中的 `ExternSquare`。
WasmEdge_Result ExternSquare(void *, WasmEdge_MemoryInstanceContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // 函数类型：{externref, i32, i32} -> {i32}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  SquareStruct &Obj = *reinterpret_cast<SquareStruct *>(Ptr);
  uint32_t C = Obj(WasmEdge_ValueGetI32(In[1]));
  Out[0] = WasmEdge_ValueGenI32(C);
  return WasmEdge_Result_Success;
}
```

### 传递 STL 对象

[example Wasm binary](../test/externref/externrefTestData/stl.wasm) 提供了与 host 函数交互的函数可以访问 C++ STL 对象的示例（[WAT](../test/externref/externrefTestData/stl.wat) 是相对应的文本格式）。

假设 WASM 文件 `stl.wasm` 被复制到了当前目录。

以下文的 `std::ostream` 和 `std::string` 为例。假设有一个 host 函数可以通过 `externref` 访问 `std::ostream` 和 `std::string`。

```cpp
// Host 函数，通过 std::ostream 输出 std::string
WasmEdge_Result ExternSTLOStreamStr(void *, WasmEdge_MemoryInstanceContext *,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *) {
  // 函数类型：{externref, externref} -> {}
  void *Ptr0 = WasmEdge_ValueGetExternRef(In[0]);
  void *Ptr1 = WasmEdge_ValueGetExternRef(In[1]);
  std::ostream &RefOS = *reinterpret_cast<std::ostream *>(Ptr0);
  std::string &RefStr = *reinterpret_cast<std::string *>(Ptr1);
  RefOS << RefStr;
  return WasmEdge_Result_Success;
}
```

假设上面的 host 函数被添加到一个导入对象 `ImpObj` 中，并且 `ImpObj` 被注册到一个虚拟机上下文 `VMCxt` 中。然后你可以通过以下代码实例化 Wasm 模块：

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

最后，通过外部引用传递 `std::cout` 和 `std::string` 对象。

```cpp
std::string PrintStr("Hello world!");
WasmEdge_Value P[2], R[1];
P[0] = WasmEdge_ValueGenExternRef(&std::cout);
P[1] = WasmEdge_ValueGenExternRef(&PrintStr);
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_ostream_str");
WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt, FuncName, P, 2, R, 1);
// 将会打印 "Hello world!" 到标准输出 stdout.
WasmEdge_StringDelete(FuncName);
if (!WasmEdge_ResultOK(Res)) {
  return EXIT_FAILURE;
}
```

对于其它 C++ STL 对象的情况，例如 `std::vector<T>`，`std::map<T, U>` 或 `std::set<T>`，如果 `reinterpret_cast` 中的类型是正确的，该对象就可以在 host 函数中被正确访问到。
