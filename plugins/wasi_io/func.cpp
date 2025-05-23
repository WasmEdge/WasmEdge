// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "common/defines.h"
#include "common/errcode.h"

namespace WasmEdge {
namespace Host {

Expect<Result<Tuple<>, StreamError::T>>
OutputStream_BlockingWriteAndFlush::body(int32_t /* Self */,
                                         List<uint8_t> /* Contents */) {
  return Result<Tuple<>, StreamError::T>(StreamError::closed());
}

} // namespace Host
} // namespace WasmEdge
