// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/async.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Object model of the component-model async proposal: the entries of the
/// per-instance unified `handles` table (waitable sets, subtasks,
/// stream/future copy ends, error contexts) and the shared rendezvous state
/// between the two ends of a stream or future. The scheduler that drives
/// these objects lives in the executor (executor/component/async_runtime.h);
/// this header stays free of executor dependencies.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/types.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {

namespace Executor {
// The executor-side task and thread records; referenced opaquely by the
// runtime layer.
class ComponentTask;
struct ComponentThreadCtx;
} // namespace Executor

namespace Runtime {
namespace Instance {

class ComponentInstance;
class MemoryInstance;
class FunctionInstance;

namespace Component {

/// Event codes delivered by waitable-set.wait/poll and callbacks
/// (CanonicalABI.md `class EventCode`).
enum class AsyncEventCode : uint32_t {
  None = 0,
  Subtask = 1,
  StreamRead = 2,
  StreamWrite = 3,
  FutureRead = 4,
  FutureWrite = 5,
  TaskCancelled = 6,
};

/// Codes returned by async-with-callback core functions
/// (CanonicalABI.md `class CallbackCode`): packed `code | (wsi << 4)`.
enum class AsyncCallbackCode : uint32_t {
  Exit = 0,
  Yield = 1,
  Wait = 2,
  Max = 2,
};

/// Result of a stream/future copy operation (CanonicalABI.md `CopyResult`).
/// Stream event payloads pack `result | (progress << 4)`; future payloads
/// carry the bare result.
enum class AsyncCopyResult : uint32_t {
  Completed = 0,
  Dropped = 1,
  Cancelled = 2,
};

/// Sentinel returned by async built-ins that would block.
inline constexpr uint32_t AsyncBlocked = 0xffffffffU;

/// States of a caller-side subtask (CanonicalABI.md `Subtask.State`). The
/// low 4 bits of an async-lowered call's packed result.
enum class SubtaskState : uint32_t {
  Starting = 0,
  Started = 1,
  Returned = 2,
  CancelledBeforeStarted = 3,
  CancelledBeforeReturned = 4,
};

/// One delivered event: code plus the two payload words written to memory.
struct AsyncEvent {
  AsyncEventCode Code = AsyncEventCode::None;
  uint32_t P1 = 0;
  uint32_t P2 = 0;
};

class WaitableSetObj;

/// Base of every waitable table entry (subtasks and copy ends): a single
/// pending-event slot (a lazily-evaluated thunk, per spec), waitable-set
/// membership, and the has-sync-waiter latch.
class WaitableObj {
public:
  enum class Kind : uint8_t {
    Subtask,
    StreamRead,
    StreamWrite,
    FutureRead,
    FutureWrite,
  };

  WaitableObj(Kind K) noexcept : WKind(K) {}
  virtual ~WaitableObj() noexcept;

  Kind getKind() const noexcept { return WKind; }
  bool hasPendingEvent() const noexcept {
    return static_cast<bool>(PendingEvent);
  }
  void setPendingEvent(std::function<AsyncEvent()> Ev) noexcept {
    PendingEvent = std::move(Ev);
  }
  AsyncEvent takePendingEvent() noexcept {
    auto Thunk = std::move(PendingEvent);
    PendingEvent = nullptr;
    return Thunk();
  }
  bool inWaitableSet() const noexcept { return WSet != nullptr; }
  WaitableSetObj *getSet() const noexcept { return WSet; }
  /// Move this waitable into `Set` (nullptr = remove from any set).
  void join(WaitableSetObj *Set) noexcept;

  bool HasSyncWaiter = false;

private:
  friend class WaitableSetObj;
  Kind WKind;
  std::function<AsyncEvent()> PendingEvent;
  WaitableSetObj *WSet = nullptr;
};

/// A waitable-set table entry.
class WaitableSetObj {
public:
  ~WaitableSetObj() noexcept;

  bool hasPendingEvent() const noexcept {
    for (const auto *W : Elems) {
      if (W->hasPendingEvent()) {
        return true;
      }
    }
    return false;
  }
  /// Deterministic replacement of the spec's shuffled pick: first member in
  /// join order with a pending event.
  AsyncEvent takePendingEvent() noexcept {
    for (auto *W : Elems) {
      if (W->hasPendingEvent()) {
        return W->takePendingEvent();
      }
    }
    return {};
  }
  bool empty() const noexcept { return Elems.empty(); }

  std::vector<WaitableObj *> Elems;
  uint32_t NumWaiting = 0;
};

inline void WaitableObj::join(WaitableSetObj *Set) noexcept {
  if (WSet != nullptr) {
    auto &V = WSet->Elems;
    V.erase(std::remove(V.begin(), V.end(), this), V.end());
  }
  WSet = Set;
  if (Set != nullptr) {
    Set->Elems.push_back(this);
  }
}

inline WaitableObj::~WaitableObj() noexcept { join(nullptr); }

inline WaitableSetObj::~WaitableSetObj() noexcept {
  // Destruction order in the handles table is arbitrary: clear the members'
  // backlinks so their destructors do not touch a dead set.
  for (auto *W : Elems) {
    W->WSet = nullptr;
  }
}

/// Caller-side view of an async-lowered call (CanonicalABI.md `Subtask`).
class SubtaskObj : public WaitableObj {
public:
  SubtaskObj() noexcept : WaitableObj(Kind::Subtask) {}

  bool resolved() const noexcept {
    return State != SubtaskState::Starting && State != SubtaskState::Started;
  }
  bool resolveDelivered() const noexcept { return Delivered; }

  SubtaskState State = SubtaskState::Starting;
  /// Requests cancellation of the callee task (task.request_cancellation).
  std::function<void()> OnCancel;
  bool CancellationRequested = false;
  bool Delivered = false;
  /// Handles lent to this call via lift_borrow: released at deliverResolve.
  std::vector<std::pair<const ComponentInstance *, uint32_t>> Lenders;
  /// The callee task driven by this subtask (owned by the async runtime).
  Executor::ComponentTask *Callee = nullptr;
  /// Index in the caller's handles table once registered by an async lower;
  /// progress events are only queued after registration.
  std::optional<uint32_t> TableIdx;
};

/// Progress states of one end of a stream/future (CanonicalABI.md
/// `CopyState`), plus the cause of the Done state for exact diagnostics.
enum class CopyState : uint8_t {
  Idle,
  Copying,
  CancellingCopy,
  Done,
};

/// A guest linear-memory element buffer taking part in a copy: everything
/// the executor needs to lift from / lower into the owning side, including
/// the element type as seen from that side's type-index space.
struct GuestBufferDesc {
  const ComponentInstance *Inst = nullptr;
  MemoryInstance *Mem = nullptr;
  FunctionInstance *Realloc = nullptr;
  StringEncoding Enc = StringEncoding::UTF8;
  std::optional<ComponentValType> Elem;
  const ComponentInstance *ElemInst = nullptr;
  uint32_t Ptr = 0;
  uint32_t Length = 0;
  uint32_t Progress = 0;
  uint32_t remain() const noexcept { return Length - Progress; }
  bool zeroLength() const noexcept { return Length == 0; }
};

class CopyEndObj;

/// The state shared between the readable and writable ends of one stream or
/// future (CanonicalABI.md `SharedStreamImpl` / `SharedFutureImpl`).
class SharedCopyObj {
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
  /// Set once the parked side's buffer is exhausted: a new rendezvous op no
  /// longer joins it, but its queued Completed event can still be flipped to
  /// Dropped by a drop of the peer end before it is collected.
  bool PendingDone = false;
  CopyEndObj *PendingEnd = nullptr;
  GuestBufferDesc PendingBuffer;
};

/// One end of a stream/future in the handles table.
class CopyEndObj : public WaitableObj {
public:
  CopyEndObj(Kind K, std::shared_ptr<SharedCopyObj> S) noexcept
      : WaitableObj(K), Shared(std::move(S)) {}

  bool copying() const noexcept {
    return St == CopyState::Copying || St == CopyState::CancellingCopy;
  }
  bool isReadEnd() const noexcept {
    return getKind() == Kind::StreamRead || getKind() == Kind::FutureRead;
  }

  CopyState St = CopyState::Idle;
  /// Done because the peer dropped (vs. completed successfully): selects
  /// the exact trap message on reuse.
  bool DoneByDrop = false;
  std::shared_ptr<SharedCopyObj> Shared;
  /// This end's buffer while a copy is in flight.
  GuestBufferDesc Buffer;
  /// Index of this end in its instance's handles table (event payloads).
  uint32_t TableIdx = 0;
};

/// An error-context table entry.
struct ErrorContextObj {
  std::string Message;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
