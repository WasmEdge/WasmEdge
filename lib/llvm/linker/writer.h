// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "linker/link_graph.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

class Writer {
public:
  static Expect<Writer> open(const std::filesystem::path &Path);

  explicit Writer(int FileDescriptor);
  explicit Writer(std::vector<Byte> &Buffer) noexcept : Buffer(&Buffer) {}
  Writer(Writer &&Other);
  Writer(const Writer &) = delete;
  Writer &operator=(Writer &&) = delete;
  Writer &operator=(const Writer &) = delete;
  ~Writer();

  Expect<void> writeByte(uint8_t Data);
  Expect<void> writeULEB(uint64_t Data);
  Expect<void> writeU32(uint32_t Data) { return writeULEB(Data); }
  Expect<void> writeU64(uint64_t Data) { return writeULEB(Data); }
  Expect<void> writeLengthPrefixed(Span<const Byte> Data);
  Expect<void> writeName(std::string_view Data);
  Expect<void> write(Span<const Byte> Data);
  Expect<void> close();

private:
  Writer() = default;
  Expect<void> writeRaw(const char *Data, size_t Size);

  std::ofstream Stream;
  std::unique_ptr<llvm::raw_fd_ostream> DescriptorStream;
  int FileDescriptor = -1;
  std::vector<Byte> *Buffer = nullptr;
  bool Closed = false;
};

LinkExpect<void> writeImage(Writer &Output, Span<const Byte> Bytes,
                            std::string_view Format);
LinkExpect<std::vector<const Symbol *>>
sortedUniqueExports(const LinkGraph &Graph);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
