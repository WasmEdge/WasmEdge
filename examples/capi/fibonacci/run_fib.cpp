#include <cstdio>
#include <cstdlib>
#include <wasmedge/wasmedge.h>

int main(int Argc, const char *Argv[]) {
  const char *WasmFile = "../../wasm/fibonacci.wasm";
  int N = 8;
  if (Argc > 1) {
    N = std::atoi(Argv[1]);
  }

  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(nullptr, nullptr);
  WasmEdge_Value Params[1] = {WasmEdge_ValueGenI32(N)};
  WasmEdge_Value Returns[1];
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");

  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(
      VMCxt, WasmFile, FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("fib(%d) = %d\n", N, WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error: %s\n", WasmEdge_ResultGetMessage(Res));
    WasmEdge_StringDelete(FuncName);
    WasmEdge_VMDelete(VMCxt);
    return 1;
  }

  WasmEdge_StringDelete(FuncName);
  WasmEdge_VMDelete(VMCxt);
  return 0;
}
