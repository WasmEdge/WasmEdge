// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/component/hosts.h"

#include "host/wasi/p2/cli.h"
#include "host/wasi/p2/clocks.h"
#include "host/wasi/p2/filesystem.h"
#include "host/wasi/p2/http.h"
#include "host/wasi/p2/io.h"
#include "host/wasi/p2/random.h"
#include "host/wasi/p2/sockets.h"
#include "host/wasi/p3/cli.h"
#include "host/wasi/p3/clocks.h"
#include "host/wasi/p3/filesystem.h"
#include "host/wasi/p3/http.h"
#include "host/wasi/p3/random.h"
#include "host/wasi/p3/sockets.h"

#include <memory>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
WasiHosts::newInstances() {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  auto Append = [&Insts](auto &&Made) {
    for (auto &Inst : Made) {
      Insts.push_back(std::move(Inst));
    }
  };
  Append(WasiP2::newIoInstances(Io));
  Append(WasiP2::newClocksInstances(Io));
  Append(WasiP2::newRandomInstances());
  Append(WasiP2::newCliInstances(Environment, Io));
  Append(WasiP2::newFilesystemInstances(Environment, Io));
  Append(WasiP2::newSocketsInstances(Sockets));
  Append(WasiP2::newHttpInstances(Http));
  Insts.push_back(std::make_unique<WasiP3::ClocksTypesInstance>());
  Insts.push_back(std::make_unique<WasiP3::MonotonicClockInstance>());
  Insts.push_back(std::make_unique<WasiP3::SystemClockInstance>());
  Append(WasiP3::newRandomInstances());
  Append(WasiP3::newCliInstances(Environment));
  Append(WasiP3::newFilesystemInstances(Environment));
  Append(WasiP3::newSocketsInstances());
  Append(WasiP3::newHttpInstances(HttpP3));
  return Insts;
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
