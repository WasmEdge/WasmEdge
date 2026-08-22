// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"

#include <cstdint>
#include <filesystem>
#include <functional>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {
using BeforePublish =
    std::function<Expect<void>(const std::filesystem::path &)>;
Expect<void> publishAtomically(const std::filesystem::path &Temporary,
                               const std::filesystem::path &Output,
                               const BeforePublish &Before) noexcept;
Expect<void> publishMachO(const std::filesystem::path &Temporary,
                          const std::filesystem::path &Output,
                          const std::filesystem::path &SignExecutable) noexcept;
} // namespace Internal

enum class OutputKind : uint8_t { UniversalWasm, ELF, MachO, PE };

class NativeLinker {
public:
  static Expect<void> link(Span<const Byte> Object, Span<const Byte> Wasm,
                           const std::filesystem::path &Output,
                           OutputKind Kind) noexcept;
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
