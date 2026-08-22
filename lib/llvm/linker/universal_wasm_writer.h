// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "linker/link_graph.h"
#include "linker/writer.h"

#include <filesystem>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

class UniversalWasmWriter {
public:
  static Expect<void> write(const LinkGraph &Graph, Span<const Byte> Wasm,
                            const std::filesystem::path &Output) noexcept;
  static Expect<void> write(const LinkGraph &Graph, Span<const Byte> Wasm,
                            Writer &Output);
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
