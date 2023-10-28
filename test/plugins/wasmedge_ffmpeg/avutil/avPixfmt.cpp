#include "../utils.h"
#include "avutil/module.h"
#include "avutil/pixfmt.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVUtilTest, AVPixFmt) {

  auto *AVUtilMod = TestUtils::InitModules::createAVUtilModule();
  ASSERT_TRUE(AVUtilMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(5)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  //  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  auto *FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avpixfmtdescriptor_nb_components");
  auto &HostFuncAVPixFmtDescriptorNbComponents = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AvPixFmtDescriptorNbComponents &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVPixFmtDescriptorNbComponents.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{3}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 3);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromaw");
  auto &HostFuncAvPixFmtDescriptorLog2ChromaW = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AvPixFmtDescriptorLog2ChromaW &>(
      FuncInst->getHostFunc());

  {
    HostFuncAvPixFmtDescriptorLog2ChromaW.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{1}, Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_avpixfmtdescriptor_log2_chromah");
  auto &HostFuncAvPixFmtDescriptorLog2ChromaH = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AvPixFmtDescriptorLog2ChromaH &>(
      FuncInst->getHostFunc());

  {
    HostFuncAvPixFmtDescriptorLog2ChromaH.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{3}, Result);

    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
  }
}