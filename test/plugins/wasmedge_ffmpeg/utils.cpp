#include "utils.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

void FFmpegTest::initEmptyFrame(uint32_t FramePtr) {

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
          FuncInst->getHostFunc());
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
}

void FFmpegTest::initAVCodec(uint32_t AVCodecPtr, uint32_t AVFormatCtxPtr,
                             uint32_t FilePtr, uint32_t CodecParameterPtr,
                             uint32_t AVCodecCtxPtr) {
  initFormatCtx(AVFormatCtxPtr, FilePtr);

  uint32_t AvFormatCtxId = readUInt32(MemInst, AVFormatCtxPtr);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_av_find_best_stream");
  auto &HostFuncAVFindBestStream = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFindBestStream &>(
      FuncInst->getHostFunc());
  HostFuncAVFindBestStream.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   AvFormatCtxId, 0, -1, -1, 0, 0},
                               Result);

  uint32_t StreamIdx = Result[0].get<int32_t>();

  FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avStream_codecpar");

  auto &HostFuncAVStreamCodecpar = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVStreamCodecPar &>(
      FuncInst->getHostFunc());

  HostFuncAVStreamCodecpar.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   AvFormatCtxId, StreamIdx, CodecParameterPtr},
                               Result);

  uint32_t CodecParametersId = readUInt32(MemInst, CodecParameterPtr);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3");
  auto &HostFuncAVCodecAllocContext3 = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecAllocContext3 &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecAllocContext3.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{0, AVCodecCtxPtr},
      Result);

  uint32_t AVCodecCtxId = readUInt32(MemInst, AVCodecCtxPtr);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context");
  auto &HostFuncAVCodecParametersToContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersToContext &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecParametersToContext.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                  CodecParametersId},
      Result);

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id");
  auto &HostFuncAVCodecContextCodecId = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxCodecID &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecContextCodecId.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
      Result);

  uint32_t codec_id = Result[0].get<int32_t>();

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodec_find_decoder");
  auto &HostFuncAVCodecFindDecoder = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecFindDecoder &>(
      FuncInst->getHostFunc());

  HostFuncAVCodecFindDecoder.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{codec_id, AVCodecPtr},
      Result);

  uint32_t AVCodecId = readUInt32(MemInst, AVCodecPtr);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_open2");
  auto &HostFuncAVCodecOpen2 =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecOpen2 &>(
          FuncInst->getHostFunc());

  HostFuncAVCodecOpen2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, AVCodecId, 0},
      Result);
}

void FFmpegTest::initFormatCtx(uint32_t AVFormatCtxPtr, uint32_t StartPtr) {

  int32_t Length = 32;
  fillMemContent(MemInst, StartPtr, Length);
  std::string Url = std::string(
      "ffmpeg-assets/sample_video.mp4"); // downloaded using bash script
  fillMemContent(MemInst, StartPtr, Url);

  auto *FuncInst = AVFormatMod->findFuncExports(
      "wasmedge_ffmpeg_avformat_avformat_open_input");
  auto &HostFuncAVFormatOpenInput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatOpenInput &>(
      FuncInst->getHostFunc());
  HostFuncAVFormatOpenInput.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          AVFormatCtxPtr, StartPtr, Length, UINT32_C(0), UINT32_C(0)},
      Result);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
