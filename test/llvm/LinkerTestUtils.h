// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ScopedTempDirectory.h"
#include "linker/link_graph.h"
#include "linker/universal_wasm_writer.h"
#include "linker/writer.h"

#include <llvm/Config/llvm-config.h>
#include <llvm/IR/Function.h>
#include <llvm/Object/COFF.h>
#include <llvm/Object/ObjectFile.h>
#if LLVM_VERSION_MAJOR < 10
#else
#include <llvm/Support/Alignment.h>
#endif
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#if LLVM_VERSION_MAJOR < 14
#include <llvm/Support/TargetRegistry.h>
#else
#include <llvm/MC/TargetRegistry.h>
#endif

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace WasmEdge {
namespace LLVM {
namespace Test {

#if LLVM_VERSION_MAJOR < 10
inline constexpr auto ObjectFileType = llvm::TargetMachine::CGFT_ObjectFile;
#elif LLVM_VERSION_MAJOR < 18
inline constexpr auto ObjectFileType = llvm::CGFT_ObjectFile;
#else
inline constexpr auto ObjectFileType = llvm::CodeGenFileType::ObjectFile;
#endif

#if LLVM_VERSION_MAJOR < 10
inline unsigned alignment(uint64_t Value) noexcept {
  return static_cast<unsigned>(Value);
}
#else
inline llvm::Align alignment(uint64_t Value) noexcept {
  return llvm::Align(Value);
}
#endif

inline llvm::Argument *argument(llvm::Function *Function,
                                unsigned Index) noexcept {
#if LLVM_VERSION_MAJOR < 10
  return Function->arg_begin() + Index;
#else
  return Function->getArg(Index);
#endif
}

inline llvm::Expected<llvm::StringRef>
sectionName(const llvm::object::SectionRef &Section) {
#if LLVM_VERSION_MAJOR < 10
  llvm::StringRef Name;
  if (const auto Error = Section.getName(Name))
    return llvm::errorCodeToError(Error);
  return Name;
#else
  return Section.getName();
#endif
}

inline const llvm::object::coff_file_header *
coffHeader(const llvm::object::COFFObjectFile &COFF) noexcept {
#if LLVM_VERSION_MAJOR < 10
  const llvm::object::coff_file_header *Header = nullptr;
  return COFF.getCOFFHeader(Header) ? nullptr : Header;
#else
  return COFF.getCOFFHeader();
#endif
}

inline const llvm::object::coff_bigobj_file_header *
coffBigObjHeader(const llvm::object::COFFObjectFile &COFF) noexcept {
#if LLVM_VERSION_MAJOR < 10
  const llvm::object::coff_bigobj_file_header *Header = nullptr;
  return COFF.getCOFFBigObjHeader(Header) ? nullptr : Header;
#else
  return COFF.getCOFFBigObjHeader();
#endif
}

inline const llvm::object::pe32plus_header *
pe32PlusHeader(const llvm::object::COFFObjectFile &COFF) noexcept {
#if LLVM_VERSION_MAJOR < 10
  const llvm::object::pe32plus_header *Header = nullptr;
  return COFF.getPE32PlusHeader(Header) ? nullptr : Header;
#else
  return COFF.getPE32PlusHeader();
#endif
}

inline std::string architectureName(Linker::Target Architecture) {
  switch (Architecture) {
  case Linker::Target::ARM:
    return "ARM";
  case Linker::Target::X86_64:
    return "X86_64";
  case Linker::Target::AArch64:
    return "AArch64";
  case Linker::Target::RISCV64:
    return "RISCV64";
  case Linker::Target::S390X:
    return "S390X";
  }
  return "Unknown";
}

inline Linker::Target nativeTarget() {
#if defined(__x86_64__) || defined(_M_X64)
  return Linker::Target::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
  return Linker::Target::AArch64;
#elif defined(__arm__) || defined(_M_ARM)
  return Linker::Target::ARM;
#elif defined(__riscv) && __riscv_xlen == 64
  return Linker::Target::RISCV64;
#elif defined(__s390x__)
  return Linker::Target::S390X;
#else
#error Unsupported test host
#endif
}

inline void initializeLLVMTargets() {
  static const bool Initialized = [] {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    return true;
  }();
  static_cast<void>(Initialized);
}

inline const llvm::Target *lookupLLVMTarget(const llvm::Triple &Triple,
                                            std::string &Error) {
  initializeLLVMTargets();
  llvm::Triple LookupTriple = Triple;
  return llvm::TargetRegistry::lookupTarget("", LookupTriple, Error);
}

class ScopedCurrentPath {
public:
  explicit ScopedCurrentPath(const std::filesystem::path &Path)
      : OriginalPath(std::filesystem::current_path()) {
    std::filesystem::current_path(Path);
  }

  ScopedCurrentPath(const ScopedCurrentPath &) = delete;
  ScopedCurrentPath &operator=(const ScopedCurrentPath &) = delete;

  ~ScopedCurrentPath() noexcept {
    std::error_code Error;
    std::filesystem::current_path(OriginalPath, Error);
  }

private:
  std::filesystem::path OriginalPath;
};

inline Linker::LinkExpect<void>
writeUniversalWasmFile(const Linker::LinkGraph &Graph, Span<const Byte> Wasm,
                       const std::filesystem::path &Output) {
  std::vector<Byte> Bytes;
  Linker::Writer Buffer(Bytes);
  EXPECTED_TRY(Linker::UniversalWasmWriter::write(Graph, Wasm, Buffer));
  std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
  File.write(reinterpret_cast<const char *>(Bytes.data()),
             static_cast<std::streamsize>(Bytes.size()));
  File.close();
  if (!File)
    return Linker::diagnosticError("failed to write output",
                                   Linker::DiagnosticKind::IO);
  return {};
}

} // namespace Test
} // namespace LLVM
} // namespace WasmEdge

#define REQUIRE_LLVM_TARGET(TRIPLE)                                            \
  do {                                                                         \
    const llvm::Triple RequiredTriple(TRIPLE);                                 \
    std::string TargetError;                                                   \
    if (lookupLLVMTarget(RequiredTriple, TargetError) == nullptr) {            \
      GTEST_SKIP() << RequiredTriple.str() << ": " << TargetError;             \
    }                                                                          \
  } while (false)

#define REQUIRE_NATIVE_TARGET(TARGET)                                          \
  do {                                                                         \
    if (nativeTarget() != (TARGET)) {                                          \
      GTEST_SKIP() << architectureName(TARGET) << " is not the native target"; \
    }                                                                          \
  } while (false)
