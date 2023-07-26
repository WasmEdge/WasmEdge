// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "opencvmini_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

/// Read image from buffer
class WasmEdgeOpenCVMiniImdecode
    : public WasmEdgeOpenCVMini<WasmEdgeOpenCVMiniImdecode> {
public:
  WasmEdgeOpenCVMiniImdecode(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                        uint32_t BufLen);
};

/// Write image into buffer
class WasmEdgeOpenCVMiniImencode
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniImencode> {
public:
  WasmEdgeOpenCVMiniImencode(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t ExtPtr,
                    uint32_t ExtLen, uint32_t MatKey, uint32_t BufPtr,
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

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SrcMatKey,
                        uint32_t KernelWidth, uint32_t KernelHeight);
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

/// This is not `cv::normalize`, refers to:
/// https://github.com/WasmEdge/WasmEdge/commit/77051da4995d7318d91a82102a72ce2557151764#diff-3333d926ca87cf4285bfcd6deae45ee310307be66fca8a4ca6f0f8a946743fccR50-R54
class WasmEdgeOpenCVMiniNormalize
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniNormalize> {
public:
  WasmEdgeOpenCVMiniNormalize(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SrcMatKey);
};

class WasmEdgeOpenCVMiniBilinearSampling
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniBilinearSampling> {
public:
  WasmEdgeOpenCVMiniBilinearSampling(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SrcMatKey,
                        uint32_t OutImgW, uint32_t OutImgH);
};

} // namespace Host
} // namespace WasmEdge
