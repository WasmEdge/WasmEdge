// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/async.h - Asynchronous execution class definition -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the Async class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "errcode.h"

#include <functional>
#include <future>
#include <thread>
#include <tuple>
#include <utility>

namespace WasmEdge {

/// Async execution flow class
template <typename T> class Async {
public:
  Async() noexcept = default;
  /// Call FPtr on TargetInst on a new thread. TargetInst.startRun() must
  /// return a pointer to a run that has requestStop() and call(Func). cancel()
  /// stops that run, and the worker calls FPtr through call().
  template <typename Inst, typename... FArgsT, typename... ArgsT>
  Async(T (Inst::*FPtr)(FArgsT...), Inst &TargetInst, ArgsT &&...Args) {
    auto Run = TargetInst.startRun();
    StopFunc = [Run]() { Run->requestStop(); };
    std::promise<T> Promise;
    Future = Promise.get_future();
    Thread = std::thread([FPtr, P = std::move(Promise), Run = std::move(Run),
                          Tuple = std::tuple(
                              &TargetInst,
                              std::forward<ArgsT>(Args)...)]() mutable {
      P.set_value(Run->call([&]() -> T { return std::apply(FPtr, Tuple); }));
    });
    Thread.detach();
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

  /// Stop the run of this handle only. The run fails with Interrupted at its
  /// next checkpoint. A cancel does not interrupt a host function. Compiled
  /// code has checkpoints of its own only if it was compiled as interruptible.
  /// Without them, a cancel still reaches compiled code when it calls a
  /// function through the runtime or waits in memory.atomic.wait. A run that
  /// ends before its next checkpoint ends normally.
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
