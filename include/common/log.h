// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/log.h - Logging system --------------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the linkage of logging system.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "easyloggingpp/easylogging++.h"

namespace SSVM {
namespace Log {

void passEasyloggingppArgs(int Argc, char *Argv[]);

void setDebugLoggingLevel();

void setErrorLoggingLevel();

extern el::base::type::StoragePointer elStorage;
extern el::base::debug::CrashHandler elCrashHandler;

} // namespace Log
} // namespace SSVM
