// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mutex>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

/// An rust-like mutex

template <typename T> class MutexGuard {
private:
  T *Value;
//  std::recursive_mutex *InnerMutex;

public:
  MutexGuard(T *Value/*, std::recursive_mutex *Mutex*/) : Value(Value)/*, InnerMutex(Mutex)*/ {}

  ~MutexGuard() {
//    if (InnerMutex != nullptr) {
//      InnerMutex->unlock();
//    }
  }

  MutexGuard(MutexGuard &&Guard) noexcept {
    Value = std::exchange(Guard.Value, nullptr);
//    InnerMutex = std::exchange(Guard.mutex, nullptr);
  }

  MutexGuard &operator=(MutexGuard &&Guard) noexcept {
    Value = std::exchange(Guard.Value, nullptr);
//    InnerMutex = std::exchange(Guard.mutex, nullptr);
  }

  MutexGuard(const MutexGuard &) = delete;
  MutexGuard &operator=(const MutexGuard &) = delete;

//  operator bool() const { return InnerMutex != nullptr; }

  T &operator*() const noexcept { return *Value; }

  T *operator->() const noexcept { return Value; }
};

template <typename T> class Mutex {
private:
  mutable std::recursive_mutex InnerMutex;
  mutable T Value;

public:
  explicit Mutex(T Value) : Value(Value) {}
  Mutex(Mutex &&) noexcept = default;
  Mutex &operator=(Mutex &&) noexcept = default;
  Mutex(const Mutex &) = delete;
  Mutex &operator=(const Mutex &) = delete;
  MutexGuard<T> tryLock() const {
    if (InnerMutex.try_lock()) {
      return MutexGuard<T>(&Value, &InnerMutex);
    }
    return MutexGuard<T>(nullptr, nullptr);
  }

  MutexGuard<T> lock() const {
//    InnerMutex.lock();
    return MutexGuard<T>(&Value/*, &InnerMutex*/);
  }

  template <typename F>
  std::invoke_result_t<F &&, T const &> lockedAction(F &&Fn) const & {
    std::scoped_lock Lock(InnerMutex);
    return std::invoke(std::forward<F>(Fn), Value);
  }

  template <typename F> std::invoke_result_t<F &&, T &> lockedAction(F &&Fn) & {
    std::scoped_lock Lock(InnerMutex);
    return std::invoke(std::forward<F>(Fn), Value);
  }

  static T unwrap(Mutex<T> &&Self) { return std::move(Self.Value); }
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
