// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/stream.h - Stream definition --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the stream: one stream or future of the component
/// model, with the state machine of each of its two ends and the rendezvous
/// between them.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "runtime/instance/component/function.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;

namespace Component {

/// A stream, or a future as a stream that carries exactly one value. The end
/// that arrives first parks in the rendezvous until its peer arrives.
class Stream {
public:
  enum class End : uint8_t {
    Readable,
    Writable,
  };

  /// How a copy ended. A stream payload packs `result | (progress << 4)`, a
  /// future one does not.
  enum class Result : uint32_t {
    Completed = 0,
    Dropped = 1,
    Cancelled = 2,
  };

  /// The elements of a copy in flight: a range of guest memory, or host
  /// storage as bytes for u8 elements and component values otherwise.
  struct Buffer {
    Runtime::Component::CanonOptions Opts;
    std::optional<ComponentValType> Elem;
    const ComponentInstance *ElemInst = nullptr;
    uint64_t Ptr = 0;
    uint32_t Length = 0;
    uint32_t Progress = 0;
    bool Host = false;
    std::vector<uint8_t> Bytes;
    std::vector<ComponentValVariant> Vals;
    /// The first element of the current copy within Bytes or Vals.
    uint32_t Base = 0;
  };

  Stream(bool IsFuture, std::optional<ComponentValType> Elem,
         const ComponentInstance *Inst) noexcept
      : Future(IsFuture), ElemType(std::move(Elem)), ElemTypeInst(Inst) {}
  Stream(const Stream &) = delete;
  Stream &operator=(const Stream &) = delete;

  /// The stream a value carries as its shared state.
  static std::shared_ptr<Stream>
  from(const std::shared_ptr<void> &Shared) noexcept {
    return std::static_pointer_cast<Stream>(Shared);
  }
  /// The end opposite to E.
  static End getPeer(End E) noexcept {
    return E == End::Readable ? End::Writable : End::Readable;
  }

  /// \name The shape of the stream.
  /// @{
  bool isFuture() const noexcept { return Future; }
  const std::optional<ComponentValType> &getElemType() const noexcept {
    return ElemType;
  }
  const ComponentInstance *getElemTypeInst() const noexcept {
    return ElemTypeInst;
  }
  /// @}

  /// \name The queries on one end.
  /// @{
  /// An outcome waits to be collected.
  bool hasPending(End E) const noexcept {
    const State Status = getSide(E).Status;
    return Status == State::Completed || Status == State::Cancelled ||
           Status == State::Dropped;
  }
  /// A copy is in flight: parked, or with its outcome not yet collected.
  bool isBusy(End E) const noexcept {
    return getSide(E).Status == State::Parked || hasPending(E);
  }
  bool isDone(End E) const noexcept { return getSide(E).Status == State::Done; }
  /// Done because the peer dropped rather than because a value crossed.
  bool isDoneByDrop(End E) const noexcept { return getSide(E).DoneByDrop; }
  /// Whether the peer of E dropped its end.
  bool isPeerDropped(End E) const noexcept {
    return getSide(getPeer(E)).Dropped;
  }
  /// Whether E holds the rendezvous with its buffer.
  bool isParked(End E) const noexcept {
    return Parked.has_value() && *Parked == E;
  }
  Buffer &getBuffer(End E) noexcept { return getSide(E).CopyBuffer; }
  const Buffer &getBuffer(End E) const noexcept {
    return getSide(E).CopyBuffer;
  }
  /// The waitable set the handle of E joined; 0 for none.
  uint32_t getSetIdx(End E) const noexcept { return getSide(E).SetIdx; }
  void setSetIdx(End E, uint32_t Idx) noexcept { getSide(E).SetIdx = Idx; }
  /// Whether a synchronous built-in waits on E.
  bool isSyncWaiter(End E) const noexcept { return getSide(E).SyncWaiter; }
  void setSyncWaiter(End E, bool IsWaiter) noexcept {
    getSide(E).SyncWaiter = IsWaiter;
  }
  /// @}

  /// \name The transitions, driven by the executor.
  /// @{
  /// A copy starts on E: the number of elements to move now, or 0 when E
  /// parked, completed with nothing, or found the peer gone.
  uint32_t onCopy(End E) noexcept {
    Side &Self = getSide(E);
    const End Peer = getPeer(E);
    Side &PeerSide = getSide(Peer);
    // The peer is gone: the copy resolves at once as dropped.
    if (PeerSide.Dropped) {
      Self.Status = State::Dropped;
      return 0;
    }
    // Nobody waits, or the waiting buffer is full: this end parks.
    if (!Parked.has_value() || Exhausted || *Parked == E) {
      Parked = E;
      Exhausted = false;
      Self.Status = State::Parked;
      return 0;
    }
    // A zero-length copy completes with nothing moved and leaves the peer
    // as it is, except that a parked zero-length write yields to any read.
    if (Self.CopyBuffer.Length == 0 &&
        !(E == End::Readable && getRemaining(Peer) == 0)) {
      Self.Status = State::Completed;
      return 0;
    }
    // A parked zero-length peer only probed for readiness: it completes with
    // nothing, and this end parks in its place.
    if (getRemaining(Peer) == 0) {
      PeerSide.Status = State::Completed;
      Parked = E;
      Exhausted = false;
      Self.Status = State::Parked;
      return 0;
    }
    Active = E;
    return std::min(getRemaining(E), getRemaining(Peer));
  }

  /// Count elements moved: the active end completes, the parked one
  /// accumulates them and stays in the rendezvous while it has room left.
  void onMove(uint32_t Count) noexcept {
    if (!Active.has_value() || !Parked.has_value()) {
      return;
    }
    Side &ActiveSide = getSide(*Active);
    Side &ParkedSide = getSide(*Parked);
    ActiveSide.CopyBuffer.Progress += Count;
    ParkedSide.CopyBuffer.Progress += Count;
    ActiveSide.Status = State::Completed;
    ParkedSide.Status = State::Completed;
    if (getRemaining(*Parked) == 0) {
      Exhausted = true;
    }
    Active.reset();
  }

  /// Take the outcome of E, its result and progress, and leave the
  /// rendezvous; a drop or a completed future ends E for good.
  std::pair<Result, uint32_t> onCollect(End E) noexcept {
    Side &Self = getSide(E);
    const Result Res = Self.Status == State::Completed   ? Result::Completed
                       : Self.Status == State::Cancelled ? Result::Cancelled
                                                         : Result::Dropped;
    const uint32_t Count = Self.CopyBuffer.Progress;
    leaveRendezvous(E);
    if (Res == Result::Dropped || (Future && Res == Result::Completed)) {
      Self.Status = State::Done;
      Self.DoneByDrop = Res == Result::Dropped;
    } else {
      Self.Status = State::Idle;
    }
    return {Res, Count};
  }

  /// Cancel the copy on E: a parked end leaves with nothing, a completed
  /// stream copy reports as cancelled, every other outcome passes through.
  void onCancel(End E) noexcept {
    Side &Self = getSide(E);
    if (Self.Status == State::Parked) {
      leaveRendezvous(E);
      Self.Status = State::Cancelled;
    } else if (Self.Status == State::Completed) {
      leaveRendezvous(E);
      if (!Future) {
        Self.Status = State::Cancelled;
      }
    }
  }

  /// Drop E: a peer waiting in the rendezvous resolves as dropped, except a
  /// future whose value already crossed.
  void onDrop(End E) noexcept {
    Side &Self = getSide(E);
    Self.Dropped = true;
    leaveRendezvous(E);
    Self.Status = State::Done;
    const End Peer = getPeer(E);
    if (isParked(Peer) && (!Future || !Exhausted)) {
      leaveRendezvous(Peer);
      getSide(Peer).Status = State::Dropped;
    }
  }
  /// @}

private:
  /// The states of one end. Completed, Cancelled, and Dropped hold the
  /// outcome of a copy, with its progress, until it is collected.
  enum class State : uint8_t {
    Idle,
    Parked,
    Completed,
    Cancelled,
    Dropped,
    Done,
  };

  /// One end: its state, the buffer of its copy, and the flags of its
  /// handle.
  struct Side {
    State Status = State::Idle;
    Buffer CopyBuffer;
    bool Dropped = false;
    bool DoneByDrop = false;
    uint32_t SetIdx = 0;
    bool SyncWaiter = false;
  };

  Side &getSide(End E) noexcept { return Ends[static_cast<uint8_t>(E)]; }
  const Side &getSide(End E) const noexcept {
    return Ends[static_cast<uint8_t>(E)];
  }
  /// The elements the buffer of E still has room for.
  uint32_t getRemaining(End E) const noexcept {
    return getSide(E).CopyBuffer.Length - getSide(E).CopyBuffer.Progress;
  }
  /// E gives the rendezvous up if it holds it.
  void leaveRendezvous(End E) noexcept {
    if (isParked(E)) {
      Parked.reset();
      Exhausted = false;
    }
  }

  /// \name Data of stream.
  /// @{
  const bool Future;
  const std::optional<ComponentValType> ElemType;
  const ComponentInstance *const ElemTypeInst;
  /// The end that arrived first and waits for its peer with its buffer.
  std::optional<End> Parked;
  /// The parked buffer is full: no new copy joins the rendezvous.
  bool Exhausted = false;
  /// The end whose copy is being moved.
  std::optional<End> Active;
  Side Ends[2];
  /// @}
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
