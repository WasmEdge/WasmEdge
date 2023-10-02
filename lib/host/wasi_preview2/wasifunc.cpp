// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "host/wasi_preview2/wasifunc.h"
#include "common/defines.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define __restrict__ __restrict
#endif

namespace WasmEdge {
namespace Host {

Expect<void> DropPollable::body(const Runtime::CallingFrame &, Pollable This) {
  Env.getPollable(This);
  return {};
}

// Expect<ComponentModel::List> PollOneoff::body(const Runtime::CallingFrame &,
//                                               ComponentModel::List) {
//   return ComponentModel::List(0, 0);
// }

} // namespace Host
} // namespace WasmEdge
