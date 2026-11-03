// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "common/errcode.h"

namespace WasmEdge {
namespace LLVM {

class Module;

enum class ARMFloatABI { PureSoft, SoftFP, Hard };

struct ARMRuntimeLibcallProfile {
  bool ARM32;
  bool Linux;
  bool EABI;
  bool ARMv7OrLater;
  bool VFP;
  ARMFloatABI FloatABI;
};

ARMRuntimeLibcallProfile hostARMRuntimeLibcallProfile() noexcept;
bool supportsARMRuntimeLibcalls(
    const ARMRuntimeLibcallProfile &Profile) noexcept;
Expect<void>
addARMRuntimeLibcalls(Module &LLModule,
                      const ARMRuntimeLibcallProfile &Profile) noexcept;
Expect<void> validateARMRuntimeLibcallsForAOT(
    const ARMRuntimeLibcallProfile &Profile) noexcept;

} // namespace LLVM
} // namespace WasmEdge
