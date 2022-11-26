// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "imgocodecsbase.h"
#include <opencv2/imgcodecs.hpp>

namespace WasmEdge {
namespace Host {
using namespace cv;
class WasmEdgeOpenCvImgcodecsImread
    : public WasmEdgeOpenCvImgcodecs<WasmEdgeOpenCvImgcodecsImread> {
public:
  WasmEdgeOpenCvImgcodecsImread(WasmEdgeOpenCvImgcodecsEnvoronment &HostEnv)
      : WasmEdgeOpenCvImgcodecs(HostEnv) {}
  Expect<Mat> body(const Runtime::CallingFrame &Frame, uint32_t filename);
};

} // namespace Host
} // namespace WasmEdge
