// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasienv.h"
#include "common/defines.h"
#include "vfs-linux.ipp"
#include "vfs-macos.ipp"
#include "vfs-windows.ipp"

namespace WasmEdge {
namespace Host {

WasiEnvironment::WasiEnvironment() {}

WasiEnvironment::~WasiEnvironment() noexcept { fini(); }

void WasiEnvironment::fini() noexcept {
  FileMap.clear();
  Environs.clear();
  CmdArgs.clear();
}

/// TODO: Add preopened dir argument.
void WasiEnvironment::init(Span<const std::string> Dirs,
                           std::string ProgramName,
                           Span<const std::string> Args,
                           Span<const std::string> Envs) {
  using namespace std::string_view_literals;

  emplaceFile(0, WasiFile::stdinFile());
  emplaceFile(1, WasiFile::stdoutFile());
  emplaceFile(2, WasiFile::stderrFile());

  /// Open dir for WASI environment.
  int NewFd = 3;
  for (const auto &Dir : Dirs) {
    const auto Pos = Dir.find(':');
    if (Pos != std::string::npos) {
      const auto GuestDir = Dir.substr(0, Pos);
      const auto HostDir = Dir.substr(Pos + 1);
      auto Dir = WasiFile::preopened(GuestDir, HostDir);
      if (Dir.ok()) {
        emplaceFile(NewFd++, std::move(Dir));
      }
    }
  }

  CmdArgs.resize(Args.size() + 1);
  CmdArgs.front() = std::move(ProgramName);
  std::copy(Args.begin(), Args.end(), CmdArgs.begin() + 1);
  CmdArgs.shrink_to_fit();

  Environs.resize(Envs.size());
  std::copy(Envs.begin(), Envs.end(), Environs.begin());
  Environs.shrink_to_fit();
}

} // namespace Host
} // namespace WasmEdge
