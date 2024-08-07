// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "image_base.h"

#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeImage {

class LoadJPG : public Func<LoadJPG> {
public:
  LoadJPG(ImgEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t InImgBufPtr, uint32_t InImgBufLen,
                        uint32_t OutImgW, uint32_t OutImgH, uint32_t OutType,
                        uint32_t OutBufPtr, uint32_t OutBufLen);
};

class LoadPNG : public Func<LoadPNG> {
public:
  LoadPNG(ImgEnv &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t InImgBufPtr, uint32_t InImgBufLen,
                        uint32_t OutImgW, uint32_t OutImgH, uint32_t OutType,
                        uint32_t OutBufPtr, uint32_t OutBufLen);
};

} // namespace WasmEdgeImage
} // namespace Host
} // namespace WasmEdge
