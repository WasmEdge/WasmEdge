// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/waitable.h - Waitable Records -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the waitable records of the component model async
/// proposal: the entries of the per-instance unified `handles` table and the
/// stream or future rendezvous state.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/span.h"
#include "runtime/component/canonopt.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace WasmEdge {

namespace Runtime {

namespace Component {
class Task;
} // namespace Component

namespace Instance {

class ComponentInstance;

namespace Component {

/// Event codes delivered by waitable-set.wait/poll and callbacks.
enum class AsyncEventCode : uint32_t {
  None = 0,
  Subtask = 1,
  StreamRead = 2,
  StreamWrite = 3,
  FutureRead = 4,
  FutureWrite = 5,
  TaskCancelled = 6,
};

/// Codes returned by async-with-callback core functions, packed as
/// `code | (wsi << 4)`.
enum class AsyncCallbackCode : uint32_t {
  Exit = 0,
  Yield = 1,
  Wait = 2,
  Max = 2,
};

/// Result of a stream/future copy. A stream payload packs
/// `result | (progress << 4)`, a future one does not.
enum class TransmitResult : uint32_t {
  Completed = 0,
  Dropped = 1,
  Cancelled = 2,
};

/// Sentinel that an async built-in returns in place of a block.
inline constexpr uint32_t TransmitBlocked = 0xffffffffU;

/// One delivered event: code plus the two payload words written to memory.
struct AsyncEvent {
  AsyncEventCode Code = AsyncEventCode::None;
  uint32_t P1 = 0;
  uint32_t P2 = 0;
};

class WaitableSet;

/// Base of every waitable table entry: one pending-event slot, waitable-set
/// membership, and the has-sync-waiter latch.
class WaitableBase {
public:
  /// The kinds of waitables, at the values of their event codes.
  enum class Kind : uint8_t {
    Subtask = static_cast<uint8_t>(AsyncEventCode::Subtask),
    StreamRead = static_cast<uint8_t>(AsyncEventCode::StreamRead),
    StreamWrite = static_cast<uint8_t>(AsyncEventCode::StreamWrite),
    FutureRead = static_cast<uint8_t>(AsyncEventCode::FutureRead),
    FutureWrite = static_cast<uint8_t>(AsyncEventCode::FutureWrite),
  };

  WaitableBase(Kind K) noexcept : WKind(K) {}
  virtual ~WaitableBase() noexcept;

  Kind getKind() const noexcept { return WKind; }
  bool hasPendingEvent() const noexcept {
    return static_cast<bool>(PendingEvent);
  }
  void setPendingEvent(std::function<AsyncEvent()> Ev) noexcept {
    PendingEvent = std::move(Ev);
  }
  AsyncEvent takePendingEvent() noexcept {
    auto Make = std::move(PendingEvent);
    PendingEvent = nullptr;
    return Make();
  }
  bool isInWaitableSet() const noexcept { return WSet != nullptr; }
  /// Move this waitable into `Set` (nullptr = remove from any set).
  void join(WaitableSet *Set) noexcept;

  bool HasSyncWaiter = false;

private:
  friend class WaitableSet;
  Kind WKind;
  std::function<AsyncEvent()> PendingEvent;
  WaitableSet *WSet = nullptr;
};

/// A waitable-set table entry.
class WaitableSet {
public:
  ~WaitableSet() noexcept;

  bool hasPendingEvent() const noexcept {
    for (const auto *W : Elems) {
      if (W->hasPendingEvent()) {
        return true;
      }
    }
    return false;
  }
  /// Deterministic pick: the first member in join order with an event.
  AsyncEvent takePendingEvent() noexcept {
    for (auto *W : Elems) {
      if (W->hasPendingEvent()) {
        return W->takePendingEvent();
      }
    }
    return {};
  }
  bool isEmpty() const noexcept { return Elems.empty(); }

  std::vector<WaitableBase *> Elems;
  uint32_t NumWaiting = 0;
};

inline void WaitableBase::join(WaitableSet *Set) noexcept {
  if (WSet != nullptr) {
    auto &V = WSet->Elems;
    V.erase(std::remove(V.begin(), V.end(), this), V.end());
  }
  WSet = Set;
  if (Set != nullptr) {
    Set->Elems.push_back(this);
  }
}

inline WaitableBase::~WaitableBase() noexcept { join(nullptr); }

inline WaitableSet::~WaitableSet() noexcept {
  // Destruction order in the handles table is arbitrary: clear the members'
  // backlinks so their destructors do not touch a dead set.
  for (auto *W : Elems) {
    W->WSet = nullptr;
  }
}

/// Caller-side view of an async-lowered call.
class Subtask : public WaitableBase {
public:
  /// States of a caller-side subtask: the low 4 bits of an async-lowered
  /// call's packed result.
  enum class State : uint32_t {
    Starting = 0,
    Started = 1,
    Returned = 2,
    CancelledBeforeStarted = 3,
    CancelledBeforeReturned = 4,
  };

  Subtask() noexcept : WaitableBase(Kind::Subtask) {}

  bool isResolved() const noexcept {
    return Status != State::Starting && Status != State::Started;
  }
  bool isResolveDelivered() const noexcept { return Delivered; }
  /// Release the argument lends taken by the borrows lifted for the call.
  void deliverResolve() noexcept;
  /// Queue the SUBTASK progress event once the subtask has a handle.
  void noteProgress() noexcept;

  State Status = State::Starting;
  /// Requests cancellation of the callee task.
  std::function<void()> OnCancel;
  bool CancellationRequested = false;
  bool Delivered = false;
  /// Handles lent to this call, released once the call resolves.
  std::vector<std::pair<const ComponentInstance *, uint32_t>> Lenders;
  /// The callee task driven by this subtask (owned by the async runtime).
  Runtime::Component::Task *Callee = nullptr;
  /// Index in the handles table of the caller, set when an async lower
  /// registers the subtask. A progress event queues only after that.
  std::optional<uint32_t> TableIdx;
};

/// The host-memory side of a copy with a host-owned end: the elements as
/// bytes when the element type is u8, else as component values. A sink
/// consumes the elements written to it at once instead of storing them.
struct HostTransmitBuffer {
  std::vector<uint8_t> Bytes;
  std::vector<ComponentValVariant> Vals;
  /// The first element of the current copy within Bytes or Vals.
  uint32_t Base = 0;
  /// The sink of a permanently reading end; false from it ends the stream.
  std::function<bool(Span<const uint8_t>)> ByteSink;
  std::function<bool(Span<const ComponentValVariant>)> ValSink;
  bool isSink() const noexcept {
    return static_cast<bool>(ByteSink) || static_cast<bool>(ValSink);
  }
};

/// An element buffer in a copy: guest linear memory, or the host memory of a
/// host-owned end. It carries the element type as the type-index space of
/// the owning side sees it.
struct TransmitBuffer {
  /// The canonical options of the built-in that started this copy.
  Runtime::Component::CanonOptions Opts;
  std::optional<ComponentValType> Elem;
  const ComponentInstance *ElemInst = nullptr;
  uint64_t Ptr = 0;
  uint32_t Length = 0;
  uint32_t Progress = 0;
  /// The host buffer of a host-owned end; nullptr for guest memory at Ptr.
  HostTransmitBuffer *Host = nullptr;
  uint32_t getRemaining() const noexcept { return Length - Progress; }
  bool isZeroLength() const noexcept { return Length == 0; }
  bool isHost() const noexcept { return Host != nullptr; }
  bool isSink() const noexcept { return Host != nullptr && Host->isSink(); }
  /// True when the elements are bytes.
  bool isByteElem() const noexcept;
};

class TransmitEnd;

/// State shared between the readable and writable ends of one stream or
/// future.
class TransmitState {
public:
  bool IsStream = true;
  /// Element type as written at `stream.new` (the two ends' canon built-ins
  /// must agree structurally); nullopt = no payload.
  std::optional<ComponentValType> ElemType;
  /// Instance whose type-index space ElemType's indices refer to.
  const ComponentInstance *ElemTypeInst = nullptr;
  bool Dropped = false;
  /// The parked side of the rendezvous (reader or writer that arrived
  /// first), if any.
  TransmitEnd *PendingEnd = nullptr;
  /// Set once the parked buffer is exhausted, so no new rendezvous joins it.
  /// A peer drop can still turn its queued event into Dropped.
  bool PendingDone = false;
  /// A host end that arrived while the other end was parked: the scheduler
  /// completes their rendezvous at its next turn.
  TransmitEnd *PostedEnd = nullptr;
  /// A detached host-owned end, kept alive as long as the state.
  std::shared_ptr<TransmitEnd> HostEnd;
};

/// One end of a stream/future in the handles table, or a host-owned one.
class TransmitEnd : public WaitableBase {
public:
  /// Progress states of one end of a stream or future.
  enum class State : uint8_t {
    Idle,
    Copying,
    CancellingCopy,
    Done,
  };

  TransmitEnd(Kind K, std::shared_ptr<TransmitState> S) noexcept
      : WaitableBase(K), Shared(std::move(S)) {}
  ~TransmitEnd() noexcept override;

  bool isCopying() const noexcept {
    return Status == State::Copying || Status == State::CancellingCopy;
  }
  bool isFuture() const noexcept {
    return getKind() == Kind::FutureRead || getKind() == Kind::FutureWrite;
  }
  /// Queue the completion event of this end; the state transition happens
  /// at collection, which is at once for a host-owned end.
  void queueCopyEvent(TransmitResult Result, bool ReclaimPending) noexcept;
  /// Drop this end: the shared state is marked dropped, and a peer parked
  /// on it learns of the drop at once.
  void drop() noexcept;
  State Status = State::Idle;
  /// Done because the peer dropped (vs. completed successfully): selects
  /// the exact trap message on reuse.
  bool DoneByDrop = false;
  std::shared_ptr<TransmitState> Shared;
  /// This end's buffer while a copy is in flight.
  TransmitBuffer Buffer;
  /// Index of this end in its instance's handles table (event payloads).
  uint32_t TableIdx = 0;

  /// Set on a host-owned end: its events are delivered here at once instead
  /// of parking as the pending event.
  std::function<void(AsyncEvent)> OnEvent;
};

inline TransmitEnd::~TransmitEnd() noexcept {
  // The peer must not find a parked or posted end that is gone.
  if (Shared && Shared->PendingEnd == this) {
    Shared->PendingDone = false;
    Shared->PendingEnd = nullptr;
  }
  if (Shared && Shared->PostedEnd == this) {
    Shared->PostedEnd = nullptr;
  }
}

inline void TransmitEnd::queueCopyEvent(TransmitResult Result,
                                        bool ReclaimPending) noexcept {
  auto S = Shared;
  const bool Future = isFuture();
  const uint32_t Idx = TableIdx;
  setPendingEvent([this, Idx, Result, ReclaimPending, S,
                   Future]() -> AsyncEvent {
    if (ReclaimPending && S->PendingEnd == this) {
      S->PendingDone = false;
      S->PendingEnd = nullptr;
    }
    if (Future) {
      Status = (Result == TransmitResult::Dropped ||
                Result == TransmitResult::Completed)
                   ? State::Done
                   : State::Idle;
    } else {
      Status = Result == TransmitResult::Dropped ? State::Done : State::Idle;
    }
    if (Status == State::Done) {
      DoneByDrop = Result == TransmitResult::Dropped;
    }
    const uint32_t Payload =
        Future ? static_cast<uint32_t>(Result)
               : (static_cast<uint32_t>(Result) | (Buffer.Progress << 4));
    return {static_cast<AsyncEventCode>(getKind()), Idx, Payload};
  });
  if (OnEvent) {
    const AsyncEvent Ev = takePendingEvent();
    OnEvent(Ev);
  }
}

inline void TransmitEnd::drop() noexcept {
  auto S = Shared;
  if (!S) {
    return;
  }
  if (S->PendingEnd == this) {
    S->PendingDone = false;
    S->PendingEnd = nullptr;
  }
  if (S->PostedEnd == this) {
    S->PostedEnd = nullptr;
  }
  if (!S->Dropped) {
    S->Dropped = true;
    if (S->PendingEnd != nullptr) {
      auto *Peer = S->PendingEnd;
      S->PendingEnd = nullptr;
      Peer->queueCopyEvent(TransmitResult::Dropped, false);
    }
    if (S->PostedEnd != nullptr) {
      auto *Peer = S->PostedEnd;
      S->PostedEnd = nullptr;
      Peer->queueCopyEvent(TransmitResult::Dropped, false);
    }
  }
}

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
