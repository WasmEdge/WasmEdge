// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/async_thunk.h -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Synthesized core wasm functions backing the async-model canonical
/// built-ins over the task runtime: task/context/yield/backpressure,
/// waitable sets, subtasks, stream/future copies, and error contexts.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "executor/component/canonical_abi.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/component/component.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace Executor {

class Executor;

/// Immediates and options of one async canonical built-in, resolved at
/// instantiation time from the Canonical AST node.
struct AsyncBuiltinInfo {
  ComponentCanonOpCode Code;
  const Runtime::Instance::ComponentInstance *Inst = nullptr;
  Runtime::Instance::MemoryInstance *Mem = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  StringEncoding Enc = StringEncoding::UTF8;
  /// The `async`/`cancellable` immediate of wait/poll/yield/cancel ops.
  bool Cancellable = false;
  /// stream/future ops: the element type declared at the built-in.
  std::optional<ComponentValType> Elem;
  bool IsStream = true;
  /// context.get/set slot index.
  uint32_t CtxIdx = 0;
  /// task.return: declared result types.
  std::vector<ComponentValType> RetTypes;
  /// thread.new-indirect: the core table holding start functions.
  Runtime::Instance::TableInstance *Table = nullptr;
};

class CanonAsyncBuiltinHostFunc : public Runtime::HostFunctionBase {
public:
  CanonAsyncBuiltinHostFunc(Executor *ExecIn, AsyncBuiltinInfo InfoIn) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  /// stream/future read/write rendezvous.
  Expect<void> runCopy(Span<const ValVariant> Args, Span<ValVariant> Rets);
  /// stream/future cancel-read/cancel-write.
  Expect<void> runCancelCopy(Span<const ValVariant> Args,
                             Span<ValVariant> Rets);
  /// stream/future drop-readable/drop-writable.
  Expect<void> runDropEnd(Span<const ValVariant> Args);

  Executor *Exec;
  AsyncBuiltinInfo Info;
};

} // namespace Executor
} // namespace WasmEdge
