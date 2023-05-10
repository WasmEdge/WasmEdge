// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/log.h"
#include "plugin/plugin.h"
#include "vm/vm.h"

#include <cstdint>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#define WASI_ENTRY_POINT "wasi_thread_start"

namespace WasmEdge {
namespace Host {

using wasmedge_tid_t = int32_t;
class TidManager {
public:
  /// @brief  Tid is in [1, 2**29], 0 is reserved
  const static wasmedge_tid_t MinTid = 1;
  const static wasmedge_tid_t MaxTid = 1 << 29;

  TidManager() : Mutex(), Distribution(MinTid, MaxTid), Engine(), TidMap() {
    std::random_device Device;

    if (Device.entropy() == 0) {
      spdlog::warn("Quality of random device is bad");
    }

    Engine.seed(Device());
  }

  wasmedge_tid_t allocate() {
    std::unique_lock Lock(Mutex);

    wasmedge_tid_t NewTid;
    bool Success = false;

    while (!Success) {
      NewTid = Distribution(Engine);
      Success = TidMap.find(NewTid) == TidMap.end();
    }

    TidMap.emplace(NewTid);
    return NewTid;
  }

  Expect<void> free(wasmedge_tid_t Tid) noexcept {
    std::unique_lock Lock(Mutex);

    if (auto It = TidMap.find(Tid); It == TidMap.end()) {
      return Unexpect(ErrCode::Value::HostFuncError);
    } else {
      TidMap.erase(It);
      return {};
    }
  }

private:
  mutable std::shared_mutex Mutex;

  std::uniform_int_distribution<wasmedge_tid_t> Distribution;
  std::default_random_engine Engine;
  std::unordered_set<wasmedge_tid_t> TidMap;
};

class WasiThreadsEnvironment {
public:
  WasiThreadsEnvironment() noexcept {}
  ~WasiThreadsEnvironment() noexcept {}

  Expect<wasmedge_tid_t>
  wasiThreadSpawn(Executor::Executor *Exec,
                  Runtime::Instance::ModuleInstance *Mods,
                  uint32_t ThreadStartArg);

  static Plugin::PluginRegister Register;

private:
  TidManager Manager;
};

} // namespace Host
} // namespace WasmEdge
