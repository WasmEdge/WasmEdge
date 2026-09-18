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
// Visit the values within Val a host holds in another form: the stream and
// future ends, and the strings in transfer.
Expect<void>
forEachHostVal(ComponentValVariant &Val,
               const std::function<Expect<void>(ComponentValVariant &)> &Func) {
  auto *Comp = std::get_if<std::shared_ptr<ValComp>>(&Val);
  if (Comp == nullptr || !*Comp) {
    return {};
  }
  auto &Inner = (*Comp)->V;
  if (std::holds_alternative<StreamFutureVal>(Inner) ||
      std::holds_alternative<StringVal>(Inner)) {
    return Func(Val);
  }
  if (auto *Record = std::get_if<RecordVal>(&Inner)) {
    for (auto &Field : Record->Fields) {
      EXPECTED_TRY(forEachHostVal(Field, Func));
    }
  } else if (auto *Tuple = std::get_if<TupleVal>(&Inner)) {
    for (auto &Elem : Tuple->Values) {
      EXPECTED_TRY(forEachHostVal(Elem, Func));
    }
  } else if (auto *List = std::get_if<ListVal>(&Inner)) {
    for (auto &Elem : List->Elements) {
      EXPECTED_TRY(forEachHostVal(Elem, Func));
    }
  } else if (auto *Case = std::get_if<VariantVal>(&Inner)) {
    if (Case->Payload.has_value()) {
      EXPECTED_TRY(forEachHostVal(*Case->Payload, Func));
    }
  } else if (auto *Option = std::get_if<OptionVal>(&Inner)) {
    if (Option->Value.has_value()) {
      EXPECTED_TRY(forEachHostVal(*Option->Value, Func));
    }
  } else if (auto *Result = std::get_if<ResultVal>(&Inner)) {
    if (Result->Payload.has_value()) {
      EXPECTED_TRY(forEachHostVal(*Result->Payload, Func));
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
  const uint32_t WriteIdx = Root->addHostStreamHandle(
      *S, Runtime::Instance::Component::StreamInstance::Role::Writer);
  const uint32_t ReadIdx = Root->addHostStreamHandle(
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
      Frame.getInstance()->getRoot()->findHostStreamHandle(HandleIdx);
  if (Slot == nullptr || Slot->Role != Role) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    unknown host stream handle {}"sv, HandleIdx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  return Slot->Stream;
}

// A value enters a host. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::lowerHostValue(
    Runtime::Instance::ComponentInstance &HostRoot, ComponentValVariant &Val) {
  return forEachHostVal(
      Val, [&HostRoot](ComponentValVariant &HostVal) -> Expect<void> {
        if (isComponentVal<StringVal>(HostVal)) {
          std::string Str = getComponentVal<StringVal>(HostVal).Str;
          HostVal = std::move(Str);
          return {};
        }
        auto &End = std::get<StreamFutureVal>(
            std::get<std::shared_ptr<ValComp>>(HostVal)->V);
        if (End.Stream != nullptr) {
          End.HandleIdx = HostRoot.addHostStreamHandle(
              *End.Stream,
              Runtime::Instance::Component::StreamInstance::Role::Reader);
          End.Stream = nullptr;
        }
        return {};
      });
}

// A value leaves a host. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::liftHostValue(Runtime::Instance::ComponentInstance &HostRoot,
                                 ComponentValVariant &Val) {
  return forEachHostVal(
      Val, [&HostRoot](ComponentValVariant &HostVal) -> Expect<void> {
        auto *Found = std::get_if<StreamFutureVal>(
            &std::get<std::shared_ptr<ValComp>>(HostVal)->V);
        if (Found == nullptr) {
          return {};
        }
        StreamFutureVal &End = *Found;
        const auto *Slot = HostRoot.findHostStreamHandle(End.HandleIdx);
        if (Slot == nullptr ||
            Slot->Role !=
                Runtime::Instance::Component::StreamInstance::Role::Reader) {
          spdlog::error(ErrCode::Value::FuncSigMismatch);
          spdlog::error("    host value names no readable end: handle {}"sv,
                        End.HandleIdx);
          return Unexpect(ErrCode::Value::FuncSigMismatch);
        }
        End.Stream = HostRoot.removeHostStreamHandle(End.HandleIdx);
        End.HandleIdx = 0;
        return {};
      });
}

// Write bytes as the writer. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::StreamInstance::CopyResult>
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
  return writeHostBuffer(Frame, HandleIdx, S,
                         static_cast<uint32_t>(Data.size()));
}

// Write values as the writer. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::StreamInstance::CopyResult>
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
    if (S.getElemType().has_value() && S.getElemTypeInst() != nullptr) {
      EXPECTED_TRY(checkHostValue(*S.getElemTypeInst(), *S.getElemType(), Val));
    }
    EXPECTED_TRY(liftHostValue(*Frame.getInstance()->getRoot(), Val));
  }
  Buffer.HostVals = std::move(Vals);
  Buffer.HostBytes.clear();
  return writeHostBuffer(Frame, HandleIdx, S,
                         static_cast<uint32_t>(Buffer.HostVals.size()));
}

// Read bytes as the reader. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::StreamInstance::CopyResult>
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
  // The bytes of an earlier read stay: only what a copy moved is given out.
  Buffer.HostBytes.resize(Max);
  Buffer.HostVals.clear();
  loadHostBuffer(S, Runtime::Instance::Component::StreamInstance::Role::Reader,
                 Max);
  EXPECTED_TRY(auto Outcome,
               copyHostBuffer(
                   Frame, HandleIdx, S,
                   Runtime::Instance::Component::StreamInstance::Role::Reader));
  Out.assign(Buffer.HostBytes.begin(),
             Buffer.HostBytes.begin() + Outcome.second);
  return Outcome.first;
}

// Read values as the reader. See
// "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::StreamInstance::CopyResult>
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
  loadHostBuffer(S, Runtime::Instance::Component::StreamInstance::Role::Reader,
                 Max);
  EXPECTED_TRY(auto Outcome,
               copyHostBuffer(
                   Frame, HandleIdx, S,
                   Runtime::Instance::Component::StreamInstance::Role::Reader));
  Out.assign(Buffer.HostVals.begin(), Buffer.HostVals.begin() + Outcome.second);
  for (auto &Val : Out) {
    EXPECTED_TRY(lowerHostValue(*Frame.getInstance()->getRoot(), Val));
  }
  return Outcome.first;
}

// Drop a handle the host holds. See
// "include/executor/component/executor.h".
void ComponentExecutor::dropStreamHandle(
    const Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx) {
  auto *Root = Frame.getInstance()->getRoot();
  if (const auto *Slot = Root->findHostStreamHandle(HandleIdx);
      Slot != nullptr) {
    const auto Role = Slot->Role;
    Root->removeHostStreamHandle(HandleIdx)->onDrop(Role);
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
    EXPECTED_TRY(const auto Outcome,
                 readStream(Frame, HandleIdx, Chunk, SinkChunkSize));
    const bool Stopped = (!Chunk.empty() && !Consume(Chunk)) ||
                         Outcome == Runtime::Instance::Component::
                                        StreamInstance::CopyResult::Cancelled ||
                         Frame.isCancelled();
    if (Stopped ||
        Outcome ==
            Runtime::Instance::Component::StreamInstance::CopyResult::Dropped) {
      dropStreamHandle(Frame, HandleIdx);
      return Stopped;
    }
  }
}

// Load host storage for a copy. See "include/executor/component/executor.h".
void ComponentExecutor::loadHostBuffer(
    Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role,
    uint32_t Length) noexcept {
  auto &Buffer = S.getBuffer(Role);
  Buffer.Opts = Runtime::Component::CanonOptions{};
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
    Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
    Runtime::Instance::Component::StreamInstance &S,
    Runtime::Instance::Component::StreamInstance::Role Role) {
  EXPECTED_TRY(startStreamCopy(S, Role));
  if (!S.hasEvent(Role)) {
    auto Res = Frame.waitUntil([&S, Role]() { return S.hasEvent(Role); });
    if (!S.hasEvent(Role)) {
      // Cancelled or aborted before the peer arrived: the copy withdraws.
      S.onCancel(Role);
    }
    if (!Res) {
      // The task is over, so the end it copied on goes with it.
      S.onCollect(Role);
      dropStreamHandle(Frame, HandleIdx);
      return Unexpect(Res);
    }
  }
  const auto [Outcome, Moved] = S.onCollect(Role);
  return std::make_pair(Outcome, Moved);
}

// Write everything loaded. See "include/executor/component/executor.h".
Expect<Runtime::Instance::Component::StreamInstance::CopyResult>
ComponentExecutor::writeHostBuffer(
    Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
    Runtime::Instance::Component::StreamInstance &S, uint32_t Total) {
  uint32_t Moved = 0;
  while (true) {
    loadHostBuffer(S,
                   Runtime::Instance::Component::StreamInstance::Role::Writer,
                   Total - Moved);
    S.getBuffer(Runtime::Instance::Component::StreamInstance::Role::Writer)
        .HostBase = Moved;
    EXPECTED_TRY(
        auto Outcome,
        copyHostBuffer(
            Frame, HandleIdx, S,
            Runtime::Instance::Component::StreamInstance::Role::Writer));
    Moved += Outcome.second;
    if (Outcome.first != Runtime::Instance::Component::StreamInstance::
                             CopyResult::Completed ||
        Moved >= Total || Outcome.second == 0) {
      return Outcome.first;
    }
  }
}

} // namespace Executor
} // namespace WasmEdge
