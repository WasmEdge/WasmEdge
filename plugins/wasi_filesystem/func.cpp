// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "common/defines.h"
#include "common/errcode.h"

namespace WasmEdge {
namespace Host {

Expect<List<Tuple<GetDirectories::Descriptor, std::string>>>
GetDirectories::body() {
  List<Tuple<Descriptor, std::string>> L{};

  return L;
}

} // namespace Host
} // namespace WasmEdge
