// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC
//
#include "wasmedge/wincli_helper.h"
#if WASMEDGE_WINCLI_HELPER_ENABLE
#include "system/winapi.h"
#include <cwchar>

static const char *UTF16ToUTF8(const wchar_t *U16Str) {
  static const char *EmptyString = "";

  int U16StrLen = int(wcslen(U16Str));
  auto BufSize = winapi::WideCharToMultiByte(
      winapi::CP_UTF8_, 0, U16Str, U16StrLen, nullptr, 0, nullptr, nullptr);
  if (BufSize == 0) {
    return EmptyString;
  }
  // for '\0'
  ++BufSize;

  char *Buf = new char[static_cast<size_t>(BufSize)];

  winapi::WideCharToMultiByte(winapi::CP_UTF8_, 0, U16Str, U16StrLen, Buf,
                              BufSize, nullptr, nullptr);
  return Buf;
}

void WasmEdge_SetConsoleOutputCPtoUTF8() {
#if WINAPI_PARTITION_DESKTOP
  winapi::SetConsoleOutputCP(CP_UTF8_);
#endif
}
wchar_t *WasmEdge_GetCommandLineW() { return winapi::GetCommandLineW(); }

const char **WasmEdge_CommandLineToUTF8ArgvW(const wchar_t *CmdLine,
                                             int *Argc) {
  auto ArgvW = winapi::CommandLineToArgvW(CmdLine, Argc);

  if (ArgvW == nullptr)
    return nullptr;

  const char **Argv = new const char *[static_cast<size_t>(*Argc)];
  for (int I = 0; I < *Argc; I++) {
    Argv[I] = UTF16ToUTF8(ArgvW[I]);
  }

  winapi::LocalFree(ArgvW);
  return Argv;
}
#endif
