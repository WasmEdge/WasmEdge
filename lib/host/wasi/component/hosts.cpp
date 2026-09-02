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
WasiHosts::makeInstances() {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  auto Append = [&Insts](auto &&Made) {
    for (auto &Inst : Made) {
      Insts.push_back(std::move(Inst));
    }
  };
  Append(WasiP2::makeIoInstances(Io));
  Append(WasiP2::makeClocksInstances(Io));
  Append(WasiP2::makeRandomInstances());
  Append(WasiP2::makeCliInstances(Environment, Io));
  Append(WasiP2::makeFilesystemInstances(Environment, Io));
  Append(WasiP2::makeSocketsInstances(Sockets));
  Append(WasiP2::makeHttpInstances(Http));
  Insts.push_back(std::make_unique<WasiP3::ClocksTypesInstance>());
  Insts.push_back(std::make_unique<WasiP3::MonotonicClockInstance>());
  Insts.push_back(std::make_unique<WasiP3::SystemClockInstance>());
  Append(WasiP3::makeRandomInstances());
  Append(WasiP3::makeCliInstances(Environment));
  Append(WasiP3::makeFilesystemInstances(Environment));
  Append(WasiP3::makeSocketsInstances());
  Append(WasiP3::makeHttpInstances(HttpP3));
  return Insts;
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
