// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/native_linker.h"

#include "common/defines.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "linker/architecture.h"
#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/elf_writer.h"
#include "linker/layout.h"
#include "linker/macho_writer.h"
#include "linker/object_reader.h"
#include "linker/pe_writer.h"
#include "linker/relocation.h"
#include "linker/universal_wasm_writer.h"
#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>

#include <array>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#if !WASMEDGE_OS_WINDOWS
#include <cerrno>
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using namespace std::literals;

Expect<void> linkError() noexcept {
  return Unexpect(ErrCode::Value::IllegalPath);
}

#if !WASMEDGE_OS_WINDOWS
constexpr mode_t PermissionBits = 07777;
#endif

Expect<void> stageError(std::string_view Stage) noexcept {
  spdlog::error("native linker: {} failed"sv, Stage);
  return linkError();
}

template <typename T>
Expect<T> logStage(Expect<T> Result, std::string_view Stage) noexcept {
  if (!Result)
    spdlog::error("native linker: {} failed"sv, Stage);
  return Result;
}

ErrCode reportDiagnostic(const Diagnostic &Value) {
  std::string Message = "native linker: " + Value.Message;
  if (Value.Section)
    Message += "; section=" + std::to_string(*Value.Section);
  if (Value.Symbol)
    Message += "; symbol=" + std::to_string(*Value.Symbol);
  if (Value.RelocationType)
    Message += "; relocation=" + std::to_string(*Value.RelocationType);
  if (Value.Offset)
    Message += "; offset=" + std::to_string(*Value.Offset);
  if (!Value.SectionName.empty())
    Message += "; section name='" + Value.SectionName + "'";
  if (!Value.SymbolName.empty())
    Message += "; symbol name='" + Value.SymbolName + "'";
  spdlog::error("{}"sv, Message);
  switch (Value.Kind) {
  case DiagnosticKind::Malformed:
    return ErrCode::Value::MalformedSection;
  case DiagnosticKind::Unsupported:
    return ErrCode::Value::AOTNotImpl;
  case DiagnosticKind::IO:
    return ErrCode::Value::IllegalPath;
  }
  assumingUnreachable();
}

Expect<void> reportAsPathError(LinkExpect<void> Result) {
  if (Result)
    return {};
  reportDiagnostic(Result.error());
  return linkError();
}

template <typename T> Expect<T> report(LinkExpect<T> Result) {
  if (Result) {
    if constexpr (std::is_void_v<T>)
      return {};
    else
      return std::move(*Result);
  }
  return Unexpect(reportDiagnostic(Result.error()));
}

std::optional<Target> hostTarget() noexcept {
  return Internal::target(AOT::kHostArchitecture);
}

ObjectFormat hostFormat() noexcept {
#if WASMEDGE_OS_LINUX
  return ObjectFormat::ELF;
#elif WASMEDGE_OS_MACOS
  return ObjectFormat::MachO;
#elif WASMEDGE_OS_WINDOWS
  return ObjectFormat::COFF;
#else
#error Unsupported host object format
#endif
}

struct TempFile {
  std::filesystem::path Path;
  int File = -1;
};

Expect<TempFile> createUniqueSibling(const std::filesystem::path &Output) {
  constexpr std::string_view Hex = "0123456789abcdef";
  constexpr uint32_t MaxAttempts = 128;
  constexpr uint32_t SuffixDigits = 6;
  constexpr unsigned HexDigitMask = 15;
  for (uint32_t Attempt = 0; Attempt < MaxAttempts; ++Attempt) {
    auto Candidate = Output;
    Candidate += ".tmp-";
    for (uint32_t Digit = 0; Digit < SuffixDigits; ++Digit) {
      Candidate += Hex[llvm::sys::Process::GetRandomNumber() & HexDigitMask];
    }
    int File = -1;
    const auto Error = llvm::sys::fs::openFileForWrite(
        u8string(Candidate), File, llvm::sys::fs::CD_CreateNew);
    if (!Error)
      return TempFile{std::move(Candidate), File};
    if (File >= 0)
      llvm::sys::Process::SafelyCloseFileDescriptor(File);
    if (Error != std::errc::file_exists &&
        Error != std::errc::permission_denied)
      return Unexpect(ErrCode::Value::IllegalPath);
  }
  return Unexpect(ErrCode::Value::IllegalPath);
}

class TempGuard {
public:
  TempGuard(std::filesystem::path Value, int File)
      : Path(std::move(Value)), File(File) {}
  ~TempGuard() {
    if (File != -1)
      llvm::sys::Process::SafelyCloseFileDescriptor(File);
    if (!Published) {
      std::error_code Error;
      std::filesystem::remove(Path, Error);
    }
  }

  void publish() noexcept { Published = true; }
  const std::filesystem::path &path() const noexcept { return Path; }
  int release() noexcept {
    const int Result = File;
    File = -1;
    return Result;
  }

private:
  std::filesystem::path Path;
  int File;
  bool Published = false;
};

Expect<uint32_t> prepareSigningCopy(const std::filesystem::path &Source,
                                    int Destination) noexcept {
#if WASMEDGE_OS_WINDOWS
  static_cast<void>(Source);
  static_cast<void>(Destination);
  return Unexpect(ErrCode::Value::IllegalPath);
#else
  const int SourceFile = ::open(Source.c_str(), O_RDONLY);
  if (SourceFile == -1)
    return Unexpect(ErrCode::Value::IllegalPath);
  struct Guard {
    int File;
    ~Guard() { llvm::sys::Process::SafelyCloseFileDescriptor(File); }
  } SourceGuard{SourceFile};
  struct stat SourceStat{};
  if (::fstat(SourceFile, &SourceStat) != 0 ||
      ::ftruncate(Destination, 0) != 0 || ::lseek(Destination, 0, SEEK_SET) < 0)
    return Unexpect(ErrCode::Value::IllegalPath);
  std::array<char, 16384> Buffer;
  while (true) {
    ssize_t Read = -1;
    do {
      Read = ::read(SourceFile, Buffer.data(), Buffer.size());
    } while (Read == -1 && errno == EINTR);
    if (Read == 0) {
      if (::fchmod(Destination, SourceStat.st_mode & PermissionBits) != 0)
        return Unexpect(ErrCode::Value::IllegalPath);
      return static_cast<uint32_t>(SourceStat.st_mode & PermissionBits);
    }
    if (Read < 0)
      return Unexpect(ErrCode::Value::IllegalPath);
    ssize_t Written = 0;
    while (Written < Read) {
      ssize_t Result = -1;
      do {
        Result = ::write(Destination, Buffer.data() + Written,
                         static_cast<size_t>(Read - Written));
      } while (Result == -1 && errno == EINTR);
      if (Result <= 0)
        return Unexpect(ErrCode::Value::IllegalPath);
      Written += Result;
    }
  }
#endif
}

Expect<void> restoreSigningMode(const std::filesystem::path &Path,
                                uint32_t Mode) noexcept {
#if WASMEDGE_OS_WINDOWS
  static_cast<void>(Path);
  static_cast<void>(Mode);
  return linkError();
#else
  if (::chmod(Path.c_str(), static_cast<mode_t>(Mode)) != 0)
    return linkError();
  return {};
#endif
}

Expect<void> signMachO(const std::filesystem::path &Path,
                       const std::filesystem::path &SignExecutable) noexcept {
#if WASMEDGE_OS_WINDOWS
  static_cast<void>(Path);
  static_cast<void>(SignExecutable);
  return linkError();
#else
  try {
    const auto Program = SignExecutable.string();
    const auto File = Path.string();
    const std::array<const char *, 6> Arguments{
        Program.c_str(), "--force", "--sign", "-", File.c_str(), nullptr};
    pid_t Child = -1;
    const int SpawnResult =
        ::posix_spawn(&Child, Program.c_str(), nullptr, nullptr,
                      const_cast<char *const *>(Arguments.data()), environ);
    if (SpawnResult != 0)
      return linkError();
    int Status = 0;
    while (::waitpid(Child, &Status, 0) == -1) {
      if (errno != EINTR)
        return linkError();
    }
    if (!WIFEXITED(Status) || WEXITSTATUS(Status) != 0)
      return linkError();
    return {};
  } catch (...) {
    return linkError();
  }
#endif
}

std::optional<Expect<void>>
trySignAndPublish(const std::filesystem::path &Temporary,
                  const std::filesystem::path &Output,
                  const std::filesystem::path &SignExecutable) {
  auto SignTemp = createUniqueSibling(Temporary);
  if (!SignTemp)
    return std::nullopt;
  TempGuard SignGuard(std::move(SignTemp->Path), SignTemp->File);
  const auto Prepared = prepareSigningCopy(Temporary, SignTemp->File);
  if (!Prepared)
    return std::nullopt;
  if (llvm::sys::Process::SafelyCloseFileDescriptor(SignGuard.release()))
    return std::nullopt;
  if (!signMachO(SignGuard.path(), SignExecutable) ||
      !restoreSigningMode(SignGuard.path(), *Prepared))
    return std::nullopt;
  std::error_code Error;
  std::filesystem::remove(Temporary, Error);
  if (Error)
    return std::nullopt;
  auto Published = Internal::publishAtomically(SignGuard.path(), Output);
  if (Published)
    SignGuard.publish();
  return Published;
}

} // namespace

bool Internal::armFloatABICompatible(ObjectFormat Format, uint32_t Flags,
                                     bool HardFloatHost) noexcept {
  if (Format != ObjectFormat::ELF)
    return true;
  constexpr uint32_t Mask =
      llvm::ELF::EF_ARM_ABI_FLOAT_SOFT | llvm::ELF::EF_ARM_ABI_FLOAT_HARD;
  const uint32_t Expected = HardFloatHost ? llvm::ELF::EF_ARM_ABI_FLOAT_HARD
                                          : llvm::ELF::EF_ARM_ABI_FLOAT_SOFT;
  return (Flags & Mask) == Expected;
}

Expect<void> Internal::publishAtomically(const std::filesystem::path &Temporary,
                                         const std::filesystem::path &Output,
                                         const BeforePublish &Before) noexcept {
  try {
    TempGuard Cleanup(Temporary, -1);
    EXPECTED_TRY(Before(Temporary));
#if WASMEDGE_OS_WINDOWS
    if (!winapi::MoveFileExW(Temporary.c_str(), Output.c_str(),
                             winapi::MOVEFILE_REPLACE_EXISTING_))
      return linkError();
#else
    std::error_code Error;
    std::filesystem::rename(Temporary, Output, Error);
    if (Error)
      return linkError();
#endif
    Cleanup.publish();
    return {};
  } catch (...) {
    return linkError();
  }
}

Expect<void>
Internal::publishAtomically(const std::filesystem::path &Temporary,
                            const std::filesystem::path &Output) noexcept {
  return publishAtomically(
      Temporary, Output,
      [](const std::filesystem::path &) { return Expect<void>{}; });
}

Expect<void>
Internal::publishMachO(const std::filesystem::path &Temporary,
                       const std::filesystem::path &Output,
                       const std::filesystem::path &SignExecutable) noexcept {
  try {
    if (auto Signed = trySignAndPublish(Temporary, Output, SignExecutable))
      return *Signed;
    spdlog::warn(
        "native linker: Mach-O signing failed; publishing unsigned output"sv);
    return publishAtomically(Temporary, Output);
  } catch (...) {
    return linkError();
  }
}

namespace {

Expect<void> validateOutputKind(const std::filesystem::path &Output,
                                [[maybe_unused]] OutputKind Kind) noexcept {
  if (Output.empty()) {
    return stageError("output path validation"sv);
  }
#if WASMEDGE_OS_LINUX
  if (Kind != OutputKind::UniversalWasm && Kind != OutputKind::ELF)
    return stageError("output kind validation"sv);
#elif WASMEDGE_OS_MACOS
  if (Kind != OutputKind::UniversalWasm && Kind != OutputKind::MachO)
    return stageError("output kind validation"sv);
#elif WASMEDGE_OS_WINDOWS
  if (Kind != OutputKind::UniversalWasm && Kind != OutputKind::PE)
    return stageError("output kind validation"sv);
#endif
  return {};
}

Expect<void> layoutForOutput(LinkGraph &Graph, OutputKind Kind) {
#if WASMEDGE_OS_MACOS && defined(__aarch64__)
  constexpr uint64_t HostPageSize = 16384;
#else
  constexpr uint64_t HostPageSize = 4096;
#endif
  switch (Kind) {
  case OutputKind::ELF:
    if (!ELFWriter::layout(Graph))
      return stageError("ELF layout"sv);
    return {};
  case OutputKind::MachO:
    if (!Graph.compactUnwind().empty()) {
      EXPECTED_TRY(report(Graph.pruneUnreferencedMachOEHFrame()));
      if (!reserveMachOUnwindInfo(Graph))
        return stageError("unwind info reservation"sv);
    }
    if (!MachOWriter::layout(Graph))
      return stageError("Mach-O layout"sv);
    return {};
  case OutputKind::PE:
    if (!PEWriter::layout(Graph))
      return stageError("PE layout"sv);
    return {};
  case OutputKind::UniversalWasm: {
    if (Graph.format() == ObjectFormat::MachO) {
      EXPECTED_TRY(logStage(compactUnwindToEHFrame(Graph),
                            "compact unwind conversion"sv));
    }
    const uint64_t ImageBase =
        Graph.format() == ObjectFormat::COFF ? PEImageBase : 0;
    return report(layout(Graph, ImageBase, HostPageSize));
  }
  }
  assumingUnreachable();
}

Expect<void> prepareGraph(LinkGraph &Graph, OutputKind Kind) {
  if (Graph.format() != hostFormat())
    return stageError("host format validation"sv);
#if defined(__arm__)
#if defined(__ARM_PCS_VFP)
  constexpr bool HardFloatHost = true;
#else
  constexpr bool HardFloatHost = false;
#endif
  if (!Internal::armFloatABICompatible(Graph.format(), Graph.elfFlags(),
                                       HardFloatHost)) {
    spdlog::error("native linker: incompatible ARM float ABI"sv);
    return Unexpect(ErrCode::Value::AOTNotImpl);
  }
#endif
  EXPECTED_TRY(layoutForOutput(Graph, Kind));
  if (Graph.format() == ObjectFormat::MachO) {
    EXPECTED_TRY(
        logStage(normalizeMachOEHFrame(Graph), "EH frame normalization"sv));
    EXPECTED_TRY(logStage(validateMachOEHFrameCoverage(Graph),
                          "EH frame coverage validation"sv));
  }
  EXPECTED_TRY(report(applyRelocations(Graph)));
  if (Kind == OutputKind::MachO && !Graph.compactUnwind().empty() &&
      !populateMachOUnwindInfo(Graph))
    return stageError("unwind info population"sv);
  return {};
}

Expect<void> writeGraph(const LinkGraph &Graph, Span<const Byte> Wasm,
                        const std::filesystem::path &Output, OutputKind Kind,
                        Writer &OutputWriter) {
  switch (Kind) {
  case OutputKind::ELF:
    return report(ELFWriter::write(Graph, OutputWriter));
  case OutputKind::MachO:
    return reportAsPathError(MachOWriter::write(Graph, OutputWriter));
  case OutputKind::PE:
    return report(
        PEWriter::write(Graph, Output.filename().string(), OutputWriter));
  case OutputKind::UniversalWasm:
    return reportAsPathError(
        UniversalWasmWriter::write(Graph, Wasm, OutputWriter));
  }
  assumingUnreachable();
}

Expect<void> writeOutput(const LinkGraph &Graph, Span<const Byte> Wasm,
                         const std::filesystem::path &Output, OutputKind Kind) {
  EXPECTED_TRY(auto Temp, logStage(createUniqueSibling(Output),
                                   "temporary output creation"sv));
  TempGuard Guard(std::move(Temp.Path), Temp.File);
#if !WASMEDGE_OS_WINDOWS
  struct stat DestinationStat{};
  if (::stat(Output.c_str(), &DestinationStat) == 0 &&
      ::fchmod(Temp.File, DestinationStat.st_mode & PermissionBits) != 0)
    return stageError("output permission copy"sv);
#endif
  {
    Writer OutputWriter(Guard.release());
    EXPECTED_TRY(writeGraph(Graph, Wasm, Output, Kind, OutputWriter));
    EXPECTED_TRY(logStage(OutputWriter.close(), "output close"sv));
  }
#if WASMEDGE_OS_MACOS
  if (Kind == OutputKind::MachO) {
    if (!Internal::publishMachO(Guard.path(), Output, "/usr/bin/codesign"))
      return stageError("Mach-O signing and publication"sv);
    Guard.publish();
    return {};
  }
#endif
  EXPECTED_TRY(logStage(Internal::publishAtomically(Guard.path(), Output),
                        "output publication"sv));
  Guard.publish();
  return {};
}

} // namespace

Expect<void> NativeLinker::link(Span<const Byte> Object, Span<const Byte> Wasm,
                                const std::filesystem::path &Output,
                                OutputKind Kind) noexcept {
  try {
    EXPECTED_TRY(validateOutputKind(Output, Kind));
    const auto TargetValue = hostTarget();
    if (!TargetValue) {
      spdlog::error("native linker: unsupported host architecture"sv);
      return Unexpect(ErrCode::Value::AOTNotImpl);
    }
    const auto InputPolicy = Internal::nativeObjectInputPolicy(hostFormat());
    EXPECTED_TRY(auto Graph,
                 report(ObjectReader::read(Object, *TargetValue, InputPolicy)));
    EXPECTED_TRY(prepareGraph(Graph, Kind));
    return writeOutput(Graph, Wasm, Output, Kind);
  } catch (...) {
    return linkError();
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
