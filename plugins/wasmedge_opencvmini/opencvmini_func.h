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

class WasmEdgeOpenCVMiniNormalize
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniNormalize> {
public:
  WasmEdgeOpenCVMiniNormalize(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SrcMatKey,
                        uint32_t Alpha, uint32_t Beta, uint32_t NormType,
                        uint32_t DataType, uint32_t MaskMatKey);
};

class WasmEdgeOpenCVMiniBilinearSampling
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniBilinearSampling> {
public:
  WasmEdgeOpenCVMiniBilinearSampling(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SrcMatKey,
                        uint32_t OutImgW, uint32_t OutImgH);
};

class WasmEdgeOpenCVMiniRectangle
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniRectangle> {
public:
  WasmEdgeOpenCVMiniRectangle(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &, uint32_t SrcMatKey,
                    uint32_t Top, uint32_t Left, uint32_t Bot, uint32_t Right,
                    double R, double G, double B, int32_t Thickness,
                    int32_t LineType, int32_t Shift);
};

class WasmEdgeOpenCVMiniCvtColor
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniCvtColor> {
public:
  WasmEdgeOpenCVMiniCvtColor(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t SrcMatKey,
                        int32_t Code, int32_t DestChannelN);
};

class WasmEdgeOpenCVMiniResize
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniResize> {
public:
  WasmEdgeOpenCVMiniResize(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SrcMatKey,
                        uint32_t OutImgW, uint32_t OutImgH, double ScaleFactorX,
                        double ScaleFactorY, uint32_t Interpolation);
};

class WasmEdgeOpenCVMiniNoArray
    : public WasmEdgeOpenCVMini<class WasmEdgeOpenCVMiniNoArray> {
public:
  WasmEdgeOpenCVMiniNoArray(WasmEdgeOpenCVMiniEnvironment &HostEnv)
      : WasmEdgeOpenCVMini(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

} // namespace Host
} // namespace WasmEdge
