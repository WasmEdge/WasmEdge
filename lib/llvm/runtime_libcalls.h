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
  bool ARMv7;
  bool VFP;
  ARMFloatABI FloatABI;
};

ARMRuntimeLibcallProfile hostARMRuntimeLibcallProfile() noexcept;
bool supportsARMRuntimeLibcalls(
    const ARMRuntimeLibcallProfile &Profile) noexcept;
Expect<void>
addARMRuntimeLibcalls(Module &LLModule,
                      const ARMRuntimeLibcallProfile &Profile) noexcept;
Expect<void> validateARMRuntimeLibcallsForNativeLinker(
    const ARMRuntimeLibcallProfile &Profile) noexcept;

/// Returns true on an x86-64 host without SSE4.1, where LLVM lowers the Wasm
/// rounding operations to libm calls.
bool hostNeedsRoundingLibcalls() noexcept;
/// Defines ceil, floor, trunc, and roundeven for f32 and f64 in the module
/// with the C calling convention, so the native linker resolves those calls.
Expect<void> addRoundingLibcalls(Module &LLModule) noexcept;

} // namespace LLVM
} // namespace WasmEdge
