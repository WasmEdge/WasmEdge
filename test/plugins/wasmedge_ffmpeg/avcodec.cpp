#include "runtime/instance/module.h"
#include "common/defines.h"
#include "avcodec/module.h"
#include "avcodec/avcodec_func.h"
#include "common/types.h"

#include <gtest/gtest.h>
//
//namespace {
//WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
//  using namespace std::literals::string_view_literals;
//  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
//      "../../../plugins/wasmedge_ffmpeg/"
//      "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
//  if (const auto *Plugin =
//          WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
//    if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_avcodec"sv)) {
//      return Module->create().release();
//    }
//  }
//  return nullptr;
//}
//}
//
//TEST(WasmEdgeAVCodecTest, Module) {
//// Create the wasmedge_ffmpeg_avcodec module instance.
//auto *AVCodecMod =
//    dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule *>(createModule());
//EXPECT_FALSE(AVCodecMod == nullptr);
//EXPECT_EQ(AVCodecMod->getFuncExportNum(), 30U);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_alloc_context3"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_parameters_free"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_free_context"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_get_type"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_open2"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_find_decoder"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_codec_is_encoder"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_codec_is_decoder"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_close"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_parameters_to_context"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_receive_frame"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_send_packet"), nullptr);
//
//// AVCodecContext Struct Field Access
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodeccontext_codec_type"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodeccontext_time_base"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodeccontext_width"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodeccontext_height"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodeccontext_sample_aspect_ratio"), nullptr);
//
//// AVCodec Struct Fields Access.
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_id"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_type"), nullptr);
//
//// AVCodecParam Struct Fields Access.
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodecparam_codec_id"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodecparam_codec_type"), nullptr);
//
//// AVPacket Functions.
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_alloc"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_new_packet"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_unref"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_grow_packet"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_shrink_packet"), nullptr);
//EXPECT_NE(AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_stream_index"), nullptr);
//delete AVCodecMod;
//}

//TEST(WasmEdgeAVCodecTest,FunctionsTest){
//
//  auto *AVCodecMod = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::WasmEdgeFFmpegAVCodecModule *>(createModule());
//
//  // Create the calling frame with memory instance.
//  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
//  Mod.addHostMemory(
//  "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
//      WasmEdge::AST::MemoryType(1)));
//  auto *MemInstPtr = Mod.findMemoryExports("memory");
//  EXPECT_NE(MemInstPtr, nullptr);
//  auto &MemInst = *MemInstPtr;
//  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
//
////  uint32_t StorePtr = UINT32_C(65536);
//  auto *FuncInst = AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_avcodec_parameters_alloc");
//  EXPECT_TRUE(FuncInst->isHostFunction());
//
//  auto &HostFuncInst =
//    dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParametersAlloc &>(FuncInst->getHostFunc());
//
//}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
