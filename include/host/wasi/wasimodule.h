// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/wasi/environ.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiModule : public Runtime::Instance::ModuleInstance {
public:
  WasiModule();

  __wasi_exitcode_t getExitCode() const noexcept { return Env.getExitCode(); }

  void init(Span<const std::string> Dirs, const std::string &ProgramName,
            Span<const std::string> Args,
            Span<const std::string> Envs) noexcept {
    Env.init(Dirs, ProgramName, Args, Envs);
  }

  WASI::WasiExpect<uint64_t> getNativeHandler(__wasi_fd_t Fd) const noexcept {
    return Env.getNativeHandler(Fd);
  }

  WASI::WasiExpect<void>
  initWithFds(Span<const std::string> Dirs, std::string ProgramName,
              Span<const std::string> Args, Span<const std::string> Envs,
              int32_t StdInFd, int32_t StdOutFd, int32_t StdErrFd) {
    return Env.initWithFds(Dirs, ProgramName, Args, Envs, StdInFd, StdOutFd,
                           StdErrFd);
  }

  void setMaxWasiFd(uint32_t MaxFd) noexcept { Env.setMaxWasiFd(MaxFd); }

  const WASI::Environ *getEnv() const noexcept { return &Env; }

private:
  WASI::Environ Env;
};

} // namespace Host
} // namespace WasmEdge
