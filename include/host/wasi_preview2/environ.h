// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"
#include "runtime/instance/memory.h"
#include <map>

namespace WasmEdge {
namespace Host {

namespace ComponentModel {

using String = std::tuple</* address */ uint32_t, /* length */ uint32_t>;

template <typename T>
using List = std::tuple</* address */ uint32_t, /* length */ uint32_t>;

} // namespace ComponentModel

using Pollable = uint32_t;

class PollableObject {
public:
  bool isReady() { return false; }
};

namespace WASIPreview2 {

class Environ {
public:
  Expect<PollableObject> getPollable(Pollable Id) {
    if (auto V = this->Polls.find(Id); V != this->Polls.end()) {
      return V->second;
    } else {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
  }

  Expect<void> dropPollable(Pollable Id) {
    this->Polls.erase(Id);
    return {};
  }

  template <typename T>
  Span<T> load(Runtime::Instance::MemoryInstance *Mem,
               ComponentModel::List<T> Arg) {
    auto Offset = std::get<0>(Arg);
    auto Len = std::get<0>(Arg);
    return Mem->getSpan<T>(Offset, Len);
  }

  ~Environ() noexcept;

private:
  std::map<Pollable, PollableObject> Polls;
};

} // namespace WASIPreview2
} // namespace Host
} // namespace WasmEdge
