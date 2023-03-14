// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-close-bpf-object.h"
#include <shared_mutex>

using namespace WasmEdge;

Expect<int32_t> CloseBpfObject::body(const WasmEdge::Runtime::CallingFrame &,
                                     handle_t program) {
  std::shared_lock guard(this->state->lock);
  auto &handles = this->state->handles;
  if (!handles.count(program)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return handles.erase(program) > 0? 0 : -1;
}