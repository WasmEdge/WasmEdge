// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/native_linker.h"

#include "common/defines.h"
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

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>

#include <array>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#if !WASMEDGE_OS_WINDOWS
#include <cerrno>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

Expect<void> stageError(std::string_view Stage) noexcept {
  spdlog::error("native linker: {} failed"sv, Stage);
  return linkError();
}

ErrCode reportDiagnostic(const Diagnostic &Value) {
  std::string Message = "native linker: " + Value.Message;
  if (Value.Section)
    Message +=
        "; section=" + std::to_string(static_cast<uint32_t>(*Value.Section));
  if (Value.Symbol)
    Message +=
        "; symbol=" + std::to_string(static_cast<uint32_t>(*Value.Symbol));
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

struct TempFile {
  std::filesystem::path Path;
  int File = -1;
};

Expect<TempFile> createUniqueSibling(const std::filesystem::path &Output) {
  llvm::SmallString<128> Path;
  int File = -1;
  const auto Model = Output.u8string() + ".tmp-%%%%%%";
  if (llvm::sys::fs::createUniqueFile(Model, File, Path)) {
    if (File != -1) {
      llvm::sys::Process::SafelyCloseFileDescriptor(File);
    }
    std::error_code Error;
    std::filesystem::remove(std::filesystem::path(Path.str().str()), Error);
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  return TempFile{std::filesystem::u8path(Path.str().str()), File};
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

} // namespace

Expect<void> Internal::publishAtomically(const std::filesystem::path &Temporary,
                                         const std::filesystem::path &Output,
                                         const BeforePublish &Before) noexcept {
  try {
    struct Guard {
      const std::filesystem::path &Path;
      bool Published = false;
      ~Guard() {
        if (!Published) {
          std::error_code Error;
          std::filesystem::remove(Path, Error);
        }
      }
    } Cleanup{Temporary};
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
    Cleanup.Published = true;
    return {};
  } catch (...) {
    return linkError();
  }
}

Expect<void>
Internal::publishMachO(const std::filesystem::path &Temporary,
                       const std::filesystem::path &Output,
                       const std::filesystem::path &SignExecutable) noexcept {
  try {
    return publishAtomically(Temporary, Output,
                             [&](const std::filesystem::path &Path) {
                               return signMachO(Path, SignExecutable);
                             });
  } catch (...) {
    return linkError();
  }
}

Expect<void> NativeLinker::link(Span<const Byte> Object, Span<const Byte> Wasm,
                                const std::filesystem::path &Output,
                                OutputKind Kind) noexcept {
  try {
    if (Output.empty()) {
      return linkError();
    }
#if WASMEDGE_OS_LINUX
    if (Kind != OutputKind::UniversalWasm && Kind != OutputKind::ELF)
      return linkError();
#elif WASMEDGE_OS_MACOS
    if (Kind != OutputKind::UniversalWasm && Kind != OutputKind::MachO)
      return linkError();
#elif WASMEDGE_OS_WINDOWS
    if (Kind != OutputKind::UniversalWasm && Kind != OutputKind::PE)
      return linkError();
#endif
    const auto TargetValue = hostTarget();
    if (!TargetValue) {
      return Unexpect(ErrCode::Value::AOTNotImpl);
    }
    const auto InputPolicy = Internal::nativeObjectInputPolicy(hostFormat());
    auto ReadResult =
        report(ObjectReader::read(Object, *TargetValue, InputPolicy));
    if (!ReadResult)
      return Unexpect(ReadResult);
    auto Graph = std::move(*ReadResult);
    if (Graph.format() != hostFormat()) {
      if (Kind == OutputKind::MachO)
        return stageError("host format validation"sv);
      return linkError();
    }
#if WASMEDGE_OS_MACOS && defined(__aarch64__)
    constexpr uint64_t HostPageSize = 16384;
#else
    constexpr uint64_t HostPageSize = 4096;
#endif
    if (Kind == OutputKind::ELF && !ELFWriter::layout(Graph)) {
      return linkError();
    }
    if (Kind == OutputKind::MachO) {
      if (!Graph.compactUnwind().empty()) {
        auto Pruned = Graph.pruneUnreferencedMachOEHFrame();
        if (!Pruned)
          return report(std::move(Pruned));
        auto Reserved = reserveMachOUnwindInfo(Graph);
        if (!Reserved)
          return stageError("unwind info reservation"sv);
      }
      if (!MachOWriter::layout(Graph)) {
        return stageError("Mach-O layout"sv);
      }
    }
    if (Kind == OutputKind::PE && !PEWriter::layout(Graph)) {
      return linkError();
    }
    if (Kind == OutputKind::UniversalWasm &&
        Graph.format() == ObjectFormat::MachO) {
      EXPECTED_TRY(compactUnwindToEHFrame(Graph));
    }
    if (Kind != OutputKind::ELF && Kind != OutputKind::MachO &&
        Kind != OutputKind::PE) {
      const uint64_t ImageBase =
          Graph.format() == ObjectFormat::COFF ? PEImageBase : 0;
      auto LaidOut = report(layout(Graph, ImageBase, HostPageSize));
      if (!LaidOut)
        return Unexpect(LaidOut);
    }
    if (Graph.format() == ObjectFormat::MachO) {
      auto Normalized = normalizeMachOEHFrame(Graph);
      if (!Normalized) {
        spdlog::error("native linker: EH frame normalization failed"sv);
        return Unexpect(Normalized.error());
      }
      auto Validated = validateMachOEHFrameCoverage(Graph);
      if (!Validated) {
        spdlog::error("native linker: EH frame coverage validation failed"sv);
        return Unexpect(Validated.error());
      }
    }
    auto Relocated = applyRelocations(Graph);
    if (!Relocated)
      return report(std::move(Relocated));
    if (Kind == OutputKind::MachO && !Graph.compactUnwind().empty()) {
      auto Populated = populateMachOUnwindInfo(Graph);
      if (!Populated)
        return stageError("unwind info population"sv);
    }

    auto TempResult = createUniqueSibling(Output);
    if (!TempResult) {
      if (Kind == OutputKind::MachO)
        return stageError("temporary output creation"sv);
      return Unexpect(TempResult.error());
    }
    auto Temp = std::move(*TempResult);
    TempGuard Guard(std::move(Temp.Path), Temp.File);
#if !WASMEDGE_OS_WINDOWS
    struct stat DestinationStat{};
    if (::stat(Output.c_str(), &DestinationStat) == 0 &&
        ::fchmod(Temp.File, DestinationStat.st_mode & 07777) != 0)
      return linkError();
#endif
    {
      Writer OutputWriter(Guard.release());
      if (Kind == OutputKind::ELF) {
        auto Written = report(ELFWriter::write(Graph, OutputWriter));
        if (!Written)
          return Unexpect(Written);
      } else if (Kind == OutputKind::MachO) {
        auto Written = MachOWriter::write(Graph, OutputWriter);
        if (!Written)
          return stageError("Mach-O writing"sv);
      } else if (Kind == OutputKind::PE) {
        auto Written = report(
            PEWriter::write(Graph, Output.filename().string(), OutputWriter));
        if (!Written)
          return Unexpect(Written);
      } else {
        EXPECTED_TRY(UniversalWasmWriter::write(Graph, Wasm, OutputWriter));
      }
      EXPECTED_TRY(OutputWriter.close());
    }
#if WASMEDGE_OS_MACOS
    if (Kind == OutputKind::MachO) {
      auto Published =
          Internal::publishMachO(Guard.path(), Output, "/usr/bin/codesign");
      if (!Published)
        return stageError("Mach-O signing and publication"sv);
      Guard.publish();
      return {};
    }
#endif
    EXPECTED_TRY(Internal::publishAtomically(
        Guard.path(), Output,
        [](const std::filesystem::path &) { return Expect<void>{}; }));
    Guard.publish();
    return {};
  } catch (...) {
    return linkError();
  }
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
