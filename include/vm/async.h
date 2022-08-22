// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/vm/async.h - Asynchronous Result class definition --------===//
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

#include "vm.h"

#include <future>
#include <thread>

namespace WasmEdge {
namespace VM {

/// VM execution flow class
template <typename T> class Async {
public:
  Async() noexcept = default;
  template <typename... FArgsT, typename... ArgsT>
  Async(T (VM::*FPtr)(FArgsT...), VM &TargetVM, ArgsT &&...Args)
      : VMPtr(&TargetVM) {
    std::promise<T> Promise;
    Future = Promise.get_future();
    Thread =
        std::thread([FPtr, P = std::move(Promise),
                     Tuple = std::tuple(
                         &TargetVM, std::forward<ArgsT>(Args)...)]() mutable {
          std::get<0>(Tuple)->newThread();
          P.set_value(std::apply(FPtr, Tuple));
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
    swap(LHS.VMPtr, RHS.VMPtr);
  }

  void cancel() noexcept {
    if (likely(VMPtr)) {
      VMPtr->stop();
    }
  }

private:
  std::shared_future<T> Future;
  std::thread Thread;
  VM *VMPtr;
};

} // namespace VM
} // namespace WasmEdge
