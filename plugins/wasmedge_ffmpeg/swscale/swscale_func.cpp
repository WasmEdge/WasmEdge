#include "swscale_func.h"

extern "C" {
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale {

Expect<int32_t>
SwsGetContext::body(const Runtime::CallingFrame &Frame, uint32_t SwsCtxPtr,
                    uint32_t SrcW, uint32_t SrcH, uint32_t SrcAvPixFormatId,
                    uint32_t DesW, uint32_t DesH, uint32_t DesAvPixFormatId,
                    int32_t Flags, uint32_t SrcFilterId, uint32_t DesFilterId) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsCtxId, MemInst, uint32_t, SwsCtxPtr,
                "Failed when accessing the return SWSContext Memory")

  FFMPEG_PTR_FETCH(SwsCtx, *SwsCtxId, SwsContext)
  FFMPEG_PTR_FETCH(SrcSwsFilter, SrcFilterId, SwsFilter)
  FFMPEG_PTR_FETCH(DesSwsFilter, DesFilterId, SwsFilter)

  AVPixelFormat const SrcAvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(SrcAvPixFormatId);
  AVPixelFormat const DestAvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(DesAvPixFormatId);
  SwsCtx = sws_getContext(SrcW, SrcH, SrcAvPixelFormat, DesW, DesH,
                          DestAvPixelFormat, Flags, SrcSwsFilter, DesSwsFilter,
                          NULL); // Not using param anywhere in Rust SDK.
  if (SwsCtx == NULL)
    return static_cast<int32_t>(ErrNo::InternalError);
  FFMPEG_PTR_STORE(SwsCtx, SwsCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsFreeContext::body(const Runtime::CallingFrame &,
                                     uint32_t SwsCtxId) {

  FFMPEG_PTR_FETCH(SwsCtx, SwsCtxId, SwsContext)
  sws_freeContext(SwsCtx);
  FFMPEG_PTR_DELETE(SwsCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsScale::body(const Runtime::CallingFrame &, uint32_t SwsCtxId,
                               uint32_t InputFrameId, int32_t SrcSliceY,
                               int32_t SrcSliceH, uint32_t OutputFrameId) {

  FFMPEG_PTR_FETCH(SwsCtx, SwsCtxId, SwsContext);
  FFMPEG_PTR_FETCH(InputFrame, InputFrameId, AVFrame);
  FFMPEG_PTR_FETCH(OutputFrame, OutputFrameId, AVFrame);
  return sws_scale(SwsCtx, InputFrame->data, InputFrame->linesize, SrcSliceY,
                   SrcSliceH, OutputFrame->data, OutputFrame->linesize);
}

Expect<int32_t> SwsGetCachedContext::body(
    const Runtime::CallingFrame &Frame, uint32_t SwsCachedCtxPtr,
    uint32_t SwsCtxId, uint32_t SrcW, uint32_t SrcH, uint32_t SrcAvPixFormatId,
    uint32_t DesW, uint32_t DesH, uint32_t DesAvPixFormatId, int32_t Flags,
    uint32_t SrcFilterId, uint32_t DesFilterId) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsCachedCtxId, MemInst, uint32_t, SwsCachedCtxPtr, "")

  FFMPEG_PTR_FETCH(SwsCachedCtx, *SwsCachedCtxId, SwsContext);
  FFMPEG_PTR_FETCH(SwsCtx, SwsCtxId, SwsContext);
  FFMPEG_PTR_FETCH(SrcSwsFilter, SrcFilterId, SwsFilter)
  FFMPEG_PTR_FETCH(DesSwsFilter, DesFilterId, SwsFilter)

  AVPixelFormat const SrcAvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(SrcAvPixFormatId);
  AVPixelFormat const DestAvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(DesAvPixFormatId);
  SwsCachedCtx = sws_getCachedContext(SwsCtx, SrcW, SrcH, SrcAvPixelFormat,
                                      DesW, DesH, DestAvPixelFormat, Flags,
                                      SrcSwsFilter, DesSwsFilter, NULL);
  if (SwsCachedCtx == NULL)
    return static_cast<int32_t>(ErrNo::InternalError);

  FFMPEG_PTR_STORE(SwsCachedCtx, SwsCachedCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsIsSupportedInput::body(const Runtime::CallingFrame &,
                                          uint32_t AvPixFormatId) {
  AVPixelFormat const AvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  return sws_isSupportedInput(AvPixelFormat);
}

Expect<int32_t> SwsIsSupportedOutput::body(const Runtime::CallingFrame &,
                                           uint32_t AvPixFormatId) {
  AVPixelFormat const AvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  return sws_isSupportedOutput(AvPixelFormat);
}

Expect<int32_t>
SwsIsSupportedEndiannessConversion::body(const Runtime::CallingFrame &,
                                         uint32_t AvPixFormatId) {
  AVPixelFormat const AvPixelFormat =
      FFmpegUtils::PixFmt::intoAVPixFmt(AvPixFormatId);
  return sws_isSupportedEndiannessConversion(AvPixelFormat);
}

Expect<int32_t> SwsGetDefaultFilter::body(
    const Runtime::CallingFrame &Frame, uint32_t SwsFilterPtr, float LumaGBlur,
    float ChromaGBlur, float LumaSharpen, float ChromaSharpen,
    float ChromaHShift, float ChromaVShift, int32_t Verbose) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsFilterId, MemInst, uint32_t, SwsFilterPtr, "")

  SwsFilter *Filter =
      sws_getDefaultFilter(LumaGBlur, ChromaGBlur, LumaSharpen, ChromaSharpen,
                           ChromaHShift, ChromaVShift, Verbose);
  if (Filter == NULL)
    return static_cast<int32_t>(ErrNo::InternalError);
  FFMPEG_PTR_STORE(Filter, SwsFilterId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsGetLumaH::body(const Runtime::CallingFrame &Frame,
                                  uint32_t SwsFilterId, uint32_t SwsVectorPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsVectorId, MemInst, uint32_t, SwsVectorPtr, "")
  FFMPEG_PTR_FETCH(Filter, SwsFilterId, SwsFilter);

  SwsVector *Vector = Filter->lumH;
  FFMPEG_PTR_STORE(Vector, SwsVectorId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsGetLumaV::body(const Runtime::CallingFrame &Frame,
                                  uint32_t SwsFilterId, uint32_t SwsVectorPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsVectorId, MemInst, uint32_t, SwsVectorPtr, "")
  FFMPEG_PTR_FETCH(Filter, SwsFilterId, SwsFilter);

  SwsVector *Vector = Filter->lumV;
  FFMPEG_PTR_STORE(Vector, SwsVectorId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsGetChromaH::body(const Runtime::CallingFrame &Frame,
                                    uint32_t SwsFilterId,
                                    uint32_t SwsVectorPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsVectorId, MemInst, uint32_t, SwsVectorPtr, "")
  FFMPEG_PTR_FETCH(Filter, SwsFilterId, SwsFilter);

  SwsVector *Vector = Filter->chrH;
  FFMPEG_PTR_STORE(Vector, SwsVectorId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsGetChromaV::body(const Runtime::CallingFrame &Frame,
                                    uint32_t SwsFilterId,
                                    uint32_t SwsVectorPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsVectorId, MemInst, uint32_t, SwsVectorPtr, "")
  FFMPEG_PTR_FETCH(Filter, SwsFilterId, SwsFilter);

  SwsVector *Vector = Filter->chrV;
  FFMPEG_PTR_STORE(Vector, SwsVectorId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsFreeFilter::body(const Runtime::CallingFrame &,
                                    uint32_t SwsFilterId) {

  FFMPEG_PTR_FETCH(Filter, SwsFilterId, SwsFilter);
  sws_freeFilter(Filter);
  FFMPEG_PTR_DELETE(SwsFilterId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsAllocVec::body(const Runtime::CallingFrame &Frame,
                                  uint32_t SwsVectorPtr, int32_t Length) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsVectorId, MemInst, uint32_t, SwsVectorPtr, "")

  SwsVector *Vector = sws_allocVec(Length);
  FFMPEG_PTR_STORE(Vector, SwsVectorId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsGetGaussianVec::body(const Runtime::CallingFrame &Frame,
                                        uint32_t SwsVectorPtr, double Variance,
                                        double Quality) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(SwsVectorId, MemInst, uint32_t, SwsVectorPtr, "")

  SwsVector *Vector = sws_getGaussianVec(Variance, Quality);
  FFMPEG_PTR_STORE(Vector, SwsVectorId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsScaleVec::body(const Runtime::CallingFrame &,
                                  uint32_t SwsVectorId, double Scalar) {

  FFMPEG_PTR_FETCH(Vector, SwsVectorId, SwsVector);
  sws_scaleVec(Vector, Scalar);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsNormalizeVec::body(const Runtime::CallingFrame &,
                                      uint32_t SwsVectorId, double Height) {

  FFMPEG_PTR_FETCH(Vector, SwsVectorId, SwsVector);
  sws_normalizeVec(Vector, Height);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsGetCoeffVecLength::body(const Runtime::CallingFrame &,
                                           uint32_t SwsVectorId) {

  FFMPEG_PTR_FETCH(Vector, SwsVectorId, SwsVector);
  return Vector->length *
         sizeof(double); // Getting the size in uint_8* (Cuz Passing uint8_t*
                         // array from Rust SDK).
}

Expect<int32_t> SwsGetCoeff::body(const Runtime::CallingFrame &Frame,
                                  uint32_t SwsVectorId, uint32_t CoeffBufPtr,
                                  uint32_t Len) {

  MEMINST_CHECK(MemInst, Frame, 0)
  MEM_SPAN_CHECK(Buffer, MemInst, uint8_t, CoeffBufPtr, Len, "");
  FFMPEG_PTR_FETCH(Vector, SwsVectorId, SwsVector);

  double *Coeff = Vector->coeff;
  memmove(Buffer.data(), Coeff, Len);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> SwsFreeVec::body(const Runtime::CallingFrame &,
                                 uint32_t SwsVectorId) {

  FFMPEG_PTR_FETCH(Vector, SwsVectorId, SwsVector);
  sws_freeVec(Vector);
  FFMPEG_PTR_DELETE(SwsVectorId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> SwscaleVersion::body(const Runtime::CallingFrame &) {
  return swscale_version();
}

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
