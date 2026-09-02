// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/component/env.h - WASI component environment ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the WASI environment shared by the 0.2 and 0.3 host
/// components of one VM.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi/environ.h"
#include "host/wasi/vinode.h"

#include <cstdint>
#include <memory>
#include <string>

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

/// The preview-1 environment behind the component-model WASI hosts carries
/// the arguments, environment variables, preopened directories, standard
/// streams, and exit code they share.
class Env {
public:
  void init(Span<const std::string> Dirs, const std::string &ProgramName,
            Span<const std::string> Args,
            Span<const std::string> Envs) noexcept {
    Preview1.init(Dirs, ProgramName, Args, Envs);
  }

  WASI::WasiExpect<void>
  initWithFds(Span<const std::string> Dirs, std::string ProgramName,
              Span<const std::string> Args, Span<const std::string> Envs,
              int32_t StdInFd, int32_t StdOutFd, int32_t StdErrFd) {
    return Preview1.initWithFds(Dirs, std::move(ProgramName), Args, Envs,
                                StdInFd, StdOutFd, StdErrFd);
  }

  WASI::Environ &preview1() noexcept { return Preview1; }
  const WASI::Environ &preview1() const noexcept { return Preview1; }

  __wasi_exitcode_t getExitCode() const noexcept {
    return Preview1.getExitCode();
  }

  /// The standard stream Fd (0, 1, or 2) as the environment opened it.
  std::shared_ptr<WASI::VINode> stdioNode(uint32_t Fd) const noexcept {
    return Preview1.getNodeOrNull(static_cast<__wasi_fd_t>(Fd));
  }

  /// True when the standard stream Fd is attached to a terminal.
  bool isTerminal(uint32_t Fd) const noexcept;

private:
  WASI::Environ Preview1;
};

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
