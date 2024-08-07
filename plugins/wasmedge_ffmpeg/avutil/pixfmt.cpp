// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "pixfmt.h"

extern "C" {
#include "libavutil/pixdesc.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t>
AvPixFmtDescriptorNbComponents::body(const Runtime::CallingFrame &,
                                     uint32_t PixFormatId) {
  AVPixelFormat const PixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(PixFormatId);
  const AVPixFmtDescriptor *AvPixFmtDescriptor =
      av_pix_fmt_desc_get(PixelFormat);
  return AvPixFmtDescriptor->nb_components;
}

Expect<int32_t>
AvPixFmtDescriptorLog2ChromaW::body(const Runtime::CallingFrame &,
                                    uint32_t PixFormatId) {
  AVPixelFormat const PixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(PixFormatId);
  const AVPixFmtDescriptor *AvPixFmtDescriptor =
      av_pix_fmt_desc_get(PixelFormat);
  return AvPixFmtDescriptor->log2_chroma_w;
}

Expect<int32_t>
AvPixFmtDescriptorLog2ChromaH::body(const Runtime::CallingFrame &,
                                    uint32_t PixFormatId) {
  AVPixelFormat const PixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(PixFormatId);
  const AVPixFmtDescriptor *AvPixFmtDescriptor =
      av_pix_fmt_desc_get(PixelFormat);
  return AvPixFmtDescriptor->log2_chroma_h;
}

Expect<int32_t> AVColorRangeNameLength::body(const Runtime::CallingFrame &,
                                             int32_t RangeId) {
  AVColorRange const ColorRange = static_cast<AVColorRange>(RangeId);
  const char *Name = av_color_range_name(ColorRange);
  return strlen(Name);
}

Expect<int32_t> AVColorRangeName::body(const Runtime::CallingFrame &Frame,
                                       int32_t RangeId, uint32_t RangeNamePtr,
                                       uint32_t RangeLength) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(RangeNameBuf, MemInst, char, RangeNamePtr, RangeLength, "");

  AVColorRange const ColorRange = static_cast<AVColorRange>(RangeId);
  const char *RangeName = av_color_range_name(ColorRange);
  std::copy_n(RangeName, RangeLength, RangeNameBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVColorTransferNameLength::body(const Runtime::CallingFrame &,
                                                int32_t TransferId) {
  AVColorTransferCharacteristic const Characteristic =
      static_cast<AVColorTransferCharacteristic>(TransferId);
  const char *Name = av_color_transfer_name(Characteristic);
  return strlen(Name);
}

Expect<int32_t> AVColorTransferName::body(const Runtime::CallingFrame &Frame,
                                          int32_t TransferId,
                                          uint32_t TransferNamePtr,
                                          uint32_t TransferLength) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(TransferNameBuf, MemInst, char, TransferNamePtr,
                 TransferLength, "");

  AVColorTransferCharacteristic const Characteristic =
      static_cast<AVColorTransferCharacteristic>(TransferId);
  const char *TransferName = av_color_transfer_name(Characteristic);
  std::copy_n(TransferName, TransferLength, TransferNameBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVColorSpaceNameLength::body(const Runtime::CallingFrame &,
                                             int32_t ColorSpaceId) {
  AVColorSpace const ColorSpace = static_cast<AVColorSpace>(ColorSpaceId);
  const char *Name = av_color_space_name(ColorSpace);
  return strlen(Name);
}

Expect<int32_t> AVColorSpaceName::body(const Runtime::CallingFrame &Frame,
                                       int32_t ColorSpaceId,
                                       uint32_t ColorSpaceNamePtr,
                                       uint32_t ColorSpaceLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ColorSpaceBuf, MemInst, char, ColorSpaceNamePtr, ColorSpaceLen,
                 "");

  AVColorSpace const ColorSpace = static_cast<AVColorSpace>(ColorSpaceId);
  const char *ColorSpaceName = av_color_space_name(ColorSpace);
  std::copy_n(ColorSpaceName, ColorSpaceLen, ColorSpaceBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVColorPrimariesNameLength::body(const Runtime::CallingFrame &,
                                                 int32_t ColorPrimariesId) {
  AVColorPrimaries const ColorPrimaries =
      FFmpegUtils::ColorPrimaries::intoAVColorPrimaries(ColorPrimariesId);
  const char *Name = av_color_primaries_name(ColorPrimaries);
  return strlen(Name);
}

Expect<int32_t> AVColorPrimariesName::body(const Runtime::CallingFrame &Frame,
                                           int32_t ColorPrimariesId,
                                           uint32_t ColorPrimariesNamePtr,
                                           uint32_t ColorPrimariesLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ColorPrimariesBuf, MemInst, char, ColorPrimariesNamePtr,
                 ColorPrimariesLen, "");

  AVColorPrimaries const ColorPrimaries =
      FFmpegUtils::ColorPrimaries::intoAVColorPrimaries(ColorPrimariesId);
  const char *PrimariesName = av_color_primaries_name(ColorPrimaries);
  std::copy_n(PrimariesName, ColorPrimariesLen, ColorPrimariesBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVPixelFormatNameLength::body(const Runtime::CallingFrame &,
                                              uint32_t AvPixFormatId) {
  AVPixelFormat const PixFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  const AVPixFmtDescriptor *PixFmtDescriptor = av_pix_fmt_desc_get(PixFormat);

  return strlen(PixFmtDescriptor->name);
}

Expect<int32_t> AVPixelFormatName::body(const Runtime::CallingFrame &Frame,
                                        uint32_t PixFormatId,
                                        uint32_t PixFormatNamePtr,
                                        uint32_t PixFormatNameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(PixFormatBuf, MemInst, char, PixFormatNamePtr,
                 PixFormatNameLen, "");

  AVPixelFormat const PixFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(PixFormatId);
  const AVPixFmtDescriptor *PixFmtDescriptor = av_pix_fmt_desc_get(PixFormat);
  const char *PixFormatName = PixFmtDescriptor->name;
  std::copy_n(PixFormatName, PixFormatNameLen, PixFormatBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVPixelFormatMask::body(const Runtime::CallingFrame &,
                                        uint32_t PixFormatId) {
  AVPixelFormat const PixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(PixFormatId);
  return static_cast<int32_t>(PixelFormat);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
