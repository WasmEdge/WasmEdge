// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/spdlog.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
// Visit the stream and future values within Val.
Expect<void>
forEachStreamVal(ComponentValVariant &Val,
                 const std::function<Expect<void>(StreamFutureVal &)> &Func) {
  auto *Comp = std::get_if<std::shared_ptr<ValComp>>(&Val);
  if (Comp == nullptr || !*Comp) {
    return {};
  }
  auto &Inner = (*Comp)->V;
  if (auto *End = std::get_if<StreamFutureVal>(&Inner)) {
    return Func(*End);
  }
  if (auto *Record = std::get_if<RecordVal>(&Inner)) {
    for (auto &Field : Record->Fields) {
      EXPECTED_TRY(forEachStreamVal(Field.second, Func));
    }
  } else if (auto *Tuple = std::get_if<TupleVal>(&Inner)) {
    for (auto &Elem : Tuple->Values) {
      EXPECTED_TRY(forEachStreamVal(Elem, Func));
    }
  } else if (auto *List = std::get_if<ListVal>(&Inner)) {
    for (auto &Elem : List->Elements) {
      EXPECTED_TRY(forEachStreamVal(Elem, Func));
    }
  } else if (auto *Case = std::get_if<VariantVal>(&Inner)) {
    if (Case->Payload.has_value()) {
      EXPECTED_TRY(forEachStreamVal(*Case->Payload, Func));
    }
  } else if (auto *Option = std::get_if<OptionVal>(&Inner)) {
    if (Option->Value.has_value()) {
      EXPECTED_TRY(forEachStreamVal(*Option->Value, Func));
    }
  } else if (auto *Result = std::get_if<ResultVal>(&Inner)) {
    if (Result->Payload.has_value()) {
      EXPECTED_TRY(forEachStreamVal(*Result->Payload, Func));
    }
  }
  return {};
}
} // namespace

// A stream or future the host holds. See
// "include/executor/component/executor.h".
std::pair<uint32_t, uint32_t>
ComponentExecutor::newHostStream(const Runtime::Component::CallingFrame &Frame,
                                 bool IsFuture,
                                 std::optional<ComponentValType> Elem) {
  auto *Root = Frame.getInstance()->getRoot();
  auto *S = Root->newStream(IsFuture, std::move(Elem), Frame.getInstance());
  const uint32_t WriteIdx = Root->addStreamHandle(
      *S, Runtime::Instance::Component::StreamInstance::Role::Writer);
  const uint32_t ReadIdx = Root->addStreamHandle(
      *S, Runtime::Instance::Component::StreamInstance::Role::Reader);
  return {WriteIdx, ReadIdx};
}

// The stream behind a host handle. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::StreamInstance *>
ComponentExecutor::getHostStream(
    const Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
    Runtime::Instance::Component::StreamInstance::Role Role) {
  const auto *Slot =
      Frame.getInstance()->getRoot()->findStreamHandle(HandleIdx);
  if (Slot == nullptr || Slot->Role != Role) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    unknown host stream handle {}"sv, HandleIdx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  return Slot->Stream;
}

// Stream values enter a host. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::lowerHostStreams(
    Runtime::Instance::ComponentInstance &HostRoot, ComponentValVariant &Val) {
  return forEachStreamVal(
      Val, [&HostRoot](StreamFutureVal &End) -> Expect<void> {
        if (End.Stream != nullptr) {
          End.HandleIdx = HostRoot.addStreamHandle(
              *End.Stream,
              Runtime::Instance::Component::StreamInstance::Role::Reader);
          End.Stream = nullptr;
        }
        return {};
      });
}

// Stream values leave a host. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::liftHostStreams(
    Runtime::Instance::ComponentInstance &HostRoot, ComponentValVariant &Val) {
  return forEachStreamVal(
      Val, [&HostRoot](StreamFutureVal &End) -> Expect<void> {
        const auto *Slot = HostRoot.findStreamHandle(End.HandleIdx);
        if (Slot == nullptr ||
            Slot->Role !=
                Runtime::Instance::Component::StreamInstance::Role::Reader) {
          spdlog::error(ErrCode::Value::FuncSigMismatch);
          spdlog::error("    host value names no readable end: handle {}"sv,
                        End.HandleIdx);
          return Unexpect(ErrCode::Value::FuncSigMismatch);
        }
        End.Stream = HostRoot.removeStreamHandle(End.HandleIdx);
        End.HandleIdx = 0;
        return {};
      });
}

// Write bytes as the writer. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                 uint32_t>>
ComponentExecutor::writeStream(Runtime::Component::CallingFrame &Frame,
                               uint32_t HandleIdx, Span<const uint8_t> Data) {
  EXPECTED_TRY(auto *Stream,
               getHostStream(
                   Frame, HandleIdx,
                   Runtime::Instance::Component::StreamInstance::Role::Writer));
  Runtime::Instance::Component::StreamInstance &S = *Stream;
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::StreamInstance::Role::Writer);
  Buffer.HostBytes.assign(Data.begin(), Data.end());
  Buffer.HostVals.clear();
  return writeHostBuffer(Frame, S, static_cast<uint32_t>(Data.size()));
}

// Write values as the writer. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                 uint32_t>>
ComponentExecutor::writeStream(Runtime::Component::CallingFrame &Frame,
                               uint32_t HandleIdx,
                               std::vector<ComponentValVariant> Vals) {
  EXPECTED_TRY(auto *Stream,
               getHostStream(
                   Frame, HandleIdx,
                   Runtime::Instance::Component::StreamInstance::Role::Writer));
  Runtime::Instance::Component::StreamInstance &S = *Stream;
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::StreamInstance::Role::Writer);
  for (auto &Val : Vals) {
    EXPECTED_TRY(liftHostStreams(*Frame.getInstance()->getRoot(), Val));
  }
  Buffer.HostVals = std::move(Vals);
  Buffer.HostBytes.clear();
  return writeHostBuffer(Frame, S,
                         static_cast<uint32_t>(Buffer.HostVals.size()));
}

// Read bytes as the reader. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                 uint32_t>>
ComponentExecutor::readStream(Runtime::Component::CallingFrame &Frame,
                              uint32_t HandleIdx, std::vector<uint8_t> &Out,
                              uint32_t Max) {
  EXPECTED_TRY(auto *Stream,
               getHostStream(
                   Frame, HandleIdx,
                   Runtime::Instance::Component::StreamInstance::Role::Reader));
  Runtime::Instance::Component::StreamInstance &S = *Stream;
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::StreamInstance::Role::Reader);
  Buffer.HostBytes.assign(Max, 0);
  Buffer.HostVals.clear();
  loadHostBuffer(Frame, S,
                 Runtime::Instance::Component::StreamInstance::Role::Reader,
                 Max);
  EXPECTED_TRY(auto Outcome,
               copyHostBuffer(
                   Frame, S,
                   Runtime::Instance::Component::StreamInstance::Role::Reader));
  Out.assign(Buffer.HostBytes.begin(),
             Buffer.HostBytes.begin() + Outcome.second);
  return Outcome;
}

// Read values as the reader. See
// "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                 uint32_t>>
ComponentExecutor::readStream(Runtime::Component::CallingFrame &Frame,
                              uint32_t HandleIdx,
                              std::vector<ComponentValVariant> &Out,
                              uint32_t Max) {
  EXPECTED_TRY(auto *Stream,
               getHostStream(
                   Frame, HandleIdx,
                   Runtime::Instance::Component::StreamInstance::Role::Reader));
  Runtime::Instance::Component::StreamInstance &S = *Stream;
  auto &Buffer =
      S.getBuffer(Runtime::Instance::Component::StreamInstance::Role::Reader);
  Buffer.HostVals.assign(Max, ComponentValVariant{});
  Buffer.HostBytes.clear();
  loadHostBuffer(Frame, S,
                 Runtime::Instance::Component::StreamInstance::Role::Reader,
                 Max);
  EXPECTED_TRY(auto Outcome,
               copyHostBuffer(
                   Frame, S,
                   Runtime::Instance::Component::StreamInstance::Role::Reader));
  Out.assign(Buffer.HostVals.begin(), Buffer.HostVals.begin() + Outcome.second);
  for (auto &Val : Out) {
    EXPECTED_TRY(lowerHostStreams(*Frame.getInstance()->getRoot(), Val));
  }
  return Outcome;
}

// Drop a handle the host holds. See
// "include/executor/component/executor.h".
void ComponentExecutor::dropStreamHandle(
    const Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx) {
  auto *Root = Frame.getInstance()->getRoot();
  if (const auto *Slot = Root->findStreamHandle(HandleIdx); Slot != nullptr) {
    const auto Role = Slot->Role;
    Root->removeStreamHandle(HandleIdx)->onDrop(Role);
  }
}

// Write one value from a detached task. See
// "include/executor/component/executor.h".
void ComponentExecutor::spawnStreamWrite(
    Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
    ComponentValVariant Val) {
  Frame.spawn(
      [HandleIdx, Written = std::move(Val)](
          Runtime::Component::CallingFrame &Spawned) mutable -> Expect<void> {
        auto &Exec = Spawned.getExecutor();
        auto Res = Exec.writeStream(
            Spawned, HandleIdx,
            std::vector<ComponentValVariant>{std::move(Written)});
        Exec.dropStreamHandle(Spawned, HandleIdx);
        EXPECTED_TRY(Res);
        return {};
      });
}

// Drain the guest writes of a stream. See
// "include/executor/component/executor.h".
Expect<bool> ComponentExecutor::drainStream(
    Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
    const std::function<bool(Span<const uint8_t>)> &Consume) {
  std::vector<uint8_t> Chunk;
  while (true) {
    EXPECTED_TRY(auto Outcome,
                 readStream(Frame, HandleIdx, Chunk, SinkChunkSize));
    const bool Stopped = (Outcome.second > 0 && !Consume(Chunk)) ||
                         Outcome.first ==
                             Runtime::Instance::Component::StreamInstance::
                                 CopyResult::Cancelled ||
                         Frame.isCancelled();
    if (Stopped ||
        Outcome.first ==
            Runtime::Instance::Component::StreamInstance::CopyResult::Dropped) {
      dropStreamHandle(Frame, HandleIdx);
      return Stopped;
    }
  }
}

// Load host storage for a copy. See "include/executor/component/executor.h".
void ComponentExecutor::loadHostBuffer(
    const Runtime::Component::CallingFrame &Frame,
    Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role,
    uint32_t Length) noexcept {
  auto &Buffer = S.getBuffer(Role);
  Buffer.Opts = Runtime::Component::CanonOptions{};
  Buffer.Opts.Inst = Frame.getInstance();
  Buffer.ElemType = S.getElemType();
  Buffer.ElemTypeInst = S.getElemTypeInst();
  Buffer.Ptr = 0;
  Buffer.Length = Length;
  Buffer.Progress = 0;
  Buffer.IsHost = true;
  Buffer.HostBase = 0;
}

// One copy on a host role. See "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                 uint32_t>>
ComponentExecutor::copyHostBuffer(
    Runtime::Component::CallingFrame &Frame,
    Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role) {
  EXPECTED_TRY(startStreamCopy(S, Role));
  if (!S.hasEvent(Role)) {
    auto Res = Frame.waitUntil([&S, Role]() { return S.hasEvent(Role); });
    if (!S.hasEvent(Role)) {
      // Cancelled or aborted before the peer arrived: the copy withdraws.
      S.onCancel(Role);
    }
    EXPECTED_TRY(Res);
  }
  const auto [Outcome, Moved] = S.onCollect(Role);
  return std::make_pair(Outcome, Moved);
}

// Write everything loaded. See "include/executor/component/executor.h".
Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                 uint32_t>>
ComponentExecutor::writeHostBuffer(
    Runtime::Component::CallingFrame &Frame,
    Runtime::Instance::Component::StreamInstance &S, uint32_t Total) {
  uint32_t Moved = 0;
  while (true) {
    loadHostBuffer(Frame, S,
                   Runtime::Instance::Component::StreamInstance::Role::Writer,
                   Total - Moved);
    S.getBuffer(Runtime::Instance::Component::StreamInstance::Role::Writer)
        .HostBase = Moved;
    EXPECTED_TRY(
        auto Outcome,
        copyHostBuffer(
            Frame, S,
            Runtime::Instance::Component::StreamInstance::Role::Writer));
    Moved += Outcome.second;
    if (Outcome.first != Runtime::Instance::Component::StreamInstance::
                             CopyResult::Completed ||
        Moved >= Total || Outcome.second == 0) {
      return std::make_pair(Outcome.first, Moved);
    }
  }
}

} // namespace Executor
} // namespace WasmEdge
