# C SDK

WasmEdge C API 提供了應用程式接口將 WasmEdge 嵌入至 C 程式中。以下内容是如何使用 WasmEdge C API 的簡單說明。如果要了解 WasmEdge C API 的更多細節，請參考[完整的說明文件](c/ref.md)。

## WasmEdge 執行 Wasm 快速指南

以下是執行一個 WASM 檔案的範例。
假設 WASM 檔案 [`fibonacci.wasm`](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) 已經被複製到當前資料夾，並且 C 檔案 `test_wasmedge.c` 的原始碼如下：

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main(int Argc, const char* Argv[]) {
  /* 建立 configure context 以及增加 WASI 支援。 */
  /* 除非您需要使用 WASI ，否則可以省略這一步。 */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
  /* 建立 VM context 的時候， store 與 configure 可以傳入 NULL 。 */
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

  /* 參數列及回傳值列。 */
  WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(32) };
  WasmEdge_Value Returns[1];
  /* 要呼叫的函式名稱。 */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
  /* 執行 Wasm 檔案中的函式。 */
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VMCxt, Argv[1], FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("Get result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }

  /* 清除佔用資源。 */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_StringDelete(FuncName);
  return 0;
}
```

接下來您可以編譯並執行：（索引從 0 開始的情况下， Fibonacci 數列的第 32 個數值是 3524578 ）

```bash
$ gcc test_wasmedge.c -lwasmedge -o test_wasmedge
$ ./test_wasmedge fibonacci.wasm
Get result: 3524578
```

如果要了解 API 的細節，請參考 [API 標頭檔](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge/wasmedge.h).

## WasmEdge AOT 編譯器快速指南

假設 WASM 檔案 [`fibonacci.wasm`](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) 已經被複製到當前資料夾，並且 C 檔案 `test_wasmedge_compiler.c` 的原始碼如下：

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main(int Argc, const char* Argv[]) {
  /* 建立 configure context 。 */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  /* ... 可以調整 configure context 的設定。 */
  /* 結果資料結構。 */
  WasmEdge_Result Res;

  /* 建立 compiler context 。 Configure context 可以為 NULL 。 */
  WasmEdge_CompilerContext *CompilerCxt = WasmEdge_CompilerCreate(ConfCxt);
  /* 根據命令列參數來指定輸入輸出檔案位置。 */
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

接下來可以編譯並執行（輸出的檔案是 "fibonacci.wasm.so" ）：

```bash
$ gcc test_wasmedge_compiler.c -lwasmedge -o test_wasmedge_compiler
$ ./test_wasmedge_compiler fibonacci.wasm fibonacci.wasm.so
[2021-07-02 11:08:08.651] [info] compile start
[2021-07-02 11:08:08.653] [info] verify start
[2021-07-02 11:08:08.653] [info] optimize start
[2021-07-02 11:08:08.670] [info] codegen start
[2021-07-02 11:08:08.706] [info] compile done
```

編譯後的 WASM 檔案也可以直接使用 WasmEdge 執行。
以下是直譯模式和 AOT 模式的比較：

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

如果要了解 API 的細節，請參考 [API 標頭檔](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge/wasmedge.h)。
