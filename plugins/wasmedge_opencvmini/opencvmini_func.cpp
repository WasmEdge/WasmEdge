// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "opencvmini_func.h"
#include "common/defines.h"

namespace WasmEdge {
namespace Host {

Expect<void>
WasmEdgeOpenCVMiniImdecode::body(const Runtime::CallingFrame &Frame,
                                 uint32_t NamePtr, uint32_t NameLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return {};
}

} // namespace Host
} // namespace WasmEdge
