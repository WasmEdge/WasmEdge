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
#include "linker/output_file.h"
#include "linker/pe_writer.h"
#include "linker/relocation.h"
#include "linker/thunks.h"
#include "linker/universal_wasm_writer.h"
#if WASMEDGE_OS_WINDOWS
#endif

#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#if !WASMEDGE_OS_WINDOWS
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
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

Unexpected<ErrCode> stageError(std::string_view Stage) noexcept {
  spdlog::error("native linker: {} failed"sv, Stage);
  return Unexpect(ErrCode::Value::IllegalPath);
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

Expect<uint32_t> signingMode(const std::filesystem::path &Path) noexcept {
#if WASMEDGE_OS_WINDOWS
  static_cast<void>(Path);
  return Unexpect(ErrCode::Value::IllegalPath);
#else
  struct stat Status{};
  if (::stat(Path.c_str(), &Status) != 0)
    return Unexpect(ErrCode::Value::IllegalPath);
  return static_cast<uint32_t>(Status.st_mode & PermissionBits);
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
  try {
    const std::array<std::string, 4> Arguments{"--force"s, "--sign"s, "-"s,
                                               Path.string()};
    return runProcess(SignExecutable.string(), Arguments);
  } catch (...) {
    return linkError();
  }
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

Expect<void>
Internal::publishAtomically(const std::filesystem::path &Temporary,
                            const std::filesystem::path &Output) noexcept {
  try {
    TempGuard Cleanup(Temporary, -1);
#if WASMEDGE_OS_WINDOWS
    if (llvm::sys::fs::rename(u8string(Temporary), u8string(Output)))
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

Expect<void> Internal::publishMachO(const std::filesystem::path &Temporary,
                                    const std::filesystem::path &Output,
                                    const std::filesystem::path &SignExecutable,
                                    bool PublishUnsigned) noexcept {
  try {
    if (!PublishUnsigned) {
      TempGuard Cleanup(Temporary, -1);
      const auto Mode = signingMode(Temporary);
      if (!Mode || !signMachO(Temporary, SignExecutable) ||
          !restoreSigningMode(Temporary, *Mode))
        return stageError("Mach-O signing"sv);
      Cleanup.publish();
      return publishAtomically(Temporary, Output);
    }
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
                                OutputKind Kind) noexcept {
  if (Output.empty()) {
    return stageError("output path validation"sv);
  }
  if (Kind != OutputKind::UniversalWasm && Kind != hostNativeOutputKind())
    return stageError("output kind validation"sv);
  return {};
}

Expect<void> prepareLayout(LinkGraph &Graph, OutputKind Kind) {
  if (Kind == OutputKind::MachO && !Graph.compactUnwind().empty()) {
    EXPECTED_TRY(report(Graph.pruneUnreferencedMachOEHFrame()));
    EXPECTED_TRY(
        logStage(reserveMachOUnwindInfo(Graph), "unwind info reservation"sv));
  }
  if (Kind == OutputKind::ELF) {
    EXPECTED_TRY(report(addARMExidxBoundaries(Graph)));
  }
  if (Kind == OutputKind::UniversalWasm &&
      Graph.format() == ObjectFormat::MachO) {
    EXPECTED_TRY(
        logStage(compactUnwindToEHFrame(Graph), "compact unwind conversion"sv));
  }
  return {};
}

Expect<void> placeSections(LinkGraph &Graph, OutputKind Kind) {
  switch (Kind) {
  case OutputKind::ELF:
    return report(ELFWriter::layout(Graph));
  case OutputKind::MachO:
    return report(MachOWriter::layout(Graph));
  case OutputKind::PE:
    return report(PEWriter::layout(Graph));
  case OutputKind::UniversalWasm: {
    const uint64_t ImageBase =
        Graph.format() == ObjectFormat::COFF ? PEImageBase : 0;
    return report(
        layout(Graph, ImageBase,
               Internal::maximumPageSize(Graph.format(), Graph.target())));
  }
  }
  assumingUnreachable();
}

Expect<void> checkLoaderExports(const LinkGraph &Graph) {
  constexpr std::array<std::string_view, 4> Required{
      "version"sv, "intrinsics"sv, "wasm.code"sv, "wasm.size"sv};
  std::array<bool, Required.size()> Found{};
  for (const auto &Value : Graph.symbols()) {
    if (!Value.Exported)
      continue;
    const auto Name = Internal::semanticSymbolName(Graph, Value);
    if (!Name)
      continue;
    for (size_t I = 0; I < Required.size(); ++I)
      Found[I] |= *Name == Required[I];
  }
  for (size_t I = 0; I < Required.size(); ++I) {
    if (!Found[I]) {
      spdlog::error("native linker: object does not export {}"sv, Required[I]);
      return Unexpect(ErrCode::Value::IllegalPath);
    }
  }
  return {};
}

Expect<void> layoutForOutput(LinkGraph &Graph, OutputKind Kind) {
  constexpr unsigned MaxThunkPasses = 4;
  EXPECTED_TRY(prepareLayout(Graph, Kind));
  EXPECTED_TRY(report(splitTextSections(Graph, AArch64TextPieceSize)));
  EXPECTED_TRY(placeSections(Graph, Kind));
  for (unsigned Pass = 0; Pass < MaxThunkPasses; ++Pass) {
    EXPECTED_TRY(const bool Inserted, report(insertBranchThunks(Graph)));
    if (!Inserted)
      break;
    EXPECTED_TRY(placeSections(Graph, Kind));
  }
  return {};
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
  if (Kind == OutputKind::MachO && !Graph.compactUnwind().empty()) {
    EXPECTED_TRY(
        logStage(populateMachOUnwindInfo(Graph), "unwind info population"sv));
  }
  return {};
}

Expect<void> writeGraph(const LinkGraph &Graph, Span<const Byte> Wasm,
                        std::string_view DLLName, OutputKind Kind,
                        Writer &OutputWriter) {
  switch (Kind) {
  case OutputKind::ELF:
    return report(ELFWriter::write(Graph, OutputWriter));
  case OutputKind::MachO:
    return report(MachOWriter::write(Graph, OutputWriter));
  case OutputKind::PE:
    return report(PEWriter::write(Graph, DLLName, OutputWriter));
  case OutputKind::UniversalWasm:
    return report(UniversalWasmWriter::write(Graph, Wasm, OutputWriter));
  }
  assumingUnreachable();
}

enum class OutputMode : uint8_t { Replace, Stream, Overwrite };

struct OutputTarget {
  std::filesystem::path Path;
  OutputMode Mode = OutputMode::Replace;
};

Expect<OutputTarget> resolveOutput(const std::filesystem::path &Output,
                                   OutputKind Kind) {
  constexpr unsigned SymlinkHopLimit = 40;
  std::error_code Error;
  OutputTarget Result{Output};
  for (unsigned Hops = 0;; ++Hops) {
    const auto LinkStatus = std::filesystem::symlink_status(Result.Path, Error);
    if (!std::filesystem::is_symlink(LinkStatus))
      break;
    if (Hops == SymlinkHopLimit)
      return stageError("output symlink resolution"sv);
    auto Link = std::filesystem::read_symlink(Result.Path, Error);
    if (Error)
      return stageError("output symlink resolution"sv);
    Result.Path =
        Link.is_absolute() ? std::move(Link) : Result.Path.parent_path() / Link;
  }
  const auto Status = std::filesystem::status(Result.Path, Error);
  if (Status.type() == std::filesystem::file_type::not_found)
    return Result;
  if (Error)
    return stageError("output status"sv);
  if (llvm::sys::fs::access(u8string(Result.Path),
                            llvm::sys::fs::AccessMode::Write))
    return stageError("output write permission check"sv);
  if (!std::filesystem::is_regular_file(Status)) {
    if (Kind == OutputKind::MachO) {
      spdlog::error(
          "native linker: cannot sign Mach-O output to a non-regular file"sv);
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    Result.Mode = OutputMode::Stream;
  } else if (Kind == OutputKind::UniversalWasm) {
    const auto Links = std::filesystem::hard_link_count(Result.Path, Error);
    if (Error)
      return stageError("output status"sv);
    if (Links > 1)
      Result.Mode = OutputMode::Overwrite;
  }
  return Result;
}

Expect<void> writeInPlace(const LinkGraph &Graph, Span<const Byte> Wasm,
                          const OutputTarget &Target, OutputKind Kind) {
  int File = -1;
  if (llvm::sys::fs::openFileForWrite(u8string(Target.Path), File,
                                      Target.Mode == OutputMode::Overwrite
                                          ? llvm::sys::fs::CD_CreateAlways
                                          : llvm::sys::fs::CD_OpenExisting,
                                      llvm::sys::fs::OF_None))
    return stageError("output open"sv);
  Writer OutputWriter(File);
  EXPECTED_TRY(writeGraph(Graph, Wasm, u8string(Target.Path.filename()), Kind,
                          OutputWriter));
  return logStage(OutputWriter.close(), "output close"sv);
}

Expect<void> writeOutput(const LinkGraph &Graph, Span<const Byte> Wasm,
                         const std::filesystem::path &Output, OutputKind Kind) {
  EXPECTED_TRY(const auto Target, resolveOutput(Output, Kind));
  if (Target.Mode != OutputMode::Replace)
    return writeInPlace(Graph, Wasm, Target, Kind);
  const unsigned Mode = Kind == OutputKind::UniversalWasm ? 0666 : 0777;
  EXPECTED_TRY(auto Temp, logStage(createUniqueSibling(Target.Path, {}, Mode),
                                   "temporary output creation"sv));
  TempGuard Guard(std::move(Temp.Path), Temp.File);
#if !WASMEDGE_OS_WINDOWS
  struct stat DestinationStat{};
  if (::stat(Target.Path.c_str(), &DestinationStat) == 0 &&
      ::fchmod(Temp.File, DestinationStat.st_mode & PermissionBits) != 0)
    return stageError("output permission copy"sv);
#endif
  {
    Writer OutputWriter(Guard.release());
    EXPECTED_TRY(writeGraph(Graph, Wasm, u8string(Output.filename()), Kind,
                            OutputWriter));
    EXPECTED_TRY(logStage(OutputWriter.close(), "output close"sv));
  }
#if WASMEDGE_OS_MACOS
  if (Kind == OutputKind::MachO) {
    EXPECTED_TRY(logStage(
        Internal::publishMachO(Guard.path(), Target.Path, "/usr/bin/codesign"),
        "Mach-O signing and publication"sv));
    Guard.publish();
    return {};
  }
#endif
  EXPECTED_TRY(logStage(Internal::publishAtomically(Guard.path(), Target.Path),
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
    if (Kind != OutputKind::UniversalWasm) {
      EXPECTED_TRY(checkLoaderExports(Graph));
    }
    EXPECTED_TRY(prepareGraph(Graph, Kind));
    return writeOutput(Graph, Wasm, Output, Kind);
  } catch (...) {
    return linkError();
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
