// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/getcalldatasize.h"

namespace SSVM {
namespace Compiler {

uint32_t EEIGetCallDataSize::run() { return Env.getCallData().size(); }

} // namespace Compiler
} // namespace SSVM
