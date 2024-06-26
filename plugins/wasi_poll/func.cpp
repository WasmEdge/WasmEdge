// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "common/defines.h"
#include "common/errcode.h"

namespace WasmEdge {
namespace Host {

bool isPollable(Pollable) {
  // TODO: use a global HashMap to note this
  return false;
}

Expect<void> Drop::body(Pollable) {
  // TODO: ensure this affect the global HashMap
  return {};
}

Expect<List<bool>> PollOneoff::body(List<Pollable> In) {
  std::vector<bool> Res;
  for (auto P : In.collection()) {
    Res.push_back(isPollable(P));
  }
  return List<bool>(std::move(Res));
}

} // namespace Host
} // namespace WasmEdge
