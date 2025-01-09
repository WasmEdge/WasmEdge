// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "image_func.h"

#include "common/span.h"
#include "common/spdlog.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <sstream>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeImage {

namespace {

bool decodeImgToSize(Span<const uint8_t> Buf, uint32_t W, uint32_t H,
                     DataType OutType, Span<uint8_t> DstBuf) noexcept {
  // Specify the target data format.
  bool IsRGB = true;
  bool IsU8 = true;
  switch (OutType) {
  case DataType::BGR8:
    IsRGB = false;
    [[fallthrough]];
  case DataType::RGB8:
    break;
  case DataType::BGR32F:
    IsRGB = false;
    [[fallthrough]];
  case DataType::RGB32F:
    IsU8 = false;
    break;
  default:
    return false;
  }

  // Load and decode the image from buffer.
  union RawImagePtr {
    uint8_t *U8;
    float *F32;
  };
  RawImagePtr RawImg;
  RawImg.U8 = nullptr;
  int IW, IH, IC;
  if (IsU8) {
    RawImg.U8 = stbi_load_from_memory(Buf.data(), Buf.size(), &IW, &IH, &IC, 3);
  } else {
    RawImg.F32 =
        stbi_loadf_from_memory(Buf.data(), Buf.size(), &IW, &IH, &IC, 3);
  }
  if (RawImg.U8 == nullptr) {
    spdlog::error("[WasmEdge-Image] Load image failed."sv);
    return false;
  }

  // Resize.
  if (unlikely(DstBuf.size() <
               W * H * 3 * (IsU8 ? sizeof(uint8_t) : sizeof(float)))) {
    spdlog::error("[WasmEdge-Image] Output buffer size {} not enough. "sv
                  "At least need {} bytes."sv,
                  DstBuf.size(),
                  W * H * 3 * (IsU8 ? sizeof(uint8_t) : sizeof(float)));
    return false;
  }
  if (IsU8) {
    stbir_resize_uint8_linear(RawImg.U8, IW, IH, 0, DstBuf.data(),
                              static_cast<int>(W), static_cast<int>(H), 0,
                              STBIR_RGB);
  } else {
    stbir_resize_float_linear(
        RawImg.F32, IW, IH, 0, reinterpret_cast<float *>(DstBuf.data()),
        static_cast<int>(W), static_cast<int>(H), 0, STBIR_RGB);
  }

  // Handle BGR case.
  if (!IsRGB) {
    if (IsU8) {
      for (uint32_t I = 0; I < W * H; I++) {
        std::swap(DstBuf[I * 3], DstBuf[I * 3 + 2]);
      }
    } else {
      auto F32DstBuf = Span<float>(reinterpret_cast<float *>(DstBuf.data()),
                                   DstBuf.size() / sizeof(float));
      for (uint32_t I = 0; I < W * H; I++) {
        std::swap(F32DstBuf[I * 3], F32DstBuf[I * 3 + 2]);
      }
    }
  }
  stbi_image_free(RawImg.U8);
  return true;
}

#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Image] Memory instance not found."sv);            \
    return static_cast<uint32_t>(ErrNo::Fail);                                 \
  }

#define MEM_SPAN_CHECK(OutSpan, MemInst, Type, BufPtr, BufLen, Message)        \
  auto OutSpan = MemInst->getSpan<Type>(BufPtr, BufLen);                       \
  if (unlikely(OutSpan.size() != BufLen)) {                                    \
    spdlog::error("[WasmEdge-Image] "sv Message);                              \
    return static_cast<uint32_t>(ErrNo::Fail);                                 \
  }

} // namespace

Expect<uint32_t> LoadJPG::body(const Runtime::CallingFrame &Frame,
                               uint32_t InImgBufPtr, uint32_t InImgBufLen,
                               uint32_t OutImgW, uint32_t OutImgH,
                               uint32_t OutType, uint32_t OutBufPtr,
                               uint32_t OutBufLen) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input image buffer.
  MEM_SPAN_CHECK(ImgBufSpan, MemInst, uint8_t, InImgBufPtr, InImgBufLen,
                 "Failed when accessing the input image buffer memory."sv)

  // Check the output decoded image buffer.
  MEM_SPAN_CHECK(OutBufSpan, MemInst, uint8_t, OutBufPtr, OutBufLen,
                 "Failed when accessing the output image data buffer memory."sv)

  if (unlikely(!decodeImgToSize(ImgBufSpan, OutImgW, OutImgH,
                                static_cast<DataType>(OutType), OutBufSpan))) {
    return static_cast<uint32_t>(ErrNo::Fail);
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> LoadPNG::body(const Runtime::CallingFrame &Frame,
                               uint32_t InImgBufPtr, uint32_t InImgBufLen,
                               uint32_t OutImgW, uint32_t OutImgH,
                               uint32_t OutType, uint32_t OutBufPtr,
                               uint32_t OutBufLen) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input image buffer.
  MEM_SPAN_CHECK(ImgBufSpan, MemInst, uint8_t, InImgBufPtr, InImgBufLen,
                 "Failed when accessing the input image buffer memory."sv)

  // Check the output decoded image buffer.
  MEM_SPAN_CHECK(OutBufSpan, MemInst, uint8_t, OutBufPtr, OutBufLen,
                 "Failed when accessing the output image data buffer memory."sv)

  if (unlikely(!decodeImgToSize(ImgBufSpan, OutImgW, OutImgH,
                                static_cast<DataType>(OutType), OutBufSpan))) {
    return static_cast<uint32_t>(ErrNo::Fail);
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

Expect<uint32_t> LoadImage::body(const Runtime::CallingFrame &Frame,
                                 uint32_t InImgBufPtr, uint32_t InImgBufLen,
                                 uint32_t OutImgW, uint32_t OutImgH,
                                 uint32_t OutType, uint32_t OutBufPtr,
                                 uint32_t OutBufLen) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input image buffer.
  MEM_SPAN_CHECK(ImgBufSpan, MemInst, uint8_t, InImgBufPtr, InImgBufLen,
                 "Failed when accessing the input image buffer memory."sv)

  // Check the output decoded image buffer.
  MEM_SPAN_CHECK(OutBufSpan, MemInst, uint8_t, OutBufPtr, OutBufLen,
                 "Failed when accessing the output image data buffer memory."sv)

  if (unlikely(!decodeImgToSize(ImgBufSpan, OutImgW, OutImgH,
                                static_cast<DataType>(OutType), OutBufSpan))) {
    return static_cast<uint32_t>(ErrNo::Fail);
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

} // namespace WasmEdgeImage
} // namespace Host
} // namespace WasmEdge
