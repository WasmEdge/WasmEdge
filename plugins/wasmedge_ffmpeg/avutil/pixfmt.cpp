#include "pixfmt.h"
extern "C" {
#include "libavutil/pixdesc.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

// Expect<int32_t> AVPixFmtDescGet::body(const Runtime::CallingFrame
// &Frame,uint32_t AvPixFmtId, uint32_t AvPixFmtDescPtr){
//
//   MEMINST_CHECK(MemInst,Frame,0);
//   MEM_PTR_CHECK(AvPixFmtDescId,MemInst,uint32_t,AvPixFmtDescPtr,"Failed to
//   access Memory for AvPixFmtDesc")
//
//   FFMPEG_PTR_FETCH(AvPixFmtDesc,*AvPixFmtDescId,const AVPixFmtDescriptor);
//
//   AVPixelFormat const AvPixelFormat =
//   FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFmtId); AvPixFmtDesc =
//   av_pix_fmt_desc_get(AvPixelFormat); return
//   static_cast<int32_t>(ErrNo::Success);
// }

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
  const char *Name = av_color_range_name(ColorRange);
  memmove(RangeNameBuf.data(), Name, RangeLength);
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
  const char *Name = av_color_transfer_name(Characteristic);
  memmove(TransferNameBuf.data(), Name, TransferLength);
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
  const char *Name = av_color_space_name(ColorSpace);
  memmove(ColorSpaceBuf.data(), Name, ColorSpaceLen);
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
  const char *Name = av_color_primaries_name(ColorPrimaries);
  memmove(ColorPrimariesBuf.data(), Name, ColorPrimariesLen);
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
  const char *Name = PixFmtDescriptor->name;
  memmove(PixFormatBuf.data(), Name, PixFormatNameLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVPixelFormatMask::body(const Runtime::CallingFrame &,
                   uint32_t PixFormatId) {
  AVPixelFormat const PixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(PixFormatId);
  return static_cast<int32_t>(PixelFormat);
}

// Expect<int32_t> AvPixFmtDescriptorName::body(const Runtime::CallingFrame
// &Frame,uint32_t AvPixFormatId){
//
//   AVPixelFormat const AvPixelFormat =
//   FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId); const AVPixFmtDescriptor*
//   AvPixFmtDescriptor = av_pix_fmt_desc_get(AvPixelFormat);
//   AvPixFmtDescriptor->name;
//   return 0;
// }

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
