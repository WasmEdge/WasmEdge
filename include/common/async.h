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

#include <future>
#include <memory>
#include <thread>
#include <variant>

namespace WasmEdge {

/// Async execution flow class.
///
/// With a GC-controlled target (VM or Executor), the handle keeps the
/// target's launch lease for its whole lifetime, because the result can hold
/// GC references into the target's heap. The target's destructor waits until
/// the handle is destroyed, so destroy the handle first. The C API rejects the
/// delete of a target that still has a handle.
template <typename T> class Async {
public:
  Async() noexcept = default;

  /// Launch a detached invocation. See the keep-alive overload below; this
  /// forwards with no worker keep-alive.
  template <typename Inst, typename... FArgsT, typename... ArgsT>
  Async(T (Inst::*FPtr)(FArgsT...), Inst &TargetInst, ArgsT &&...Args)
      : Async(std::shared_ptr<void>{}, FPtr, TargetInst,
              std::forward<ArgsT>(Args)...) {}

  /// Launch a detached invocation with a worker keep-alive: an opaque object
  /// that the worker destroys after the invocation returns and before it
  /// publishes the result, so a caller woken by get() or wait() never races the
  /// release. The GC paths use it to root the call parameters until the worker
  /// moves them onto its stack, and to pin the target's defining module. It is
  /// type-erased so this template needs no GC knowledge. The launch lease has
  /// a separate lifetime: it is released when both the worker and this handle
  /// are destroyed.
  template <typename Inst, typename... FArgsT, typename... ArgsT>
  Async(std::shared_ptr<void> WorkerKeepAlive, T (Inst::*FPtr)(FArgsT...),
        Inst &TargetInst, ArgsT &&...Args) {
    // Take a launch lease from the target's GC controller before the worker
    // starts. Controller teardown waits for all leases, so neither the worker
    // nor this handle can use a freed target. A target without
    // getController() gets a std::monostate (no lease). If the controller is
    // closing, it refuses the lease: then start no worker and leave this Async
    // invalid.
    auto LaunchLease = tryLease(TargetInst, 0);
    if (!leaseAdmitted(LaunchLease, 0)) {
      return;
    }
    // The worker, this handle (LeaseHolder) and StopFunc share the lease. It
    // is released when the last of them is destroyed, because the result in
    // Future can hold GC references that Executor::invoke retained.
    auto Held = std::make_shared<decltype(LaunchLease)>(std::move(LaunchLease));
    LeaseHolder = Held;
    // A token that only this handle holds, so that a fallible teardown (the
    // C API delete) can detect a handle that is not yet destroyed.
    auto HandleLease = tryHandleLease(TargetInst, 0);
    HandleToken =
        std::make_shared<decltype(HandleLease)>(std::move(HandleLease));
    StopFunc = [Held, &TargetInst]() { TargetInst.stop(); };
    std::promise<T> Promise;
    Future = Promise.get_future();
    Thread =
        std::thread([FPtr, P = std::move(Promise), Lease = Held,
                     Keep = std::move(WorkerKeepAlive),
                     Tuple = std::tuple(
                         &TargetInst, std::forward<ArgsT>(Args)...)]() mutable {
          auto Result = std::apply(FPtr, Tuple);
          // Release the keep-alive before the result is visible: a caller
          // that returns from get() or wait() can destroy the target's module
          // at once, and ~ModuleInstance asserts that no pin is left. The
          // parameter roots are no longer needed, and Executor::invoke retains
          // any reference in the result.
          Keep.reset();
          P.set_value(std::move(Result));
          // The Lease capture exists only to keep the lease alive until this
          // lambda is destroyed, after the result is published. This use
          // prevents an unused-capture warning.
          (void)Lease;
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
    swap(LHS.LeaseHolder, RHS.LeaseHolder);
    swap(LHS.HandleToken, RHS.HandleToken);
  }

  void cancel() noexcept {
    if (likely(StopFunc.operator bool())) {
      StopFunc();
    }
  }

protected:
  // Lease-acquisition trait. When Inst has getController().acquireLease() (VM
  // and Executor), the int overload is the better match and returns a launch
  // lease. Otherwise SFINAE selects the std::monostate overload (no lease).
  template <typename I>
  static auto tryLease(I &Inst, int)
      -> decltype(Inst.getController().acquireLease()) {
    return Inst.getController().acquireLease();
  }
  template <typename I> static std::monostate tryLease(I &, long) noexcept {
    return {};
  }

  // Handle-lease trait, selected as for tryLease: a token held only by this
  // handle, or std::monostate for a target without a controller.
  template <typename I>
  static auto tryHandleLease(I &Inst, int)
      -> decltype(Inst.getController().acquireHandleLease()) {
    return Inst.getController().acquireHandleLease();
  }
  template <typename I>
  static std::monostate tryHandleLease(I &, long) noexcept {
    return {};
  }

  // Admission trait for the lease returned by tryLease. A Controller::Lease
  // has valid(), which is false when the controller is closing. A
  // std::monostate has no valid() and is always admitted, because a target
  // without a GC controller has no teardown to race.
  template <typename L>
  static auto leaseAdmitted(const L &Lease, int) -> decltype(Lease.valid()) {
    return Lease.valid();
  }
  template <typename L> static bool leaseAdmitted(const L &, long) noexcept {
    return true;
  }

  std::shared_future<T> Future;
  std::thread Thread;
  std::function<void()> StopFunc;
  // Shares the launch lease with the worker (see the keep-alive constructor).
  std::shared_ptr<void> LeaseHolder;
  // The handle-lease token (see tryHandleLease), held for this handle's
  // lifetime.
  std::shared_ptr<void> HandleToken;
};

} // namespace WasmEdge
