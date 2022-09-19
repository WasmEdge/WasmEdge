// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/wincli_helper.h - WasmEdge Windows CLI Helper-------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the help function to parse cli arguments under Windows.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_WINCLI_HELPER
#define WASMEDGE_WINCLI_HELPER
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#define WASMEDGE_WINCLI_HELPER_ENABLE 1

void WasmEdge_SetConsoleOutputCPtoUTF8();
wchar_t *WasmEdge_GetCommandLineW();
const char **WasmEdge_CommandLineToUTF8ArgvW(const wchar_t *CmdLine, int *Argc);
#endif
#else
#define WASMEDGE_WINCLI_HELPER_ENABLE 0
#endif
