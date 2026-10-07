// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/writer.h"

#include <algorithm>
#include <limits>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#if WASMEDGE_OS_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

Unexpected<ErrCode> writeError() noexcept {
  return Unexpect(ErrCode::Value::IllegalPath);
}

} // namespace

Expect<Writer> Writer::open(const std::filesystem::path &Path) {
  Writer Result;
  Result.Stream.open(Path, std::ios_base::binary | std::ios_base::trunc);
  if (!Result.Stream)
    return writeError();
  return Result;
}

Writer::Writer(int FileDescriptor) : FileDescriptor(FileDescriptor) {
  try {
    DescriptorStream =
        std::make_unique<llvm::raw_fd_ostream>(FileDescriptor, true);
  } catch (...) {
    llvm::sys::Process::SafelyCloseFileDescriptor(FileDescriptor);
    throw;
  }
}

Writer::Writer(Writer &&Other)
    : Stream(std::move(Other.Stream)),
      DescriptorStream(std::move(Other.DescriptorStream)),
      FileDescriptor(Other.FileDescriptor), Buffer(Other.Buffer),
      Closed(Other.Closed) {
  Other.FileDescriptor = -1;
  Other.Buffer = nullptr;
  Other.Closed = true;
}

Writer::~Writer() {
  if (DescriptorStream)
    static_cast<void>(close());
}

Expect<void> Writer::writeRaw(const char *Data, size_t Size) {
  if (Closed)
    return writeError();
  if (Buffer != nullptr) {
    const auto *Bytes = reinterpret_cast<const Byte *>(Data);
    Buffer->insert(Buffer->end(), Bytes, Bytes + Size);
    return {};
  }
  if (DescriptorStream) {
    DescriptorStream->write(Data, Size);
    if (DescriptorStream->has_error()) {
      DescriptorStream->clear_error();
      return writeError();
    }
    return {};
  }
  if (Size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
    return writeError();
  }
  Stream.write(Data, static_cast<std::streamsize>(Size));
  return Stream ? Expect<void>{} : writeError();
}

Expect<void> Writer::writeByte(uint8_t Data) {
  const char Value = static_cast<char>(Data);
  return writeRaw(&Value, 1);
}

Expect<void> Writer::writeULEB(uint64_t Data) {
  constexpr uint8_t PayloadMask = UINT8_C(0x7F);
  constexpr uint8_t ContinuationBit = UINT8_C(0x80);
  constexpr uint8_t PayloadBits = 7;
  do {
    uint8_t Byte = static_cast<uint8_t>(Data & PayloadMask);
    Data >>= PayloadBits;
    if (Data != 0) {
      Byte |= ContinuationBit;
    }
    EXPECTED_TRY(writeByte(Byte));
  } while (Data != 0);
  return {};
}

Expect<void> Writer::writeLengthPrefixed(Span<const Byte> Data) {
  if (Closed)
    return writeError();
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

Expect<void> Writer::close() {
  if (Closed)
    return {};
  Closed = true;
  if (Buffer != nullptr) {
    return {};
  }
  if (DescriptorStream) {
    DescriptorStream->flush();
    bool Failed = DescriptorStream->has_error();
    if (Failed)
      DescriptorStream->clear_error();
#if WASMEDGE_OS_WINDOWS
    if (::_commit(FileDescriptor) != 0)
      Failed = true;
#else
    if (::fsync(FileDescriptor) != 0)
      Failed = true;
#endif
    DescriptorStream->close();
    FileDescriptor = -1;
    if (DescriptorStream->has_error()) {
      DescriptorStream->clear_error();
      Failed = true;
    }
    return Failed ? writeError() : Expect<void>{};
  }
  Stream.flush();
  if (!Stream) {
    return writeError();
  }
  Stream.close();
  return Stream ? Expect<void>{} : writeError();
}

LinkExpect<void> writeImage(Writer &Output, Span<const Byte> Bytes,
                            std::string_view Format) {
  if (!Output.write(Bytes))
    return diagnosticError("failed to write " + std::string(Format) + " output",
                           DiagnosticKind::IO);
  if (!Output.close())
    return diagnosticError("failed to close " + std::string(Format) + " output",
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
