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
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                        uint32_t BufLen);
};

class WasmEdgeOpenCVMiniImshow
    : public WasmEdgeOpenCVMini<WasmEdgeOpenCVMiniImshow> {
public:
  WasmEdgeOpenCVMiniImshow(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t WindowNamePtr,
                    uint32_t WindowNameLen, uint32_t MatKey);
};

class WasmEdgeOpenCVMiniWaitKey
    : public WasmEdgeOpenCVMini<WasmEdgeOpenCVMiniWaitKey> {
public:
  WasmEdgeOpenCVMiniWaitKey(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Delay);
};

class WasmEdgeOpenCVMiniBlur
    : public WasmEdgeOpenCVMini<WasmEdgeOpenCVMiniBlur> {
public:
  WasmEdgeOpenCVMiniBlur(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SrcMatKey);
};

class WasmEdgeOpenCVMiniImwrite
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniImwrite> {
public:
  WasmEdgeOpenCVMiniImwrite(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame,
                    uint32_t TargetFileNamePtr, uint32_t TargetFileNameLen,
                    uint32_t SrcMatKey);
};

} // namespace Host
} // namespace WasmEdge
