#include "wasmedge/wincli_helper.h"

#if WASMEDGE_WINCLI_HELPER_ENABLE
#include <Windows.h>

#include <string.h>

static const char *UTF16ToUTF8(const wchar_t *U16Str) {
  static const char *EmptyString = "";

  int U16StrLen = int(wcslen(U16Str));
  auto BufSize = WideCharToMultiByte(CP_UTF8, 0, U16Str, U16StrLen, nullptr, 0,
                                     nullptr, nullptr);
  if (BufSize == 0) {
    return EmptyString;
  }
  // for '\0'
  BufSize = BufSize + 1;

  char *Buf = new char[size_t(BufSize)];
  ::memset(Buf, 0, size_t(BufSize));

  WideCharToMultiByte(CP_UTF8, 0, U16Str, U16StrLen, Buf, BufSize, nullptr,
                      nullptr);
  return Buf;
}

void WasmEdge_SetConsoleOutputCPtoUTF8() { SetConsoleOutputCP(CP_UTF8); }
wchar_t *WasmEdge_GetCommandLineW() { return GetCommandLineW(); }

const char **WasmEdge_CommandLineToUTF8ArgvW(const wchar_t *CmdLine,
                                             int *Argc) {
  auto ArgvW = CommandLineToArgvW(CmdLine, Argc);

  if (ArgvW == nullptr)
    return nullptr;

  const char **Argv = new const char *[size_t(*Argc)];
  for (int I = 0; I < *Argc; I++) {
    Argv[I] = UTF16ToUTF8(ArgvW[I]);
  }

  LocalFree(ArgvW);
  return Argv;
}
#endif
