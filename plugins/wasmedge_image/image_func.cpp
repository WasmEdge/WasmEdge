// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "image_func.h"

#include "common/span.h"
#include "common/spdlog.h"

#include <boost/gil.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/numeric/resample.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/io/io.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeImage {

namespace {

// Helper function to decode and resize image.
template <typename Image, typename FormatTag>
bool decodeImgToSize(Span<const char> Buf, uint32_t W, uint32_t H,
                     Span<char> DstBuf) {
  std::stringstream ImgStream;
  ImgStream.write(Buf.data(), Buf.size());
  Image Img;
  try {
    boost::gil::read_and_convert_image(ImgStream, Img, FormatTag());
  } catch (std::exception const &e) {
    spdlog::error("[WasmEdge-Image] Decode image fail: {}"sv, e.what());
    return false;
  }

  uint32_t C = boost::gil::num_channels<typename Image::view_t>::value;
  typename Image::view_t ImgView = boost::gil::interleaved_view(
      W, H, reinterpret_cast<typename Image::value_type *>(DstBuf.data()),
      W * C * sizeof(char));
  boost::gil::resize_view(boost::gil::const_view(Img), ImgView,
                          boost::gil::bilinear_sampler());
  return true;
}

// Helper function to normalize image.
void normalizeImg(Span<const char> SrcBuf, Span<float> DstBuf) {
  for (uint32_t I = 0; I < DstBuf.size(); I++) {
    DstBuf[I] = static_cast<uint8_t>(SrcBuf[I]) / 255.0;
  }
}

// Template to decode and resize image to the target format.
template <typename Image, typename FormatTag>
uint32_t readBufToImg(Span<const char> InBuf, uint32_t W, uint32_t H,
                      Span<char> OutBuf) {
  if (unlikely(!decodeImgToSize<Image, FormatTag>(InBuf, W, H, OutBuf))) {
    return static_cast<uint32_t>(ErrNo::Fail);
  }
  return static_cast<uint32_t>(ErrNo::Success);
}

// Template to decode and resize image to the target format.
template <typename Image, typename FormatTag>
uint32_t readBufToFlattenImg(Span<const char> InBuf, uint32_t W, uint32_t H,
                             Span<char> OutBuf) {
  std::vector<char> ImgData(3 * W * H);
  if (unlikely(!decodeImgToSize<Image, FormatTag>(
          InBuf, W, H, Span<char>(ImgData.data(), ImgData.size())))) {
    return static_cast<uint32_t>(ErrNo::Fail);
  }
  normalizeImg(ImgData, Span<float>(reinterpret_cast<float *>(OutBuf.data()),
                                    OutBuf.size() / sizeof(float)));
  return static_cast<uint32_t>(ErrNo::Success);
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
  MEM_SPAN_CHECK(ImgBufSpan, MemInst, char, InImgBufPtr, InImgBufLen,
                 "Failed when accessing the input image buffer memory."sv)

  // Check the output decoded image buffer.
  MEM_SPAN_CHECK(OutBufSpan, MemInst, char, OutBufPtr, OutBufLen,
                 "Failed when accessing the output image data buffer memory."sv)

  switch (static_cast<DataType>(OutType)) {
  case DataType::RGB8:
    return readBufToImg<boost::gil::rgb8_image_t, boost::gil::jpeg_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
  case DataType::BGR8:
    return readBufToImg<boost::gil::bgr8_image_t, boost::gil::jpeg_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
  case DataType::RGB32F:
    return readBufToFlattenImg<boost::gil::rgb8_image_t, boost::gil::jpeg_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
  case DataType::BGR32F:
    return readBufToFlattenImg<boost::gil::bgr8_image_t, boost::gil::jpeg_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
    break;
  default:
    spdlog::error("[WasmEdge-Image] Invalid output data format."sv);
    return static_cast<uint32_t>(ErrNo::Fail);
  }
}

Expect<uint32_t> LoadPNG::body(const Runtime::CallingFrame &Frame,
                               uint32_t InImgBufPtr, uint32_t InImgBufLen,
                               uint32_t OutImgW, uint32_t OutImgH,
                               uint32_t OutType, uint32_t OutBufPtr,
                               uint32_t OutBufLen) {
  // Check memory instance from module.
  MEMINST_CHECK(MemInst, Frame, 0)

  // Check the input image buffer.
  MEM_SPAN_CHECK(ImgBufSpan, MemInst, char, InImgBufPtr, InImgBufLen,
                 "Failed when accessing the input image buffer memory."sv)

  // Check the output decoded image buffer.
  MEM_SPAN_CHECK(OutBufSpan, MemInst, char, OutBufPtr, OutBufLen,
                 "Failed when accessing the output image data buffer memory."sv)

  switch (static_cast<DataType>(OutType)) {
  case DataType::RGB8:
    return readBufToImg<boost::gil::rgb8_image_t, boost::gil::png_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
  case DataType::BGR8:
    return readBufToImg<boost::gil::bgr8_image_t, boost::gil::png_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
  case DataType::RGB32F:
    return readBufToFlattenImg<boost::gil::rgb8_image_t, boost::gil::png_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
  case DataType::BGR32F:
    return readBufToFlattenImg<boost::gil::bgr8_image_t, boost::gil::png_tag>(
        ImgBufSpan, OutImgW, OutImgH, OutBufSpan);
    break;
  default:
    spdlog::error("[WasmEdge-Image] Invalid output data format."sv);
    return static_cast<uint32_t>(ErrNo::Fail);
  }
}

} // namespace WasmEdgeImage
} // namespace Host
} // namespace WasmEdge
