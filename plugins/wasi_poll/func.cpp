// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "common/defines.h"
#include "common/errcode.h"

namespace WasmEdge {
namespace Host {

Expect<List<bool>> PollOneoff::body(List<Pollable> In) {
  std::vector<bool> Res;
  for (auto P : In.collection()) {
    Res.push_back(Env.isPollable(P));
  }
  return List<bool>(std::move(Res));
}

} // namespace Host
} // namespace WasmEdge
