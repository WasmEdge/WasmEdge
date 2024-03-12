#pragma once
#include "avcodec/module.h"
#include "avfilter/module.h"
#include "avformat/module.h"
#include "avutil/module.h"
#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "swresample/module.h"
#include "swscale/module.h"
#include "gtest/gtest.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
inline void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t Value, uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst->getPointer<uint32_t *>(Ptr);
  *BufPtr = Value;
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

inline void writeSInt32(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
                        int32_t Value, uint32_t &Ptr) {
  int32_t *BufPtr = MemInst->getPointer<int32_t *>(Ptr);
  *BufPtr = Value;
}

inline int32_t readSInt32(WasmEdge::Runtime::Instance::MemoryInstance *MemInst,
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
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avfilter"sv)) {
        AVFilterMod = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::
                                       WasmEdgeFFmpegAVFilterModule *>(
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
    if (AVFilterMod) {
      delete AVFilterMod;
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
  void allocPacket(uint32_t PacketPtr);

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
  WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::WasmEdgeFFmpegAVFilterModule
      *AVFilterMod = nullptr;
};
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge