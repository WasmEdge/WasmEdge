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

#include "ast/component/type.h"
#include "common/types.h"
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
  enum class Kind : uint8_t {
    Subtask,
    StreamRead,
    StreamWrite,
    FutureRead,
    FutureWrite,
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
  bool inWaitableSet() const noexcept { return WSet != nullptr; }
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
  bool empty() const noexcept { return Elems.empty(); }

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

  bool resolved() const noexcept {
    return St != State::Starting && St != State::Started;
  }
  bool resolveDelivered() const noexcept { return Delivered; }

  State St = State::Starting;
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

/// A guest linear-memory element buffer in a copy. It carries the element
/// type as the type-index space of the owning side sees it.
struct TransmitBuffer {
  /// The canonical options of the built-in that started this copy.
  Runtime::Component::CanonOptions Opts;
  std::optional<ComponentValType> Elem;
  const ComponentInstance *ElemInst = nullptr;
  uint32_t Ptr = 0;
  uint32_t Length = 0;
  uint32_t Progress = 0;
  uint32_t remain() const noexcept { return Length - Progress; }
  bool zeroLength() const noexcept { return Length == 0; }
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
  bool HasPending = false;
  /// Set once the parked buffer is exhausted, so no new rendezvous joins it.
  /// A peer drop can still turn its queued event into Dropped.
  bool PendingDone = false;
  TransmitEnd *PendingEnd = nullptr;
};

/// One end of a stream/future in the handles table.
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

  bool copying() const noexcept {
    return St == State::Copying || St == State::CancellingCopy;
  }
  State St = State::Idle;
  /// Done because the peer dropped (vs. completed successfully): selects
  /// the exact trap message on reuse.
  bool DoneByDrop = false;
  std::shared_ptr<TransmitState> Shared;
  /// This end's buffer while a copy is in flight.
  TransmitBuffer Buffer;
  /// Index of this end in its instance's handles table (event payloads).
  uint32_t TableIdx = 0;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
