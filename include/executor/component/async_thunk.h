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
/// built-ins over the task runtime.
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
namespace Component {

class Executor;

/// Immediates and options of one async canonical built-in, resolved at
/// instantiation time from the Canonical AST node.
struct AsyncBuiltinInfo {
  ComponentCanonOpCode Code;
  const Runtime::Instance::ComponentInstance *Inst = nullptr;
  Runtime::Instance::MemoryInstance *Mem = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  StringEncoding Enc = StringEncoding::UTF8;
  /// The built-in's optional flag: the `async?` or `cancel?` immediate, or
  /// the `async` canonical option. Read it through async() or cancellable(),
  /// so that every use names the rule it follows.
  bool Flag = false;
  /// stream/future ops: the element type declared at the built-in.
  std::optional<ComponentValType> Elem;
  bool IsStream = true;
  /// context.get/set slot index and slot type.
  uint32_t CtxIdx = 0;
  ValType CtxType = TypeCode::I32;
  /// task.return: declared result types.
  std::vector<ComponentValType> RetTypes;
  /// thread.new-indirect: the core table holding start functions.
  Runtime::Instance::TableInstance *Table = nullptr;

  bool async() const noexcept { return Flag; }
  bool cancellable() const noexcept { return Flag; }
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

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
