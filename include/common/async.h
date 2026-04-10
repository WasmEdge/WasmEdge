// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/async.h - Asynchronous execution class definition -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Async class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "errcode.h"

#include <future>
#include <memory>
#include <thread>
#if defined(__APPLE__) || defined(__linux__)
#include <pthread.h>
#endif

namespace WasmEdge {
namespace detail {

template <typename Fn> void runDetachedWithLargeStack(Fn &&Func) {
#if defined(__APPLE__) || defined(__linux__)
  using FnBox = std::decay_t<Fn>;
  auto *const BoxPtr = new FnBox(std::forward<Fn>(Func));
  pthread_attr_t Attr;
  pthread_attr_init(&Attr);
  constexpr size_t StackBytes = 8U * 1024U * 1024U;
  pthread_attr_setstacksize(&Attr, StackBytes);
  pthread_t Th;
  const int Err = pthread_create(
      &Th, &Attr,
      +[](void *Arg) -> void * {
        std::unique_ptr<FnBox> Box(static_cast<FnBox *>(Arg));
        (*Box)();
        return nullptr;
      },
      BoxPtr);
  pthread_attr_destroy(&Attr);
  if (Err != 0) {
    std::thread([Held = std::unique_ptr<FnBox>(BoxPtr)]() mutable {
      (*Held)();
    }).detach();
    return;
  }
  pthread_detach(Th);
#else
  std::thread(std::forward<Fn>(Func)).detach();
#endif
}

} // namespace detail

/// Async execution flow class
template <typename T> class Async {
public:
  Async() noexcept = default;
  template <typename Inst, typename... FArgsT, typename... ArgsT>
  Async(T (Inst::*FPtr)(FArgsT...), Inst &TargetInst, ArgsT &&...Args)
      : StopFunc([&TargetInst]() { TargetInst.stop(); }) {
    std::promise<T> Promise;
    Future = Promise.get_future();
    detail::runDetachedWithLargeStack(
        [FPtr, P = std::move(Promise),
         Tuple =
             std::tuple(&TargetInst, std::forward<ArgsT>(Args)...)]() mutable {
          P.set_value(std::apply(FPtr, Tuple));
        });
  }
  Async(const Async &) noexcept = delete;
  Async(Async &&Other) noexcept : Async() { swap(*this, Other); }
  Async &operator=(const Async &) = delete;
  Async &operator=(Async &&Other) noexcept {
    swap(*this, Other);
    return *this;
  }

  bool valid() const noexcept { return Future.valid(); }

  T get() const { return Future.get(); }

  void wait() const { Future.wait(); }

  template <typename RT, typename PT>
  bool waitFor(const std::chrono::duration<RT, PT> &Timeout) const {
    return Future.wait_for(Timeout) == std::future_status::ready;
  }

  template <typename CT, typename DT>
  bool waitUntil(const std::chrono::time_point<CT, DT> &Timeout) const {
    return Future.wait_until(Timeout) == std::future_status::ready;
  }

  friend void swap(Async &LHS, Async &RHS) noexcept {
    using std::swap;
    swap(LHS.Future, RHS.Future);
    swap(LHS.Thread, RHS.Thread);
    swap(LHS.StopFunc, RHS.StopFunc);
  }

  void cancel() noexcept {
    if (likely(StopFunc.operator bool())) {
      StopFunc();
    }
  }

protected:
  std::shared_future<T> Future;
  std::thread Thread;
  std::function<void()> StopFunc;
};

} // namespace WasmEdge
