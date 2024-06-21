// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "common/defines.h"
#include "common/errcode.h"

namespace WasmEdge {
namespace Host {

Expect<void> Drop::body(const Runtime::CallingFrame &, Pollable) { return {}; }

} // namespace Host
} // namespace WasmEdge
