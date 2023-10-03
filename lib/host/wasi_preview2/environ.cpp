// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "host/wasi_preview2/environ.h"
#include "common/errcode.h"
#include "common/log.h"

namespace WasmEdge {
namespace Host {

bool PollableObject::isReady() { return false; }

namespace WASIPreview2 {

Expect<PollableObject> Environ::getPollable(Pollable Id) {
  if (auto V = this->Polls.find(Id); V != this->Polls.end()) {
    return V->second;
  } else {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
}

void Environ::dropPollable(Pollable Id) {
  this->Polls.erase(Id);
  return;
}

template <typename T>
Span<T> Environ::load(Runtime::Instance::MemoryInstance *Mem,
                      ComponentModel::List<T> Arg) {
  auto Offset = std::get<0>(Arg);
  auto Len = std::get<0>(Arg);
  return Mem->getSpan<T>(Offset, Len);
}

Environ::~Environ() noexcept {}

} // namespace WASIPreview2
} // namespace Host
} // namespace WasmEdge
