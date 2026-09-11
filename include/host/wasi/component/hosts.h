// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/component/hosts.h - WASI host components -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the state behind the built-in WASI 0.2 and 0.3 host
/// components and the factory of their component instances.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi/component/env.h"
#include "host/wasi/p2/http.h"
#include "host/wasi/p2/io.h"
#include "host/wasi/p2/sockets.h"
#include "host/wasi/p3/http.h"
#include "runtime/instance/component/component.h"

#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

/// The shared environment and the per-interface hosts behind the built-in
/// WASI component instances, which borrow them for their lifetime.
class WasiHosts {
public:
  WasiHosts() noexcept : Io(Environment), Sockets(Io), Http(Io) {}

  Env &getEnv() noexcept { return Environment; }

  /// One component instance per WASI 0.2 and 0.3 interface over this state.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  makeInstances();

private:
  Env Environment;
  WasiP2::IoHost Io;
  WasiP2::SocketsHost Sockets;
  WasiP2::HttpHost Http;
  WasiP3::HttpHost HttpP3;
};

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
