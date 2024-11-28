// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "common/errcode.h"
#include "func.h"

namespace WasmEdge {
namespace Host {

namespace Types {

Expect<Option<ErrorCode>> HttpErrorCode::body(uint32_t Err) {
  return std::nullopt;
}

} // namespace Types

} // namespace Host
} // namespace WasmEdge
