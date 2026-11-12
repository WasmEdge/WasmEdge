// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "llvm.h"

#include <filesystem>

namespace WasmEdge {
namespace LLVM {
namespace LLD {

/// Add the definitions that lld needs in the module before code emission.
void prepareModule(Context LLContext, Module &LLModule) noexcept;

/// Link the object with lld into a native shared library.
Expect<void> outputNativeLibrary(const std::filesystem::path &OutputPath,
                                 const MemoryBuffer &OSVec) noexcept;

/// Link the object with lld, then write the Wasm module with the linked
/// sections in the AOT custom section.
Expect<void> outputWasmLibrary(Context LLContext,
                               const std::filesystem::path &OutputPath,
                               Span<const Byte> Data,
                               const MemoryBuffer &OSVec) noexcept;

} // namespace LLD
} // namespace LLVM
} // namespace WasmEdge
