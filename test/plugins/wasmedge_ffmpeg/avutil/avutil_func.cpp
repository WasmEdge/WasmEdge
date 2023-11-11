#include "avutil/avutil_func.h"
#include "../utils.h"
#include "avutil/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVUtilTest, AVUtilFunc) {

  auto *AVUtilMod = TestUtils::InitModules::createAVUtilModule();
  ASSERT_TRUE(AVUtilMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(5)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  uint32_t NamePtr = UINT32_C(4);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_set_level");
  auto &HostFuncAVLogSetLevel =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogSetLevel &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVLogSetLevel.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{24}, Result);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_get_level");
  auto &HostFuncAVLogGetLevel =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogGetLevel &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVLogGetLevel.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 32);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_set_flags");
  auto &HostFuncAVLogSetFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogSetFlags &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVLogSetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_log_get_flags");
  auto &HostFuncAVLogGetFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVLogGetFlags &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVLogGetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 32);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_rescale_q");
  auto &HostFuncAVRescaleQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVRescaleQ &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVRescaleQ.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{20, 5, 10, 5, 20}, Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_rescale_q_rnd");
  auto &HostFuncAVRescaleQRnd =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVRescaleQRnd &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVRescaleQRnd.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{20, 5, 10, 5, 20, 2},
        Result);

    EXPECT_TRUE(Result[0].get<int64_t>() > 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_avutil_version");
  auto &HostFuncAVUtilVersion =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilVersion &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_TRUE(Result[0].get<uint32_t>() > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_channel_layout_nb_channels");
  auto &HostFuncAVGetChannelLayoutNbChannels = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetChannelLayoutNbChannels &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetChannelLayoutNbChannels.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);
    EXPECT_TRUE(Result[0].get<uint32_t>() > 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_get_default_channel_layout");
  auto &HostFuncAVGetDefaultChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVGetDefaultChannelLayout &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVGetDefaultChannelLayout.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);
    EXPECT_TRUE(Result[0].get<uint32_t>() > 0);
  }

  uint32_t Length;
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avutil_configuration_length");
  auto &HostFuncAVUtilConfigurationLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilConfigurationLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVUtilConfigurationLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill NamePtr with 0.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_avutil_configuration");
  auto &HostFuncAVUtilConfiguration = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilConfiguration &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVUtilConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avutil_license_length");
  auto &HostFuncAVUtilLicenseLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilLicenseLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVUtilLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill NamePtr with 0.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_avutil_license");
  auto &HostFuncAVUtilLicense =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilLicense &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}