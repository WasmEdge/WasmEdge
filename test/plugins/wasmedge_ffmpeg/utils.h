// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "avcodec/module.h"
#include "avdevice/module.h"
#include "avfilter/module.h"
#include "avformat/module.h"
#include "avutil/module.h"
#include "common/filesystem.h"
#include "swresample/module.h"
#include "swscale/module.h"

#include "common/defines.h"
#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <fcntl.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

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
                           uint32_t Offset, std::string_view Str) noexcept {
  char *Buf = MemInst->getPointer<char *>(Offset);
  std::copy_n(Str.data(), Str.length(), Buf);
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

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
inline int countOpenDescriptors() {
  int Count = 0;
  for (int Fd = 0; Fd < 1024; ++Fd) {
    if (fcntl(Fd, F_GETFD) != -1) {
      ++Count;
    }
  }
  return Count;
}
#endif

// The RIFF chunk size at offset 4 of a WAV file, and the file's size. The
// wav muxer writes the chunk size as 0xFFFFFFFF in its header and patches in
// the real one, the file size less 8, only in its trailer.
inline std::pair<uint32_t, uint32_t>
wavRiffAndFileSize(const std::string &Path) {
  std::ifstream In(Path, std::ios::binary);
  std::vector<unsigned char> Bytes((std::istreambuf_iterator<char>(In)),
                                   std::istreambuf_iterator<char>());
  uint32_t RiffSize = 0;
  if (Bytes.size() >= 8) {
    RiffSize = static_cast<uint32_t>(Bytes[4]) |
               (static_cast<uint32_t>(Bytes[5]) << 8) |
               (static_cast<uint32_t>(Bytes[6]) << 16) |
               (static_cast<uint32_t>(Bytes[7]) << 24);
  }
  return {RiffSize, static_cast<uint32_t>(Bytes.size())};
}

class FFmpegTest : public ::testing::Test {
public:
  FFmpegTest() : Mod(""), CallFrame(nullptr, &Mod) {
    Mod.addHostMemory(
        "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                      WasmEdge::AST::MemoryType(1)));
    MemInst = Mod.findMemoryExports("memory");

    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(WasmEdge::u8path(
        "../../../plugins/wasmedge_ffmpeg/" WASMEDGE_LIB_PREFIX
        "wasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avformat"sv)) {
        AVFormatMod =
            dynamicPointerCast<WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::
                                   WasmEdgeFFmpegAVFormatModule>(
                Module->create());
      }
      if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_avutil"sv)) {
        AVUtilMod = dynamicPointerCast<
            WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule>(
            Module->create());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_swscale"sv)) {
        SWScaleMod =
            dynamicPointerCast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::
                                   WasmEdgeFFmpegSWScaleModule>(
                Module->create());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avcodec"sv)) {
        AVCodecMod =
            dynamicPointerCast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::
                                   WasmEdgeFFmpegAVCodecModule>(
                Module->create());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_swresample"sv)) {
        SWResampleMod =
            dynamicPointerCast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::
                                   WasmEdgeFFmpegSWResampleModule>(
                Module->create());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avfilter"sv)) {
        AVFilterMod =
            dynamicPointerCast<WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::
                                   WasmEdgeFFmpegAVFilterModule>(
                Module->create());
      }
      if (const auto *Module =
              Plugin->findModule("wasmedge_ffmpeg_avdevice"sv)) {
        AVDeviceMod =
            dynamicPointerCast<WasmEdge::Host::WasmEdgeFFmpeg::AVDevice::
                                   WasmEdgeFFmpegAVDeviceModule>(
                Module->create());
      }
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

  // Results of Funcs are stored here.
  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod;
  WasmEdge::Runtime::Instance::MemoryInstance *MemInst;
  WasmEdge::Runtime::CallingFrame CallFrame;

  // Wasm Modules.
  std::unique_ptr<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFormat::WasmEdgeFFmpegAVFormatModule>
      AVFormatMod;
  std::unique_ptr<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule>
      AVUtilMod;
  std::unique_ptr<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::
                      WasmEdgeFFmpegSWResampleModule>
      SWResampleMod;
  std::unique_ptr<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::WasmEdgeFFmpegSWScaleModule>
      SWScaleMod;
  std::unique_ptr<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule>
      AVCodecMod;
  std::unique_ptr<
      WasmEdge::Host::WasmEdgeFFmpeg::AVFilter::WasmEdgeFFmpegAVFilterModule>
      AVFilterMod;
  std::unique_ptr<
      WasmEdge::Host::WasmEdgeFFmpeg::AVDevice::WasmEdgeFFmpegAVDeviceModule>
      AVDeviceMod;
};

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
