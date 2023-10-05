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
                                     uint32_t AvPixFormatId) {
  AVPixelFormat const AvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  const AVPixFmtDescriptor *AvPixFmtDescriptor =
      av_pix_fmt_desc_get(AvPixelFormat);
  return AvPixFmtDescriptor->nb_components;
}

Expect<int32_t>
AvPixFmtDescriptorLog2ChromaW::body(const Runtime::CallingFrame &,
                                    uint32_t AvPixFormatId) {

  AVPixelFormat const AvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  const AVPixFmtDescriptor *AvPixFmtDescriptor =
      av_pix_fmt_desc_get(AvPixelFormat);
  return AvPixFmtDescriptor->log2_chroma_w;
}

Expect<int32_t>
AvPixFmtDescriptorLog2ChromaH::body(const Runtime::CallingFrame &,
                                    uint32_t AvPixFormatId) {

  AVPixelFormat const AvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  const AVPixFmtDescriptor *AvPixFmtDescriptor =
      av_pix_fmt_desc_get(AvPixelFormat);
  return AvPixFmtDescriptor->log2_chroma_h;
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
