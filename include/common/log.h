// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/common/log.h - Logging system ----------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the linkage of logging system.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "easyloggingpp/easylogging++.h"

namespace WasmEdge {
namespace Log {

void passEasyloggingppArgs(int Argc, char *Argv[]);

void setDebugLoggingLevel();

void setErrorLoggingLevel();

extern el::base::type::StoragePointer elStorage;
extern el::base::debug::CrashHandler elCrashHandler;

} // namespace Log
} // namespace WasmEdge
