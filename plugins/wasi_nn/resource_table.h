// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// A thread-safe table of shared-ownership resources addressed by opaque
// uint32_t handles, backing the WASI-NN graph and execution-context pools.
//
// Contract:
// - Handles are never reused: a stale handle stops resolving instead of
//   aliasing a later resource. insert() fails once the id space is exhausted.
// - get() pins the resource for the caller; the payload address stays stable
//   for the resource's whole lifetime.
// - remove() returns the table's reference, so destruction runs at the final
//   release and never under the table mutex.
// - The mutex only guards the id map; user code never runs under it.
template <typename T> class ResourceTable {
public:
  ResourceTable() noexcept = default;
  explicit ResourceTable(uint32_t FirstId) noexcept : NextId(FirstId) {}
  ResourceTable(const ResourceTable &) = delete;
  ResourceTable &operator=(const ResourceTable &) = delete;

  // Publish a fully-built resource; std::nullopt once the id space is gone.
  std::optional<uint32_t> insert(std::shared_ptr<T> Value) noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);
    if (NextId == std::numeric_limits<uint32_t>::max()) {
      return std::nullopt;
    }
    const uint32_t Id = NextId++;
    Map.emplace(Id, std::move(Value));
    return Id;
  }

  // Pin a handle's resource; nullptr if it never existed or was removed.
  std::shared_ptr<T> get(uint32_t Id) const noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);
    if (auto It = Map.find(Id); It != Map.end()) {
      return It->second;
    }
    return nullptr;
  }

  // Detach a handle and return the table's reference; nullptr if the handle
  // does not resolve, so a double remove is a no-op.
  std::shared_ptr<T> remove(uint32_t Id) noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);
    if (auto It = Map.find(Id); It != Map.end()) {
      auto Value = std::move(It->second);
      Map.erase(It);
      return Value;
    }
    return nullptr;
  }

  uint32_t size() const noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);
    return static_cast<uint32_t>(Map.size());
  }

  // Swap contents including the id counter; lets tests stage a scratch table.
  void swap(ResourceTable &Other) noexcept {
    if (this == &Other) {
      return;
    }
    std::scoped_lock Lock(Mutex, Other.Mutex);
    Map.swap(Other.Map);
    std::swap(NextId, Other.NextId);
  }

private:
  mutable std::mutex Mutex;
  uint32_t NextId = 0;
  std::unordered_map<uint32_t, std::shared_ptr<T>> Map;
};

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
