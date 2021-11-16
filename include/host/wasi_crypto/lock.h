// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

//https://stackoverflow.com/questions/67993098/how-to-simulate-rusts-mutexobject-in-c
template <typename T>
class Mutex {
private:
  T value;
  std::mutex mutex;

public:
  // Fill out some constructors, probably some kind of emplacement constructor too.
  // For simplicity of the example, this should be okay:
  explicit Mutex(T value)
      : value(std::move(value))
  {}

  template <typename F>
  auto locked(F&& fn) const& -> std::invoke_result_t<F&&, T const&> {
    // Lock the mutex while invoking the function.
    // scoped_lock automatically unlocks at the end of the scope
    std::scoped_lock lock(mutex);
    return std::invoke(std::forward<F>(fn), value);
  }

  template <typename F>
  auto locked(F&& fn) & -> std::invoke_result_t<F&&, T&> {
    std::scoped_lock lock(mutex);
    return std::invoke(std::forward<F>(fn), value);
  }

  // Can be worth repeating for const&& and && as well
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
