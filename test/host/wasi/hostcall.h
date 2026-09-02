// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/hostcall.h - Direct host component function calls --===//
//
// The host functions run as root tasks of a bare component executor, the way
// an embedder invokes them.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_variant.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "runtime/instance/component/component.h"

#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace HostTest {

/// Invoke the host function Name of Inst through a component executor.
inline Expect<std::vector<ComponentValVariant>>
call(Runtime::Instance::ComponentInstance &Inst, std::string_view Name,
     std::vector<ComponentValVariant> Args) {
  auto *Func = Inst.findFunction(Name);
  if (Func == nullptr) {
    return Unexpect(ErrCode::Value::FuncNotFound);
  }
  Configure Conf;
  Executor::Executor Core(Conf);
  Executor::ComponentExecutor Exec(Core);
  EXPECTED_TRY(auto Returns, Exec.invoke(Func, Args, {}));
  std::vector<ComponentValVariant> Rets;
  Rets.reserve(Returns.size());
  for (auto &R : Returns) {
    Rets.push_back(std::move(R.first));
  }
  return Rets;
}

inline uint64_t u64Of(const Expect<std::vector<ComponentValVariant>> &Res) {
  return std::get<uint64_t>((*Res)[0]);
}

} // namespace HostTest
} // namespace WasmEdge
