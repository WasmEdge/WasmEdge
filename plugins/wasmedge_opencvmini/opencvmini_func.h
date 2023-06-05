// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "opencvmini_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCVMiniImdecode
    : public WasmEdgeOpenCVMini<WasmEdgeOpenCVMiniImdecode> {
public:
  WasmEdgeOpenCVMiniImdecode(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t NamePtr,
                    uint32_t NameLen);
};

} // namespace Host
} // namespace WasmEdge
