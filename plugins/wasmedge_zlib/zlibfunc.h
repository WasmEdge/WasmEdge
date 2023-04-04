// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "zlibbase.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibDeflateInit_ : public WasmEdgeZlib<WasmEdgeZlibDeflateInit_> {
public:
  WasmEdgeZlibDeflateInit_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                    int32_t Level);
};

} // namespace Host
} // namespace WasmEdge
