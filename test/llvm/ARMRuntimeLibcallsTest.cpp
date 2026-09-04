// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "arm_runtime_libcalls.h"
#include "common/span.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "linker/elf_writer.h"
#include "linker/layout.h"
#include "linker/object_reader.h"
#include "linker/relocation.h"
#include "linker/universal_wasm_writer.h"
#include "linker/writer.h"
#include "llvm.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include <gtest/gtest.h>

#include <llvm-c/ExecutionEngine.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#if LLVM_VERSION_MAJOR >= 13
#include <llvm/MC/TargetRegistry.h>
#else
#include <llvm/Support/TargetRegistry.h>
#endif
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#if LLVM_VERSION_MAJOR >= 13
#include <llvm/Passes/PassBuilder.h>
#else
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif
#if LLVM_VERSION_MAJOR == 18
#include <llvm-c/TargetMachine.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

using WasmEdge::Byte;
using WasmEdge::Span;
using WasmEdge::LLVM::ARMFloatABI;
using WasmEdge::LLVM::ARMRuntimeLibcallProfile;
using namespace WasmEdge::LLVM::Linker;

class ScopedLoggingCallback {
public:
  explicit ScopedLoggingCallback(std::string &Message)
      : Logger(spdlog::default_logger()), Level(spdlog::get_level()) {
    WasmEdge::Log::setLoggingCallback(
        [&](const spdlog::details::log_msg &LogMessage) {
          Message.assign(LogMessage.payload.data(), LogMessage.payload.size());
        });
    WasmEdge::Log::setErrorLoggingLevel();
  }

  ~ScopedLoggingCallback() {
    spdlog::set_default_logger(Logger);
    spdlog::set_level(Level);
  }

private:
  std::shared_ptr<spdlog::logger> Logger;
  spdlog::level::level_enum Level;
};

std::string moduleIR(WasmEdge::LLVM::Module &Module) {
  char *Text = LLVMPrintModuleToString(Module.unwrap());
  std::string Result(Text);
  LLVMDisposeMessage(Text);
  return Result;
}

template <typename T> struct DivModResult {
  T Quotient;
  T Remainder;
};

template <typename T> DivModResult<T> referenceUnsignedDivMod(T Left, T Right) {
  return {static_cast<T>(Left / Right), static_cast<T>(Left % Right)};
}

template <typename T> DivModResult<T> referenceSignedDivMod(T Left, T Right) {
  if (Left == std::numeric_limits<T>::min() && Right == static_cast<T>(-1))
    return {std::numeric_limits<T>::min(), 0};
  return {static_cast<T>(Left / Right), static_cast<T>(Left % Right)};
}

struct RuntimeLibcallExecutionEngine {
  std::unique_ptr<llvm::LLVMContext> Context;
  std::unique_ptr<llvm::ExecutionEngine> Engine;
};

template <typename To, typename From> To bitCopy(From Value) {
  static_assert(sizeof(To) == sizeof(From));
  To Result;
  std::memcpy(&Result, &Value, sizeof(Result));
  return Result;
}

RuntimeLibcallExecutionEngine makeRuntimeLibcallExecutionEngine() {
  LLVMLinkInMCJIT();
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  auto Context = std::make_unique<llvm::LLVMContext>();
  auto Module =
      std::make_unique<llvm::Module>("arm-runtime-libcalls-jit", *Context);
  auto *LLModule = Module.get();
  WasmEdge::LLVM::Module RuntimeModule(
      reinterpret_cast<LLVMModuleRef>(LLModule));
  const ARMRuntimeLibcallProfile Profile{true, true, true,
                                         true, true, ARMFloatABI::SoftFP};
  EXPECT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(RuntimeModule, Profile));
  RuntimeModule.release();
  LLModule->setModuleInlineAsm("");
  EXPECT_FALSE(llvm::verifyModule(*LLModule, &llvm::errs()));
  std::string Error;
  auto Engine = std::unique_ptr<llvm::ExecutionEngine>(
      llvm::EngineBuilder(std::move(Module))
          .setErrorStr(&Error)
          .setEngineKind(llvm::EngineKind::JIT)
          .create());
  EXPECT_NE(Engine, nullptr) << Error;
  if (Engine)
    Engine->finalizeObject();
  return {std::move(Context), std::move(Engine)};
}

TEST(ARMRuntimeLibcallsCoreTest, UsesOnlyFixedWidthPrimitiveIntegerIR) {
  auto Context = WasmEdge::LLVM::Context::create();
  WasmEdge::LLVM::Module Module(Context, "arm-runtime-libcalls-ir");
  const ARMRuntimeLibcallProfile Profile{true, true, true,
                                         true, true, ARMFloatABI::SoftFP};
  ASSERT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(Module, Profile));
  auto *LLModule = reinterpret_cast<llvm::Module *>(Module.unwrap());
  for (const char *Name :
       {"__wasmedge_aeabi_idivmod_core", "__wasmedge_aeabi_uidivmod_core",
        "__wasmedge_aeabi_ldivmod_core", "__wasmedge_aeabi_uldivmod_core",
        "__wasmedge_aeabi_l2f_core", "__wasmedge_aeabi_ul2f_core",
        "__wasmedge_aeabi_l2d_core", "__wasmedge_aeabi_ul2d_core",
        "__wasmedge_aeabi_f2lz_core", "__wasmedge_aeabi_f2ulz_core",
        "__wasmedge_aeabi_d2lz_core", "__wasmedge_aeabi_d2ulz_core"}) {
    auto *Core = LLModule->getFunction(Name);
    ASSERT_NE(Core, nullptr);
    EXPECT_GT(Core->size(), 1U) << Name;
    for (const auto &Block : *Core)
      for (const auto &Instruction : Block) {
        EXPECT_FALSE(Instruction.getOpcode() == llvm::Instruction::UDiv ||
                     Instruction.getOpcode() == llvm::Instruction::SDiv ||
                     Instruction.getOpcode() == llvm::Instruction::URem ||
                     Instruction.getOpcode() == llvm::Instruction::SRem)
            << Name << ": " << Instruction.getOpcodeName();
        EXPECT_FALSE(Instruction.getOpcode() == llvm::Instruction::SIToFP ||
                     Instruction.getOpcode() == llvm::Instruction::UIToFP ||
                     Instruction.getOpcode() == llvm::Instruction::FPToSI ||
                     Instruction.getOpcode() == llvm::Instruction::FPToUI)
            << Name << ": " << Instruction.getOpcodeName();
        if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Instruction)) {
          EXPECT_TRUE(Call->getCalledFunction() != nullptr &&
                      Call->getCalledFunction()->isIntrinsic())
              << Name << ": " << Instruction.getOpcodeName();
        }
      }
  }
}

TEST(ARMRuntimeLibcallsCoreTest, DefinesBaseAAPCSConversionHelpers) {
  auto Context = WasmEdge::LLVM::Context::create();
  WasmEdge::LLVM::Module Module(Context, "arm-runtime-libcalls-conversions");
  const ARMRuntimeLibcallProfile Profile{true, true, true,
                                         true, true, ARMFloatABI::Hard};
  ASSERT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(Module, Profile));
  auto *LLModule = reinterpret_cast<llvm::Module *>(Module.unwrap());
  for (const auto &[Name, ArgumentWidth, ResultWidth] :
       std::array<std::tuple<const char *, unsigned, unsigned>, 8>{{
           {"__aeabi_l2f", 64, 32},
           {"__aeabi_ul2f", 64, 32},
           {"__aeabi_l2d", 64, 64},
           {"__aeabi_ul2d", 64, 64},
           {"__aeabi_f2lz", 32, 64},
           {"__aeabi_f2ulz", 32, 64},
           {"__aeabi_d2lz", 64, 64},
           {"__aeabi_d2ulz", 64, 64},
       }}) {
    auto *Function = LLModule->getFunction(Name);
    ASSERT_NE(Function, nullptr) << Name;
    EXPECT_TRUE(Function->getArg(0)->getType()->isIntegerTy(ArgumentWidth));
    EXPECT_TRUE(Function->getReturnType()->isIntegerTy(ResultWidth));
    EXPECT_EQ(Function->getCallingConv(), llvm::CallingConv::ARM_AAPCS);
    EXPECT_EQ(Function->getVisibility(), llvm::GlobalValue::HiddenVisibility);
    EXPECT_TRUE(Function->isDSOLocal());
    EXPECT_TRUE(Function->hasFnAttribute(llvm::Attribute::NoUnwind));
  }
}

TEST(ARMRuntimeLibcallsCoreTest, DefinesScalarHelpersWithProfilePCS) {
  const std::array<const char *, 12> Names{
      "ceil",      "ceilf",      "floor", "floorf", "trunc", "truncf",
      "roundeven", "roundevenf", "fmin",  "fminf",  "fmax",  "fmaxf"};
  for (const ARMFloatABI ABI : {ARMFloatABI::SoftFP, ARMFloatABI::Hard}) {
    auto Context = WasmEdge::LLVM::Context::create();
    WasmEdge::LLVM::Module Module(Context, "arm-runtime-scalar-pcs");
    const ARMRuntimeLibcallProfile Profile{true, true, true, true, true, ABI};
    ASSERT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(Module, Profile));
    auto *LLModule = reinterpret_cast<llvm::Module *>(Module.unwrap());
    for (const char *Name : Names) {
      auto *Function = LLModule->getFunction(Name);
      ASSERT_NE(Function, nullptr) << Name;
      EXPECT_EQ(Function->getCallingConv(),
                ABI == ARMFloatABI::Hard ? llvm::CallingConv::ARM_AAPCS_VFP
                                         : llvm::CallingConv::ARM_AAPCS)
          << Name;
      EXPECT_TRUE(Function->getReturnType()->isFloatingPointTy()) << Name;
      EXPECT_EQ(Function->getVisibility(), llvm::GlobalValue::HiddenVisibility)
          << Name;
      EXPECT_TRUE(Function->isDSOLocal()) << Name;
      EXPECT_TRUE(Function->hasFnAttribute(llvm::Attribute::NoUnwind)) << Name;
    }
  }
}

TEST(ARMRuntimeLibcallsCoreTest, ScalarCoresContainNoCallsOrFloatingPointIR) {
  auto Context = WasmEdge::LLVM::Context::create();
  WasmEdge::LLVM::Module Module(Context, "arm-runtime-scalar-ir");
  const ARMRuntimeLibcallProfile Profile{true, true, true,
                                         true, true, ARMFloatABI::SoftFP};
  ASSERT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(Module, Profile));
  auto *LLModule = reinterpret_cast<llvm::Module *>(Module.unwrap());
  for (const char *Name :
       {"__wasmedge_ceil_core", "__wasmedge_ceilf_core",
        "__wasmedge_floor_core", "__wasmedge_floorf_core",
        "__wasmedge_trunc_core", "__wasmedge_truncf_core",
        "__wasmedge_roundeven_core", "__wasmedge_roundevenf_core",
        "__wasmedge_fmin_core", "__wasmedge_fminf_core", "__wasmedge_fmax_core",
        "__wasmedge_fmaxf_core"}) {
    auto *Core = LLModule->getFunction(Name);
    ASSERT_NE(Core, nullptr) << Name;
    EXPECT_TRUE(Core->getReturnType()->isIntegerTy()) << Name;
    for (const auto &Block : *Core)
      for (const auto &Instruction : Block) {
        EXPECT_FALSE(llvm::isa<llvm::CallBase>(Instruction)) << Name;
        EXPECT_FALSE(Instruction.getType()->isFloatingPointTy()) << Name;
        for (const auto &Operand : Instruction.operands())
          EXPECT_FALSE(Operand->getType()->isFloatingPointTy()) << Name;
      }
  }
}

TEST(ARMRuntimeLibcallsCoreTest, RoundsIEEEBitsExactly) {
  auto JIT = makeRuntimeLibcallExecutionEngine();
  ASSERT_NE(JIT.Engine, nullptr);
  using F32Core = uint32_t (*)(uint32_t);
  using F64Core = uint64_t (*)(uint64_t);
  const auto Function32 = [&](const char *Name) {
    const auto Function =
        reinterpret_cast<F32Core>(JIT.Engine->getFunctionAddress(
            (std::string("__wasmedge_") + Name + "f_core").c_str()));
    EXPECT_NE(Function, nullptr) << Name;
    return Function;
  };
  const auto Function64 = [&](const char *Name) {
    const auto Function =
        reinterpret_cast<F64Core>(JIT.Engine->getFunctionAddress(
            (std::string("__wasmedge_") + Name + "_core").c_str()));
    EXPECT_NE(Function, nullptr) << Name;
    return Function;
  };
  const auto CeilF = Function32("ceil");
  const auto FloorF = Function32("floor");
  const auto TruncF = Function32("trunc");
  const auto EvenF = Function32("roundeven");
  const auto Ceil = Function64("ceil");
  const auto Floor = Function64("floor");
  const auto Trunc = Function64("trunc");
  const auto Even = Function64("roundeven");
  ASSERT_NE(CeilF, nullptr);
  ASSERT_NE(FloorF, nullptr);
  ASSERT_NE(TruncF, nullptr);
  ASSERT_NE(EvenF, nullptr);
  ASSERT_NE(Ceil, nullptr);
  ASSERT_NE(Floor, nullptr);
  ASSERT_NE(Trunc, nullptr);
  ASSERT_NE(Even, nullptr);

  struct F32Case {
    uint32_t Input, Ceil, Floor, Trunc, Even;
  };
  const std::array<F32Case, 16> F32Cases{{
      {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
      {0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
      {0x00000001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000},
      {0x80000001, 0x80000000, 0xbf800000, 0x80000000, 0x80000000},
      {0x3f000000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000},
      {0xbf000000, 0x80000000, 0xbf800000, 0x80000000, 0x80000000},
      {0x3fc00000, 0x40000000, 0x3f800000, 0x3f800000, 0x40000000},
      {0x40200000, 0x40400000, 0x40000000, 0x40000000, 0x40000000},
      {0xc0200000, 0xc0000000, 0xc0400000, 0xc0000000, 0xc0000000},
      {0x3f7fffff, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000},
      {0xbf7fffff, 0x80000000, 0xbf800000, 0x80000000, 0xbf800000},
      {0x4b000000, 0x4b000000, 0x4b000000, 0x4b000000, 0x4b000000},
      {0xcb000000, 0xcb000000, 0xcb000000, 0xcb000000, 0xcb000000},
      {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000},
      {0xff800000, 0xff800000, 0xff800000, 0xff800000, 0xff800000},
      {0x7f812345, 0x7fc12345, 0x7fc12345, 0x7fc12345, 0x7fc12345},
  }};
  for (const auto &Case : F32Cases) {
    EXPECT_EQ(CeilF(Case.Input), Case.Ceil);
    EXPECT_EQ(FloorF(Case.Input), Case.Floor);
    EXPECT_EQ(TruncF(Case.Input), Case.Trunc);
    EXPECT_EQ(EvenF(Case.Input), Case.Even);
  }

  struct F64Case {
    uint64_t Input, Ceil, Floor, Trunc, Even;
  };
  const std::array<F64Case, 12> F64Cases{{
      {0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
       0x0000000000000000, 0x0000000000000000},
      {0x8000000000000000, 0x8000000000000000, 0x8000000000000000,
       0x8000000000000000, 0x8000000000000000},
      {0x0000000000000001, 0x3ff0000000000000, 0x0000000000000000,
       0x0000000000000000, 0x0000000000000000},
      {0x8000000000000001, 0x8000000000000000, 0xbff0000000000000,
       0x8000000000000000, 0x8000000000000000},
      {0x3fe0000000000000, 0x3ff0000000000000, 0x0000000000000000,
       0x0000000000000000, 0x0000000000000000},
      {0x3ff8000000000000, 0x4000000000000000, 0x3ff0000000000000,
       0x3ff0000000000000, 0x4000000000000000},
      {0x4004000000000000, 0x4008000000000000, 0x4000000000000000,
       0x4000000000000000, 0x4000000000000000},
      {0xc004000000000000, 0xc000000000000000, 0xc008000000000000,
       0xc000000000000000, 0xc000000000000000},
      {0x4330000000000000, 0x4330000000000000, 0x4330000000000000,
       0x4330000000000000, 0x4330000000000000},
      {0x7ff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000,
       0x7ff0000000000000, 0x7ff0000000000000},
      {0xfff0000000000000, 0xfff0000000000000, 0xfff0000000000000,
       0xfff0000000000000, 0xfff0000000000000},
      {0x7ff0123456789abc, 0x7ff8123456789abc, 0x7ff8123456789abc,
       0x7ff8123456789abc, 0x7ff8123456789abc},
  }};
  for (const auto &Case : F64Cases) {
    EXPECT_EQ(Ceil(Case.Input), Case.Ceil);
    EXPECT_EQ(Floor(Case.Input), Case.Floor);
    EXPECT_EQ(Trunc(Case.Input), Case.Trunc);
    EXPECT_EQ(Even(Case.Input), Case.Even);
  }
}

TEST(ARMRuntimeLibcallsCoreTest, RoundingHelpersQuietSignalingNaNs) {
  auto JIT = makeRuntimeLibcallExecutionEngine();
  ASSERT_NE(JIT.Engine, nullptr);
  using F32Core = uint32_t (*)(uint32_t);
  using F64Core = uint64_t (*)(uint64_t);
  for (const char *Name : {"ceil", "floor", "trunc", "roundeven"}) {
    const auto Function32 =
        reinterpret_cast<F32Core>(JIT.Engine->getFunctionAddress(
            (std::string("__wasmedge_") + Name + "f_core").c_str()));
    const auto Function64 =
        reinterpret_cast<F64Core>(JIT.Engine->getFunctionAddress(
            (std::string("__wasmedge_") + Name + "_core").c_str()));
    ASSERT_NE(Function32, nullptr) << Name;
    ASSERT_NE(Function64, nullptr) << Name;

    EXPECT_EQ(Function32(0x7f812345), 0x7fc12345U) << Name;
    EXPECT_EQ(Function32(0xff823456), 0xffc23456U) << Name;
    EXPECT_EQ(Function32(0x7fc34567), 0x7fc34567U) << Name;
    EXPECT_EQ(Function32(0xffc45678), 0xffc45678U) << Name;
    EXPECT_EQ(Function64(0x7ff0123456789abc), 0x7ff8123456789abcULL) << Name;
    EXPECT_EQ(Function64(0xfff023456789abcd), 0xfff823456789abcdULL) << Name;
    EXPECT_EQ(Function64(0x7ff83456789abcde), 0x7ff83456789abcdeULL) << Name;
    EXPECT_EQ(Function64(0xfff8456789abcdef), 0xfff8456789abcdefULL) << Name;
  }
}

TEST(ARMRuntimeLibcallsCoreTest, ImplementsLLVMMinNumMaxNumBitSemantics) {
  auto JIT = makeRuntimeLibcallExecutionEngine();
  ASSERT_NE(JIT.Engine, nullptr);
  using F32Core = uint32_t (*)(uint32_t, uint32_t);
  using F64Core = uint64_t (*)(uint64_t, uint64_t);
  const auto MinF = reinterpret_cast<F32Core>(
      JIT.Engine->getFunctionAddress("__wasmedge_fminf_core"));
  const auto MaxF = reinterpret_cast<F32Core>(
      JIT.Engine->getFunctionAddress("__wasmedge_fmaxf_core"));
  const auto Min = reinterpret_cast<F64Core>(
      JIT.Engine->getFunctionAddress("__wasmedge_fmin_core"));
  const auto Max = reinterpret_cast<F64Core>(
      JIT.Engine->getFunctionAddress("__wasmedge_fmax_core"));
  ASSERT_NE(MinF, nullptr);
  ASSERT_NE(MaxF, nullptr);
  ASSERT_NE(Min, nullptr);
  ASSERT_NE(Max, nullptr);

  EXPECT_EQ(MinF(0x00000000, 0x80000000), 0x80000000U);
  EXPECT_EQ(MinF(0x80000000, 0x00000000), 0x80000000U);
  EXPECT_EQ(MaxF(0x00000000, 0x80000000), 0x00000000U);
  EXPECT_EQ(MaxF(0x80000000, 0x00000000), 0x00000000U);
  EXPECT_EQ(MinF(0x7fc12345, 0x3f800000), 0x3f800000U);
  EXPECT_EQ(MaxF(0x3f800000, 0xffc54321), 0x3f800000U);
  EXPECT_EQ(MinF(0x7f812345, 0x3f800000), 0x7fc12345U);
  EXPECT_EQ(MaxF(0x3f800000, 0xff854321), 0xffc54321U);
  EXPECT_EQ(MinF(0x7fc12345, 0xffc54321), 0xffc54321U);
  EXPECT_EQ(MaxF(0x7fc12345, 0xffc54321), 0xffc54321U);
  EXPECT_EQ(MinF(0xbf800000, 0x3f800000), 0xbf800000U);
  EXPECT_EQ(MaxF(0xbf800000, 0x3f800000), 0x3f800000U);

  EXPECT_EQ(Min(0x0000000000000000, 0x8000000000000000), 0x8000000000000000ULL);
  EXPECT_EQ(Max(0x8000000000000000, 0x0000000000000000), 0x0000000000000000ULL);
  EXPECT_EQ(Min(0x7ff8000000001234, 0x3ff0000000000000), 0x3ff0000000000000ULL);
  EXPECT_EQ(Max(0x3ff0000000000000, 0xfff0000000005678), 0xfff8000000005678ULL);
  EXPECT_EQ(Min(0x7ff8000000001234, 0xfff8000000005678), 0xfff8000000005678ULL);
  EXPECT_EQ(Min(0xfff0000000000000, 0x7ff0000000000000), 0xfff0000000000000ULL);
  EXPECT_EQ(Max(0xfff0000000000000, 0x7ff0000000000000), 0x7ff0000000000000ULL);
}

TEST(ARMRuntimeLibcallsCoreTest, ConvertsIntegersToIEEEBits) {
  auto JIT = makeRuntimeLibcallExecutionEngine();
  ASSERT_NE(JIT.Engine, nullptr);
  using SignedToF32 = uint32_t (*)(int64_t);
  using UnsignedToF32 = uint32_t (*)(uint64_t);
  using SignedToF64 = uint64_t (*)(int64_t);
  using UnsignedToF64 = uint64_t (*)(uint64_t);
  const auto L2F = reinterpret_cast<SignedToF32>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_l2f_core"));
  const auto UL2F = reinterpret_cast<UnsignedToF32>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_ul2f_core"));
  const auto L2D = reinterpret_cast<SignedToF64>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_l2d_core"));
  const auto UL2D = reinterpret_cast<UnsignedToF64>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_ul2d_core"));
  ASSERT_NE(L2F, nullptr);
  ASSERT_NE(UL2F, nullptr);
  ASSERT_NE(L2D, nullptr);
  ASSERT_NE(UL2D, nullptr);

  const std::array<int64_t, 15> SignedValues{0,
                                             1,
                                             -1,
                                             INT64_MIN,
                                             INT64_MAX,
                                             INT64_C(1) << 24,
                                             (INT64_C(1) << 24) + 1,
                                             (INT64_C(1) << 24) + 3,
                                             -((INT64_C(1) << 24) + 1),
                                             INT64_C(1) << 53,
                                             (INT64_C(1) << 53) + 1,
                                             (INT64_C(1) << 53) + 3,
                                             (INT64_C(1) << 54) - 1,
                                             -((INT64_C(1) << 53) + 1),
                                             -((INT64_C(1) << 53) + 3)};
  for (const int64_t Value : SignedValues) {
    EXPECT_EQ(L2F(Value), bitCopy<uint32_t>(static_cast<float>(Value)))
        << Value;
    EXPECT_EQ(L2D(Value), bitCopy<uint64_t>(static_cast<double>(Value)))
        << Value;
  }

  const std::array<uint64_t, 12> UnsignedValues{0,
                                                1,
                                                UINT64_MAX,
                                                UINT64_C(1) << 24,
                                                (UINT64_C(1) << 24) + 1,
                                                (UINT64_C(1) << 24) + 3,
                                                UINT64_C(1) << 53,
                                                (UINT64_C(1) << 53) + 1,
                                                (UINT64_C(1) << 53) + 3,
                                                (UINT64_C(1) << 54) - 1,
                                                UINT64_C(1) << 63,
                                                (UINT64_C(1) << 63) +
                                                    (UINT64_C(1) << 39)};
  for (const uint64_t Value : UnsignedValues) {
    EXPECT_EQ(UL2F(Value), bitCopy<uint32_t>(static_cast<float>(Value)))
        << Value;
    EXPECT_EQ(UL2D(Value), bitCopy<uint64_t>(static_cast<double>(Value)))
        << Value;
  }
}

TEST(ARMRuntimeLibcallsCoreTest, TruncatesRepresentableFPInputsToIntegers) {
  auto JIT = makeRuntimeLibcallExecutionEngine();
  ASSERT_NE(JIT.Engine, nullptr);
  using F32ToSigned = int64_t (*)(uint32_t);
  using F32ToUnsigned = uint64_t (*)(uint32_t);
  using F64ToSigned = int64_t (*)(uint64_t);
  using F64ToUnsigned = uint64_t (*)(uint64_t);
  const auto F2L = reinterpret_cast<F32ToSigned>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_f2lz_core"));
  const auto F2UL = reinterpret_cast<F32ToUnsigned>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_f2ulz_core"));
  const auto D2L = reinterpret_cast<F64ToSigned>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_d2lz_core"));
  const auto D2UL = reinterpret_cast<F64ToUnsigned>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_d2ulz_core"));
  ASSERT_NE(F2L, nullptr);
  ASSERT_NE(F2UL, nullptr);
  ASSERT_NE(D2L, nullptr);
  ASSERT_NE(D2UL, nullptr);

  const std::array<float, 17> SignedF32{
      -0x1p63F,
      bitCopy<float>(UINT32_C(0xdeffffff)),
      -0x1p24F,
      -1.75F,
      -1.0F,
      -std::numeric_limits<float>::denorm_min(),
      -0.0F,
      0.0F,
      std::numeric_limits<float>::denorm_min(),
      0.75F,
      1.0F,
      1.75F,
      0x1.fffffep23F,
      0x1p24F,
      0x1.000002p24F,
      0x1p53F,
      bitCopy<float>(UINT32_C(0x5effffff))};
  for (const float Value : SignedF32)
    EXPECT_EQ(F2L(bitCopy<uint32_t>(Value)), static_cast<int64_t>(Value))
        << Value;

  const std::array<float, 12> UnsignedF32{
      -0.0F,          0.0F,    std::numeric_limits<float>::denorm_min(),
      0.75F,          1.0F,    1.75F,
      0x1.fffffep23F, 0x1p24F, 0x1.000002p24F,
      0x1p53F,        0x1p63F, bitCopy<float>(UINT32_C(0x5f7fffff))};
  for (const float Value : UnsignedF32)
    EXPECT_EQ(F2UL(bitCopy<uint32_t>(Value)), static_cast<uint64_t>(Value))
        << Value;

  const std::array<double, 18> SignedF64{
      -0x1p63,
      bitCopy<double>(UINT64_C(0xc3dfffffffffffff)),
      -0x1p53,
      -0x1p24,
      -1.75,
      -1.0,
      -std::numeric_limits<double>::denorm_min(),
      -0.0,
      0.0,
      std::numeric_limits<double>::denorm_min(),
      0.75,
      1.0,
      1.75,
      0x1.fffffep23,
      0x1p24,
      0x1.fffffffffffffp52,
      0x1p53,
      bitCopy<double>(UINT64_C(0x43dfffffffffffff))};
  for (const double Value : SignedF64)
    EXPECT_EQ(D2L(bitCopy<uint64_t>(Value)), static_cast<int64_t>(Value))
        << Value;

  const std::array<double, 14> UnsignedF64{
      -0.0,
      0.0,
      std::numeric_limits<double>::denorm_min(),
      0.75,
      1.0,
      1.75,
      0x1.fffffep23,
      0x1p24,
      0x1.fffffffffffffp52,
      0x1p53,
      0x1.0000000000001p53,
      0x1p63,
      bitCopy<double>(UINT64_C(0x43dfffffffffffff)),
      bitCopy<double>(UINT64_C(0x43efffffffffffff))};
  for (const double Value : UnsignedF64)
    EXPECT_EQ(D2UL(bitCopy<uint64_t>(Value)), static_cast<uint64_t>(Value))
        << Value;
}

TEST(ARMRuntimeLibcallsCoreTest, ComputesUnsignedAndSignedDivision) {
  auto JIT = makeRuntimeLibcallExecutionEngine();
  ASSERT_NE(JIT.Engine, nullptr);
  using NarrowCore = void (*)(uint32_t, uint32_t, DivModResult<uint32_t> *);
  using NarrowSignedCore = void (*)(int32_t, int32_t, DivModResult<int32_t> *);
  using WideCore = void (*)(uint64_t, uint64_t, uint64_t *, uint64_t *);
  using WideSignedCore = void (*)(int64_t, int64_t, int64_t *, int64_t *);
  const auto Unsigned32 = reinterpret_cast<NarrowCore>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_uidivmod_core"));
  const auto Signed32 = reinterpret_cast<NarrowSignedCore>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_idivmod_core"));
  const auto Unsigned64 = reinterpret_cast<WideCore>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_uldivmod_core"));
  const auto Signed64 = reinterpret_cast<WideSignedCore>(
      JIT.Engine->getFunctionAddress("__wasmedge_aeabi_ldivmod_core"));
  ASSERT_NE(Unsigned32, nullptr);
  ASSERT_NE(Signed32, nullptr);
  ASSERT_NE(Unsigned64, nullptr);
  ASSERT_NE(Signed64, nullptr);

  const std::array<uint64_t, 13> Values{0,
                                        1,
                                        2,
                                        3,
                                        7,
                                        8,
                                        0x55555555ULL,
                                        0xaaaaaaaaULL,
                                        UINT32_MAX,
                                        UINT64_C(1) << 32,
                                        UINT64_C(1) << 63,
                                        UINT64_MAX - 1,
                                        UINT64_MAX};
  for (const uint64_t Left : Values)
    for (const uint64_t Right : Values) {
      if (Right == 0)
        continue;
      if (static_cast<uint32_t>(Right) != 0) {
        DivModResult<uint32_t> U32{};
        Unsigned32(static_cast<uint32_t>(Left), static_cast<uint32_t>(Right),
                   &U32);
        const auto ExpectedU32 = referenceUnsignedDivMod(
            static_cast<uint32_t>(Left), static_cast<uint32_t>(Right));
        EXPECT_EQ(U32.Quotient, ExpectedU32.Quotient);
        EXPECT_EQ(U32.Remainder, ExpectedU32.Remainder);
      }

      uint64_t U64Quotient = 0, U64Remainder = 0;
      Unsigned64(Left, Right, &U64Quotient, &U64Remainder);
      const auto ExpectedU64 = referenceUnsignedDivMod(Left, Right);
      EXPECT_EQ(U64Quotient, ExpectedU64.Quotient);
      EXPECT_EQ(U64Remainder, ExpectedU64.Remainder);
    }

  const std::array<int64_t, 12> SignedValues{0,
                                             1,
                                             -1,
                                             2,
                                             -2,
                                             7,
                                             -7,
                                             INT32_MIN,
                                             INT32_MAX,
                                             INT64_MIN,
                                             INT64_MIN + 1,
                                             INT64_MAX};
  for (const int64_t Left : SignedValues)
    for (const int64_t Right : SignedValues) {
      if (Right == 0)
        continue;
      if (static_cast<int32_t>(Right) != 0) {
        DivModResult<int32_t> I32{};
        Signed32(static_cast<int32_t>(Left), static_cast<int32_t>(Right), &I32);
        const auto ExpectedI32 = referenceSignedDivMod(
            static_cast<int32_t>(Left), static_cast<int32_t>(Right));
        EXPECT_EQ(I32.Quotient, ExpectedI32.Quotient);
        EXPECT_EQ(I32.Remainder, ExpectedI32.Remainder);
      }

      int64_t I64Quotient = 0, I64Remainder = 0;
      Signed64(Left, Right, &I64Quotient, &I64Remainder);
      const auto ExpectedI64 = referenceSignedDivMod(Left, Right);
      EXPECT_EQ(I64Quotient, ExpectedI64.Quotient);
      EXPECT_EQ(I64Remainder, ExpectedI64.Remainder);
    }

  for (uint32_t Left = 0; Left < 256; ++Left)
    for (uint32_t Right = 1; Right < 256; ++Right) {
      DivModResult<uint32_t> Result{};
      Unsigned32(Left, Right, &Result);
      const auto Expected = referenceUnsignedDivMod(Left, Right);
      ASSERT_EQ(Result.Quotient, Expected.Quotient);
      ASSERT_EQ(Result.Remainder, Expected.Remainder);
    }
  for (int32_t Left = -128; Left < 128; ++Left)
    for (int32_t Right = -128; Right < 128; ++Right) {
      if (Right == 0)
        continue;
      DivModResult<int32_t> Result{};
      Signed32(Left, Right, &Result);
      const auto Expected = referenceSignedDivMod(Left, Right);
      ASSERT_EQ(Result.Quotient, Expected.Quotient);
      ASSERT_EQ(Result.Remainder, Expected.Remainder);
    }

  std::mt19937_64 Generator(0x5741534d45444745ULL);
  for (unsigned I = 0; I < 4096; ++I) {
    const uint64_t Left = Generator();
    const uint64_t Right = Generator();
    if (Right == 0)
      continue;
    uint64_t Quotient = 0, Remainder = 0;
    Unsigned64(Left, Right, &Quotient, &Remainder);
    const auto Expected = referenceUnsignedDivMod(Left, Right);
    ASSERT_EQ(Quotient, Expected.Quotient);
    ASSERT_EQ(Remainder, Expected.Remainder);
  }
  for (unsigned I = 0; I < 4096; ++I) {
    const int64_t Left = static_cast<int64_t>(Generator());
    const int64_t Right = static_cast<int64_t>(Generator());
    if (Right == 0)
      continue;
    int64_t Quotient = 0, Remainder = 0;
    Signed64(Left, Right, &Quotient, &Remainder);
    const auto Expected = referenceSignedDivMod(Left, Right);
    ASSERT_EQ(Quotient, Expected.Quotient);
    ASSERT_EQ(Remainder, Expected.Remainder);
  }
}

TEST(ARMRuntimeLibcallsCoreTest, DefinesDefaultRTABIDivideByZeroHooks) {
  auto JIT = makeRuntimeLibcallExecutionEngine();
  ASSERT_NE(JIT.Engine, nullptr);
  using NarrowHook = int32_t (*)(int32_t);
  using WideHook = int64_t (*)(int64_t);
  const auto Narrow = reinterpret_cast<NarrowHook>(
      JIT.Engine->getFunctionAddress("__aeabi_idiv0"));
  const auto Wide = reinterpret_cast<WideHook>(
      JIT.Engine->getFunctionAddress("__aeabi_ldiv0"));
  ASSERT_NE(Narrow, nullptr);
  ASSERT_NE(Wide, nullptr);
  for (const int32_t Value : {INT32_MIN, -1, 0, 1, INT32_MAX})
    EXPECT_EQ(Narrow(Value), Value);
  for (const int64_t Value :
       {INT64_MIN, INT64_C(-1), INT64_C(0), INT64_C(1), INT64_MAX})
    EXPECT_EQ(Wide(Value), Value);
}

TEST(ARMRuntimeLibcallsProfileTest, AddsDivisionWrapperAndCoreShapes) {
  for (const ARMFloatABI FloatABI : {ARMFloatABI::Hard, ARMFloatABI::SoftFP}) {
    const ARMRuntimeLibcallProfile Profile{true, true, true,
                                           true, true, FloatABI};
    EXPECT_TRUE(WasmEdge::LLVM::supportsARMRuntimeLibcalls(Profile));

    auto Context = WasmEdge::LLVM::Context::create();
    WasmEdge::LLVM::Module Module(Context, "arm-runtime-libcalls-profile");
    EXPECT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(Module, Profile));
    auto *LLModule = reinterpret_cast<llvm::Module *>(Module.unwrap());
    for (const auto &[Name, Width] :
         std::array<std::pair<const char *, unsigned>, 4>{{
             {"__wasmedge_aeabi_idivmod_core", 32},
             {"__wasmedge_aeabi_uidivmod_core", 32},
             {"__wasmedge_aeabi_ldivmod_core", 64},
             {"__wasmedge_aeabi_uldivmod_core", 64},
         }}) {
      auto *Core = LLModule->getFunction(Name);
      ASSERT_NE(Core, nullptr) << Name;
      EXPECT_EQ(Core->getReturnType(),
                llvm::Type::getVoidTy(LLModule->getContext()));
      EXPECT_EQ(Core->arg_size(), Width == 32 ? 3U : 4U);
      EXPECT_TRUE(Core->getArg(0)->getType()->isIntegerTy(Width));
      EXPECT_TRUE(Core->getArg(1)->getType()->isIntegerTy(Width));
      EXPECT_TRUE(Core->getArg(2)->getType()->isPointerTy());
      EXPECT_EQ(Core->getVisibility(), llvm::GlobalValue::HiddenVisibility);
      EXPECT_TRUE(Core->isDSOLocal());
      EXPECT_TRUE(Core->hasFnAttribute(llvm::Attribute::NoUnwind));
      EXPECT_FALSE(Core->hasFnAttribute(llvm::Attribute::UWTable));
    }
    EXPECT_NE(moduleIR(Module).find("@llvm.compiler.used"), std::string::npos);
  }
}

TEST(ARMRuntimeLibcallsProfileTest, ClassifiesHostCompileTimeABI) {
  const auto Profile = WasmEdge::LLVM::hostARMRuntimeLibcallProfile();
#if defined(__arm__)
  EXPECT_TRUE(Profile.ARM32);
#else
  EXPECT_FALSE(Profile.ARM32);
#endif
#if defined(__linux__)
  EXPECT_TRUE(Profile.Linux);
#else
  EXPECT_FALSE(Profile.Linux);
#endif
#if defined(__ARM_EABI__)
  EXPECT_TRUE(Profile.EABI);
#else
  EXPECT_FALSE(Profile.EABI);
#endif
#if defined(__ARM_ARCH) && __ARM_ARCH >= 7
  EXPECT_TRUE(Profile.ARMv7OrLater);
#else
  EXPECT_FALSE(Profile.ARMv7OrLater);
#endif
#if defined(__ARM_FP) && __ARM_FP != 0
  EXPECT_TRUE(Profile.VFP);
#else
  EXPECT_FALSE(Profile.VFP);
#endif
#if defined(__ARM_PCS_VFP)
  EXPECT_EQ(Profile.FloatABI, ARMFloatABI::Hard);
#elif defined(__ARM_FP) && __ARM_FP != 0
  EXPECT_EQ(Profile.FloatABI, ARMFloatABI::SoftFP);
#else
  EXPECT_EQ(Profile.FloatABI, ARMFloatABI::PureSoft);
#endif
}

TEST(ARMRuntimeLibcallsProfileTest, DefersPureSoftRejectionUntilAOT) {
  const ARMRuntimeLibcallProfile Profile{true, true,  true,
                                         true, false, ARMFloatABI::PureSoft};
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  std::string Message;
  {
    ScopedLoggingCallback Capture(Message);
    auto Context = WasmEdge::LLVM::Context::create();
    WasmEdge::LLVM::Module Module(Context, "arm-runtime-libcalls-profile");
    const std::string Before = moduleIR(Module);
    const auto HelperResult =
        WasmEdge::LLVM::addARMRuntimeLibcalls(Module, Profile);
    const auto AOTResult =
        WasmEdge::LLVM::validateARMRuntimeLibcallsForAOT(Profile);

    EXPECT_TRUE(HelperResult);
    EXPECT_EQ(moduleIR(Module), Before);
    ASSERT_FALSE(AOTResult);
    EXPECT_EQ(AOTResult.error(), WasmEdge::ErrCode::Value::InvalidAOTConfigure);
    EXPECT_EQ(Message,
              "ARM pure-soft float ABI is unsupported for AOT compilation.");
  }
  EXPECT_EQ(spdlog::default_logger(), OriginalLogger);
  EXPECT_EQ(spdlog::get_level(), OriginalLevel);
}

TEST(ARMRuntimeLibcallsProfileTest, IgnoresInapplicableProfiles) {
  const std::array<ARMRuntimeLibcallProfile, 4> Profiles{
      ARMRuntimeLibcallProfile{false, true, true, true, true,
                               ARMFloatABI::Hard},
      ARMRuntimeLibcallProfile{true, false, true, true, true,
                               ARMFloatABI::Hard},
      ARMRuntimeLibcallProfile{true, true, false, true, true,
                               ARMFloatABI::Hard},
      ARMRuntimeLibcallProfile{true, true, true, false, true,
                               ARMFloatABI::Hard}};

  for (const auto &Profile : Profiles) {
    EXPECT_FALSE(WasmEdge::LLVM::supportsARMRuntimeLibcalls(Profile));
    auto Context = WasmEdge::LLVM::Context::create();
    WasmEdge::LLVM::Module Module(Context, "arm-runtime-libcalls-profile");
    const std::string Before = moduleIR(Module);
    EXPECT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(Module, Profile));
    EXPECT_EQ(moduleIR(Module), Before);
  }
}

enum class ARMOptimization { O0, O1, O2, O3, Os, Oz };

struct ARMObjectConfig {
  std::string Triple;
  std::string CPU;
  std::string Features;
  bool HardFloat;
  bool Thumb;
  ARMOptimization Optimization;
};

#if LLVM_VERSION_MAJOR < 13
unsigned optimizationLevel(ARMOptimization Optimization) {
  switch (Optimization) {
  case ARMOptimization::O0:
    return 0;
  case ARMOptimization::O1:
    return 1;
  case ARMOptimization::O2:
  case ARMOptimization::Os:
  case ARMOptimization::Oz:
    return 2;
  case ARMOptimization::O3:
    return 3;
  }
  return 0;
}
#endif

const char *optimizationName(ARMOptimization Optimization) {
  switch (Optimization) {
  case ARMOptimization::O0:
    return "O0";
  case ARMOptimization::O1:
    return "O1";
  case ARMOptimization::O2:
    return "O2";
  case ARMOptimization::O3:
    return "O3";
  case ARMOptimization::Os:
    return "Os";
  case ARMOptimization::Oz:
    return "Oz";
  }
  return "unknown";
}

bool containsARMCallInstruction(llvm::StringRef Code, bool Thumb) {
  const auto U8 = [&Code](size_t Offset) {
    return static_cast<uint8_t>(Code[Offset]);
  };
  if (!Thumb) {
    for (size_t Offset = 0; Offset + 4 <= Code.size(); Offset += 4) {
      const uint32_t Word = U8(Offset) | (U8(Offset + 1) << 8) |
                            (U8(Offset + 2) << 16) | (U8(Offset + 3) << 24);
      if ((Word & UINT32_C(0x0f000000)) == UINT32_C(0x0b000000) ||
          (Word & UINT32_C(0xfe000000)) == UINT32_C(0xfa000000) ||
          (Word & UINT32_C(0x0ffffff0)) == UINT32_C(0x012fff30))
        return true;
    }
    return false;
  }

  for (size_t Offset = 0; Offset + 2 <= Code.size();) {
    const uint16_t First = U8(Offset) | (U8(Offset + 1) << 8);
    if ((First & UINT16_C(0xff87)) == UINT16_C(0x4780))
      return true;
    const bool Wide = (First & UINT16_C(0xe000)) == UINT16_C(0xe000) &&
                      (First & UINT16_C(0x1800)) != 0;
    if (!Wide || Offset + 4 > Code.size()) {
      Offset += 2;
      continue;
    }
    const uint16_t Second = U8(Offset + 2) | (U8(Offset + 3) << 8);
    if ((First & UINT16_C(0xf800)) == UINT16_C(0xf000) &&
        ((Second & UINT16_C(0xd000)) == UINT16_C(0xd000) ||
         (Second & UINT16_C(0xd001)) == UINT16_C(0xc000)))
      return true;
    Offset += 4;
  }
  return false;
}

void initializeLLVMTargets() {
  static const bool Initialized = [] {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    return true;
  }();
  (void)Initialized;
}

const llvm::Target *lookupLLVMTarget(const llvm::Triple &Triple,
                                     std::string &Error) {
  initializeLLVMTargets();
  llvm::Triple LookupTriple = Triple;
  return llvm::TargetRegistry::lookupTarget("", LookupTriple, Error);
}

#define REQUIRE_LLVM_TARGET(TRIPLE)                                            \
  do {                                                                         \
    const llvm::Triple RequiredTriple(TRIPLE);                                 \
    std::string TargetError;                                                   \
    if (lookupLLVMTarget(RequiredTriple, TargetError) == nullptr)              \
      GTEST_SKIP() << RequiredTriple.str() << ": " << TargetError;             \
  } while (false)

llvm::Function *createFunction(llvm::Module &Module, llvm::StringRef Name,
                               llvm::Type *Result,
                               llvm::ArrayRef<llvm::Type *> Parameters) {
  auto *Function =
      llvm::Function::Create(llvm::FunctionType::get(Result, Parameters, false),
                             llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->addFnAttr(llvm::Attribute::NoInline);
  Function->addFnAttr(llvm::Attribute::NoUnwind);
  return Function;
}

void addIntegerOperation(llvm::Module &Module, llvm::StringRef Name,
                         llvm::Type *Type, llvm::Instruction::BinaryOps Op) {
  auto *Function = createFunction(Module, Name, Type, {Type, Type});
  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", Function));
  auto Argument = Function->arg_begin();
  auto *Left = &*Argument++;
  auto *Right = &*Argument;
  Builder.CreateRet(Builder.CreateBinOp(Op, Left, Right));
}

void addConversion(llvm::Module &Module, llvm::StringRef Name,
                   llvm::Type *Source, llvm::Type *Result,
                   llvm::Instruction::CastOps Op) {
  auto *Function = createFunction(Module, Name, Result, {Source});
  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", Function));
  Builder.CreateRet(Builder.CreateCast(Op, Function->getArg(0), Result));
}

void addIntrinsic(llvm::Module &Module, llvm::StringRef Name, llvm::Type *Type,
                  llvm::Intrinsic::ID ID, bool Binary) {
  std::vector<llvm::Type *> Parameters(Binary ? 2 : 1, Type);
  auto *Function = createFunction(Module, Name, Type, Parameters);
#if LLVM_VERSION_MAJOR >= 21
  auto Intrinsic = llvm::Intrinsic::getOrInsertDeclaration(&Module, ID, {Type});
#else
  auto *Intrinsic = llvm::Intrinsic::getDeclaration(&Module, ID, {Type});
#endif
  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", Function));
  if (Binary)
    Builder.CreateRet(Builder.CreateCall(
        Intrinsic, {Function->getArg(0), Function->getArg(1)}));
  else
    Builder.CreateRet(Builder.CreateCall(Intrinsic, {Function->getArg(0)}));
}

void addAtomicOperations(llvm::Module &Module, llvm::Type *Type,
                         llvm::StringRef Suffix) {
#if LLVM_VERSION_MAJOR >= 15
  auto *Pointer = llvm::PointerType::getUnqual(Module.getContext());
#else
  auto *Pointer = llvm::PointerType::getUnqual(Type);
#endif
  const llvm::Align Alignment(
      std::max<uint64_t>(1, Type->getIntegerBitWidth() / 8));
  const std::string NameSuffix = Suffix.str();

  auto *Load =
      createFunction(Module, "atomic_load_" + NameSuffix, Type, {Pointer});
  llvm::IRBuilder<> LoadBuilder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", Load));
  auto *Loaded = LoadBuilder.CreateLoad(Type, Load->getArg(0));
  Loaded->setAtomic(llvm::AtomicOrdering::SequentiallyConsistent);
  Loaded->setAlignment(Alignment);
  LoadBuilder.CreateRet(Loaded);

  auto *Store = createFunction(Module, "atomic_store_" + NameSuffix, Type,
                               {Pointer, Type});
  llvm::IRBuilder<> StoreBuilder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", Store));
  auto *Stored = StoreBuilder.CreateStore(Store->getArg(1), Store->getArg(0));
  Stored->setAtomic(llvm::AtomicOrdering::SequentiallyConsistent);
  Stored->setAlignment(Alignment);
  StoreBuilder.CreateRet(Store->getArg(1));

  auto *RMW =
      createFunction(Module, "atomic_rmw_" + NameSuffix, Type, {Pointer, Type});
  llvm::IRBuilder<> RMWBuilder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", RMW));
  auto *RMWResult = RMWBuilder.CreateAtomicRMW(
      llvm::AtomicRMWInst::Add, RMW->getArg(0), RMW->getArg(1),
#if LLVM_VERSION_MAJOR >= 13
      Alignment,
#endif
      llvm::AtomicOrdering::SequentiallyConsistent);
  RMWResult->setAlignment(Alignment);
  RMWBuilder.CreateRet(RMWResult);

  auto *CmpXchg = createFunction(Module, "atomic_cmpxchg_" + NameSuffix, Type,
                                 {Pointer, Type, Type});
  llvm::IRBuilder<> CmpXchgBuilder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", CmpXchg));
  auto *Pair = CmpXchgBuilder.CreateAtomicCmpXchg(
      CmpXchg->getArg(0), CmpXchg->getArg(1), CmpXchg->getArg(2),
#if LLVM_VERSION_MAJOR >= 13
      Alignment,
#endif
      llvm::AtomicOrdering::SequentiallyConsistent,
      llvm::AtomicOrdering::SequentiallyConsistent);
  Pair->setAlignment(Alignment);
  CmpXchgBuilder.CreateRet(CmpXchgBuilder.CreateExtractValue(Pair, 0));
}

void addNumericOperations(llvm::Module &Module) {
  llvm::LLVMContext &Context = Module.getContext();
  const std::array<llvm::Type *, 2> Integers{llvm::Type::getInt32Ty(Context),
                                             llvm::Type::getInt64Ty(Context)};
  const std::array<llvm::Instruction::BinaryOps, 4> IntegerOps{
      llvm::Instruction::SDiv, llvm::Instruction::UDiv, llvm::Instruction::SRem,
      llvm::Instruction::URem};
  const std::array<const char *, 4> IntegerNames{"sdiv", "udiv", "srem",
                                                 "urem"};
  for (size_t Width = 0; Width < Integers.size(); ++Width)
    for (size_t Op = 0; Op < IntegerOps.size(); ++Op)
      addIntegerOperation(Module, IntegerNames[Op] + std::to_string(Width + 1),
                          Integers[Width], IntegerOps[Op]);

  auto *I64 = llvm::Type::getInt64Ty(Context);
  const std::array<llvm::Type *, 2> Floats{llvm::Type::getFloatTy(Context),
                                           llvm::Type::getDoubleTy(Context)};
  for (size_t Width = 0; Width < Floats.size(); ++Width) {
    auto *Float = Floats[Width];
    const std::string Suffix = std::to_string(Width + 1);
    addConversion(Module, "sitofp" + Suffix, I64, Float,
                  llvm::Instruction::SIToFP);
    addConversion(Module, "uitofp" + Suffix, I64, Float,
                  llvm::Instruction::UIToFP);
    addConversion(Module, "fptosi" + Suffix, Float, I64,
                  llvm::Instruction::FPToSI);
    addConversion(Module, "fptoui" + Suffix, Float, I64,
                  llvm::Instruction::FPToUI);
    addIntrinsic(Module, "ceil" + Suffix, Float, llvm::Intrinsic::ceil, false);
    addIntrinsic(Module, "floor" + Suffix, Float, llvm::Intrinsic::floor,
                 false);
    addIntrinsic(Module, "trunc" + Suffix, Float, llvm::Intrinsic::trunc,
                 false);
    addIntrinsic(Module, "roundeven" + Suffix, Float,
                 llvm::Intrinsic::roundeven, false);
    addIntrinsic(Module, "minnum" + Suffix, Float, llvm::Intrinsic::minnum,
                 true);
    addIntrinsic(Module, "maxnum" + Suffix, Float, llvm::Intrinsic::maxnum,
                 true);
    addIntrinsic(Module, "sqrt" + Suffix, Float, llvm::Intrinsic::sqrt, false);
  }

  const std::array<unsigned, 4> AtomicWidths{8, 16, 32, 64};
  for (const unsigned Width : AtomicWidths)
    addAtomicOperations(Module, llvm::Type::getIntNTy(Context, Width),
                        std::to_string(Width));
}

std::vector<Byte> makeARMNumericObject(const ARMObjectConfig &Config) {
  const llvm::Triple Triple(Config.Triple);
  std::string Error;
  const llvm::Target *Target = lookupLLVMTarget(Triple, Error);
  EXPECT_NE(Target, nullptr) << Error;
  if (Target == nullptr)
    return {};

  std::string Features = Config.Features;
  if (Config.Thumb)
    Features += Features.empty() ? "+thumb-mode" : ",+thumb-mode";
#if LLVM_VERSION_MAJOR == 18
  auto *MachineOptions = LLVMCreateTargetMachineOptions();
  LLVMTargetMachineOptionsSetCPU(MachineOptions, Config.CPU.c_str());
  LLVMTargetMachineOptionsSetFeatures(MachineOptions, Features.c_str());
  if (Config.HardFloat)
    LLVMTargetMachineOptionsSetABI(MachineOptions, "aapcs-vfp");
  LLVMTargetMachineOptionsSetCodeGenOptLevel(
      MachineOptions,
      Config.Optimization == ARMOptimization::O0   ? LLVMCodeGenLevelNone
      : Config.Optimization == ARMOptimization::O1 ? LLVMCodeGenLevelLess
      : Config.Optimization == ARMOptimization::O3 ? LLVMCodeGenLevelAggressive
                                                   : LLVMCodeGenLevelDefault);
  LLVMTargetMachineOptionsSetRelocMode(MachineOptions, LLVMRelocPIC);
  auto *MachineRef = LLVMCreateTargetMachineWithOptions(
      reinterpret_cast<LLVMTargetRef>(const_cast<llvm::Target *>(Target)),
      Config.Triple.c_str(), MachineOptions);
  LLVMDisposeTargetMachineOptions(MachineOptions);
  std::unique_ptr<llvm::TargetMachine> Machine(
      reinterpret_cast<llvm::TargetMachine *>(MachineRef));
#else
  llvm::TargetOptions Options;
  if (Config.HardFloat) {
    Options.FloatABIType = llvm::FloatABI::Hard;
    Options.MCOptions.ABIName = "aapcs-vfp";
  }
  std::unique_ptr<llvm::TargetMachine> Machine(Target->createTargetMachine(
#if LLVM_VERSION_MAJOR >= 21
      Triple,
#else
      Triple.str(),
#endif
      Config.CPU, Features, Options, llvm::Reloc::PIC_));
#endif
  EXPECT_NE(Machine, nullptr);
  if (Machine == nullptr)
    return {};
#if LLVM_VERSION_MAJOR >= 19
  Machine->setOptLevel(
      Config.Optimization == ARMOptimization::O0   ? llvm::CodeGenOptLevel::None
      : Config.Optimization == ARMOptimization::O1 ? llvm::CodeGenOptLevel::Less
      : Config.Optimization == ARMOptimization::O3
          ? llvm::CodeGenOptLevel::Aggressive
          : llvm::CodeGenOptLevel::Default);
#elif LLVM_VERSION_MAJOR < 18
  Machine->setOptLevel(
      Config.Optimization == ARMOptimization::O0   ? llvm::CodeGenOpt::None
      : Config.Optimization == ARMOptimization::O1 ? llvm::CodeGenOpt::Less
      : Config.Optimization == ARMOptimization::O3
          ? llvm::CodeGenOpt::Aggressive
          : llvm::CodeGenOpt::Default);
#endif

  llvm::LLVMContext Context;
  llvm::Module Module("arm-runtime-libcalls-inventory", Context);
#if LLVM_VERSION_MAJOR >= 21
  Module.setTargetTriple(Triple);
#else
  Module.setTargetTriple(Triple.str());
#endif
  Module.setDataLayout(Machine->createDataLayout());
  addNumericOperations(Module);

  llvm::SmallVector<char, 0> Storage;
  llvm::raw_svector_ostream Stream(Storage);
  llvm::legacy::PassManager Passes;
#if LLVM_VERSION_MAJOR >= 18
  const auto FileType = llvm::CodeGenFileType::ObjectFile;
#else
  const auto FileType = llvm::CGFT_ObjectFile;
#endif
  EXPECT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  return std::vector<Byte>(Storage.begin(), Storage.end());
}

std::vector<Byte> makeARMRuntimeObject(const ARMObjectConfig &Config,
                                       bool MixedCaller = false,
                                       bool NumericCallers = false,
                                       bool SemanticSymbols = false) {
  const llvm::Triple Triple(Config.Triple);
  std::string Error;
  const llvm::Target *Target = lookupLLVMTarget(Triple, Error);
  EXPECT_NE(Target, nullptr) << Error;
  if (Target == nullptr)
    return {};

  std::string Features = Config.Features;
  if (Config.Thumb)
    Features += Features.empty() ? "+thumb-mode" : ",+thumb-mode";
  llvm::TargetOptions Options;
  if (Config.HardFloat) {
    Options.FloatABIType = llvm::FloatABI::Hard;
    Options.MCOptions.ABIName = "aapcs-vfp";
  }
#if LLVM_VERSION_MAJOR == 18
  auto *MachineOptions = LLVMCreateTargetMachineOptions();
  LLVMTargetMachineOptionsSetCPU(MachineOptions, Config.CPU.c_str());
  LLVMTargetMachineOptionsSetFeatures(MachineOptions, Features.c_str());
  if (Config.HardFloat)
    LLVMTargetMachineOptionsSetABI(MachineOptions, "aapcs-vfp");
  LLVMTargetMachineOptionsSetCodeGenOptLevel(
      MachineOptions,
      Config.Optimization == ARMOptimization::O0   ? LLVMCodeGenLevelNone
      : Config.Optimization == ARMOptimization::O1 ? LLVMCodeGenLevelLess
      : Config.Optimization == ARMOptimization::O3 ? LLVMCodeGenLevelAggressive
                                                   : LLVMCodeGenLevelDefault);
  LLVMTargetMachineOptionsSetRelocMode(MachineOptions, LLVMRelocPIC);
  auto *MachineRef = LLVMCreateTargetMachineWithOptions(
      reinterpret_cast<LLVMTargetRef>(const_cast<llvm::Target *>(Target)),
      Config.Triple.c_str(), MachineOptions);
  LLVMDisposeTargetMachineOptions(MachineOptions);
  std::unique_ptr<llvm::TargetMachine> Machine(
      reinterpret_cast<llvm::TargetMachine *>(MachineRef));
#else
  std::unique_ptr<llvm::TargetMachine> Machine(Target->createTargetMachine(
#if LLVM_VERSION_MAJOR >= 21
      Triple,
#else
      Triple.str(),
#endif
      Config.CPU, Features, Options, llvm::Reloc::PIC_));
#endif
  EXPECT_NE(Machine, nullptr);
  if (Machine == nullptr)
    return {};
#if LLVM_VERSION_MAJOR >= 19
  Machine->setOptLevel(
      Config.Optimization == ARMOptimization::O0   ? llvm::CodeGenOptLevel::None
      : Config.Optimization == ARMOptimization::O1 ? llvm::CodeGenOptLevel::Less
      : Config.Optimization == ARMOptimization::O3
          ? llvm::CodeGenOptLevel::Aggressive
          : llvm::CodeGenOptLevel::Default);
#elif LLVM_VERSION_MAJOR < 18
  Machine->setOptLevel(
      Config.Optimization == ARMOptimization::O0   ? llvm::CodeGenOpt::None
      : Config.Optimization == ARMOptimization::O1 ? llvm::CodeGenOpt::Less
      : Config.Optimization == ARMOptimization::O3
          ? llvm::CodeGenOpt::Aggressive
          : llvm::CodeGenOpt::Default);
#endif

  llvm::LLVMContext Context;
  llvm::Module Module("arm-runtime-libcalls", Context);
  const std::string ModuleTriple =
      Config.Thumb ? "thumbv7-unknown-linux-gnueabi" : Config.Triple;
#if LLVM_VERSION_MAJOR >= 21
  Module.setTargetTriple(llvm::Triple(ModuleTriple));
#else
  Module.setTargetTriple(ModuleTriple);
#endif
  Module.setDataLayout(Machine->createDataLayout());
  if (NumericCallers)
    addNumericOperations(Module);
  if (SemanticSymbols) {
    auto *ByteType = llvm::Type::getInt8Ty(Context);
    new llvm::GlobalVariable(Module, ByteType, false,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(ByteType, 1), "version");
    new llvm::GlobalVariable(Module, ByteType, false,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(ByteType, 2), "intrinsics");
  }
  WasmEdge::LLVM::Module RuntimeModule(
      reinterpret_cast<LLVMModuleRef>(&Module));
  const ARMRuntimeLibcallProfile Profile{
      true, true, true,
      true, true, Config.HardFloat ? ARMFloatABI::Hard : ARMFloatABI::SoftFP};
  EXPECT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(RuntimeModule, Profile));
  if (MixedCaller) {
    const std::string Caller = Config.Thumb ? R"(
.arm
.globl opposite_mode_caller
.hidden opposite_mode_caller
.type opposite_mode_caller, %function
opposite_mode_caller:
  bl __aeabi_idivmod
  bx lr
.size opposite_mode_caller, . - opposite_mode_caller
)"
                                            : R"(
.thumb
.globl opposite_mode_caller
.hidden opposite_mode_caller
.thumb_func
.type opposite_mode_caller, %function
opposite_mode_caller:
  bl __aeabi_idivmod
  bx lr
.size opposite_mode_caller, . - opposite_mode_caller
)";
    Module.setModuleInlineAsm(Module.getModuleInlineAsm() + Caller);
  }
  RuntimeModule.release();

  if (Config.Optimization != ARMOptimization::O0) {
#if LLVM_VERSION_MAJOR >= 13
    llvm::LoopAnalysisManager LoopAnalyses;
    llvm::FunctionAnalysisManager FunctionAnalyses;
    llvm::CGSCCAnalysisManager CGSCCAnalyses;
    llvm::ModuleAnalysisManager ModuleAnalyses;
    llvm::PassBuilder Pipeline;
    Pipeline.registerModuleAnalyses(ModuleAnalyses);
    Pipeline.registerCGSCCAnalyses(CGSCCAnalyses);
    Pipeline.registerFunctionAnalyses(FunctionAnalyses);
    Pipeline.registerLoopAnalyses(LoopAnalyses);
    Pipeline.crossRegisterProxies(LoopAnalyses, FunctionAnalyses, CGSCCAnalyses,
                                  ModuleAnalyses);
    const auto Level = [&] {
      switch (Config.Optimization) {
      case ARMOptimization::O0:
        return llvm::OptimizationLevel::O0;
      case ARMOptimization::O1:
        return llvm::OptimizationLevel::O1;
      case ARMOptimization::O2:
        return llvm::OptimizationLevel::O2;
      case ARMOptimization::O3:
        return llvm::OptimizationLevel::O3;
      case ARMOptimization::Os:
        return llvm::OptimizationLevel::Os;
      case ARMOptimization::Oz:
        return llvm::OptimizationLevel::Oz;
      }
      return llvm::OptimizationLevel::O0;
    }();
    auto Optimizer = Pipeline.buildPerModuleDefaultPipeline(Level);
    Optimizer.run(Module, ModuleAnalyses);
#else
    llvm::legacy::PassManager Optimizer;
    llvm::PassManagerBuilder Pipeline;
    Pipeline.OptLevel = optimizationLevel(Config.Optimization);
    Pipeline.SizeLevel = Config.Optimization == ARMOptimization::Os   ? 1
                         : Config.Optimization == ARMOptimization::Oz ? 2
                                                                      : 0;
    Pipeline.populateModulePassManager(Optimizer);
    Optimizer.run(Module);
#endif
  }

  llvm::SmallVector<char, 0> Storage;
  llvm::raw_svector_ostream Stream(Storage);
  llvm::legacy::PassManager Passes;
#if LLVM_VERSION_MAJOR >= 18
  const auto FileType = llvm::CodeGenFileType::ObjectFile;
#else
  const auto FileType = llvm::CGFT_ObjectFile;
#endif
  EXPECT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  return std::vector<Byte>(Storage.begin(), Storage.end());
}

std::vector<Byte> makeARMAssemblyObject(std::string Assembly) {
  const ARMObjectConfig Config{"armv7-unknown-linux-gnueabi",
                               "cortex-a9",
                               "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                               false,
                               true,
                               ARMOptimization::O0};
  const llvm::Triple Triple(Config.Triple);
  std::string Error;
  const llvm::Target *Target = lookupLLVMTarget(Triple, Error);
  EXPECT_NE(Target, nullptr) << Error;
  if (Target == nullptr)
    return {};
  llvm::TargetOptions Options;
#if LLVM_VERSION_MAJOR == 18
  auto *MachineOptions = LLVMCreateTargetMachineOptions();
  LLVMTargetMachineOptionsSetCPU(MachineOptions, Config.CPU.c_str());
  LLVMTargetMachineOptionsSetFeatures(
      MachineOptions, (Config.Features + ",+thumb-mode").c_str());
  LLVMTargetMachineOptionsSetCodeGenOptLevel(MachineOptions,
                                             LLVMCodeGenLevelNone);
  LLVMTargetMachineOptionsSetRelocMode(MachineOptions, LLVMRelocPIC);
  auto *MachineRef = LLVMCreateTargetMachineWithOptions(
      reinterpret_cast<LLVMTargetRef>(const_cast<llvm::Target *>(Target)),
      Config.Triple.c_str(), MachineOptions);
  LLVMDisposeTargetMachineOptions(MachineOptions);
  auto Machine = std::unique_ptr<llvm::TargetMachine>(
      reinterpret_cast<llvm::TargetMachine *>(MachineRef));
#else
  auto Machine =
      std::unique_ptr<llvm::TargetMachine>(Target->createTargetMachine(
#if LLVM_VERSION_MAJOR >= 21
          Triple,
#else
          Triple.str(),
#endif
          Config.CPU, Config.Features + ",+thumb-mode", Options,
          llvm::Reloc::PIC_));
#endif
  EXPECT_NE(Machine, nullptr);
  if (Machine == nullptr)
    return {};
  llvm::LLVMContext Context;
  llvm::Module Module("arm-runtime-negative", Context);
#if LLVM_VERSION_MAJOR >= 21
  Module.setTargetTriple(Triple);
#else
  Module.setTargetTriple(Triple.str());
#endif
  Module.setDataLayout(Machine->createDataLayout());
  Module.setModuleInlineAsm(std::move(Assembly));
  llvm::SmallVector<char, 0> Storage;
  llvm::raw_svector_ostream Stream(Storage);
  llvm::legacy::PassManager Passes;
#if LLVM_VERSION_MAJOR >= 18
  const auto FileType = llvm::CodeGenFileType::ObjectFile;
#else
  const auto FileType = llvm::CGFT_ObjectFile;
#endif
  EXPECT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  return std::vector<Byte>(Storage.begin(), Storage.end());
}

std::set<std::string> undefinedSymbols(Span<const Byte> Object) {
  std::set<std::string> Result;
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "arm-runtime-libcalls.o"));
  EXPECT_TRUE(static_cast<bool>(Parsed));
  if (!Parsed) {
    llvm::consumeError(Parsed.takeError());
    return Result;
  }
  for (const auto &Symbol : (*Parsed)->symbols()) {
    auto Flags = Symbol.getFlags();
    EXPECT_TRUE(static_cast<bool>(Flags));
    if (!Flags) {
      llvm::consumeError(Flags.takeError());
      continue;
    }
    if ((*Flags & llvm::object::SymbolRef::SF_Undefined) == 0)
      continue;
    auto Name = Symbol.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name) {
      llvm::consumeError(Name.takeError());
      continue;
    }
    Result.emplace(Name->str());
  }
  return Result;
}

std::vector<ARMObjectConfig> armVFPConfigurations() {
  std::vector<ARMObjectConfig> Result;
  for (const bool HardFloat : {false, true}) {
    const std::string Triple = HardFloat ? "armv7-unknown-linux-gnueabihf"
                                         : "armv7-unknown-linux-gnueabi";
    for (const bool Thumb : {false, true}) {
      Result.push_back({Triple, "cortex-a9", "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                        HardFloat, Thumb, ARMOptimization::O0});
      Result.push_back({Triple, "cortex-a15", "+vfp3,-neon,+hwdiv-arm,+hwdiv",
                        HardFloat, Thumb, ARMOptimization::O2});
    }
  }
  Result.push_back({"armv7-unknown-linux-gnueabi", "cortex-a9",
                    "+vfp3,-neon,-hwdiv-arm,-hwdiv", false, false,
                    ARMOptimization::O2});
  Result.push_back({"armv7-unknown-linux-gnueabihf", "cortex-a15",
                    "+vfp3,-neon,+hwdiv-arm,+hwdiv", true, true,
                    ARMOptimization::O0});
  return Result;
}

std::string describe(const ARMObjectConfig &Config) {
  std::ostringstream Stream;
  Stream << Config.Triple << " CPU=" << Config.CPU
         << " features=" << Config.Features
         << " mode=" << (Config.Thumb ? "Thumb" : "ARM")
         << " opt=" << optimizationName(Config.Optimization);
  return Stream.str();
}

const std::set<std::string> &expectedARMComputationalSymbols() {
  static const std::set<std::string> Symbols{"__aeabi_d2lz",
                                             "__aeabi_d2ulz",
                                             "__aeabi_f2lz",
                                             "__aeabi_f2ulz",
                                             "__aeabi_idiv",
                                             "__aeabi_idivmod",
                                             "__aeabi_l2d",
                                             "__aeabi_l2f",
                                             "__aeabi_ldivmod",
                                             "__aeabi_uidiv",
                                             "__aeabi_uidivmod",
                                             "__aeabi_ul2d",
                                             "__aeabi_ul2f",
                                             "__aeabi_uldivmod",
                                             "ceil",
                                             "ceilf",
                                             "floor",
                                             "floorf",
                                             "fmax",
                                             "fmaxf",
                                             "fmin",
                                             "fminf",
                                             "roundeven",
                                             "roundevenf",
                                             "trunc",
                                             "truncf"};
  return Symbols;
}

bool supportsHostARMRuntimeLibcalls() noexcept {
  return WasmEdge::LLVM::supportsARMRuntimeLibcalls(
      WasmEdge::LLVM::hostARMRuntimeLibcallProfile());
}

std::set<std::string>
requiredARMComputationalSymbols(const ARMObjectConfig &Config) {
  std::set<std::string> Symbols = expectedARMComputationalSymbols();
  if (Config.Features.find("+hwdiv") != std::string::npos) {
    Symbols.erase("__aeabi_idiv");
    Symbols.erase("__aeabi_idivmod");
    Symbols.erase("__aeabi_uidiv");
    Symbols.erase("__aeabi_uidivmod");
  }
  return Symbols;
}

TEST(ARMRuntimeLibcallsInventoryTest, RecordsComputationalUndefinedSymbols) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  std::set<std::string> Observed;
  for (const ARMObjectConfig &Config : armVFPConfigurations()) {
    const auto Symbols = undefinedSymbols(makeARMNumericObject(Config));
    const auto Required = requiredARMComputationalSymbols(Config);
    Observed.insert(Symbols.begin(), Symbols.end());
    EXPECT_TRUE(std::includes(Symbols.begin(), Symbols.end(), Required.begin(),
                              Required.end()))
        << describe(Config);
    for (const auto &Name : Symbols)
      EXPECT_NE(expectedARMComputationalSymbols().count(Name), 0U)
          << describe(Config) << ": " << Name;
  }
  EXPECT_EQ(Observed, expectedARMComputationalSymbols());
}

TEST(ARMRuntimeLibcallsInventoryTest,
     UsesBaseAAPCSConversionSymbolsAtCallSites) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  const std::set<std::pair<std::string, std::string>> Expected{
      {"sitofp1", "__aeabi_l2f"},  {"uitofp1", "__aeabi_ul2f"},
      {"sitofp2", "__aeabi_l2d"},  {"uitofp2", "__aeabi_ul2d"},
      {"fptosi1", "__aeabi_f2lz"}, {"fptoui1", "__aeabi_f2ulz"},
      {"fptosi2", "__aeabi_d2lz"}, {"fptoui2", "__aeabi_d2ulz"}};
  for (const bool HardFloat : {false, true})
    for (const bool Thumb : {false, true}) {
      const ARMObjectConfig Config{HardFloat ? "armv7-unknown-linux-gnueabihf"
                                             : "armv7-unknown-linux-gnueabi",
                                   "cortex-a9",
                                   "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                   HardFloat,
                                   Thumb,
                                   ARMOptimization::O0};
      const auto Object = makeARMNumericObject(Config);
      auto Parsed =
          llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
              llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                              Object.size()),
              "arm-numeric.o"));
      ASSERT_TRUE(static_cast<bool>(Parsed));
      struct FunctionRange {
        std::string Name;
        uint64_t Address;
        uint64_t Size;
        uint64_t Section;
      };
      std::vector<FunctionRange> Functions;
      for (const auto &Symbol : (*Parsed)->symbols()) {
        auto Type = Symbol.getType();
        ASSERT_TRUE(static_cast<bool>(Type));
        if (*Type != llvm::object::SymbolRef::ST_Function)
          continue;
        auto Name = Symbol.getName();
        auto Address = Symbol.getAddress();
        auto Section = Symbol.getSection();
        ASSERT_TRUE(static_cast<bool>(Name));
        ASSERT_TRUE(static_cast<bool>(Address));
        ASSERT_TRUE(static_cast<bool>(Section));
        ASSERT_NE(*Section, (*Parsed)->section_end());
        Functions.push_back({Name->str(), *Address & ~UINT64_C(1),
                             llvm::object::ELFSymbolRef(Symbol).getSize(),
                             (*Section)->getIndex()});
      }

      std::set<std::pair<std::string, std::string>> Calls;
      for (const auto &Section : (*Parsed)->sections()) {
        auto Relocated = Section.getRelocatedSection();
        ASSERT_TRUE(static_cast<bool>(Relocated));
        const uint64_t RelocatedSection = *Relocated == (*Parsed)->section_end()
                                              ? Section.getIndex()
                                              : (*Relocated)->getIndex();
        for (const auto &Relocation : Section.relocations()) {
          const auto Source = std::find_if(
              Functions.begin(), Functions.end(), [&](const auto &Function) {
                return Function.Section == RelocatedSection &&
                       Relocation.getOffset() >= Function.Address &&
                       Relocation.getOffset() <
                           Function.Address + Function.Size;
              });
          if (Source == Functions.end())
            continue;
          auto Target = Relocation.getSymbol();
          ASSERT_NE(Target, (*Parsed)->symbol_end());
          auto TargetName = Target->getName();
          ASSERT_TRUE(static_cast<bool>(TargetName));
          if (TargetName->take_front(8) == "__aeabi_")
            Calls.emplace(Source->Name, TargetName->str());
        }
      }
      EXPECT_TRUE(std::includes(Calls.begin(), Calls.end(), Expected.begin(),
                                Expected.end()))
          << describe(Config);
    }
}

TEST(ARMRuntimeLibcallsInventoryTest, ExcludesNonComputationalSymbols) {
  const std::array<const char *, 11> Excluded{"__aeabi_unwind_cpp_pr0",
                                              "__aeabi_unwind_cpp_pr1",
                                              "__aeabi_unwind_cpp_pr2",
                                              "__aeabi_not_real",
                                              "__aeabi_memcpy",
                                              "__divdi3",
                                              "__atomic_load_8",
                                              "malloc",
                                              "_Unwind_Resume",
                                              "__cxa_throw",
                                              "user_import"};
  for (const char *Name : Excluded)
    EXPECT_EQ(expectedARMComputationalSymbols().count(Name), 0U) << Name;
}

TEST(ARMRuntimeLibcallsObjectReaderTest, StrictlyRejectsExactUndefinedNames) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  for (const char *Name :
       {"__aeabi_unwind_cpp_pr0", "__aeabi_unwind_cpp_pr1",
        "__aeabi_unwind_cpp_pr2", "__aeabi_not_real", "__aeabi_memcpy",
        "__divdi3", "__atomic_load_8", "malloc", "_Unwind_Resume",
        "__cxa_throw", "user_import"}) {
    const auto Object = makeARMAssemblyObject(
        ".syntax unified\n.thumb\n.text\n.globl caller\n.thumb_func\n"
        ".type caller,%function\ncaller:\n bl " +
        std::string(Name) + "\n bx lr\n");
    auto Result = ObjectReader::read(Object, Target::ARM);
    ASSERT_FALSE(Result) << Name;
    EXPECT_EQ(Result.error().Message, "undefined symbol") << Name;
    EXPECT_EQ(Result.error().SymbolName, Name);
  }
}

TEST(ARMRuntimeLibcallsObjectReaderTest,
     ApprovedNamesDoNotBypassNormalValidation) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  const auto WrongFormat = makeARMAssemblyObject(
      ".syntax unified\n.thumb\n.text\n.globl ceil\n.thumb_func\n"
      ".type ceil,%function\nceil:\n bx lr\n");
  EXPECT_FALSE(ObjectReader::read(WrongFormat, Target::X86_64));

  const auto WrongType = makeARMAssemblyObject(
      ".syntax unified\n.data\n.globl ceil\n.type ceil,%object\n"
      " .long ceil\n");
  auto TypeResult = ObjectReader::read(WrongType, Target::ARM);
  ASSERT_FALSE(TypeResult);
  EXPECT_EQ(TypeResult.error().Message, "undefined symbol");
  EXPECT_EQ(TypeResult.error().SymbolName, "ceil");

  const auto WrongRelocation = makeARMAssemblyObject(
      ".syntax unified\n.data\n.globl ceil\n.type ceil,%function\nceil:\n"
      " .reloc ., R_ARM_CALL, ceil\n .long 0\n.size ceil,4\n");
  auto RelocationGraph = ObjectReader::read(WrongRelocation, Target::ARM);
  ASSERT_TRUE(RelocationGraph) << RelocationGraph.error().Message;
  ASSERT_TRUE(layout(*RelocationGraph, 0x1000));
  EXPECT_FALSE(applyRelocations(*RelocationGraph));
}

TEST(ARMRuntimeLibcallsInventoryTest,
     RetainsReviewedABIAndImplementationSymbolsAcrossAllConfigurations) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  const std::array<ARMOptimization, 6> Optimizations{
      ARMOptimization::O0, ARMOptimization::O1, ARMOptimization::O2,
      ARMOptimization::O3, ARMOptimization::Os, ARMOptimization::Oz};
  const auto &ReviewedPublicABIHelpers = expectedARMComputationalSymbols();
  const std::set<std::string> HiddenImplementationDependencies{
      "__aeabi_idiv0",
      "__aeabi_ldiv0",
      "__wasmedge_aeabi_d2lz_core",
      "__wasmedge_aeabi_d2ulz_core",
      "__wasmedge_aeabi_f2lz_core",
      "__wasmedge_aeabi_f2ulz_core",
      "__wasmedge_aeabi_idivmod_core",
      "__wasmedge_aeabi_l2d_core",
      "__wasmedge_aeabi_l2f_core",
      "__wasmedge_aeabi_ldivmod_core",
      "__wasmedge_aeabi_uidivmod_core",
      "__wasmedge_aeabi_ul2d_core",
      "__wasmedge_aeabi_ul2f_core",
      "__wasmedge_aeabi_uldivmod_core",
      "__wasmedge_ceil_core",
      "__wasmedge_ceilf_core",
      "__wasmedge_floor_core",
      "__wasmedge_floorf_core",
      "__wasmedge_fmax_core",
      "__wasmedge_fmaxf_core",
      "__wasmedge_fmin_core",
      "__wasmedge_fminf_core",
      "__wasmedge_roundeven_core",
      "__wasmedge_roundevenf_core",
      "__wasmedge_trunc_core",
      "__wasmedge_truncf_core"};
  const auto IsSemanticIndex = [](llvm::StringRef Name) {
    return Name.size() > 1 && (Name[0] == 't' || Name[0] == 'f') &&
           std::all_of(Name.begin() + 1, Name.end(), [](char Value) {
             return std::isdigit(static_cast<unsigned char>(Value)) != 0;
           });
  };

  size_t ConfigurationCount = 0;
  for (const auto Optimization : Optimizations)
    for (const bool Thumb : {false, true})
      for (const bool HardFloat : {false, true}) {
        ++ConfigurationCount;
        const ARMObjectConfig Config{HardFloat ? "armv7-unknown-linux-gnueabihf"
                                               : "armv7-unknown-linux-gnueabi",
                                     "cortex-a9",
                                     "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                     HardFloat,
                                     Thumb,
                                     Optimization};
        const auto Object = makeARMRuntimeObject(Config);
        auto Parsed =
            llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
                llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                                Object.size()),
                "arm-runtime-inventory.o"));
        ASSERT_TRUE(static_cast<bool>(Parsed)) << describe(Config);
        std::set<std::string> ELFPublicABIHelpers;
        std::set<std::string> ELFImplementationDependencies;
        for (const auto &Symbol : (*Parsed)->symbols()) {
          auto Name = Symbol.getName();
          auto Type = Symbol.getType();
          auto Flags = Symbol.getFlags();
          ASSERT_TRUE(Name && Type && Flags) << describe(Config);
          if ((*Flags & llvm::object::SymbolRef::SF_Undefined) == 0) {
            EXPECT_NE(*Name, "version") << describe(Config);
            EXPECT_NE(*Name, "intrinsics") << describe(Config);
            EXPECT_FALSE(IsSemanticIndex(*Name))
                << describe(Config) << ": " << Name->str();
          }
          const bool Public = ReviewedPublicABIHelpers.count(Name->str()) != 0;
          const bool Implementation =
              HiddenImplementationDependencies.count(Name->str()) != 0;
          if (!Public && !Implementation)
            continue;
          (Public ? ELFPublicABIHelpers : ELFImplementationDependencies)
              .emplace(Name->str());
          const llvm::object::ELFSymbolRef ELFSymbol(Symbol);
          EXPECT_EQ(*Type, llvm::object::SymbolRef::ST_Function)
              << describe(Config) << ": " << Name->str();
          EXPECT_EQ(ELFSymbol.getELFType(), llvm::ELF::STT_FUNC)
              << describe(Config) << ": " << Name->str();
          EXPECT_EQ(ELFSymbol.getBinding(), llvm::ELF::STB_GLOBAL)
              << describe(Config) << ": " << Name->str();
          EXPECT_EQ(ELFSymbol.getOther() & 0x03, llvm::ELF::STV_HIDDEN)
              << describe(Config) << ": " << Name->str();
          EXPECT_EQ(*Flags & llvm::object::SymbolRef::SF_Undefined, 0U)
              << describe(Config) << ": " << Name->str();
          EXPECT_NE(*Flags & llvm::object::SymbolRef::SF_Hidden, 0U)
              << describe(Config) << ": " << Name->str();
        }
        EXPECT_EQ(ELFPublicABIHelpers, ReviewedPublicABIHelpers)
            << describe(Config);
        EXPECT_EQ(ELFImplementationDependencies,
                  HiddenImplementationDependencies)
            << describe(Config);

        auto Graph = ObjectReader::read(Object, Target::ARM);
        ASSERT_TRUE(Graph) << describe(Config) << ": " << Graph.error().Message;
        std::set<std::string> GraphPublicABIHelpers;
        std::set<std::string> GraphImplementationDependencies;
        std::set<std::string> ExportedSemanticNames;
        for (const auto &Symbol : Graph->symbols()) {
          if (Symbol.Exported)
            ExportedSemanticNames.emplace(Symbol.ExportName ? *Symbol.ExportName
                                                            : Symbol.Name);
          const bool Public = ReviewedPublicABIHelpers.count(Symbol.Name) != 0;
          const bool Implementation =
              HiddenImplementationDependencies.count(Symbol.Name) != 0;
          if (!Public && !Implementation)
            continue;
          (Public ? GraphPublicABIHelpers : GraphImplementationDependencies)
              .emplace(Symbol.Name);
          EXPECT_TRUE(Symbol.Global) << describe(Config) << ": " << Symbol.Name;
          EXPECT_FALSE(Symbol.Exported)
              << describe(Config) << ": " << Symbol.Name;
          EXPECT_NE(Symbol.Name, "version") << describe(Config);
          EXPECT_NE(Symbol.Name, "intrinsics") << describe(Config);
          EXPECT_FALSE(IsSemanticIndex(Symbol.Name))
              << describe(Config) << ": " << Symbol.Name;
        }
        EXPECT_EQ(GraphPublicABIHelpers, ReviewedPublicABIHelpers)
            << describe(Config);
        EXPECT_EQ(GraphImplementationDependencies,
                  HiddenImplementationDependencies)
            << describe(Config);
        for (const auto &Name : ReviewedPublicABIHelpers)
          EXPECT_EQ(ExportedSemanticNames.count(Name), 0U) << describe(Config);
        for (const auto &Name : HiddenImplementationDependencies)
          EXPECT_EQ(ExportedSemanticNames.count(Name), 0U) << describe(Config);
      }
  EXPECT_EQ(ConfigurationCount, 24U);
}

TEST(ARMRuntimeLibcallsInventoryTest,
     InternalImplementationRetainsExactAssemblyReferenceName) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  const ARMObjectConfig Config{"armv7-unknown-linux-gnueabi",
                               "cortex-a9",
                               "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                               false,
                               true,
                               ARMOptimization::O0};
  const llvm::Triple Triple(Config.Triple);
  std::string Error;
  const llvm::Target *LLVMTarget = lookupLLVMTarget(Triple, Error);
  ASSERT_NE(LLVMTarget, nullptr) << Error;
  llvm::TargetOptions Options;
#if LLVM_VERSION_MAJOR == 18
  auto *MachineOptions = LLVMCreateTargetMachineOptions();
  LLVMTargetMachineOptionsSetCPU(MachineOptions, Config.CPU.c_str());
  LLVMTargetMachineOptionsSetFeatures(
      MachineOptions, (Config.Features + ",+thumb-mode").c_str());
  LLVMTargetMachineOptionsSetCodeGenOptLevel(MachineOptions,
                                             LLVMCodeGenLevelNone);
  LLVMTargetMachineOptionsSetRelocMode(MachineOptions, LLVMRelocPIC);
  auto *MachineRef = LLVMCreateTargetMachineWithOptions(
      reinterpret_cast<LLVMTargetRef>(const_cast<llvm::Target *>(LLVMTarget)),
      Config.Triple.c_str(), MachineOptions);
  LLVMDisposeTargetMachineOptions(MachineOptions);
  auto Machine = std::unique_ptr<llvm::TargetMachine>(
      reinterpret_cast<llvm::TargetMachine *>(MachineRef));
#else
  auto Machine =
      std::unique_ptr<llvm::TargetMachine>(LLVMTarget->createTargetMachine(
#if LLVM_VERSION_MAJOR >= 21
          Triple,
#else
          Triple.str(),
#endif
          Config.CPU, Config.Features + ",+thumb-mode", Options,
          llvm::Reloc::PIC_));
#endif
  ASSERT_NE(Machine, nullptr);
  llvm::LLVMContext Context;
  llvm::Module Module("arm-runtime-internal-linkage", Context);
#if LLVM_VERSION_MAJOR >= 21
  Module.setTargetTriple(Triple);
#else
  Module.setTargetTriple(Triple.str());
#endif
  Module.setDataLayout(Machine->createDataLayout());
  WasmEdge::LLVM::Module RuntimeModule(
      reinterpret_cast<LLVMModuleRef>(&Module));
  const ARMRuntimeLibcallProfile Profile{true, true, true,
                                         true, true, ARMFloatABI::SoftFP};
  ASSERT_TRUE(WasmEdge::LLVM::addARMRuntimeLibcalls(RuntimeModule, Profile));
  RuntimeModule.release();
  auto *Core = Module.getFunction("__wasmedge_aeabi_idivmod_core");
  ASSERT_NE(Core, nullptr);
  Core->setLinkage(llvm::GlobalValue::InternalLinkage);

  llvm::SmallVector<char, 0> Storage;
  llvm::raw_svector_ostream Stream(Storage);
  llvm::legacy::PassManager Passes;
#if LLVM_VERSION_MAJOR >= 18
  const auto FileType = llvm::CodeGenFileType::ObjectFile;
#else
  const auto FileType = llvm::CGFT_ObjectFile;
#endif
  ASSERT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  const std::vector<Byte> Object(Storage.begin(), Storage.end());

  auto Graph = ObjectReader::read(Object, Target::ARM);
  ASSERT_TRUE(Graph) << Graph.error().Message;
  const auto EmittedCore = std::find_if(
      Graph->symbols().begin(), Graph->symbols().end(), [](const auto &Symbol) {
        return Symbol.Name == "__wasmedge_aeabi_idivmod_core";
      });
  ASSERT_NE(EmittedCore, Graph->symbols().end());
  EXPECT_FALSE(EmittedCore->Global);
  EXPECT_FALSE(EmittedCore->Exported);
  EXPECT_TRUE(std::any_of(Graph->relocations().begin(),
                          Graph->relocations().end(),
                          [&](const auto &Relocation) {
                            return Graph->symbols()[Relocation.Symbol].Name ==
                                   "__wasmedge_aeabi_idivmod_core";
                          }));
}

TEST(ARMRuntimeLibcallsWrapperTest, EmitsDefinedHiddenFunctionsWithoutEHABI) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  for (const bool Thumb : {false, true}) {
    const ARMObjectConfig Config{"armv7-unknown-linux-gnueabi",
                                 "cortex-a9",
                                 "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                 false,
                                 Thumb,
                                 ARMOptimization::O0};
    const auto Object = makeARMRuntimeObject(Config);
    EXPECT_TRUE(undefinedSymbols(Object).empty()) << describe(Config);

    auto Parsed =
        llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
            llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                            Object.size()),
            "arm-runtime-libcalls.o"));
    ASSERT_TRUE(static_cast<bool>(Parsed));
    struct FunctionInfo {
      uint64_t Address;
      uint64_t Size;
      uint64_t Section;
    };
    std::map<std::string, FunctionInfo> Functions;
    for (const auto &Symbol : (*Parsed)->symbols()) {
      auto Name = Symbol.getName();
      auto Type = Symbol.getType();
      auto Flags = Symbol.getFlags();
      ASSERT_TRUE(static_cast<bool>(Name));
      ASSERT_TRUE(static_cast<bool>(Type));
      ASSERT_TRUE(static_cast<bool>(Flags));
      if (*Type != llvm::object::SymbolRef::ST_Function)
        continue;
      if (*Name == "__aeabi_idiv0" || *Name == "__aeabi_ldiv0" ||
          expectedARMComputationalSymbols().count(Name->str()) != 0 ||
          Name->take_front(11) == "__wasmedge_") {
        auto Address = Symbol.getAddress();
        const auto Size = llvm::object::ELFSymbolRef(Symbol).getSize();
        auto Section = Symbol.getSection();
        ASSERT_TRUE(static_cast<bool>(Address));
        ASSERT_TRUE(static_cast<bool>(Section));
        ASSERT_NE(*Section, (*Parsed)->section_end());
        Functions.emplace(Name->str(),
                          FunctionInfo{*Address & ~UINT64_C(1), Size,
                                       (*Section)->getIndex()});
        EXPECT_NE(*Flags & llvm::object::SymbolRef::SF_Hidden, 0U)
            << Name->str();
      }
    }
    EXPECT_EQ(Functions.size(), 52U) << describe(Config);

    for (const auto &[Name, Function] : Functions) {
      if (Name.compare(0, 8, "__aeabi_") != 0 &&
          Name.compare(0, 11, "__wasmedge_") != 0 &&
          expectedARMComputationalSymbols().count(Name) == 0)
        continue;
      if (Name == "__aeabi_idiv0" || Name == "__aeabi_ldiv0" ||
          Name == "__aeabi_idiv" || Name == "__aeabi_uidiv")
        continue;
      EXPECT_GT(Function.Size, 0U) << Name;
      const uint64_t SectionIndex = Function.Section;
      const auto Section =
          std::find_if((*Parsed)->section_begin(), (*Parsed)->section_end(),
                       [SectionIndex](const auto &Value) {
                         return Value.getIndex() == SectionIndex;
                       });
      ASSERT_NE(Section, (*Parsed)->section_end());
      auto Contents = Section->getContents();
      ASSERT_TRUE(static_cast<bool>(Contents));
      ASSERT_LE(Function.Address + Function.Size, Contents->size());
      if (Name == "__aeabi_idivmod" || Name == "__aeabi_uidivmod") {
        const llvm::StringRef Code =
            Contents->substr(Function.Address, Function.Size);
        const std::array<uint8_t, 4> ARMLoad{0x04, 0x10, 0x9D, 0xE5};
        const std::array<uint8_t, 2> ThumbLoad{0x01, 0x99};
        const std::array<uint8_t, 4> ARMZeroStore{0x04, 0x00, 0x8D, 0xE5};
        const std::array<uint8_t, 2> ThumbZeroStore{0x01, 0x90};
        const size_t ZeroStoreOffset = Thumb ? 6 : 12;
        if (Thumb) {
          ASSERT_LE(ZeroStoreOffset + ThumbZeroStore.size(), Code.size());
          EXPECT_TRUE(std::equal(ThumbZeroStore.begin(), ThumbZeroStore.end(),
                                 Code.bytes_begin() + ZeroStoreOffset))
              << describe(Config) << ": " << Name;
        } else {
          ASSERT_LE(ZeroStoreOffset + ARMZeroStore.size(), Code.size());
          EXPECT_TRUE(std::equal(ARMZeroStore.begin(), ARMZeroStore.end(),
                                 Code.bytes_begin() + ZeroStoreOffset))
              << describe(Config) << ": " << Name;
        }
        if (Thumb)
          EXPECT_NE(std::search(Code.bytes_begin(), Code.bytes_end(),
                                ThumbLoad.begin(), ThumbLoad.end()),
                    Code.bytes_end())
              << describe(Config) << ": " << Name;
        else
          EXPECT_NE(std::search(Code.bytes_begin(), Code.bytes_end(),
                                ARMLoad.begin(), ARMLoad.end()),
                    Code.bytes_end())
              << describe(Config) << ": " << Name;
      }
    }

    std::set<std::pair<std::string, std::string>> Calls;
    for (const auto &Section : (*Parsed)->sections()) {
      auto Relocated = Section.getRelocatedSection();
      ASSERT_TRUE(static_cast<bool>(Relocated));
      const uint64_t RelocatedSection = *Relocated == (*Parsed)->section_end()
                                            ? Section.getIndex()
                                            : (*Relocated)->getIndex();
      for (const auto &Relocation : Section.relocations()) {
        if (Relocation.getType() !=
            (Thumb ? llvm::ELF::R_ARM_THM_CALL : llvm::ELF::R_ARM_CALL))
          continue;
        const auto Source = std::find_if(
            Functions.begin(), Functions.end(), [&](const auto &Function) {
              return Function.second.Section == RelocatedSection &&
                     Relocation.getOffset() >= Function.second.Address &&
                     Relocation.getOffset() <
                         Function.second.Address + Function.second.Size;
            });
        ASSERT_NE(Source, Functions.end())
            << describe(Config) << " section=" << Section.getIndex()
            << " relocated=" << RelocatedSection
            << " offset=" << Relocation.getOffset();
        auto Target = Relocation.getSymbol();
        ASSERT_NE(Target, (*Parsed)->symbol_end());
        auto TargetName = Target->getName();
        ASSERT_TRUE(static_cast<bool>(TargetName));
        Calls.emplace(Source->first, TargetName->str());
      }
    }
    EXPECT_EQ(Calls, (std::set<std::pair<std::string, std::string>>{
                         {"__aeabi_idiv", "__aeabi_idiv0"},
                         {"__aeabi_idiv", "__wasmedge_aeabi_idivmod_core"},
                         {"__aeabi_uidiv", "__aeabi_idiv0"},
                         {"__aeabi_uidiv", "__wasmedge_aeabi_uidivmod_core"},
                         {"__aeabi_idivmod", "__aeabi_idiv0"},
                         {"__aeabi_idivmod", "__wasmedge_aeabi_idivmod_core"},
                         {"__aeabi_uidivmod", "__aeabi_idiv0"},
                         {"__aeabi_uidivmod", "__wasmedge_aeabi_uidivmod_core"},
                         {"__aeabi_ldivmod", "__aeabi_ldiv0"},
                         {"__aeabi_ldivmod", "__wasmedge_aeabi_ldivmod_core"},
                         {"__aeabi_uldivmod", "__aeabi_ldiv0"},
                         {"__aeabi_uldivmod", "__wasmedge_aeabi_uldivmod_core"},
                     }))
        << describe(Config);
  }
}

TEST(ARMRuntimeLibcallsWrapperTest, EmitsOppositeModePublicCallRelocation) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  for (const bool WrapperThumb : {false, true}) {
    const ARMObjectConfig Config{"armv7-unknown-linux-gnueabi",
                                 "cortex-a9",
                                 "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                 false,
                                 WrapperThumb,
                                 ARMOptimization::O0};
    const auto Object = makeARMRuntimeObject(Config, true);
    auto Parsed =
        llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
            llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                            Object.size()),
            "arm-runtime-mixed-mode.o"));
    ASSERT_TRUE(static_cast<bool>(Parsed));
    bool Found = false;
    for (const auto &Section : (*Parsed)->sections())
      for (const auto &Relocation : Section.relocations()) {
        if (Relocation.getType() !=
            (WrapperThumb ? llvm::ELF::R_ARM_CALL : llvm::ELF::R_ARM_THM_CALL))
          continue;
        auto Symbol = Relocation.getSymbol();
        ASSERT_NE(Symbol, (*Parsed)->symbol_end());
        auto Name = Symbol->getName();
        ASSERT_TRUE(static_cast<bool>(Name));
        if (*Name == "__aeabi_idivmod")
          Found = true;
      }
    EXPECT_TRUE(Found) << describe(Config);
    auto Graph = ObjectReader::read(Object, Target::ARM);
    ASSERT_TRUE(Graph) << Graph.error().Message;
    ASSERT_TRUE(layout(*Graph, 0x1000));
    auto Relocated = applyRelocations(*Graph);
    EXPECT_TRUE(Relocated)
        << Relocated.error().Message
        << " section=" << Relocated.error().Section.value_or(UINT32_MAX)
        << " offset=" << Relocated.error().Offset.value_or(UINT64_MAX)
        << " type=" << Relocated.error().RelocationType.value_or(UINT32_MAX)
        << " symbol=" << Relocated.error().SymbolName;
  }
}

TEST(ARMRuntimeLibcallsWrapperTest, ResolvesGeneratedScalarCallersForExactPCS) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  for (const bool HardFloat : {false, true})
    for (const bool Thumb : {false, true}) {
      const ARMObjectConfig Config{HardFloat ? "armv7-unknown-linux-gnueabihf"
                                             : "armv7-unknown-linux-gnueabi",
                                   "cortex-a9",
                                   "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                   HardFloat,
                                   Thumb,
                                   ARMOptimization::O2};
      const auto Object = makeARMRuntimeObject(Config, false, true);
      EXPECT_TRUE(undefinedSymbols(Object).empty()) << describe(Config);
    }
}

TEST(ARMRuntimeLibcallsIntegrationTest,
     WritesGeneratedNumericObjectsToELFForEveryModeAndPCS) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  for (const bool HardFloat : {false, true})
    for (const bool Thumb : {false, true}) {
      const ARMObjectConfig Config{HardFloat ? "armv7-unknown-linux-gnueabihf"
                                             : "armv7-unknown-linux-gnueabi",
                                   "cortex-a9",
                                   "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                   HardFloat,
                                   Thumb,
                                   ARMOptimization::O2};
      const auto Object = makeARMRuntimeObject(Config, false, true);
      EXPECT_TRUE(undefinedSymbols(Object).empty()) << describe(Config);
      auto Graph = ObjectReader::read(Object, Target::ARM);
      ASSERT_TRUE(Graph) << describe(Config) << ": " << Graph.error().Message
                         << " section=" << Graph.error().SectionName
                         << " offset="
                         << Graph.error().Offset.value_or(UINT64_MAX)
                         << " type="
                         << Graph.error().RelocationType.value_or(UINT32_MAX)
                         << " symbol=" << Graph.error().SymbolName;
      const auto Helper = std::find_if(
          Graph->symbols().begin(), Graph->symbols().end(),
          [](const auto &Value) { return Value.Name == "__aeabi_ldivmod"; });
      ASSERT_NE(Helper, Graph->symbols().end()) << describe(Config);
      EXPECT_TRUE(Helper->Global) << describe(Config);
      EXPECT_FALSE(Helper->Exported) << describe(Config);
      const auto HelperSection = Helper->Section;
      const auto HelperOffset = Helper->Offset;
      const auto HelperSize = Helper->Size;
      ASSERT_GT(HelperSize, 0U) << describe(Config);

      ASSERT_TRUE(ELFWriter::layout(*Graph)) << describe(Config);
      ASSERT_TRUE(applyRelocations(*Graph)) << describe(Config);
      ASSERT_LE(HelperOffset + HelperSize,
                Graph->sections()[HelperSection].Content.size())
          << describe(Config);
      const auto ExpectedHelper = std::vector<Byte>(
          Graph->sections()[HelperSection].Content.begin() + HelperOffset,
          Graph->sections()[HelperSection].Content.begin() + HelperOffset +
              HelperSize);
      std::vector<Byte> ELF;
      Writer Output(ELF);
      auto Written = ELFWriter::write(*Graph, Output);
      ASSERT_TRUE(Written) << describe(Config) << ": "
                           << Written.error().Message;

      auto Parsed =
          llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
              llvm::StringRef(reinterpret_cast<const char *>(ELF.data()),
                              ELF.size()),
              "arm-runtime.so"));
      ASSERT_TRUE(static_cast<bool>(Parsed)) << describe(Config);
      EXPECT_EQ((*Parsed)->getArch(), llvm::Triple::arm) << describe(Config);
      std::set<std::string> DynamicSymbols;
      for (const auto &Symbol : (*Parsed)->symbols()) {
        auto Name = Symbol.getName();
        ASSERT_TRUE(static_cast<bool>(Name)) << describe(Config);
        DynamicSymbols.emplace(Name->str());
      }
      EXPECT_EQ(DynamicSymbols.count("__aeabi_ldivmod"), 0U)
          << describe(Config);

      const auto HelperAddress =
          Graph->sections()[HelperSection].Address + HelperOffset;
      bool FoundHelperCode = false;
      for (const auto &Section : (*Parsed)->sections()) {
        const auto Address = Section.getAddress();
        auto Contents = Section.getContents();
        ASSERT_TRUE(static_cast<bool>(Contents)) << describe(Config);
        if (Address <= HelperAddress &&
            HelperAddress + HelperSize <= Address + Contents->size()) {
          const auto Offset = HelperAddress - Address;
          FoundHelperCode = std::equal(
              ExpectedHelper.begin(), ExpectedHelper.end(),
              reinterpret_cast<const Byte *>(Contents->data()) + Offset);
        }
      }
      EXPECT_TRUE(FoundHelperCode) << describe(Config);
    }
}

TEST(ARMRuntimeLibcallsIntegrationTest,
     UniversalWasmContainsHelpersOutsideSemanticTables) {
  if (!supportsHostARMRuntimeLibcalls())
    GTEST_SKIP() << "ARM runtime helpers require Linux EABI ARMv7+ with VFP";
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  constexpr std::array<Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                          0x01, 0x00, 0x00, 0x00};
  for (const bool HardFloat : {false, true})
    for (const bool Thumb : {false, true}) {
      const ARMObjectConfig Config{HardFloat ? "armv7-unknown-linux-gnueabihf"
                                             : "armv7-unknown-linux-gnueabi",
                                   "cortex-a9",
                                   "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                   HardFloat,
                                   Thumb,
                                   ARMOptimization::O2};
      const auto Object = makeARMRuntimeObject(Config, false, true, true);
      auto Graph = ObjectReader::read(Object, Target::ARM);
      ASSERT_TRUE(Graph) << describe(Config) << ": " << Graph.error().Message;
      const auto Helper = std::find_if(
          Graph->symbols().begin(), Graph->symbols().end(),
          [](const auto &Value) { return Value.Name == "__aeabi_ldivmod"; });
      ASSERT_NE(Helper, Graph->symbols().end()) << describe(Config);
      const auto HelperSection = Helper->Section;
      const auto HelperOffset = Helper->Offset;
      const auto HelperSize = Helper->Size;
      ASSERT_TRUE(layout(*Graph, 65536)) << describe(Config);
      ASSERT_TRUE(applyRelocations(*Graph)) << describe(Config);
      ASSERT_LE(HelperOffset + HelperSize,
                Graph->sections()[HelperSection].Content.size())
          << describe(Config);
      const auto HelperAddress = Graph->sections()[HelperSection].Address +
                                 HelperOffset +
                                 static_cast<uint64_t>(Helper->Thumb);

      std::vector<Byte> Wasm;
      Writer Output(Wasm);
      ASSERT_TRUE(UniversalWasmWriter::write(*Graph, EmptyWasm, Output))
          << describe(Config);
      WasmEdge::Configure Conf;
      Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
      WasmEdge::Loader::Loader Loader(Conf);
      auto Module = Loader.parseModule(Wasm);
      ASSERT_TRUE(Module) << describe(Config);
      const auto &AOT = (*Module)->getAOTSection();
      EXPECT_TRUE(AOT.getTypesAddress().empty()) << describe(Config);
      EXPECT_TRUE(AOT.getCodesAddress().empty()) << describe(Config);
      EXPECT_NE(AOT.getVersionAddress(), HelperAddress) << describe(Config);
      EXPECT_NE(AOT.getIntrinsicsAddress(), HelperAddress) << describe(Config);
      const auto HelperImageAddress =
          Graph->sections()[HelperSection].Address + HelperOffset;
      const auto HelperBytes = Span<const Byte>(
          Graph->sections()[HelperSection].Content.data() + HelperOffset,
          static_cast<size_t>(HelperSize));
      EXPECT_TRUE(std::any_of(
          AOT.getSections().begin(), AOT.getSections().end(),
          [&](const auto &Section) {
            const auto Address = std::get<1>(Section);
            const auto &Content = std::get<3>(Section);
            if (Address > HelperImageAddress ||
                HelperImageAddress + HelperSize > Address + Content.size())
              return false;
            return std::equal(HelperBytes.begin(), HelperBytes.end(),
                              Content.begin() + (HelperImageAddress - Address));
          }))
          << describe(Config);
    }
}

TEST(ARMRuntimeLibcallsIntegrationTest,
     CompilerAndCodeGenExecuteNumericWasmOnARMHost) {
  if (!supportsHostARMRuntimeLibcalls())
    GTEST_SKIP() << "ARM runtime helpers require Linux EABI ARMv7+ with VFP";
  constexpr std::array<Byte, 37> NumericWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05,
      0x01, 0x60, 0x00, 0x01, 0x7E, 0x03, 0x02, 0x01, 0x00, 0x07,
      0x05, 0x01, 0x01, 0x66, 0x00, 0x00, 0x0A, 0x09, 0x01, 0x07,
      0x00, 0x42, 0x08, 0x42, 0x02, 0x7F, 0x0B};
  const auto Prefix =
      std::filesystem::temp_directory_path() /
      ("WasmEdgeARMRuntime-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  for (const auto Format : {WasmEdge::CompilerConfigure::OutputFormat::Native,
                            WasmEdge::CompilerConfigure::OutputFormat::Wasm}) {
    WasmEdge::Configure Conf;
    Conf.getCompilerConfigure().setOutputFormat(Format);
    Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator Validator(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);
    auto Module = Loader.parseModule(NumericWasm);
    ASSERT_TRUE(Module);
    ASSERT_TRUE(Validator.validate(**Module));
    auto Data = Compiler.compile(**Module);
    ASSERT_TRUE(Data);
    const auto Output =
        Prefix.string() +
        (Format == WasmEdge::CompilerConfigure::OutputFormat::Native
             ? WASMEDGE_LIB_EXTENSION
             : ".aot.wasm");
    ASSERT_TRUE(CodeGen.codegen(NumericWasm, std::move(*Data), Output));

    WasmEdge::VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(Output));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Result = VM.execute("f");
    ASSERT_TRUE(Result);
    ASSERT_EQ(Result->size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint64_t>(), 4U);
    EXPECT_TRUE(std::filesystem::remove(Output));
  }
}

TEST(ARMRuntimeLibcallsCoreTest, OptimizedObjectsHaveNoSecondaryCalls) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabi");
  for (const bool HardFloat : {false, true})
    for (const bool Thumb : {false, true}) {
      const ARMObjectConfig Config{HardFloat ? "armv7-unknown-linux-gnueabihf"
                                             : "armv7-unknown-linux-gnueabi",
                                   "cortex-a9",
                                   "+vfp3,-neon,-hwdiv-arm,-hwdiv",
                                   HardFloat,
                                   Thumb,
                                   ARMOptimization::O2};
      const auto Object = makeARMRuntimeObject(Config);
      EXPECT_TRUE(undefinedSymbols(Object).empty()) << describe(Config);
      auto Parsed =
          llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
              llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                              Object.size()),
              "arm-runtime-optimized.o"));
      ASSERT_TRUE(static_cast<bool>(Parsed));

      struct FunctionRange {
        uint64_t Address;
        uint64_t Size;
        uint64_t Section;
      };
      std::vector<FunctionRange> Cores;
      const std::set<std::string> PublicWrappers{
          "__aeabi_l2f",  "__aeabi_ul2f",  "__aeabi_l2d",  "__aeabi_ul2d",
          "__aeabi_f2lz", "__aeabi_f2ulz", "__aeabi_d2lz", "__aeabi_d2ulz",
          "ceil",         "ceilf",         "floor",        "floorf",
          "fmax",         "fmaxf",         "fmin",         "fminf",
          "roundeven",    "roundevenf",    "trunc",        "truncf"};
      for (const auto &Symbol : (*Parsed)->symbols()) {
        auto Name = Symbol.getName();
        auto Type = Symbol.getType();
        ASSERT_TRUE(static_cast<bool>(Name));
        ASSERT_TRUE(static_cast<bool>(Type));
        if (*Type != llvm::object::SymbolRef::ST_Function ||
            (Name->take_front(11) != "__wasmedge_" &&
             PublicWrappers.count(Name->str()) == 0))
          continue;
        auto Address = Symbol.getAddress();
        auto Section = Symbol.getSection();
        ASSERT_TRUE(static_cast<bool>(Address));
        ASSERT_TRUE(static_cast<bool>(Section));
        ASSERT_NE(*Section, (*Parsed)->section_end());
        Cores.push_back({*Address & ~UINT64_C(1),
                         llvm::object::ELFSymbolRef(Symbol).getSize(),
                         (*Section)->getIndex()});
      }
      ASSERT_EQ(Cores.size(), 44U);
      for (const auto &Core : Cores) {
        const auto Section =
            std::find_if((*Parsed)->section_begin(), (*Parsed)->section_end(),
                         [&Core](const auto &Value) {
                           return Value.getIndex() == Core.Section;
                         });
        ASSERT_NE(Section, (*Parsed)->section_end());
        auto Contents = Section->getContents();
        ASSERT_TRUE(static_cast<bool>(Contents));
        ASSERT_LE(Core.Address + Core.Size, Contents->size());
        EXPECT_FALSE(containsARMCallInstruction(
            Contents->substr(Core.Address, Core.Size), Thumb))
            << describe(Config);
      }

      for (const auto &Section : (*Parsed)->sections()) {
        auto Relocated = Section.getRelocatedSection();
        ASSERT_TRUE(static_cast<bool>(Relocated));
        const uint64_t RelocatedSection = *Relocated == (*Parsed)->section_end()
                                              ? Section.getIndex()
                                              : (*Relocated)->getIndex();
        for (const auto &Relocation : Section.relocations()) {
          auto Symbol = Relocation.getSymbol();
          ASSERT_NE(Symbol, (*Parsed)->symbol_end());
          auto Name = Symbol->getName();
          ASSERT_TRUE(static_cast<bool>(Name));
          for (const auto &Core : Cores)
            EXPECT_FALSE(Core.Section == RelocatedSection &&
                         Relocation.getOffset() >= Core.Address &&
                         Relocation.getOffset() < Core.Address + Core.Size)
                << describe(Config) << ": private core calls " << Name->str();
        }
      }
    }
}

} // namespace
