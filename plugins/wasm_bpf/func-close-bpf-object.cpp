// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-close-bpf-object.h"
#include <shared_mutex>

using namespace WasmEdge;

Expect<int32_t> CloseBpfObject::body(const WasmEdge::Runtime::CallingFrame &,
                                     handle_t program) {
  std::shared_lock guard(this->state->lock);
  auto &handles = this->state->handles;
  auto &mapfds = this->state->map_fd_cache;
  if (!handles.count(program)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto val = handles[program];
  handles.erase(program);
  bpf_map *curr_map = nullptr;
  bpf_map__for_each(curr_map, val->obj.get()) {
    mapfds.erase(bpf_map__fd(curr_map));
  }
  delete val;
  return 0;
}