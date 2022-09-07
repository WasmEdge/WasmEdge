// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "vm/vm.h"

#include <cstdint>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <iostream>

namespace WasmEdge {
namespace Host {

using wasmedge_tid_t = uint32_t;
class TidManager {
public:
  std::shared_ptr<std::thread> get(wasmedge_tid_t Tid) const {
    std::shared_lock Lock(TidMutex);
    if (auto It = TidMap.find(Tid); It != TidMap.end()) {
      return It->second;
    }
    return {};
  }

  wasmedge_tid_t allocate(std::shared_ptr<std::thread> Thread) {
    std::random_device Device;
    std::default_random_engine Engine(Device());
    std::uniform_int_distribution<wasmedge_tid_t> Distribution(0, 0x7FFFFFFF);
    bool Success = false;
    wasmedge_tid_t NewTid;

    while (!Success) {
      NewTid = Distribution(Engine);
      std::unique_lock Lock(TidMutex);
      Success = TidMap.emplace(NewTid, Thread).second;
    }
    return NewTid;
  }

  Expect<void> free(wasmedge_tid_t Tid) noexcept {
    std::unique_lock Lock(TidMutex);
    if (auto It = TidMap.find(Tid); It == TidMap.end()) {
      return Unexpect(ErrCode::Value::HostFuncError);
    } else {
      TidMap.erase(It);
      return {};
    }
  }

private:
  mutable std::shared_mutex TidMutex; ///< Protect TidMap
  std::unordered_map<wasmedge_tid_t, std::shared_ptr<std::thread>> TidMap;
};

class WasmEdgeThreadEnvironment {
public:
  WasmEdgeThreadEnvironment() noexcept {}
  ~WasmEdgeThreadEnvironment() noexcept {}

  Expect<wasmedge_tid_t> threadCreate(Executor::Executor *Exec,
                                      uint32_t ThreadFunc, uint32_t Arg);
  Expect<void> threadJoin(wasmedge_tid_t Tid, void **WasiRetval);

  static Plugin::PluginRegister Register;

private:
  TidManager Manager;
};

} // namespace Host
} // namespace WasmEdge
