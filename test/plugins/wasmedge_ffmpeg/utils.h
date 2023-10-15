#pragma once
#include "avformat/avformat_func.h"
#include "avformat/module.h"
#include "avutil/module.h"
#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "swresample/module.h"
#include "swscale/module.h"

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

    fillMemContent(MemInst, AVFormatCtxPtr, 32);
    std::string Url = std::string("ffmpeg-assets/sample_video.mp4");
    fillMemContent(MemInst, AVFormatCtxPtr, Url);

    auto *FuncInst = AVFormatMod->findFuncExports(
        "wasmedge_ffmpeg_avformat_avformat_open_input");
    auto &HostFuncAVFormatOpenInput = dynamic_cast<
        WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::AVFormatOpenInput &>(
        FuncInst->getHostFunc());
    HostFuncAVFormatOpenInput.run(CallFrame,
                                  std::initializer_list<WasmEdge::ValVariant>{
                                      AVFormatCtxPtr, UINT32_C(1), UINT32_C(30),
                                      UINT32_C(0), UINT32_C(0)},
                                  Result);
  }
};

} // namespace TestUtils