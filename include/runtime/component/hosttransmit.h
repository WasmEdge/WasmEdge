// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/hosttransmit.h - Host Transmit Ends ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host-owned end of a stream or future: the peer of
/// a guest end that the host reads or writes through a host buffer. The end
/// only posts its copy; the executor's scheduler completes the rendezvous.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/span.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/taskmgr.h"
#include "runtime/instance/component/waitable.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// One host copy: how it ended and the number of elements moved.
struct CopyOutcome {
  Instance::Component::TransmitResult Result;
  uint32_t Progress;
};

/// A host-owned end of a stream or future. It copies through a host buffer,
/// and its events arrive at once instead of through a waitable set.
class HostTransmitEnd : public Instance::Component::TransmitEnd,
                        public std::enable_shared_from_this<HostTransmitEnd> {
public:
  HostTransmitEnd(Kind K, std::shared_ptr<Instance::Component::TransmitState> S,
                  TaskManager &Manager) noexcept
      : TransmitEnd(K, std::move(S)), TaskMgr(Manager) {}
  /// An end the host still owns drops with its owner.
  ~HostTransmitEnd() noexcept override {
    if (Shared) {
      TransmitEnd::drop();
    }
  }

  /// A stream or future the host writes and the guest reads: the guest gets
  /// the readable end as getGuestValue(). The element type reads against the
  /// instance of the activation.
  static std::shared_ptr<HostTransmitEnd>
  newWritable(CallingFrame &Frame, bool IsStream,
              std::optional<ComponentValType> Elem) {
    auto S = std::make_shared<Instance::Component::TransmitState>();
    S->IsStream = IsStream;
    S->ElemType = Elem;
    S->ElemTypeInst = Frame.getInstance();
    return std::make_shared<HostTransmitEnd>(IsStream ? Kind::StreamWrite
                                                      : Kind::FutureWrite,
                                             S, Frame.getTaskManager());
  }

  /// The host reads what the guest writes into the end it transferred.
  static std::shared_ptr<HostTransmitEnd>
  adoptReadable(CallingFrame &Frame, const std::shared_ptr<void> &SharedV) {
    auto S =
        std::static_pointer_cast<Instance::Component::TransmitState>(SharedV);
    return std::make_shared<HostTransmitEnd>(S->IsStream ? Kind::StreamRead
                                                         : Kind::FutureRead,
                                             S, Frame.getTaskManager());
  }

  /// The shared state a `Stream<T>` or `Future<T>` value carries.
  std::shared_ptr<void> getGuestValue() const noexcept { return Shared; }

  /// Write Data, waiting until the guest read all of it or dropped its end.
  Expect<CopyOutcome> write(CallingFrame &Frame,
                            Span<const uint8_t> Data) noexcept {
    Host.Bytes.assign(Data.begin(), Data.end());
    return writeAll(Frame, static_cast<uint32_t>(Data.size()));
  }
  Expect<CopyOutcome> write(CallingFrame &Frame,
                            std::vector<ComponentValVariant> Vals) noexcept {
    Host.Vals = std::move(Vals);
    return writeAll(Frame, static_cast<uint32_t>(Host.Vals.size()));
  }

  /// Read up to Max elements, waiting for the guest to write or drop.
  Expect<CopyOutcome> read(CallingFrame &Frame, std::vector<uint8_t> &Out,
                           uint32_t Max) noexcept {
    Host.Bytes.assign(Max, 0);
    prepare(Max);
    EXPECTED_TRY(auto Outcome, copy(Frame));
    Out.assign(Host.Bytes.begin(), Host.Bytes.begin() + Outcome.Progress);
    return Outcome;
  }
  Expect<CopyOutcome> read(CallingFrame &Frame,
                           std::vector<ComponentValVariant> &Out,
                           uint32_t Max) noexcept {
    Host.Vals.assign(Max, ComponentValVariant{});
    prepare(Max);
    EXPECTED_TRY(auto Outcome, copy(Frame));
    Out.assign(Host.Vals.begin(), Host.Vals.begin() + Outcome.Progress);
    return Outcome;
  }

  /// Park one value for the guest to read. The end then lives on the shared
  /// state until the value is read or the guest drops its end.
  void writeDetached(ComponentValVariant Val) noexcept {
    Host.Vals.assign(1, std::move(Val));
    prepare(1);
    Shared->HostEnd = shared_from_this();
    OnEvent = [this](Instance::Component::AsyncEvent) { finish(); };
    postCopy();
  }

  /// Consume every guest write at once through Consume, which returns false
  /// to end the stream. OnDone(Failed) runs when the guest drops its end or
  /// Consume failed.
  void sink(std::function<bool(Span<const uint8_t>)> Consume,
            std::function<void(bool)> OnDone) noexcept {
    Host.ByteSink = std::move(Consume);
    prepare(SinkCapacity);
    Shared->HostEnd = shared_from_this();
    OnEvent = [this,
               Done = std::move(OnDone)](Instance::Component::AsyncEvent Ev) {
      const auto Result =
          static_cast<Instance::Component::TransmitResult>(Ev.P2 & 0xFU);
      finish();
      Done(Result != Instance::Component::TransmitResult::Dropped);
    };
    postCopy();
  }

  /// Drop this end.
  void drop() noexcept {
    if (Shared) {
      TransmitEnd::drop();
      finish();
    }
  }

private:
  /// The element count a sink reports as its capacity.
  static inline constexpr const uint32_t SinkCapacity = (1U << 28) - 1;

  /// Point the buffer at the host storage for a copy of Length elements.
  void prepare(uint32_t Length) noexcept {
    Buffer = Instance::Component::TransmitBuffer{};
    Buffer.Opts.Inst = Shared->ElemTypeInst;
    Buffer.Elem = Shared->ElemType;
    Buffer.ElemInst = Shared->ElemTypeInst;
    Buffer.Length = Length;
    Buffer.Host = &Host;
    Host.Base = 0;
  }

  /// Post the prepared copy: park this end when nothing is pending, else hand
  /// the rendezvous with the parked peer to the scheduler. Either way the
  /// outcome reaches OnEvent.
  void postCopy() noexcept {
    Status = State::Copying;
    if (Shared->Dropped) {
      // The peer end is gone: the copy resolves at once as dropped.
      queueCopyEvent(Instance::Component::TransmitResult::Dropped, false);
      return;
    }
    if (Shared->PendingEnd == nullptr || Shared->PendingDone) {
      // Park this side; an exhausted uncollected side no longer joins.
      Shared->PendingDone = false;
      Shared->PendingEnd = this;
      return;
    }
    Shared->PostedEnd = this;
    TaskMgr.addPostedTransmit(Shared);
  }

  /// Withdraw a posted copy that got no outcome.
  void withdrawCopy() noexcept {
    if (Shared->PendingEnd == this) {
      Shared->PendingDone = false;
      Shared->PendingEnd = nullptr;
    }
    if (Shared->PostedEnd == this) {
      Shared->PostedEnd = nullptr;
    }
    Status = State::Idle;
  }

  /// Copies of the loaded host buffer until Total elements moved.
  Expect<CopyOutcome> writeAll(CallingFrame &Frame, uint32_t Total) noexcept {
    uint32_t Moved = 0;
    while (true) {
      prepare(Total - Moved);
      Host.Base = Moved;
      EXPECTED_TRY(auto Outcome, copy(Frame));
      Moved += Outcome.Progress;
      if (Outcome.Result != Instance::Component::TransmitResult::Completed ||
          Moved >= Total || Outcome.Progress == 0) {
        return CopyOutcome{Outcome.Result, Moved};
      }
    }
  }

  /// One copy over the prepared buffer, waiting for its outcome.
  Expect<CopyOutcome> copy(CallingFrame &Frame) noexcept {
    if (!Shared) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    std::optional<Instance::Component::AsyncEvent> Got;
    OnEvent = [&Got](Instance::Component::AsyncEvent Ev) { Got = Ev; };
    postCopy();
    if (!Got.has_value()) {
      auto Res = Frame.waitUntil([&Got]() { return Got.has_value(); });
      if (!Res || Frame.isCancelled()) {
        OnEvent = nullptr;
        if (!Got.has_value()) {
          withdrawCopy();
        }
        if (!Res) {
          return Unexpect(Res.error());
        }
        return CopyOutcome{Instance::Component::TransmitResult::Cancelled,
                           Buffer.Progress};
      }
    }
    OnEvent = nullptr;
    return CopyOutcome{
        static_cast<Instance::Component::TransmitResult>(Got->P2 & 0xFU),
        Buffer.Progress};
  }

  /// Let go of the shared state, so the guest ends alone decide its life.
  void finish() noexcept { Shared.reset(); }

  /// \name Data of host transmit end.
  /// @{
  Instance::Component::HostTransmitBuffer Host;
  /// The task manager whose scheduler completes the posted copies.
  TaskManager &TaskMgr;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
