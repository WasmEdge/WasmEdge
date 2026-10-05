// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/stream.h - Stream -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the stream instance of the component model.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "runtime/instance/component/function.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;

namespace Component {

/// The elements of a copy in flight, in guest memory or host storage.
struct StreamBuffer {
  /// The elements the buffer still has room for.
  uint32_t getRemaining() const noexcept { return Length - Progress; }

  Runtime::Component::CanonOptions Opts;
  std::optional<ComponentValType> ElemType;
  const ComponentInstance *ElemTypeInst = nullptr;
  uint64_t Ptr = 0;
  uint32_t Length = 0;
  uint32_t Progress = 0;
  bool IsHost = false;
  std::vector<uint8_t> HostBytes;
  std::vector<ComponentValVariant> HostVals;
  /// The first element of the current copy within HostBytes or HostVals.
  uint32_t HostBase = 0;
};

/// A stream, or a future as a stream carrying exactly one value; each end
/// runs the end state machine of the specification and names its peer.
class StreamInstance {
public:
  /// The two ends of a stream.
  enum class Role : uint8_t {
    Reader,
    Writer,
  };

  /// How a copy ended. A stream payload packs `result | (progress << 4)`, a
  /// future one does not.
  enum class CopyResult : uint32_t {
    Completed = 0,
    Dropped = 1,
    Cancelled = 2,
  };

  StreamInstance(bool IsFuture, std::optional<ComponentValType> Elem,
                 const ComponentInstance *Inst) noexcept
      : Future(IsFuture), ElemType(std::move(Elem)), ElemTypeInst(Inst) {
    Peer[getIdx(Role::Reader)] = this;
    Peer[getIdx(Role::Writer)] = this;
  }
  StreamInstance(const StreamInstance &) = delete;
  StreamInstance &operator=(const StreamInstance &) = delete;

  /// The role of the peer end.
  static Role getPeerRole(Role Role) noexcept {
    return Role == StreamInstance::Role::Reader ? StreamInstance::Role::Writer
                                                : StreamInstance::Role::Reader;
  }

  /// \name The shape of the stream.
  /// @{
  bool isFuture() const noexcept { return Future; }
  const std::optional<ComponentValType> &getElemType() const noexcept {
    return ElemType;
  }
  /// The instance that created the stream and owns the element type; null
  /// once it is gone.
  const ComponentInstance *getElemTypeInst() const noexcept {
    return ElemTypeInst;
  }
  /// @}

  /// \name The queries on one end.
  /// @{
  /// A copy result, or the drop of the peer, waits to be collected.
  bool hasEvent(Role Role) const noexcept { return HasEvent[getIdx(Role)]; }
  /// A copy is in flight or being cancelled.
  bool isCopying(Role Role) const noexcept {
    const EndpointState Cur = Status[getIdx(Role)];
    return Cur == EndpointState::Copying ||
           Cur == EndpointState::CancellingCopy;
  }
  /// The end collected its last event: a drop of its peer, or the value of a
  /// future.
  bool isDone(Role Role) const noexcept {
    return Status[getIdx(Role)] == EndpointState::Done;
  }
  /// Done because the peer dropped.
  bool isDoneByDrop(Role Role) const noexcept {
    return DoneByDrop[getIdx(Role)];
  }
  /// The stream the peer end belongs to; null once the peer is gone.
  StreamInstance *getPeer(Role Role) const noexcept {
    return Peer[getIdx(Role)];
  }
  /// The buffer of the copy on the end, loaded before it starts.
  StreamBuffer &getBuffer(Role Role) noexcept { return Buffer[getIdx(Role)]; }
  const StreamBuffer &getBuffer(Role Role) const noexcept {
    return Buffer[getIdx(Role)];
  }
  /// The waitable set the end joined; 0 for none.
  uint32_t getSetIdx(Role Role) const noexcept { return SetIdx[getIdx(Role)]; }
  void setSetIdx(Role Role, uint32_t Idx) noexcept {
    SetIdx[getIdx(Role)] = Idx;
  }
  /// The instance whose handle table names the end; null while the end is in
  /// transfer or gone.
  ComponentInstance *getHolder(Role Role) const noexcept {
    return Holder[getIdx(Role)];
  }
  void setHolder(Role Role, ComponentInstance *Inst) noexcept {
    Holder[getIdx(Role)] = Inst;
  }
  /// Whether a synchronous built-in waits on the end.
  bool isSyncWaiter(Role Role) const noexcept {
    return SyncWaiter[getIdx(Role)];
  }
  void setSyncWaiter(Role Role, bool IsWaiter) noexcept {
    SyncWaiter[getIdx(Role)] = IsWaiter;
  }
  /// @}

  /// \name The queries on the whole stream.
  /// @{
  /// Both ends dropped or forwarded away.
  bool isClosed() const noexcept { return Gone[0] && Gone[1]; }
  /// @}

  /// \name The transitions, driven by the executor.
  /// @{
  /// A copy starts on the end of Role with its loaded buffer: the peer stream
  /// and the count to move now with `onMove`; null when nothing moves.
  std::pair<StreamInstance *, uint32_t> onCopy(Role Role) noexcept {
    const size_t Self = getIdx(Role);
    Status[Self] = EndpointState::Copying;
    StreamInstance *Other = Peer[Self];
    // The peer is gone: its drop is the event already waiting.
    if (Other == nullptr) {
      return {nullptr, 0};
    }
    const StreamInstance::Role PeerRole = getPeerRole(Role);
    const size_t OtherIdx = getIdx(PeerRole);
    StreamBuffer &Mine = Buffer[Self];
    StreamBuffer &Theirs = Other->Buffer[OtherIdx];
    if (!Other->Pending[OtherIdx]) {
      Pending[Self] = true;
      return {nullptr, 0};
    }
    if (Mine.getRemaining() > 0 && Theirs.getRemaining() > 0) {
      return {Other, std::min(Mine.getRemaining(), Theirs.getRemaining())};
    }
    // A zero-length copy on one side: the pending peer finishes when this one
    // has room, or when both only probe and this one reads.
    if (Mine.getRemaining() > 0 ||
        (Role == StreamInstance::Role::Reader && Theirs.getRemaining() == 0)) {
      Other->notify(PeerRole, 0);
      Other->Pending[OtherIdx] = false;
      Pending[Self] = true;
      return {nullptr, 0};
    }
    notify(Role, 0);
    return {nullptr, 0};
  }

  /// Count elements moved by the copy on the end of Role: both ends report
  /// their progress, and the peer keeps its buffer pending while it has room.
  void onMove(Role Role, uint32_t Count) noexcept {
    const size_t Self = getIdx(Role);
    StreamInstance *Other = Peer[Self];
    const StreamInstance::Role PeerRole = getPeerRole(Role);
    const size_t OtherIdx = getIdx(PeerRole);
    Buffer[Self].Progress += Count;
    Other->Buffer[OtherIdx].Progress += Count;
    notify(Role, Buffer[Self].Progress);
    Other->notify(PeerRole, Other->Buffer[OtherIdx].Progress);
    if (Other->Buffer[OtherIdx].getRemaining() == 0) {
      Other->Pending[OtherIdx] = false;
    }
  }

  /// Take the event of the end of Role: the result and the progress.
  std::pair<CopyResult, uint32_t> onCollect(Role Role) noexcept {
    const size_t Self = getIdx(Role);
    const uint32_t Progress = EventProgress[Self];
    HasEvent[Self] = false;
    EventProgress[Self] = 0;
    CopyResult Taken = CopyResult::Completed;
    if (Future && Progress == 1) {
      Status[Self] = EndpointState::Done;
    } else if (Peer[Self] == nullptr) {
      Taken = CopyResult::Dropped;
      Status[Self] = EndpointState::Done;
      DoneByDrop[Self] = true;
    } else if (Status[Self] == EndpointState::CancellingCopy) {
      Taken = CopyResult::Cancelled;
      Status[Self] = EndpointState::Idle;
    } else {
      Status[Self] = EndpointState::Idle;
    }
    Pending[Self] = false;
    return {Taken, Progress};
  }

  /// Cancel the copy on the end of Role; it reports at once.
  void onCancel(Role Role) noexcept {
    const size_t Self = getIdx(Role);
    Status[Self] = EndpointState::CancellingCopy;
    if (!HasEvent[Self]) {
      notify(Role, 0);
    }
  }

  /// Drop the end of Role: its peer learns of it unless it is done or
  /// already has an event.
  void onDrop(Role Role) noexcept {
    const size_t Self = getIdx(Role);
    unlinkPeer(Role);
    Gone[Self] = true;
    Holder[Self] = nullptr;
    Pending[Self] = false;
    HasEvent[Self] = false;
  }

  /// Forward the reader end into the writer end of Dst: both ends go and
  /// their peers meet; returns the end whose pending copy runs again, or null.
  std::pair<StreamInstance *, Role> onForward(StreamInstance &Dst) noexcept {
    const size_t Src = getIdx(Role::Reader);
    const size_t DstIdx = getIdx(Role::Writer);
    StreamInstance *Writer = Peer[Src];
    StreamInstance *Reader = Dst.Peer[DstIdx];
    // The peers cannot meet: both ends drop instead.
    if (Writer == &Dst || Writer == nullptr || Reader == nullptr) {
      onDrop(Role::Reader);
      Dst.onDrop(Role::Writer);
      return {nullptr, Role::Reader};
    }
    Writer->Peer[DstIdx] = Reader;
    Reader->Peer[Src] = Writer;
    Peer[Src] = nullptr;
    Dst.Peer[DstIdx] = nullptr;
    onDrop(Role::Reader);
    Dst.onDrop(Role::Writer);
    if (!Writer->Pending[DstIdx] || !Reader->Pending[Src]) {
      return {nullptr, Role::Reader};
    }
    // Both wait: the smaller copy runs again against the bigger one.
    if (Reader->Buffer[Src].getRemaining() >
        Writer->Buffer[DstIdx].getRemaining()) {
      Writer->Pending[DstIdx] = false;
      return {Writer, Role::Writer};
    }
    Reader->Pending[Src] = false;
    return {Reader, Role::Reader};
  }

  /// The creating instance goes: the ends still here lose their peers as if
  /// both dropped, and the handles others hold stay valid.
  void onCreatorGone() noexcept {
    ElemTypeInst = nullptr;
    for (const auto Cur : {Role::Reader, Role::Writer}) {
      if (!Gone[getIdx(Cur)]) {
        unlinkPeer(Cur);
        if (!isDone(Cur) && !hasEvent(Cur)) {
          notify(Cur, 0);
        }
      }
    }
  }
  /// @}

private:
  /// The state of one endpoint: the `End.State` of the specification.
  enum class EndpointState : uint8_t {
    Idle,
    Copying,
    CancellingCopy,
    Done,
  };

  static size_t getIdx(Role Role) noexcept { return static_cast<size_t>(Role); }
  /// Record the event of the end of Role; a later one replaces it.
  void notify(Role Role, uint32_t Progress) noexcept {
    const size_t Self = getIdx(Role);
    HasEvent[Self] = true;
    EventProgress[Self] = Progress;
  }
  /// Unlink the end of Role from its peer, which learns of it unless it is
  /// done or already has an event.
  void unlinkPeer(Role Role) noexcept {
    const size_t Self = getIdx(Role);
    StreamInstance *Other = Peer[Self];
    Peer[Self] = nullptr;
    if (Other == nullptr) {
      return;
    }
    const StreamInstance::Role PeerRole = getPeerRole(Role);
    const size_t OtherIdx = getIdx(PeerRole);
    Other->Peer[OtherIdx] = nullptr;
    if (!Other->isDone(PeerRole) && !Other->hasEvent(PeerRole)) {
      Other->notify(PeerRole, 0);
    }
  }

  /// \name Data of stream.
  /// @{
  const bool Future;
  const std::optional<ComponentValType> ElemType;
  const ComponentInstance *ElemTypeInst;
  /// The state of each end, indexed by Role.
  EndpointState Status[2] = {EndpointState::Idle, EndpointState::Idle};
  /// The stream of the peer; the peer end is always the other role.
  StreamInstance *Peer[2] = {nullptr, nullptr};
  StreamBuffer Buffer[2];
  /// The buffer waits for the peer to copy.
  bool Pending[2] = {false, false};
  /// The event waiting to be collected and its progress.
  bool HasEvent[2] = {false, false};
  uint32_t EventProgress[2] = {0, 0};
  bool DoneByDrop[2] = {false, false};
  /// The end was dropped or forwarded away.
  bool Gone[2] = {false, false};
  ComponentInstance *Holder[2] = {nullptr, nullptr};
  uint32_t SetIdx[2] = {0, 0};
  bool SyncWaiter[2] = {false, false};
  /// @}
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
