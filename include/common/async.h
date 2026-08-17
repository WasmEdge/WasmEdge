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

/// Async execution flow class
template <typename T> class Async {
public:
  Async() noexcept = default;

  /// Launch a detached invocation. See the keep-alive overload below; this
  /// forwards with no worker keep-alive.
  template <typename Inst, typename... FArgsT, typename... ArgsT>
  Async(T (Inst::*FPtr)(FArgsT...), Inst &TargetInst, ArgsT &&...Args)
      : Async(std::shared_ptr<void>{}, FPtr, TargetInst,
              std::forward<ArgsT>(Args)...) {}

  /// Launch a detached invocation carrying a worker-lifetime keep-alive: an
  /// opaque object destroyed on the worker thread once the invocation has
  /// returned and BEFORE its result is published, so a caller woken by
  /// get()/wait() never races the release. The GC async paths use it to hold
  /// boundary roots that pin managed call parameters until the worker has moved
  /// them onto its registered stack, and a dependency pin on the target's
  /// defining module -- released on the worker thread, so it is correct even if
  /// the caller drops this Async handle immediately (fire-and-forget). Kept
  /// type-erased so this template needs no GC knowledge. The launch lease
  /// (below) is a separate lifetime: it is released only when the LAST of
  /// {worker, this handle} is destroyed.
  template <typename Inst, typename... FArgsT, typename... ArgsT>
  Async(std::shared_ptr<void> WorkerKeepAlive, T (Inst::*FPtr)(FArgsT...),
        Inst &TargetInst, ArgsT &&...Args) {
    // Acquire a launch lease from the target's GC controller BEFORE detaching.
    // Controller teardown drains outstanding leases, so neither the detached
    // worker NOR this handle can ever touch a freed target. Targets without a
    // getController() (SFINAE) get an inert std::monostate -- no lease, no
    // behavioural change.
    //
    // If the controller is already closing, admission is refused (an empty,
    // invalid lease): launch no worker and leave this Async invalid
    // (Future.valid() == false, empty StopFunc), so a detached thread can
    // never be published against a target that teardown is concurrently
    // destroying.
    auto LaunchLease = tryLease(TargetInst, 0);
    if (!leaseAdmitted(LaunchLease, 0)) {
      return;
    }
    // Wrap the admitted lease in a heap holder co-owned by (a) the worker
    // lambda, (b) this handle (LeaseHolder member, moved along with it), and
    // (c) StopFunc. The underlying lease is released only when the LAST owner
    // is destroyed: after the worker has fully returned AND the handle itself
    // is gone. A successful result can carry GC-managed references (retained
    // by Executor::invoke) inside the shared_future, so the lease must cover
    // the result-retention window, not just execution -- the deletion-ordered
    // handle contract: release the Async before destroying the VM/Executor
    // that produced it; the teardown drain deliberately waits on a live
    // handle.
    auto Held = std::make_shared<decltype(LaunchLease)>(std::move(LaunchLease));
    LeaseHolder = Held;
    // Acquire a handle-only lease token so a fallible teardown entry can detect
    // this undeleted handle. Type-erased into a shared_ptr, exactly like the
    // launch lease, so Async stays GC-agnostic.
    HandleToken = std::make_shared<decltype(tryHandleLease(TargetInst, 0))>(
        tryHandleLease(TargetInst, 0));
    StopFunc = [Held, &TargetInst]() { TargetInst.stop(); };
    std::promise<T> Promise;
    Future = Promise.get_future();
    Thread =
        std::thread([FPtr, P = std::move(Promise), Lease = Held,
                     Keep = std::move(WorkerKeepAlive),
                     Tuple = std::tuple(
                         &TargetInst, std::forward<ArgsT>(Args)...)]() mutable {
          auto Result = std::apply(FPtr, Tuple);
          // Drop the keep-alive BEFORE publishing the result. Its release must
          // HAPPEN BEFORE the caller can observe completion: a caller returning
          // from get()/wait() is entitled to destroy the target's defining
          // module right away, and an embedder-held module is destroyed through
          // ~ModuleInstance (a plain unique_ptr delete), not terminate(), so it
          // cannot be deferred by the pin -- it asserts on it instead. Setting
          // the value first left the pin outstanding across the caller's wakeup
          // and tripped assuming(!Life.hasDependents()) in ~ModuleInstance.
          // Releasing here is safe for the boundary roots too: they only cover
          // the window until the worker moves the parameters onto its
          // registered stack, long since past once the invocation has returned,
          // and any managed reference in the result is retained separately by
          // Executor::invoke.
          Keep.reset();
          P.set_value(std::move(Result));
          // The worker's Lease ref dies here, after the result is published --
          // unlike the keep-alive it must span the result-retention window.
          // Referenced so the capture is not diagnosed as unused under -Werror.
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
  // Lease-acquisition trait. When Inst exposes getController().acquireLease()
  // (VM and Executor do), the first overload wins and returns a real launch
  // lease; otherwise it SFINAE-falls to the std::monostate overload (no lease).
  // The `int`/`long` argument disambiguates the overloads (int is the better
  // match), so a viable getController() path is always preferred.
  template <typename I>
  static auto tryLease(I &Inst, int)
      -> decltype(Inst.getController().acquireLease()) {
    return Inst.getController().acquireLease();
  }
  template <typename I> static std::monostate tryLease(I &, long) noexcept {
    return {};
  }

  // Handle-lease trait: when Inst exposes getController().acquireHandleLease()
  // (VM and Executor do), acquire a token co-owned ONLY by this handle so a
  // fallible teardown entry can detect an undeleted async handle. Targets
  // without a controller SFINAE-fall to an inert std::monostate.
  template <typename I>
  static auto tryHandleLease(I &Inst, int)
      -> decltype(Inst.getController().acquireHandleLease()) {
    return Inst.getController().acquireHandleLease();
  }
  template <typename I>
  static std::monostate tryHandleLease(I &, long) noexcept {
    return {};
  }

  // Admission trait for the lease returned by tryLease. A real
  // Controller::Lease exposes valid() (false when the controller refused
  // admission because it is closing); the int overload wins for it. The
  // std::monostate no-lease path has no valid() and SFINAE-falls to the long
  // overload, which is always admitted
  // -- a target without a GC controller has no teardown to race.
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
  // Co-owner of the launch lease (see the keep-alive constructor). Type-erased
  // so Async stays GC-agnostic, exactly like WorkerKeepAlive.
  std::shared_ptr<void> LeaseHolder;
  // Handle-only co-owner of a handle-lease token (see tryHandleLease). Held
  // for the handle's whole lifetime; dropped when the handle is destroyed.
  std::shared_ptr<void> HandleToken;
};

} // namespace WasmEdge
