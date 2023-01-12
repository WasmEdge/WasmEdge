// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "wasmedge/wasmedge.h"
#include "wasmedge/wincli_helper.h"

int main(int Argc, const char *Argv[]) {
#if WASMEDGE_WINCLI_HELPER_ENABLE
  WasmEdge_SetConsoleOutputCPtoUTF8();
  (void)Argv; // We can not ensure the encoding of Argv, make compiler silent.
  auto ArgvUTF8 =
      WasmEdge_CommandLineToUTF8ArgvW(WasmEdge_GetCommandLineW(), &Argc);
  return WasmEdge_Driver_Compiler(Argc, ArgvUTF8);
#else
  return WasmEdge_Driver_Compiler(Argc, Argv);
#endif
}
