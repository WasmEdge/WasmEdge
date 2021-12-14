// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

// https://stackoverflow.com/questions/67993098/how-to-simulate-rusts-mutexobject-in-c
template <typename T> class Mutex {
private:
  T value;
  std::mutex mutex;

public:
  // Fill out some constructors, probably some kind of emplacement constructor
  // too. For simplicity of the example, this should be okay:
  explicit Mutex(T value) : value(std::move(value)) {}

//  template <typename F>
//  std::invoke_result_t<F &&, T const &> locked(F &&fn) const & {
//    // Lock the mutex while invoking the function.
//    // scoped_lock automatically unlocks at the end of the scope
//    std::scoped_lock lock(mutex);
//    return std::invoke(std::forward<F>(fn), value);
//  }

  template <typename F> std::invoke_result_t<F &&, T &> locked(F &&fn) & {
    std::scoped_lock lock(mutex);
    return std::invoke(std::forward<F>(fn), value);
  }

  // Can be worth repeating for const&& and && as well

  template <typename M1, typename M2, typename F>
  friend auto acquireLocked(Mutex<M1> &m1, Mutex<M2> &m2, F &&fn);
};

template <typename M1, typename M2, typename F>
auto acquireLocked(Mutex<M1> &m1, Mutex<M2> &m2, F &&fn) {
  std::scoped_lock lock(m1.mutex, m2.mutex);
  return std::invoke(std::forward<F>(fn), m1.value, m2.value);
}

// template <typename... Args, typename F>
// friend std::invoke_result_t<F &&, Args...> acquireLocked(Mutex<Args>... Arg,
//                                                          F &&fn);
// };
//
// template <typename... Args, typename F>
// std::invoke_result_t<F &&, Args...> acquireLocked(Mutex<Args>... Arg, F &&fn)
// {
//   std::scoped_lock lock(Arg.mutex...);
//   return std::invoke(std::forward<F>(fn), std::forward<Args>(Arg.value)...);
// }
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
