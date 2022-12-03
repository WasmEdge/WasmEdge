// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "opencvbase.h"

namespace WasmEdge {
namespace Host {
class WasmEdgeOpenCvImgcodecsImread
    : public WasmEdgeOpenCv<WasmEdgeOpenCvImgcodecsImread> {
public:
  WasmEdgeOpenCvImgcodecsImread(WasmEdgeOpenCvEnvironment &HostEnv)
      : WasmEdgeOpenCv(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
				    uint32_t ImgPtr,
                                    uint32_t FilenamePtr);
};

class WasmEdgeOpenCvImgcodecsImwrite
    : public WasmEdgeOpenCv<WasmEdgeOpenCvImgcodecsImwrite> {
public:
  WasmEdgeOpenCvImgcodecsImwrite(WasmEdgeOpenCvEnvironment &HostEnv)
      : WasmEdgeOpenCv(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame,
                                     uint32_t FilenamePtr, uint32_t ImgPtr);
};

} // namespace Host
} // namespace WasmEdge
