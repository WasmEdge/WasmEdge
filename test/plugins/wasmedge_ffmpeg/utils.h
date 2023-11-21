#pragma once
#include "avcodec/avCodecContext.h"
#include "avcodec/avCodecParameters.h"
#include "avcodec/avPacket.h"
#include "avcodec/avcodec_func.h"
#include "avcodec/module.h"
#include "avformat/avStream.h"
#include "avformat/avformat_func.h"
#include "avformat/module.h"
#include "avutil/avDictionary.h"
#include "avutil/avFrame.h"
#include "avutil/module.h"
#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "swresample/module.h"
#include "swscale/module.h"
#include "gtest/gtest.h"

inline void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t Value, uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(Ptr);
  *BufPtr = Value;
  Ptr += 4;
}

inline void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t Offset, uint32_t Cnt,
                           uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

inline void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t Offset, const std::string &Str) noexcept {
  char *Buf = MemInst.getPointer<char *>(Offset);
  std::copy_n(Str.c_str(), Str.length(), Buf);
}

inline void writeIInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                        int32_t Value, uint32_t &Ptr) {
  int32_t *BufPtr = MemInst.getPointer<int32_t *>(Ptr);
  *BufPtr = Value;
  Ptr += 4;
}

inline int32_t readIInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t &Ptr) {
  int32_t *BufPtr = MemInst.getPointer<int32_t *>(Ptr);
  return *BufPtr;
}

inline uint32_t readUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(Ptr);
  return *BufPtr;
}

namespace TestUtils {
class InitModules {
public:
  static WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::
      WasmEdgeFFmpegAVFormatModule *
      createAVFormatModule() {
    WasmEdge::Runtime::Instance::ModuleInstance *Mod = nullptr;
    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
        "../../plugins/wasmedge_ffmpeg/"
        "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avformat"sv)) {
        Mod = Module->create().release();
      }
    }
    return dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::
                            WasmEdgeFFmpegAVFormatModule *>(Mod);
  }

  static WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule *
  createAVUtilModule() {
    WasmEdge::Runtime::Instance::ModuleInstance *Mod = nullptr;
    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
        "../../plugins/wasmedge_ffmpeg/"
        "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
      if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_avutil"sv)) {
        Mod = Module->create().release();
      }
    }
    return dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule *>(
        Mod);
  }

  static WasmEdge::Host::WasmEdgeFFmpeg::SWResample::
      WasmEdgeFFmpegSWResampleModule *
      createSWResampleModule() {
    WasmEdge::Runtime::Instance::ModuleInstance *Mod = nullptr;
    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
        "../../plugins/wasmedge_ffmpeg/"
        "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_swresample"sv)) {
        Mod = Module->create().release();
      }
    }

    return dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::
                            WasmEdgeFFmpegSWResampleModule *>(Mod);
  }

  static WasmEdge::Host::WasmEdgeFFmpeg::SWScale::WasmEdgeFFmpegSWScaleModule *
  createSWScaleModule() {

    WasmEdge::Runtime::Instance::ModuleInstance *Mod = nullptr;
    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
        "../../plugins/wasmedge_ffmpeg/"
        "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_swscale"sv)) {
        Mod = Module->create().release();
      }
    }
    return dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::SWScale::WasmEdgeFFmpegSWScaleModule *>(
        Mod);
  }

  static WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule *
  createAVCodecModule() {

    WasmEdge::Runtime::Instance::ModuleInstance *Mod = nullptr;
    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
        "../../plugins/wasmedge_ffmpeg/"
        "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avcodec"sv)) {
        Mod = Module->create().release();
      }
    }
    return dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule *>(
        Mod);
  }
};

class AVFormatContext {
public:
  static void initFormatCtx(WasmEdge::Runtime::Instance::ModuleInstance &Mod,
                            uint32_t AVFormatCtxPtr,
                            std::array<WasmEdge::ValVariant, 1> Result) {

    auto *AVFormatMod = InitModules::createAVFormatModule();
    WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
    auto *MemInstPtr = Mod.findMemoryExports("memory");
    auto &MemInst = *MemInstPtr;

    fillMemContent(MemInst, 100, 32);
    std::string Url = std::string("ffmpeg-assets/sample_video.mp4");
    fillMemContent(MemInst, 100, Url);

    auto *FuncInst = AVFormatMod->findFuncExports(
        "wasmedge_ffmpeg_avformat_avformat_open_input");
    auto &HostFuncAVFormatOpenInput = dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatOpenInput &>(
        FuncInst->getHostFunc());
    HostFuncAVFormatOpenInput.run(CallFrame,
                                  std::initializer_list<WasmEdge::ValVariant>{
                                      AVFormatCtxPtr, UINT32_C(100),
                                      UINT32_C(30), UINT32_C(0), UINT32_C(0)},
                                  Result);
  }
};

class AVDictionary {
public:
  static void initDict(WasmEdge::Runtime::Instance::ModuleInstance &Mod,
                       uint32_t DictPtr,
                       std::array<WasmEdge::ValVariant, 1> Result) {

    auto *AVUtilMod = InitModules::createAVUtilModule();
    WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
    auto *MemInstPtr = Mod.findMemoryExports("memory");
    auto &MemInst = *MemInstPtr;

    fillMemContent(MemInst, 133, 8);
    fillMemContent(MemInst, 100, std::string("Key"));
    fillMemContent(MemInst, 137, std::string("Value"));

    auto *FuncInst =
        AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
    auto &HostFuncAVDictSet =
        dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
            FuncInst->getHostFunc());

    HostFuncAVDictSet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            DictPtr, UINT32_C(133), UINT32_C(3), UINT32_C(137), UINT32_C(5), 0},
        Result);
  }
};

class AVFrame {
public:
  static void initEmptyFrame(WasmEdge::Runtime::Instance::ModuleInstance &Mod,
                             uint32_t AVFramePtr,
                             std::array<WasmEdge::ValVariant, 1> Result) {

    auto *AVUtilMod = InitModules::createAVUtilModule();
    WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

    auto *FuncInst =
        AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
    auto &HostFuncAVFrameAlloc =
        dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
            FuncInst->getHostFunc());
    HostFuncAVFrameAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFramePtr},
        Result);
  }

  static void initVideoFrame(WasmEdge::Runtime::Instance::ModuleInstance &Mod,
                             uint32_t AVFramePtr, uint32_t AVFormatCtxPtr,
                             uint32_t CodecParameterPtr, uint32_t AVCodecCtxPtr,
                             uint32_t AVCodecPtr, uint32_t PacketPtr,
                             std::array<WasmEdge::ValVariant, 1> Result) {

    auto *MemInstPtr = Mod.findMemoryExports("memory");
    auto &MemInst = *MemInstPtr;

    auto *AVFormatMod = InitModules::createAVFormatModule();
    auto *AVCodecMod = InitModules::createAVCodecModule();
    WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

    AVFormatContext::initFormatCtx(Mod, AVFormatCtxPtr, Result);
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

    HostFuncAVStreamCodecpar.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AvFormatCtxId, StreamIdx,
                                                    CodecParameterPtr},
        Result);

    uint32_t CodecParametersId = readUInt32(MemInst, CodecParameterPtr);

    FuncInst = AVCodecMod->findFuncExports(
        "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3");
    auto &HostFuncAVCodecAllocContext3 = dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecAllocContext3 &>(
        FuncInst->getHostFunc());

    HostFuncAVCodecAllocContext3.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{0, AVCodecCtxPtr}, Result);

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

    initEmptyFrame(Mod, AVFramePtr, Result);
    uint32_t FrameId = readUInt32(MemInst, AVFramePtr);

    FuncInst = AVCodecMod->findFuncExports(
        "wasmedge_ffmpeg_avcodec_avcodec_receive_frame");
    auto &HostFuncAVCodecReceiveFrame = dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecReceiveFrame &>(
        FuncInst->getHostFunc());

    FuncInst =
        AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_alloc");
    auto &HostFuncAVPacketAlloc =
        dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketAlloc &>(
            FuncInst->getHostFunc());

    FuncInst =
        AVFormatMod->findFuncExports("wasmedge_ffmpeg_avformat_av_read_frame");
    auto &HostFuncAVReadFrame =
        dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVReadFrame &>(
            FuncInst->getHostFunc());

    FuncInst = AVCodecMod->findFuncExports(
        "wasmedge_ffmpeg_avcodec_avcodec_send_packet");
    auto &HostFuncAVCodecSendPacket = dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecSendPacket &>(
        FuncInst->getHostFunc());

    FuncInst = AVCodecMod->findFuncExports(
        "wasmedge_ffmpeg_avcodec_av_packet_stream_index");
    auto &HostFuncAVPacketStreamIndex = dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketStreamIndex &>(
        FuncInst->getHostFunc());

    while (true) {

      HostFuncAVCodecReceiveFrame.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, FrameId},
          Result);

      // Error returned by FFmpeg are negative.
      int32_t Error = Result[0].get<int32_t>() * -1;

      if (Error == EAGAIN) {
        while (true) {

          HostFuncAVPacketAlloc.run(
              CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketPtr},
              Result);

          uint32_t PackedId = readUInt32(MemInst, PacketPtr);

          while (true) {
            HostFuncAVReadFrame.run(CallFrame,
                                    std::initializer_list<WasmEdge::ValVariant>{
                                        AvFormatCtxId, PackedId},
                                    Result);

            int32_t Res = Result[0].get<int32_t>();
            if (Res == 0 || Res == AVERROR_EOF) {
              break;
            }
          }

          HostFuncAVPacketStreamIndex.run(
              CallFrame,
              std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                          FrameId},
              Result);

          uint32_t PacketStreamIdx = Result[0].get<int32_t>();

          if (PacketStreamIdx != StreamIdx) {
            continue;
          }

          HostFuncAVCodecSendPacket.run(
              CallFrame,
              std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                          PackedId},
              Result);
          break;
        }
      } else {
        break;
      }
    }
  }
};

} // namespace TestUtils

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
inline void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t Value, uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst->getPointer<uint32_t *>(Ptr);
  *BufPtr = Value;
  Ptr += 4;
}

inline void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                           uint32_t Offset, uint32_t Cnt,
                           uint8_t C = 0) noexcept {
  std::fill_n(MemInst->getPointer<uint8_t *>(Offset), Cnt, C);
}

inline void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                           uint32_t Offset, const std::string &Str) noexcept {
  char *Buf = MemInst->getPointer<char *>(Offset);
  std::copy_n(Str.c_str(), Str.length(), Buf);
}

inline void writeIInt32(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                        int32_t Value, uint32_t &Ptr) {
  int32_t *BufPtr = MemInst->getPointer<int32_t *>(Ptr);
  *BufPtr = Value;
  Ptr += 4;
}

inline int32_t readIInt32(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                          uint32_t &Ptr) {
  int32_t *BufPtr = MemInst->getPointer<int32_t *>(Ptr);
  return *BufPtr;
}

inline uint32_t readUInt32(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                           uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst->getPointer<uint32_t *>(Ptr);
  return *BufPtr;
}

class FFmpegTest : public ::testing::Test {
public:
  FFmpegTest() : Mod(""), CallFrame(nullptr, &Mod) {
    Mod.addHostMemory(
        "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                      WasmEdge::AST::MemoryType(1)));
    MemInst = Mod.findMemoryExports("memory");

    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
        "../../../plugins/wasmedge_ffmpeg/"
        "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avformat"sv)) {
        AVFormatMod = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::
                                       WasmEdgeFFmpegAVFormatModule *>(
            Module->create().release());
      }
      if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_avutil"sv)) {
        AVUtilMod = dynamic_cast<
            WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule
                *>(Module->create().release());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_swscale"sv)) {
        SWScaleMod = dynamic_cast<
            WasmEdge::Host::WasmEdgeFFmpeg::SWScale::WasmEdgeFFmpegSWScaleModule
                *>(Module->create().release());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avcodec"sv)) {
        AVCodecMod = dynamic_cast<
            WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule
                *>(Module->create().release());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_swresample"sv)) {
        SWResampleMod =
            dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::
                             WasmEdgeFFmpegSWResampleModule *>(
                Module->create().release());
      }
    }
  }

  ~FFmpegTest() override {
    if (AVUtilMod) {
      delete AVUtilMod;
    }
    if (AVCodecMod) {
      delete AVCodecMod;
    }
    if (SWScaleMod) {
      delete SWScaleMod;
    }
    if (SWResampleMod) {
      delete SWResampleMod;
    }
    if (AVFormatMod) {
      delete AVFormatMod;
    }
  }

protected:
  void initEmptyFrame(uint32_t FramePtr);

  void initDict(uint32_t DictPtr, uint32_t KeyPtr, std::string Key,
                uint32_t ValuePtr, std::string Value);
  void initFFmpegStructs(uint32_t AVCodecPtr, uint32_t AVFormatCtxPtr,
                         uint32_t FilePtr, std::string FileName,
                         uint32_t CodecParameterPtr, uint32_t AVCodecCtxPtr,
                         uint32_t PacketPtr, uint32_t FramePtr);

  void initFormatCtx(uint32_t AVFormatCtxPtr, uint32_t FilePtr,
                     std::string FileName);

  // Result of Funcs to be stored here.
  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod;
  WasmEdge::Runtime::Instance::MemoryInstance *MemInst;
  WasmEdge::Runtime::CallingFrame CallFrame;

  // Wasm Modules.
  WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::WasmEdgeFFmpegAVFormatModule
      *AVFormatMod = nullptr;
  WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule
      *AVUtilMod = nullptr;
  WasmEdge::Host::WasmEdgeFFmpeg::SWResample::WasmEdgeFFmpegSWResampleModule
      *SWResampleMod = nullptr;
  WasmEdge::Host::WasmEdgeFFmpeg::SWScale::WasmEdgeFFmpegSWScaleModule
      *SWScaleMod = nullptr;
  WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule
      *AVCodecMod = nullptr;
};
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge