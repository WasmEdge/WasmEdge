#include "runtime/instance/module.h"
#include "common/types.h"

#include "avutil/module.h"
#include "avutil/avRational.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_ffmpeg/"
      "libwasmedgePluginWasmEdgeFFmpeg" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
      WasmEdge::Plugin::Plugin::find("wasmedge_ffmpeg"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_ffmpeg_avutil"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}
}

void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,uint32_t Value, uint32_t &Ptr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(Ptr);
  *BufPtr = Value;
  Ptr+=4;
}

void writeIInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,int32_t Value, uint32_t &Ptr) {
  int32_t *BufPtr = MemInst.getPointer<int32_t *>(Ptr);
  *BufPtr = Value;
  Ptr+=4;
}

int32_t readIInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ptr) {
  int32_t *BufPtr = MemInst.getPointer<int32_t *>(Ptr);
  return *BufPtr;
}

TEST(WasmEdgeAVUtilTest, AVRational) {

  // Create the wasmedge_process module instance.
  auto *AVUtilMod = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::WasmEdgeFFmpegAVUtilModule *>(createModule());
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

  // Addition Function
  auto *FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_add_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVAddQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVAddQ &>(FuncInst->getHostFunc());

  uint32_t ANumPtr = UINT32_C(0);    // To separate memory address. Same number leading to Collision of addresses.
  uint32_t ADenPtr = UINT32_C(4000); // To separate memory address

  {
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    EXPECT_TRUE(HostFuncAVAddQ.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       INT32_C(3), INT32_C(4), INT32_C(-6),
                                       INT32_C(7), ANumPtr, ADenPtr},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readIInt32(MemInst,ANumPtr),-3);
    EXPECT_EQ(readIInt32(MemInst,ADenPtr),28);
  }

  // Subtraction Function
  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_sub_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVSubQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSubQ &>(FuncInst->getHostFunc());

  {
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    EXPECT_TRUE(HostFuncAVSubQ.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       INT32_C(-843), INT32_C(11), INT32_C(38),
                                       INT32_C(12), ANumPtr, ADenPtr},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readIInt32(MemInst,ANumPtr),-5267);
    EXPECT_EQ(readIInt32(MemInst,ADenPtr),66);
  }


  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_mul_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVMulQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVMulQ &>(FuncInst->getHostFunc());

  {
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    EXPECT_TRUE(HostFuncAVMulQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(-6), INT32_C(7), INT32_C(3),
        INT32_C(4), ANumPtr, ADenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readIInt32(MemInst,ANumPtr),-9);
    EXPECT_EQ(readIInt32(MemInst,ADenPtr),14);
  }


  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_div_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDivQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDivQ &>(FuncInst->getHostFunc());

  {
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    EXPECT_TRUE(HostFuncAVDivQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(-6), INT32_C(7), INT32_C(3),
        INT32_C(4), ANumPtr, ADenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readIInt32(MemInst,ANumPtr),-8);
    EXPECT_EQ(readIInt32(MemInst,ADenPtr),7);
  }

  // How to Pass a Double functions.

//  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_d2q");
//  EXPECT_NE(FuncInst, nullptr);
//  EXPECT_TRUE(FuncInst->isHostFunction());
//  auto &HostFuncAVDivQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDivQ &>(FuncInst->getHostFunc());
//
//  {
//    writeUInt32(MemInst,INT32_C(0),ANumPtr);
//    writeUInt32(MemInst,INT32_C(0),ADenPtr);
//    EXPECT_TRUE(HostFuncAVDivQ.run(CallFrame,
//        std::initializer_list<WasmEdge::ValVariant>{
//        INT32_C(-6), INT32_C(7),ANumPtr, ADenPtr},
//        Result));
//    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));
//
//    EXPECT_EQ(readIInt32(MemInst,ANumPtr),-8);
//    EXPECT_EQ(readIInt32(MemInst,ADenPtr),7);
//  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_q2d");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVQ2d = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVQ2d &>(FuncInst->getHostFunc());

  {
    // Convert Rational Number to Double.
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    EXPECT_TRUE(HostFuncAVQ2d.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(1), INT32_C(2)},
        Result));
    EXPECT_EQ(Result[0].get<double_t>(), 0.5);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_inv_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInvQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVInvQ &>(FuncInst->getHostFunc());

  {
    // Inverse a Rational Number.
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    EXPECT_TRUE(HostFuncInvQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(-3), INT32_C(4),ANumPtr, ADenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readIInt32(MemInst,ANumPtr),4);
    EXPECT_EQ(readIInt32(MemInst,ADenPtr),-3);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_q2intfloat");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVQ2IntFloat = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVQ2IntFloat &>(FuncInst->getHostFunc());

  {
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    EXPECT_TRUE(HostFuncAVQ2IntFloat.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(1), INT32_C(5)},
        Result));
    EXPECT_EQ(Result[0].get<uint32_t>(), static_cast<uint32_t>(1045220557));
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_nearer_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVNearerQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVNearerQ &>(FuncInst->getHostFunc());

  {
    writeUInt32(MemInst,INT32_C(0),ANumPtr);
    writeUInt32(MemInst,INT32_C(0),ADenPtr);
    // B nearer to A
    EXPECT_TRUE(HostFuncAVNearerQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(1),INT32_C(3),INT32_C(1), INT32_C(2),INT32_C(-1), INT32_C(2)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(1));

    // C nearer to A
    EXPECT_TRUE(HostFuncAVNearerQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(-1),INT32_C(3),INT32_C(1), INT32_C(2),INT32_C(-1), INT32_C(2)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(-1));

    // Both are at same distance
    EXPECT_TRUE(HostFuncAVNearerQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(0),INT32_C(0),INT32_C(1), INT32_C(2),INT32_C(-1), INT32_C(2)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(0));
  }


  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_cmp_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVCmpQ = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVCmpQ &>(FuncInst->getHostFunc());

  {
    // A < B
    EXPECT_TRUE(HostFuncAVCmpQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(1),INT32_C(2),INT32_C(2), INT32_C(1)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(-1));

    // A > B
    EXPECT_TRUE(HostFuncAVCmpQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(2),INT32_C(1),INT32_C(1), INT32_C(2)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(1));

    // A == B
    EXPECT_TRUE(HostFuncAVCmpQ.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        INT32_C(2),INT32_C(1),INT32_C(2), INT32_C(1)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(0));
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_reduce");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVReduce = dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVReduce &>(FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVReduce.run(CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
        ANumPtr,ADenPtr,INT32_C(1),INT32_C(2),INT32_C(3)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(1));
  }
}

