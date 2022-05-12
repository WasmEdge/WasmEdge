#include <chrono>
#include <iostream>
#include <cstdio>
#include <wasmedge/wasmedge.h>
#include "script/fibonacci.h"


void run_fib_wasm() {
  /* Create the configure context and add the WASI support. */
  /* This step is not necessary unless you need WASI support. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        WasmEdge_HostRegistration_Wasi);
  /* The configure and store context to the VM creation can be NULL. */
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

  /* The parameters and returns arrays. */
  WasmEdge_Value Params[1] = {WasmEdge_ValueGenI32(32)};
  WasmEdge_Value Returns[1];
  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
  /* Run the WASM function from file. */
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(
      VMCxt, "fibonacci.wasm", FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("Get result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }

  /* Resources deallocations. */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_StringDelete(FuncName);
}

void run_fib_native() {
  int32_t ret = fib(32);
  printf("Get result: %d\n", ret);
}

int main(int Argc, const char *Argv[]) {
  auto start = std::chrono::system_clock::now();

  run_fib_native();
  auto step = std::chrono::system_clock::now();
  std::chrono::duration<double> diff_native = step - start;
  std::cout << "run native fib(32), ints : " << diff_native.count() << " s\n";

  run_fib_wasm();
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> diff_wasm = end - step;
  std::cout << "run wasm fib(32), ints : " << diff_wasm.count() << " s\n";
  return 0;
}