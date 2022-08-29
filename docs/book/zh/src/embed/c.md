# WasmEdge C SDK

文档 [WasmEdge C API](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge/wasmedge.h) 里提供了访问 WasmEdge 运行时的接口。以下内容是关于如何使用 WasmEdge C API 的简单说明。如果要了解 WasmEdge C API 的更多细节，请参考[完整的文档](c/ref.md)。

## WasmEdge 运行时快速指南

以下是运行一个 WASM 文件的示例。
假设 WASM 文件[`fibonacci.wasm`](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm)已经被拷贝到当前目录，并且 C 文件 `test_wasmedge.c` 的源代码如下所示：

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main(int Argc, const char* Argv[]) {
  /* 创建配置上下文以及 WASI 支持。 */
  /* 除非你需要使用 WASI，否则这步不是必须的。 */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
  /* 创建VM的时候可以提供空的配置。*/
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

  /* 参数以及返回的数组。 */
  WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(32) };
  WasmEdge_Value Returns[1];
  /* 要调用的函数名。 */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
  /* 运行文件里的 WASM 函数。 */
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VMCxt, Argv[1], FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("Get result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }

  /* 资源析构。 */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_StringDelete(FuncName);
  return 0;
}
```

接下来你可以编译并且运行它：（索引从 0 开始的情况下，斐波那契数列的的第 32 个数值是 3524578 ）

```bash
$ gcc test_wasmedge.c -lwasmedge -o test_wasmedge
$ ./test_wasmedge fibonacci.wasm
Get result: 3524578
```

如果要了解 API 的细节，请参考 [API 头文件](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge/wasmedge.h).

## WasmEdge AOT 编译器快速指南

假设 WASM 文件 [`fibonacci.wasm`](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) 已经被拷贝到当前目录，并且 C 文件 `test_wasmedge_compiler.c` 的代码如下所示：

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main(int Argc, const char* Argv[]) {
  /* 创建配置上下文。 */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  /* ... 通过配置上下文调整配置。 */
  /* 结果。 */
  WasmEdge_Result Res;

  /* 创建编译器上下文。配置上下文可以为空。 */
  WasmEdge_CompilerContext *CompilerCxt = WasmEdge_CompilerCreate(ConfCxt);
  /* 根据指定的输入和输出路径来编译 WASM 文件。 */
  Res = WasmEdge_CompilerCompile(CompilerCxt, Argv[1], Argv[2]);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Compilation failed: %s\n", WasmEdge_ResultGetMessage(Res));
    return 1;
  }

  WasmEdge_CompilerDelete(CompilerCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  return 0;
}
```

接下来你可以编译和运行它（输出的文件是 "fibonacci.wasm.so" ）：

```bash
$ gcc test_wasmedge_compiler.c -lwasmedge -o test_wasmedge_compiler
$ ./test_wasmedge_compiler fibonacci.wasm fibonacci.wasm.so
[2021-07-02 11:08:08.651] [info] compile start
[2021-07-02 11:08:08.653] [info] verify start
[2021-07-02 11:08:08.653] [info] optimize start
[2021-07-02 11:08:08.670] [info] codegen start
[2021-07-02 11:08:08.706] [info] compile done
```

编译后的的 WASM 文件也可以直接作为 WasmEdge 运行时的输入。
以下是解释模式和 AOT 模式的一个对比：

```bash
$ time ./test_wasmedge fibonacci.wasm
Get result: 5702887

real 0m2.715s
user 0m2.700s
sys 0m0.008s

$ time ./test_wasmedge fibonacci.wasm.so
Get result: 5702887

real 0m0.036s
user 0m0.022s
sys 0m0.011s
```

如果要了解这些 API 的细节，请参考[API 头文件](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge/wasmedge.h)。
