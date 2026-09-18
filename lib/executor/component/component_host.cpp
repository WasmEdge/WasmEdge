// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

// A stream or future the host holds. See
// "include/executor/component/executor.h".
std::shared_ptr<Runtime::Instance::Component::Stream>
ComponentExecutor::newHostStream(const Runtime::Component::CallingFrame &Frame,
                                 bool IsFuture,
                                 std::optional<ComponentValType> Elem) {
  return std::make_shared<Runtime::Instance::Component::Stream>(
      IsFuture, std::move(Elem), Frame.getInstance());
}

// Write bytes through the writable end. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
ComponentExecutor::write(Runtime::Component::CallingFrame &Frame,
                         Runtime::Instance::Component::Stream &S,
                         Span<const uint8_t> Data) {
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::Stream::End::Writable);
  Buffer.Bytes.assign(Data.begin(), Data.end());
  Buffer.Vals.clear();
  return writeHostBuffer(Frame, S, static_cast<uint32_t>(Data.size()));
}

// Write values through the writable end. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
ComponentExecutor::write(Runtime::Component::CallingFrame &Frame,
                         Runtime::Instance::Component::Stream &S,
                         std::vector<ComponentValVariant> Vals) {
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::Stream::End::Writable);
  Buffer.Vals = std::move(Vals);
  Buffer.Bytes.clear();
  return writeHostBuffer(Frame, S, static_cast<uint32_t>(Buffer.Vals.size()));
}

// Read bytes through the readable end. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
ComponentExecutor::read(Runtime::Component::CallingFrame &Frame,
                        Runtime::Instance::Component::Stream &S,
                        std::vector<uint8_t> &Out, uint32_t Max) {
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::Stream::End::Readable);
  Buffer.Bytes.assign(Max, 0);
  Buffer.Vals.clear();
  loadHostBuffer(Frame, S, Runtime::Instance::Component::Stream::End::Readable,
                 Max);
  EXPECTED_TRY(
      auto Outcome,
      copyHostBuffer(Frame, S,
                     Runtime::Instance::Component::Stream::End::Readable));
  Out.assign(Buffer.Bytes.begin(), Buffer.Bytes.begin() + Outcome.second);
  return Outcome;
}

// Read values through the readable end. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
ComponentExecutor::read(Runtime::Component::CallingFrame &Frame,
                        Runtime::Instance::Component::Stream &S,
                        std::vector<ComponentValVariant> &Out, uint32_t Max) {
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::Stream::End::Readable);
  Buffer.Vals.assign(Max, ComponentValVariant{});
  Buffer.Bytes.clear();
  loadHostBuffer(Frame, S, Runtime::Instance::Component::Stream::End::Readable,
                 Max);
  EXPECTED_TRY(
      auto Outcome,
      copyHostBuffer(Frame, S,
                     Runtime::Instance::Component::Stream::End::Readable));
  Out.assign(Buffer.Vals.begin(), Buffer.Vals.begin() + Outcome.second);
  return Outcome;
}

// Drop an end the host holds. See "include/executor/component/executor.h".
void ComponentExecutor::drop(Runtime::Instance::Component::Stream &S,
                             Runtime::Instance::Component::Stream::End E) {
  S.onDrop(E);
}

// Write one value from a detached task. See
// "include/executor/component/executor.h".
void ComponentExecutor::writeDetached(
    Runtime::Component::CallingFrame &Frame,
    std::shared_ptr<Runtime::Instance::Component::Stream> S,
    ComponentValVariant Val) {
  Frame.spawn(
      [S = std::move(S), Val = std::move(Val)](
          Runtime::Component::CallingFrame &Spawned) mutable -> Expect<void> {
        auto &Exec = Spawned.getExecutor();
        auto Res = Exec.write(Spawned, *S,
                              std::vector<ComponentValVariant>{std::move(Val)});
        Exec.drop(*S, Runtime::Instance::Component::Stream::End::Writable);
        EXPECTED_TRY(Res);
        return {};
      });
}

// Consume every guest write from a detached task. See
// "include/executor/component/executor.h".
void ComponentExecutor::sink(
    Runtime::Component::CallingFrame &Frame,
    std::shared_ptr<Runtime::Instance::Component::Stream> S,
    std::function<bool(Span<const uint8_t>)> Consume,
    std::function<void(Runtime::Component::CallingFrame &, bool)> OnDone) {
  Frame.spawn([S = std::move(S), Consume = std::move(Consume),
               OnDone = std::move(OnDone)](
                  Runtime::Component::CallingFrame &Spawned) -> Expect<void> {
    auto &Exec = Spawned.getExecutor();
    std::vector<uint8_t> Chunk;
    while (true) {
      EXPECTED_TRY(auto Outcome, Exec.read(Spawned, *S, Chunk, SinkChunkSize));
      if (Outcome.second > 0 && !Consume(Chunk)) {
        Exec.drop(*S, Runtime::Instance::Component::Stream::End::Readable);
        OnDone(Spawned, true);
        return {};
      }
      if (Outcome.first ==
          Runtime::Instance::Component::Stream::Result::Dropped) {
        Exec.drop(*S, Runtime::Instance::Component::Stream::End::Readable);
        OnDone(Spawned, false);
        return {};
      }
      if (Outcome.first ==
              Runtime::Instance::Component::Stream::Result::Cancelled ||
          Spawned.isCancelled()) {
        Exec.drop(*S, Runtime::Instance::Component::Stream::End::Readable);
        OnDone(Spawned, true);
        return {};
      }
    }
  });
}

// Load host storage for a copy. See "include/executor/component/executor.h".
void ComponentExecutor::loadHostBuffer(
    const Runtime::Component::CallingFrame &Frame,
    Runtime::Instance::Component::Stream &S,
    Runtime::Instance::Component::Stream::End E, uint32_t Length) noexcept {
  auto &Buffer = S.getBuffer(E);
  Buffer.Opts = Runtime::Component::CanonOptions{};
  Buffer.Opts.Inst = Frame.getInstance();
  Buffer.Elem = S.getElemType();
  Buffer.ElemInst = S.getElemTypeInst();
  Buffer.Ptr = 0;
  Buffer.Length = Length;
  Buffer.Progress = 0;
  Buffer.Host = true;
  Buffer.Base = 0;
}

// One copy on a host end. See "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
ComponentExecutor::copyHostBuffer(Runtime::Component::CallingFrame &Frame,
                                  Runtime::Instance::Component::Stream &S,
                                  Runtime::Instance::Component::Stream::End E) {
  EXPECTED_TRY(copy(S, E));
  if (!S.hasPending(E)) {
    auto Res = Frame.waitUntil([&S, E]() { return S.hasPending(E); });
    if (!S.hasPending(E)) {
      // Cancelled or aborted before the peer arrived: the copy withdraws.
      S.onCancel(E);
    }
    EXPECTED_TRY(Res);
  }
  const auto [Outcome, Moved] = S.onCollect(E);
  return std::make_pair(Outcome, Moved);
}

// Write everything loaded. See "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
ComponentExecutor::writeHostBuffer(Runtime::Component::CallingFrame &Frame,
                                   Runtime::Instance::Component::Stream &S,
                                   uint32_t Total) {
  uint32_t Moved = 0;
  while (true) {
    loadHostBuffer(Frame, S,
                   Runtime::Instance::Component::Stream::End::Writable,
                   Total - Moved);
    S.getBuffer(Runtime::Instance::Component::Stream::End::Writable).Base =
        Moved;
    EXPECTED_TRY(
        auto Outcome,
        copyHostBuffer(Frame, S,
                       Runtime::Instance::Component::Stream::End::Writable));
    Moved += Outcome.second;
    if (Outcome.first !=
            Runtime::Instance::Component::Stream::Result::Completed ||
        Moved >= Total || Outcome.second == 0) {
      return std::make_pair(Outcome.first, Moved);
    }
  }
}

} // namespace Executor
} // namespace WasmEdge
