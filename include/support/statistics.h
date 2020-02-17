// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "easyloggingpp/easylogging++.h"

#include <iostream>
#include <string>
#include <vector>

namespace SSVM {
namespace Support {

void passEasyloggingppArgs(int Argc, char *Argv[]);

extern el::base::type::StoragePointer elStorage;
extern el::base::debug::CrashHandler elCrashHandler;

} // namespace Support
} // namespace SSVM
