// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/tools/wasmedge/main.h - Tool main function ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the macro to define the main function of the tool
/// executables, which forwards the UTF-8 arguments to a driver entry point.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "wasmedge/wasmedge.h"

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#define WASMEDGE_DRIVER_MAIN(Entry)                                            \
  extern "C" int wmain(int Argc, const wchar_t *Argv[]);                       \
  int wmain(int Argc, const wchar_t *Argv[]) {                                 \
    WasmEdge_Driver_SetConsoleOutputCPtoUTF8();                                \
    auto NewArgv = WasmEdge_Driver_ArgvCreate(Argc, Argv);                     \
    const int Result = Entry(Argc, NewArgv);                                   \
    WasmEdge_Driver_ArgvDelete(NewArgv);                                       \
    return Result;                                                             \
  }
#else
#define WASMEDGE_DRIVER_MAIN(Entry)                                            \
  int main(int Argc, const char *Argv[]) { return Entry(Argc, Argv); }
#endif
