// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge.h"

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
extern "C" int wmain(int Argc, const wchar_t *Argv[]);
int wmain(int Argc, const wchar_t *Argv[]) {
  WasmEdge_Driver_SetConsoleOutputCPtoUTF8();
  auto NewArgv = WasmEdge_Driver_ArgvCreate(Argc, Argv);
  const int Result = WasmEdge_Driver_WasiNNRPCServer(Argc, NewArgv);
  WasmEdge_Driver_ArgvDelete(NewArgv);
  return Result;
}
#else
int main(int Argc, const char *Argv[]) {
  return WasmEdge_Driver_WasiNNRPCServer(Argc, Argv);
}
#endif
