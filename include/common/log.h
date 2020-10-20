// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "easyloggingpp/easylogging++.h"

namespace SSVM {
namespace Log {

void passEasyloggingppArgs(int Argc, char *Argv[]);

void setErrorLoggingLevel();

extern el::base::type::StoragePointer elStorage;
extern el::base::debug::CrashHandler elCrashHandler;

} // namespace Log
} // namespace SSVM
