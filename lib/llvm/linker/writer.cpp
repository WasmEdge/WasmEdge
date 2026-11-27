// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/writer.h"

#include <algorithm>
#include <array>
#include <limits>
#include <llvm/Support/LEB128.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

Unexpected<ErrCode> writeError() noexcept {
  return Unexpect(ErrCode::Value::IllegalPath);
}

} // namespace

Writer::Writer(int FileDescriptor) {
  try {
    DescriptorStream =
        std::make_unique<llvm::raw_fd_ostream>(FileDescriptor, true);
  } catch (...) {
    llvm::sys::Process::SafelyCloseFileDescriptor(FileDescriptor);
    throw;
  }
}

Writer::~Writer() {
  if (DescriptorStream)
    static_cast<void>(close());
}

Expect<void> Writer::writeRaw(const char *Data, size_t Size) {
  if (Closed)
    return writeError();
  if (Counter != nullptr) {
    if (Size > std::numeric_limits<uint64_t>::max() - *Counter)
      return writeError();
    *Counter += Size;
    return {};
  }
  if (Buffer != nullptr) {
    const auto *Bytes = reinterpret_cast<const Byte *>(Data);
    Buffer->insert(Buffer->end(), Bytes, Bytes + Size);
    return {};
  }
  if (!DescriptorStream)
    return writeError();
  DescriptorStream->write(Data, Size);
  if (DescriptorStream->has_error()) {
    DescriptorStream->clear_error();
    return writeError();
  }
  return {};
}

Expect<void> Writer::writeByte(uint8_t Data) {
  const char Value = static_cast<char>(Data);
  return writeRaw(&Value, 1);
}

Expect<void> Writer::writeULEB(uint64_t Data) {
  uint8_t Encoded[16];
  const unsigned Size = llvm::encodeULEB128(Data, Encoded);
  return writeRaw(reinterpret_cast<const char *>(Encoded), Size);
}

Expect<void> Writer::writeLengthPrefixed(Span<const Byte> Data) {
  if (Data.size() > std::numeric_limits<uint32_t>::max()) {
    return writeError();
  }
  EXPECTED_TRY(writeU32(static_cast<uint32_t>(Data.size())));
  return write(Data);
}

Expect<void> Writer::writeName(std::string_view Data) {
  return writeLengthPrefixed(Span<const Byte>(
      reinterpret_cast<const Byte *>(Data.data()), Data.size()));
}

Expect<void> Writer::write(Span<const Byte> Data) {
  return writeRaw(reinterpret_cast<const char *>(Data.data()), Data.size());
}

Expect<void> Writer::writeZeros(uint64_t Count) {
  static constexpr std::array<char, 4096> Zeros{};
  while (Count != 0) {
    const auto Size = static_cast<size_t>(
        std::min<uint64_t>(Count, static_cast<uint64_t>(Zeros.size())));
    EXPECTED_TRY(writeRaw(Zeros.data(), Size));
    Count -= Size;
  }
  return {};
}

Expect<void> Writer::flush() {
  if (Closed)
    return writeError();
  if (!DescriptorStream)
    return {};
  DescriptorStream->flush();
  if (DescriptorStream->has_error()) {
    DescriptorStream->clear_error();
    return writeError();
  }
  return {};
}

Expect<void> Writer::close() {
  if (Closed)
    return {};
  Closed = true;
  if (Buffer != nullptr || Counter != nullptr) {
    return {};
  }
  if (!DescriptorStream)
    return writeError();
  DescriptorStream->close();
  if (DescriptorStream->has_error()) {
    DescriptorStream->clear_error();
    return writeError();
  }
  return {};
}

LinkExpect<void> writeImage(Writer &Output, Span<const Byte> Bytes,
                            std::string_view Format) {
  if (!Output.write(Bytes) || !Output.flush())
    return diagnosticError("failed to write " + std::string(Format) + " output",
                           DiagnosticKind::IO);
  return {};
}

LinkExpect<std::vector<const Symbol *>>
sortedUniqueExports(const LinkGraph &Graph) {
  std::vector<const Symbol *> Exports;
  for (const auto &SymbolValue : Graph.symbols())
    if (SymbolValue.Exported)
      Exports.push_back(&SymbolValue);
  std::sort(Exports.begin(), Exports.end(),
            [](const Symbol *Left, const Symbol *Right) {
              return Left->exportedName() < Right->exportedName();
            });
  const auto Duplicate =
      std::adjacent_find(Exports.begin(), Exports.end(),
                         [](const Symbol *Left, const Symbol *Right) {
                           return Left->exportedName() == Right->exportedName();
                         });
  if (Duplicate != Exports.end())
    return diagnosticError("duplicate exported symbol '" +
                           (*Duplicate)->exportedName() + "'");
  return Exports;
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
