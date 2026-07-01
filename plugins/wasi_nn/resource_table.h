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
// - Handles are allocated monotonically and never reused, so a stale handle
//   held by the guest can never alias a resource created later: it simply
//   stops resolving. insert() fails once the id space is exhausted.
// - get() hands out a shared_ptr copy. Holding it pins the resource for the
//   duration of a host operation; no table lock is held while the resource is
//   used, and the payload address is stable for the resource's whole lifetime.
// - remove() detaches the resource from the table and returns the last
//   table-owned reference. Destruction therefore runs at the final release —
//   on the remover if nothing else pins it, or after the last in-flight
//   operation drops its pin — and never under the table mutex, so a possibly
//   heavy backend destructor (model / GPU teardown) cannot stall or deadlock
//   the table.
// - The internal mutex only guards the id map; it is never held while user
//   code (constructors, destructors, backend ops) runs.
template <typename T> class ResourceTable {
public:
  ResourceTable() noexcept = default;
  explicit ResourceTable(uint32_t FirstId) noexcept : NextId(FirstId) {}
  ResourceTable(const ResourceTable &) = delete;
  ResourceTable &operator=(const ResourceTable &) = delete;

  // Publish a fully-constructed resource and return its new handle, or
  // std::nullopt when the id space is exhausted.
  std::optional<uint32_t> insert(std::shared_ptr<T> Value) noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);
    if (NextId == std::numeric_limits<uint32_t>::max()) {
      return std::nullopt;
    }
    const uint32_t Id = NextId++;
    Map.emplace(Id, std::move(Value));
    return Id;
  }

  // Resolve a handle to its resource, pinning it for the caller. Returns
  // nullptr for a handle that was never allocated or was removed.
  std::shared_ptr<T> get(uint32_t Id) const noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);
    if (auto It = Map.find(Id); It != Map.end()) {
      return It->second;
    }
    return nullptr;
  }

  // Detach a handle, returning the table's reference so the caller controls
  // where the final release (and the payload destructor) runs. Returns nullptr
  // if the handle does not resolve; a double remove is therefore a no-op.
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

private:
  mutable std::mutex Mutex;
  uint32_t NextId = 0;
  std::unordered_map<uint32_t, std::shared_ptr<T>> Map;
};

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
