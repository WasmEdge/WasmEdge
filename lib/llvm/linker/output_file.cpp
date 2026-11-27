// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/output_file.h"

#include "common/defines.h"
#include "common/filesystem.h"
#include "common/spdlog.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Signals.h>

#include <array>
#include <cstdio>
#include <system_error>
#include <utility>
#include <vector>
#if !WASMEDGE_OS_WINDOWS
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace WasmEdge {
namespace LLVM {
namespace Linker {

using namespace std::literals;

Expect<TempFile> createUniqueSibling(const std::filesystem::path &Output,
                                     std::string_view Suffix, unsigned Mode) {
  constexpr unsigned Attempts = 128;
  const auto Directory = Output.parent_path();
  std::error_code Error;
  for (unsigned Attempt = 0; Attempt < Attempts; ++Attempt) {
    std::array<char, 17> Random{};
    std::snprintf(Random.data(), Random.size(), "%08x%08x",
                  llvm::sys::Process::GetRandomNumber(),
                  llvm::sys::Process::GetRandomNumber());
    const std::string Path =
        u8string(Directory / u8path(".wasmedge-" + std::string(Random.data()) +
                                    std::string(Suffix)));
    int File = -1;
    Error = llvm::sys::fs::openFileForWrite(
        Path, File, llvm::sys::fs::CD_CreateNew, llvm::sys::fs::OF_None, Mode);
    if (Error == std::errc::file_exists)
      continue;
    if (Error) {
      spdlog::error("cannot create a temporary file in {}: {}"sv,
                    u8string(Directory), Error.message());
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    llvm::sys::RemoveFileOnSignal(Path);
    return TempFile{u8path(Path), File};
  }
  spdlog::error("cannot create a temporary file in {}: {}"sv,
                u8string(Directory), Error.message());
  return Unexpect(ErrCode::Value::IllegalPath);
}

TempGuard::TempGuard(std::filesystem::path Value, int File) noexcept
    : Path(std::move(Value)), File(File) {}

TempGuard::~TempGuard() noexcept {
  if (File != -1)
    llvm::sys::Process::SafelyCloseFileDescriptor(File);
  if (!Published) {
    std::error_code Error;
    std::filesystem::remove(Path, Error);
  }
  llvm::sys::DontRemoveFileOnSignal(u8string(Path));
}

Expect<void> runProcess(const std::string &Program,
                        Span<const std::string> Arguments) noexcept {
#if WASMEDGE_OS_WINDOWS
  static_cast<void>(Program);
  static_cast<void>(Arguments);
  return Unexpect(ErrCode::Value::IllegalPath);
#else
  try {
    std::vector<const char *> Argv;
    Argv.reserve(Arguments.size() + 2);
    Argv.push_back(Program.c_str());
    for (const auto &Argument : Arguments)
      Argv.push_back(Argument.c_str());
    Argv.push_back(nullptr);
    pid_t Child = -1;
    if (::posix_spawn(&Child, Program.c_str(), nullptr, nullptr,
                      const_cast<char *const *>(Argv.data()), environ) != 0)
      return Unexpect(ErrCode::Value::IllegalPath);
    int Status = 0;
    while (::waitpid(Child, &Status, 0) == -1) {
      if (errno != EINTR)
        return Unexpect(ErrCode::Value::IllegalPath);
    }
    if (!WIFEXITED(Status) || WEXITSTATUS(Status) != 0)
      return Unexpect(ErrCode::Value::IllegalPath);
    return {};
  } catch (...) {
    return Unexpect(ErrCode::Value::IllegalPath);
  }
#endif
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
