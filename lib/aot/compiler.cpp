// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "aot/compiler.h"

#include "aot/version.h"
#include "common/defines.h"
#include "common/filesystem.h"
#include "common/log.h"
#include "llvm.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <lld/Common/Driver.h>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#if LLVM_VERSION_MAJOR >= 14
#include <lld/Common/CommonLinkerContext.h>
#endif

#if WASMEDGE_OS_MACOS
#include <unistd.h>
#endif
#if WASMEDGE_OS_WINDOWS
#include <llvm/Object/COFF.h>
#endif

#if WASMEDGE_OS_LINUX
#define SYMBOL(X) X
#elif WASMEDGE_OS_MACOS
#define SYMBOL(X) "_" X
#elif WASMEDGE_OS_WINDOWS
#define SYMBOL(X) X
#endif

using namespace std::literals;
namespace LLVM = WasmEdge::AOT::LLVM;

namespace {

// is x86_64
#if defined(_M_X64) && !defined(__x86_64__)
#define __x86_64__ 1
#endif

static bool
isVoidReturn(WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
static LLVM::Type toLLVMType(LLVM::Context &LLContext,
                             const WasmEdge::ValType &ValType) noexcept;
static std::vector<LLVM::Type>
toLLVMArgsType(LLVM::Context &LLContext, LLVM::Type ExecCtxPtrTy,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
static LLVM::Type
toLLVMRetsType(LLVM::Context &LLContext,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
static LLVM::Type
toLLVMType(LLVM::Context &LLContext, LLVM::Type ExecCtxPtrTy,
           const WasmEdge::AST::FunctionType &FuncType) noexcept;
static LLVM::Value
toLLVMConstantZero(LLVM::Context &LLContext,
                   const WasmEdge::ValType &ValType) noexcept;
static std::vector<LLVM::Value> unpackStruct(LLVM::Builder &Builder,
                                             LLVM::Value Struct) noexcept;
class FunctionCompiler;

// XXX: Misalignment handler not implemented yet, forcing unalignment
// force unalignment load/store
static inline constexpr const bool kForceUnalignment = true;

// force checking div/rem on zero
static inline constexpr const bool kForceDivCheck = true;

// Size of a ValVariant
static inline constexpr const uint32_t kValSize = sizeof(WasmEdge::ValVariant);

// Translate Compiler::OptimizationLevel to llvm::PassBuilder version
static inline std::pair<unsigned int, unsigned int>
toLLVMLevel(WasmEdge::CompilerConfigure::OptimizationLevel Level) noexcept {
  using OL = WasmEdge::CompilerConfigure::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return {0, 0};
  case OL::O1:
    return {1, 0};
  case OL::O2:
    return {2, 0};
  case OL::O3:
    return {3, 0};
  case OL::Os:
    return {2, 1};
  case OL::Oz:
    return {2, 2};
  default:
    assumingUnreachable();
  }
}
static inline LLVMCodeGenOptLevel toLLVMCodeGenLevel(
    WasmEdge::CompilerConfigure::OptimizationLevel Level) noexcept {
  using OL = WasmEdge::CompilerConfigure::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return LLVMCodeGenLevelNone;
  case OL::O1:
    return LLVMCodeGenLevelLess;
  case OL::O2:
    return LLVMCodeGenLevelDefault;
  case OL::O3:
    return LLVMCodeGenLevelAggressive;
  case OL::Os:
    return LLVMCodeGenLevelDefault;
  case OL::Oz:
    return LLVMCodeGenLevelDefault;
  default:
    assumingUnreachable();
  }
}

WasmEdge::Expect<void> WriteByte(std::ostream &OS, uint8_t Data) noexcept {
  OS.put(static_cast<char>(Data));
  return {};
}

WasmEdge::Expect<void> WriteU32(std::ostream &OS, uint32_t Data) noexcept {
  do {
    uint8_t Byte = static_cast<uint8_t>(Data & UINT32_C(0x7f));
    Data >>= 7;
    if (Data > UINT32_C(0)) {
      Byte |= UINT8_C(0x80);
    }
    WriteByte(OS, Byte);
  } while (Data > UINT32_C(0));
  return {};
}

WasmEdge::Expect<void> WriteU64(std::ostream &OS, uint64_t Data) noexcept {
  do {
    uint8_t Byte = static_cast<uint8_t>(Data & UINT64_C(0x7f));
    Data >>= 7;
    if (Data > UINT64_C(0)) {
      Byte |= UINT8_C(0x80);
    }
    WriteByte(OS, Byte);
  } while (Data > UINT64_C(0));
  return {};
}

WasmEdge::Expect<void> WriteName(std::ostream &OS,
                                 std::string_view Data) noexcept {
  WriteU32(OS, static_cast<uint32_t>(Data.size()));
  for (const auto C : Data) {
    WriteByte(OS, static_cast<uint8_t>(C));
  }
  return {};
}

inline constexpr bool startsWith(std::string_view Value,
                                 std::string_view Prefix) noexcept {
  return Value.size() >= Prefix.size() &&
         Value.substr(0, Prefix.size()) == Prefix;
}

std::filesystem::path uniquePath(const std::filesystem::path Model) noexcept {
  using size_type = std::filesystem::path::string_type::size_type;
  using value_type = std::filesystem::path::value_type;
  static const auto Hex = "0123456789abcdef"sv;
  std::random_device Device;
  std::default_random_engine Engine(Device());
  std::uniform_int_distribution<size_type> Distribution(0, Hex.size() - 1);
  auto String = Model.native();
  for (size_type N = String.size(), I = 0; I < N; ++I) {
    if (String[I] == static_cast<value_type>('%')) {
      String[I] = static_cast<value_type>(Hex[Distribution(Engine)]);
    }
  }
  return String;
}

std::filesystem::path createTemp(const std::filesystem::path Model) noexcept {
  while (true) {
    auto Result = uniquePath(Model);
    std::error_code Error;
    if (!std::filesystem::exists(Result, Error)) {
      if (Error) {
        return {};
      }
      return Result;
    }
  }
}

} // namespace

struct WasmEdge::AOT::Compiler::CompileContext {
  LLVM::Context &LLContext;
  LLVM::Module &LLModule;
  LLVM::Attribute Cold;
  LLVM::Attribute NoAlias;
  LLVM::Attribute NoInline;
  LLVM::Attribute NoReturn;
  LLVM::Attribute ReadOnly;
  LLVM::Attribute StrictFP;
  LLVM::Attribute NoStackArgProbe;
  LLVM::Type VoidTy;
  LLVM::Type Int8Ty;
  LLVM::Type Int16Ty;
  LLVM::Type Int32Ty;
  LLVM::Type Int64Ty;
  LLVM::Type Int128Ty;
  LLVM::Type FloatTy;
  LLVM::Type DoubleTy;
  LLVM::Type Int8x16Ty;
  LLVM::Type Int16x8Ty;
  LLVM::Type Int32x4Ty;
  LLVM::Type Floatx4Ty;
  LLVM::Type Int64x2Ty;
  LLVM::Type Doublex2Ty;
  LLVM::Type Int128x1Ty;
  LLVM::Type Int8PtrTy;
  LLVM::Type Int32PtrTy;
  LLVM::Type Int64PtrTy;
  LLVM::Type Int128PtrTy;
  LLVM::Type ExecCtxTy;
  LLVM::Type ExecCtxPtrTy;
  LLVM::Type IntrinsicsTableTy;
  LLVM::Type IntrinsicsTablePtrTy;
  LLVM::Message SubtargetFeatures;
  bool IsCustomSection;

#if defined(__x86_64__)
#if defined(__XOP__)
  bool SupportXOP = true;
#else
  bool SupportXOP = false;
#endif

#if defined(__SSE4_1__)
  bool SupportSSE4_1 = true;
#else
  bool SupportSSE4_1 = false;
#endif

#if defined(__SSSE3__)
  bool SupportSSSE3 = true;
#else
  bool SupportSSSE3 = false;
#endif

#if defined(__SSE2__)
  bool SupportSSE2 = true;
#else
  bool SupportSSE2 = false;
#endif
#endif

#if defined(__aarch64__)
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(__ARM_NEON_FP)
  bool SupportNEON = true;
#else
  bool SupportNEON = false;
#endif
#endif

  std::vector<const AST::FunctionType *> FunctionTypes;
  std::vector<LLVM::Value> FunctionWrappers;
  std::vector<std::tuple<uint32_t, LLVM::FunctionCallee,
                         const WasmEdge::AST::CodeSegment *>>
      Functions;
  std::vector<LLVM::Type> Globals;
  LLVM::Value IntrinsicsTable;
  LLVM::FunctionCallee Trap;
  CompileContext(LLVM::Context &C, LLVM::Module &M, bool IsGenericBinary,
                 bool IsCustomSection) noexcept
      : LLContext(C), LLModule(M),
        Cold(LLVM::Attribute::createEnum(C, LLVM::Core::Cold, 1)),
        NoAlias(LLVM::Attribute::createEnum(C, LLVM::Core::NoAlias, 1)),
        NoInline(LLVM::Attribute::createEnum(C, LLVM::Core::NoInline, 1)),
        NoReturn(LLVM::Attribute::createEnum(C, LLVM::Core::NoReturn, 1)),
        ReadOnly(LLVM::Attribute::createEnum(C, LLVM::Core::ReadOnly, 1)),
        StrictFP(LLVM::Attribute::createEnum(C, LLVM::Core::StrictFP, 1)),
        NoStackArgProbe(
            LLVM::Attribute::createString(C, "no-stack-arg-probe"sv, {})),
        VoidTy(LLContext.getVoidTy()), Int8Ty(LLContext.getInt8Ty()),
        Int16Ty(LLContext.getInt16Ty()), Int32Ty(LLContext.getInt32Ty()),
        Int64Ty(LLContext.getInt64Ty()), Int128Ty(LLContext.getInt128Ty()),
        FloatTy(LLContext.getFloatTy()), DoubleTy(LLContext.getDoubleTy()),
        Int8x16Ty(LLVM::Type::getVectorType(Int8Ty, 16)),
        Int16x8Ty(LLVM::Type::getVectorType(Int16Ty, 8)),
        Int32x4Ty(LLVM::Type::getVectorType(Int32Ty, 4)),
        Floatx4Ty(LLVM::Type::getVectorType(FloatTy, 4)),
        Int64x2Ty(LLVM::Type::getVectorType(Int64Ty, 2)),
        Doublex2Ty(LLVM::Type::getVectorType(DoubleTy, 2)),
        Int128x1Ty(LLVM::Type::getVectorType(Int128Ty, 1)),
        Int8PtrTy(Int8Ty.getPointerTo()), Int32PtrTy(Int32Ty.getPointerTo()),
        Int64PtrTy(Int64Ty.getPointerTo()),
        Int128PtrTy(Int128Ty.getPointerTo()),
        ExecCtxTy(LLVM::Type::getStructType(
            "ExecCtx",
            std::initializer_list<LLVM::Type>{
                // Memory
                Int8PtrTy.getPointerTo(),
                // Globals
                Int128PtrTy.getPointerTo(),
                // InstrCount
                Int64PtrTy,
                // CostTable
                LLVM::Type::getArrayType(Int64Ty, UINT16_MAX + 1)
                    .getPointerTo(),
                // Gas
                Int64PtrTy,
                // GasLimit
                Int64Ty,
                // StopToken
                Int32PtrTy,
            })),
        ExecCtxPtrTy(ExecCtxTy.getPointerTo()),
        IntrinsicsTableTy(LLVM::Type::getArrayType(
            Int8PtrTy,
            static_cast<uint32_t>(AST::Module::Intrinsics::kIntrinsicMax))),
        IntrinsicsTablePtrTy(IntrinsicsTableTy.getPointerTo()),
        IsCustomSection(IsCustomSection),
        IntrinsicsTable(LLModule.addGlobal(IntrinsicsTablePtrTy, true,
                                           LLVMExternalLinkage, LLVM::Value(),
                                           "intrinsics")) {
    Trap.Ty = LLVM::Type::getFunctionType(VoidTy, {Int32Ty});
    Trap.Fn = LLModule.addFunction(Trap.Ty, LLVMPrivateLinkage, "trap");
    Trap.Fn.setVisibility(LLVMProtectedVisibility);
    Trap.Fn.setDSOLocal(true);
    Trap.Fn.addFnAttr(NoStackArgProbe);
    Trap.Fn.addFnAttr(StrictFP);
    Trap.Fn.addFnAttr(NoReturn);
    Trap.Fn.addFnAttr(Cold);
    Trap.Fn.addFnAttr(NoInline);

    LLModule.addGlobal(Int32Ty, true, LLVMExternalLinkage,
                       LLVM::Value::getConstInt(Int32Ty, kBinaryVersion),
                       "version");

    if (!IsGenericBinary) {
      SubtargetFeatures = LLVM::getHostCPUFeatures();
      auto Features = SubtargetFeatures.string_view();
      while (!Features.empty()) {
        std::string_view Feature;
        if (auto Pos = Features.find(','); Pos != std::string_view::npos) {
          Feature = Features.substr(0, Pos);
          Features = Features.substr(Pos + 1);
        } else {
          Feature = std::exchange(Features, std::string_view());
        }
        if (Feature[0] != '+') {
          continue;
        }
        Feature = Feature.substr(1);

#if defined(__x86_64__)
        if (!SupportXOP && Feature == "xop"sv) {
          SupportXOP = true;
        }
        if (!SupportSSE4_1 && Feature == "sse4.1"sv) {
          SupportSSE4_1 = true;
        }
        if (!SupportSSSE3 && Feature == "ssse3"sv) {
          SupportSSSE3 = true;
        }
        if (!SupportSSE2 && Feature == "sse2"sv) {
          SupportSSE2 = true;
        }
#elif defined(__aarch64__)
        if (!SupportNEON && Feature == "neon"sv) {
          SupportNEON = true;
        }
#endif
      }
    }

    {
      // create trap
      LLVM::Builder Builder(LLContext);
      Builder.positionAtEnd(
          LLVM::BasicBlock::create(LLContext, Trap.Fn, "entry"));
      auto FnTy = LLVM::Type::getFunctionType(VoidTy, {Int32Ty});
      auto CallTrap = Builder.createCall(
          getIntrinsic(Builder, AST::Module::Intrinsics::kTrap, FnTy),
          {Trap.Fn.getFirstParam()});
      CallTrap.addCallSiteAttribute(NoReturn);
      Builder.createUnreachable();
    }
  }
  LLVM::Value getMemory(LLVM::Builder &Builder, LLVM::Value ExecCtx,
                        uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ExecCtx, 0);
    auto VPtr = Builder.createLoad(
        Int8PtrTy, Builder.createInBoundsGEP1(Int8PtrTy, Array,
                                              LLContext.getInt64(Index)));
    return Builder.createBitCast(VPtr, Int8PtrTy);
  }
  std::pair<LLVM::Type, LLVM::Value> getGlobal(LLVM::Builder &Builder,
                                               LLVM::Value ExecCtx,
                                               uint32_t Index) noexcept {
    auto Ty = Globals[Index];
    auto Array = Builder.createExtractValue(ExecCtx, 1);
    auto VPtr = Builder.createLoad(
        Int128PtrTy, Builder.createInBoundsGEP1(Int8PtrTy, Array,
                                                LLContext.getInt64(Index)));
    auto Ptr = Builder.createBitCast(VPtr, Ty.getPointerTo());
    return {Ty, Ptr};
  }
  LLVM::Value getInstrCount(LLVM::Builder &Builder,
                            LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 2);
  }
  LLVM::Value getCostTable(LLVM::Builder &Builder,
                           LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 3);
  }
  LLVM::Value getGas(LLVM::Builder &Builder, LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 4);
  }
  LLVM::Value getGasLimit(LLVM::Builder &Builder,
                          LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 5);
  }
  LLVM::Value getStopToken(LLVM::Builder &Builder,
                           LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 6);
  }
  LLVM::FunctionCallee getIntrinsic(LLVM::Builder &Builder,
                                    AST::Module::Intrinsics Index,
                                    LLVM::Type Ty) noexcept {
    const auto Value = static_cast<uint32_t>(Index);
    auto PtrTy = Ty.getPointerTo();
    auto PtrPtrTy = PtrTy.getPointerTo();
    auto IT = Builder.createLoad(IntrinsicsTablePtrTy, IntrinsicsTable);
    IT.setMetadata(LLContext, LLVM::Core::InvariantLoad,
                   LLVM::Metadata(LLContext, {}));
    auto VPtr =
        Builder.createInBoundsGEP2(IntrinsicsTableTy, IT, LLContext.getInt64(0),
                                   LLContext.getInt64(Value));
    auto Ptr = Builder.createBitCast(VPtr, PtrPtrTy);
    return {Ty, Builder.createLoad(PtrTy, Ptr)};
  }
  std::pair<std::vector<ValType>, std::vector<ValType>>
  resolveBlockType(const BlockType &BType) const noexcept {
    using VecT = std::vector<ValType>;
    using RetT = std::pair<VecT, VecT>;
    if (BType.isEmpty()) {
      return RetT{};
    }
    if (BType.isValType()) {
      return RetT{{}, {BType.Data.Type}};
    } else {
      // Type index case. t2* = type[index].returns
      const uint32_t TypeIdx = BType.Data.Idx;
      const auto &FType = *FunctionTypes[TypeIdx];
      return RetT{
          VecT(FType.getParamTypes().begin(), FType.getParamTypes().end()),
          VecT(FType.getReturnTypes().begin(), FType.getReturnTypes().end())};
    }
  }
};

namespace {

using namespace WasmEdge;

static bool isVoidReturn(Span<const WasmEdge::ValType> ValTypes) noexcept {
  return ValTypes.empty();
}

static LLVM::Type toLLVMType(LLVM::Context &LLContext,
                             const ValType &ValType) noexcept {
  switch (ValType) {
  case ValType::I32:
    return LLContext.getInt32Ty();
  case ValType::I64:
  case ValType::FuncRef:
  case ValType::ExternRef:
    return LLContext.getInt64Ty();
  case ValType::V128:
    return LLVM::Type::getVectorType(LLContext.getInt64Ty(), 2);
  case ValType::F32:
    return LLContext.getFloatTy();
  case ValType::F64:
    return LLContext.getDoubleTy();
  default:
    assumingUnreachable();
  }
}

static std::vector<LLVM::Type>
toLLVMTypeVector(LLVM::Context &LLContext,
                 Span<const ValType> ValTypes) noexcept {
  std::vector<LLVM::Type> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(LLContext, Type));
  }
  return Result;
}

static std::vector<LLVM::Type>
toLLVMArgsType(LLVM::Context &LLContext, LLVM::Type ExecCtxPtrTy,
               Span<const ValType> ValTypes) noexcept {
  auto Result = toLLVMTypeVector(LLContext, ValTypes);
  Result.insert(Result.begin(), ExecCtxPtrTy);
  return Result;
}

static LLVM::Type toLLVMRetsType(LLVM::Context &LLContext,
                                 Span<const ValType> ValTypes) noexcept {
  if (isVoidReturn(ValTypes)) {
    return LLContext.getVoidTy();
  }
  if (ValTypes.size() == 1) {
    return toLLVMType(LLContext, ValTypes.front());
  }
  std::vector<LLVM::Type> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(LLContext, Type));
  }
  return LLVM::Type::getStructType(Result);
}

static LLVM::Type toLLVMType(LLVM::Context &LLContext, LLVM::Type ExecCtxPtrTy,
                             const AST::FunctionType &FuncType) noexcept {
  auto ArgsTy =
      toLLVMArgsType(LLContext, ExecCtxPtrTy, FuncType.getParamTypes());
  auto RetTy = toLLVMRetsType(LLContext, FuncType.getReturnTypes());
  return LLVM::Type::getFunctionType(RetTy, ArgsTy);
}

static LLVM::Value toLLVMConstantZero(LLVM::Context &LLContext,
                                      const ValType &ValType) noexcept {
  switch (ValType) {
  case ValType::I32:
    return LLVM::Value::getConstNull(LLContext.getInt32Ty());
  case ValType::I64:
  case ValType::FuncRef:
  case ValType::ExternRef:
    return LLVM::Value::getConstNull(LLContext.getInt64Ty());
  case ValType::V128:
    return LLVM::Value::getConstNull(
        LLVM::Type::getVectorType(LLContext.getInt64Ty(), 2));
  case ValType::F32:
    return LLVM::Value::getConstNull(LLContext.getFloatTy());
  case ValType::F64:
    return LLVM::Value::getConstNull(LLContext.getDoubleTy());
  default:
    assumingUnreachable();
  }
}

class FunctionCompiler {
  struct Control;

public:
  FunctionCompiler(AOT::Compiler::CompileContext &Context,
                   LLVM::FunctionCallee F, Span<const ValType> Locals,
                   bool Interruptible, bool InstructionCounting,
                   bool GasMeasuring, bool OptNone) noexcept
      : Context(Context), LLContext(Context.LLContext),
        Interruptible(Interruptible), OptNone(OptNone), F(F),
        Builder(LLContext) {
    if (F.Fn) {
      Builder.positionAtEnd(LLVM::BasicBlock::create(LLContext, F.Fn, "entry"));
      ExecCtx = Builder.createLoad(Context.ExecCtxTy, F.Fn.getFirstParam());

      if (InstructionCounting) {
        LocalInstrCount = Builder.createAlloca(Context.Int64Ty);
        Builder.createStore(LLContext.getInt64(0), LocalInstrCount);
      }

      if (GasMeasuring) {
        LocalGas = Builder.createAlloca(Context.Int64Ty);
        Builder.createStore(LLContext.getInt64(0), LocalGas);
      }

      for (LLVM::Value Arg = F.Fn.getFirstParam().getNextParam(); Arg;
           Arg = Arg.getNextParam()) {
        LLVM::Type Ty = Arg.getType();
        LLVM::Value ArgPtr = Builder.createAlloca(Ty);
        Builder.createStore(Arg, ArgPtr);
        Local.emplace_back(Ty, ArgPtr);
      }

      for (const auto &Type : Locals) {
        LLVM::Type Ty = toLLVMType(LLContext, Type);
        LLVM::Value ArgPtr = Builder.createAlloca(Ty);
        Builder.createStore(toLLVMConstantZero(LLContext, Type), ArgPtr);
        Local.emplace_back(Ty, ArgPtr);
      }
    }
  }

  LLVM::BasicBlock getTrapBB(ErrCode::Value Error) noexcept {
    if (auto Iter = TrapBB.find(Error); Iter != TrapBB.end()) {
      return Iter->second;
    }
    auto BB = LLVM::BasicBlock::create(LLContext, F.Fn, "trap");
    TrapBB.emplace(Error, BB);
    return BB;
  }

  void
  compile(const AST::CodeSegment &Code,
          std::pair<std::vector<ValType>, std::vector<ValType>> Type) noexcept {
    auto RetBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ret");
    Type.first.clear();
    enterBlock(RetBB, {}, {}, {}, std::move(Type));
    compile(Code.getExpr().getInstrs());
    assuming(ControlStack.empty());
    compileReturn();

    for (auto &[Error, BB] : TrapBB) {
      Builder.positionAtEnd(BB);
      updateInstrCount();
      updateGasAtTrap();
      auto CallTrap = Builder.createCall(
          Context.Trap, {LLContext.getInt32(static_cast<uint32_t>(Error))});
      CallTrap.addCallSiteAttribute(Context.NoReturn);
      Builder.createUnreachable();
    }
  }

  void compile(AST::InstrView Instrs) noexcept {
    auto Dispatch = [this](const AST::Instruction &Instr) -> void {
      switch (Instr.getOpCode()) {
      case OpCode::Block: {
        auto Block = LLVM::BasicBlock::create(LLContext, F.Fn, "block");
        auto EndBlock = LLVM::BasicBlock::create(LLContext, F.Fn, "block.end");
        Builder.createBr(Block);

        Builder.positionAtEnd(Block);
        auto Type = Context.resolveBlockType(Instr.getBlockType());
        const auto Arity = Type.first.size();
        std::vector<LLVM::Value> Args(Arity);
        if (isUnreachable()) {
          for (size_t I = 0; I < Arity; ++I) {
            auto Ty = toLLVMType(LLContext, Type.first[I]);
            Args[I] = LLVM::Value::getUndef(Ty);
          }
        } else {
          for (size_t I = 0; I < Arity; ++I) {
            const size_t J = Arity - 1 - I;
            Args[J] = stackPop();
          }
        }
        enterBlock(EndBlock, {}, {}, std::move(Args), std::move(Type));
        checkStop();
        updateGas();
        return;
      }
      case OpCode::Loop: {
        auto Curr = Builder.getInsertBlock();
        auto Loop = LLVM::BasicBlock::create(LLContext, F.Fn, "loop");
        auto EndLoop = LLVM::BasicBlock::create(LLContext, F.Fn, "loop.end");
        Builder.createBr(Loop);

        Builder.positionAtEnd(Loop);
        auto Type = Context.resolveBlockType(Instr.getBlockType());
        const auto Arity = Type.first.size();
        std::vector<LLVM::Value> Args(Arity);
        if (isUnreachable()) {
          for (size_t I = 0; I < Arity; ++I) {
            auto Ty = toLLVMType(LLContext, Type.first[I]);
            auto Value = LLVM::Value::getUndef(Ty);
            auto PHINode = Builder.createPHI(Ty);
            PHINode.addIncoming(Value, Curr);
            Args[I] = PHINode;
          }
        } else {
          for (size_t I = 0; I < Arity; ++I) {
            const size_t J = Arity - 1 - I;
            auto Value = stackPop();
            auto PHINode = Builder.createPHI(Value.getType());
            PHINode.addIncoming(Value, Curr);
            Args[J] = PHINode;
          }
        }
        enterBlock(Loop, EndLoop, {}, std::move(Args), std::move(Type));
        checkStop();
        updateGas();
        return;
      }
      case OpCode::If: {
        auto Then = LLVM::BasicBlock::create(LLContext, F.Fn, "then");
        auto Else = LLVM::BasicBlock::create(LLContext, F.Fn, "else");
        auto EndIf = LLVM::BasicBlock::create(LLContext, F.Fn, "if.end");
        LLVM::Value Cond;
        if (isUnreachable()) {
          Cond = LLVM::Value::getUndef(LLContext.getInt1Ty());
        } else {
          Cond = Builder.createICmpNE(stackPop(), LLContext.getInt32(0));
        }
        Builder.createCondBr(Cond, Then, Else);

        Builder.positionAtEnd(Then);
        auto Type = Context.resolveBlockType(Instr.getBlockType());
        const auto Arity = Type.first.size();
        std::vector<LLVM::Value> Args(Arity);
        if (isUnreachable()) {
          for (size_t I = 0; I < Arity; ++I) {
            auto Ty = toLLVMType(LLContext, Type.first[I]);
            Args[I] = LLVM::Value::getUndef(Ty);
          }
        } else {
          for (size_t I = 0; I < Arity; ++I) {
            const size_t J = Arity - 1 - I;
            Args[J] = stackPop();
          }
        }
        enterBlock(EndIf, {}, Else, std::move(Args), std::move(Type));
        return;
      }
      case OpCode::End: {
        auto Entry = leaveBlock();
        if (Entry.ElseBlock) {
          auto Block = Builder.getInsertBlock();
          Builder.positionAtEnd(Entry.ElseBlock);
          enterBlock(Block, {}, {}, std::move(Entry.Args),
                     std::move(Entry.Type), std::move(Entry.ReturnPHI));
          Entry = leaveBlock();
        }
        buildPHI(Entry.Type.second, Entry.ReturnPHI);
        return;
      }
      case OpCode::Else: {
        auto Entry = leaveBlock();
        Builder.positionAtEnd(Entry.ElseBlock);
        enterBlock(Entry.JumpBlock, {}, {}, std::move(Entry.Args),
                   std::move(Entry.Type), std::move(Entry.ReturnPHI));
        return;
      }
      default:
        break;
      }

      if (isUnreachable()) {
        return;
      }

      switch (Instr.getOpCode()) {
      case OpCode::Unreachable:
        Builder.createBr(getTrapBB(ErrCode::Value::Unreachable));
        setUnreachable();
        Builder.positionAtEnd(
            LLVM::BasicBlock::create(LLContext, F.Fn, "unreachable.end"));
        break;
      case OpCode::Nop:
        break;
      case OpCode::Return:
        compileReturn();
        setUnreachable();
        Builder.positionAtEnd(
            LLVM::BasicBlock::create(LLContext, F.Fn, "ret.end"));
        break;
      case OpCode::Br: {
        const auto Label = Instr.getJump().TargetIndex;
        setLableJumpPHI(Label);
        Builder.createBr(getLabel(Label));
        setUnreachable();
        Builder.positionAtEnd(
            LLVM::BasicBlock::create(LLContext, F.Fn, "br.end"));
        break;
      }
      case OpCode::Br_if: {
        const auto Label = Instr.getJump().TargetIndex;
        auto Cond = Builder.createICmpNE(stackPop(), LLContext.getInt32(0));
        setLableJumpPHI(Label);
        auto Next = LLVM::BasicBlock::create(LLContext, F.Fn, "br_if.end");
        Builder.createCondBr(Cond, getLabel(Label), Next);
        Builder.positionAtEnd(Next);
        break;
      }
      case OpCode::Br_table: {
        auto LabelTable = Instr.getLabelList();
        assuming(LabelTable.size() <= std::numeric_limits<uint32_t>::max());
        const auto LabelTableSize =
            static_cast<uint32_t>(LabelTable.size() - 1);
        auto Value = stackPop();
        setLableJumpPHI(LabelTable[LabelTableSize].TargetIndex);
        auto Switch = Builder.createSwitch(
            Value, getLabel(LabelTable[LabelTableSize].TargetIndex),
            LabelTableSize);
        for (uint32_t I = 0; I < LabelTableSize; ++I) {
          setLableJumpPHI(LabelTable[I].TargetIndex);
          Switch.addCase(LLContext.getInt32(I),
                         getLabel(LabelTable[I].TargetIndex));
        }
        setUnreachable();
        Builder.positionAtEnd(
            LLVM::BasicBlock::create(LLContext, F.Fn, "br_table.end"));
        break;
      }
      case OpCode::Call:
        updateInstrCount();
        updateGas();
        compileCallOp(Instr.getTargetIndex());
        break;
      case OpCode::Call_indirect:
        updateInstrCount();
        updateGas();
        compileIndirectCallOp(Instr.getSourceIndex(), Instr.getTargetIndex());
        break;
      case OpCode::Return_call:
        updateInstrCount();
        updateGas();
        compileReturnCallOp(Instr.getTargetIndex());
        setUnreachable();
        Builder.positionAtEnd(
            LLVM::BasicBlock::create(LLContext, F.Fn, "ret_call.end"));
        break;
      case OpCode::Return_call_indirect:
        updateInstrCount();
        updateGas();
        compileReturnIndirectCallOp(Instr.getSourceIndex(),
                                    Instr.getTargetIndex());
        setUnreachable();
        Builder.positionAtEnd(
            LLVM::BasicBlock::create(LLContext, F.Fn, "ret_call_indir.end"));
        break;
      case OpCode::Ref__null:
        stackPush(LLContext.getInt64(0));
        break;
      case OpCode::Ref__is_null:
        stackPush(Builder.createZExt(
            Builder.createICmpEQ(stackPop(), LLContext.getInt64(0)),
            Context.Int32Ty));
        break;
      case OpCode::Ref__func:
        stackPush(Builder.createCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kRefFunc,
                                 LLVM::Type::getFunctionType(Context.Int64Ty,
                                                             {Context.Int32Ty},
                                                             false)),
            {LLContext.getInt32(Instr.getTargetIndex())}));
        break;
      case OpCode::Drop:
        stackPop();
        break;
      case OpCode::Select:
      case OpCode::Select_t: {
        auto Cond = Builder.createICmpNE(stackPop(), LLContext.getInt32(0));
        auto False = stackPop();
        auto True = stackPop();
        stackPush(Builder.createSelect(Cond, True, False));
        break;
      }
      case OpCode::Local__get: {
        const auto &L = Local[Instr.getTargetIndex()];
        stackPush(Builder.createLoad(L.first, L.second));
        break;
      }
      case OpCode::Local__set:
        Builder.createStore(stackPop(), Local[Instr.getTargetIndex()].second);
        break;
      case OpCode::Local__tee:
        Builder.createStore(Stack.back(), Local[Instr.getTargetIndex()].second);
        break;
      case OpCode::Global__get: {
        const auto G =
            Context.getGlobal(Builder, ExecCtx, Instr.getTargetIndex());
        stackPush(Builder.createLoad(G.first, G.second));
        break;
      }
      case OpCode::Global__set:
        Builder.createStore(
            stackPop(),
            Context.getGlobal(Builder, ExecCtx, Instr.getTargetIndex()).second);
        break;
      case OpCode::Table__get: {
        auto Idx = stackPop();
        stackPush(Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableGet,
                LLVM::Type::getFunctionType(Context.Int64Ty,
                                            {Context.Int32Ty, Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()), Idx}));
        break;
      }
      case OpCode::Table__set: {
        auto Ref = stackPop();
        auto Idx = stackPop();
        Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableSet,
                LLVM::Type::getFunctionType(
                    Context.Int64Ty,
                    {Context.Int32Ty, Context.Int32Ty, Context.Int64Ty},
                    false)),
            {LLContext.getInt32(Instr.getTargetIndex()), Idx, Ref});
        break;
      }
      case OpCode::Table__init: {
        auto Len = stackPop();
        auto Src = stackPop();
        auto Dst = stackPop();
        Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableInit,
                LLVM::Type::getFunctionType(Context.VoidTy,
                                            {Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()),
             LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Elem__drop: {
        Builder.createCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kElemDrop,
                                 LLVM::Type::getFunctionType(
                                     Context.VoidTy, {Context.Int32Ty}, false)),
            {LLContext.getInt32(Instr.getTargetIndex())});
        break;
      }
      case OpCode::Table__copy: {
        auto Len = stackPop();
        auto Src = stackPop();
        auto Dst = stackPop();
        Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableCopy,
                LLVM::Type::getFunctionType(Context.VoidTy,
                                            {Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()),
             LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Table__grow: {
        auto NewSize = stackPop();
        auto Val = stackPop();
        stackPush(Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableGrow,
                LLVM::Type::getFunctionType(
                    Context.Int32Ty,
                    {Context.Int32Ty, Context.Int64Ty, Context.Int32Ty},
                    false)),
            {LLContext.getInt32(Instr.getTargetIndex()), Val, NewSize}));
        break;
      }
      case OpCode::Table__size: {
        stackPush(Builder.createCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kTableSize,
                                 LLVM::Type::getFunctionType(Context.Int32Ty,
                                                             {Context.Int32Ty},
                                                             false)),
            {LLContext.getInt32(Instr.getTargetIndex())}));
        break;
      }
      case OpCode::Table__fill: {
        auto Len = stackPop();
        auto Val = stackPop();
        auto Off = stackPop();
        Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableFill,
                LLVM::Type::getFunctionType(Context.Int32Ty,
                                            {Context.Int32Ty, Context.Int32Ty,
                                             Context.Int64Ty, Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()), Off, Val, Len});
        break;
      }
      case OpCode::I32__load:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int32Ty);
        break;
      case OpCode::I64__load:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int64Ty);
        break;
      case OpCode::F32__load:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.FloatTy);
        break;
      case OpCode::F64__load:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.DoubleTy);
        break;
      case OpCode::I32__load8_s:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int8Ty, Context.Int32Ty,
                      true);
        break;
      case OpCode::I32__load8_u:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int8Ty, Context.Int32Ty,
                      false);
        break;
      case OpCode::I32__load16_s:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int16Ty, Context.Int32Ty,
                      true);
        break;
      case OpCode::I32__load16_u:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int16Ty, Context.Int32Ty,
                      false);
        break;
      case OpCode::I64__load8_s:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int8Ty, Context.Int64Ty,
                      true);
        break;
      case OpCode::I64__load8_u:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int8Ty, Context.Int64Ty,
                      false);
        break;
      case OpCode::I64__load16_s:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int16Ty, Context.Int64Ty,
                      true);
        break;
      case OpCode::I64__load16_u:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int16Ty, Context.Int64Ty,
                      false);
        break;
      case OpCode::I64__load32_s:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int32Ty, Context.Int64Ty,
                      true);
        break;
      case OpCode::I64__load32_u:
        compileLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int32Ty, Context.Int64Ty,
                      false);
        break;

      case OpCode::I32__store:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int32Ty);
        break;
      case OpCode::I64__store:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int64Ty);
        break;
      case OpCode::F32__store:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.FloatTy);
        break;
      case OpCode::F64__store:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.DoubleTy);
        break;
      case OpCode::I32__store8:
      case OpCode::I64__store8:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int8Ty, true);
        break;
      case OpCode::I32__store16:
      case OpCode::I64__store16:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int16Ty, true);
        break;
      case OpCode::I64__store32:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int32Ty, true);
        break;
      case OpCode::Memory__size:
        stackPush(Builder.createCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kMemSize,
                                 LLVM::Type::getFunctionType(Context.Int32Ty,
                                                             {Context.Int32Ty},
                                                             false)),
            {LLContext.getInt32(Instr.getTargetIndex())}));
        break;
      case OpCode::Memory__grow: {
        auto Diff = stackPop();
        stackPush(Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemGrow,
                LLVM::Type::getFunctionType(Context.Int32Ty,
                                            {Context.Int32Ty, Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()), Diff}));
        break;
      }
      case OpCode::Memory__init: {
        auto Len = stackPop();
        auto Src = stackPop();
        auto Dst = stackPop();
        Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemInit,
                LLVM::Type::getFunctionType(Context.VoidTy,
                                            {Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()),
             LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Data__drop: {
        Builder.createCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kDataDrop,
                                 LLVM::Type::getFunctionType(
                                     Context.VoidTy, {Context.Int32Ty}, false)),
            {LLContext.getInt32(Instr.getTargetIndex())});
        break;
      }
      case OpCode::Memory__copy: {
        auto Len = stackPop();
        auto Src = stackPop();
        auto Dst = stackPop();
        Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemCopy,
                LLVM::Type::getFunctionType(Context.VoidTy,
                                            {Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty, Context.Int32Ty,
                                             Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()),
             LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Memory__fill: {
        auto Len = stackPop();
        auto Val = Builder.createTrunc(stackPop(), Context.Int8Ty);
        auto Off = stackPop();
        Builder.createCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemFill,
                LLVM::Type::getFunctionType(Context.VoidTy,
                                            {Context.Int32Ty, Context.Int32Ty,
                                             Context.Int8Ty, Context.Int32Ty},
                                            false)),
            {LLContext.getInt32(Instr.getTargetIndex()), Off, Val, Len});
        break;
      }
      case OpCode::I32__const:
        stackPush(LLContext.getInt32(Instr.getNum().get<uint32_t>()));
        break;
      case OpCode::I64__const:
        stackPush(LLContext.getInt64(Instr.getNum().get<uint64_t>()));
        break;
      case OpCode::F32__const:
        stackPush(LLContext.getFloat(Instr.getNum().get<float>()));
        break;
      case OpCode::F64__const:
        stackPush(LLContext.getDouble(Instr.getNum().get<double>()));
        break;
      case OpCode::I32__eqz:
        stackPush(Builder.createZExt(
            Builder.createICmpEQ(stackPop(), LLContext.getInt32(0)),
            Context.Int32Ty));
        break;
      case OpCode::I64__eqz:
        stackPush(Builder.createZExt(
            Builder.createICmpEQ(stackPop(), LLContext.getInt64(0)),
            Context.Int32Ty));
        break;
      case OpCode::I32__clz:
        assuming(LLVM::Core::Ctlz != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::Ctlz, {Context.Int32Ty},
                                          {stackPop(), LLContext.getFalse()}));
        break;
      case OpCode::I64__clz:
        assuming(LLVM::Core::Ctlz != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::Ctlz, {Context.Int64Ty},
                                          {stackPop(), LLContext.getFalse()}));
        break;
      case OpCode::I32__ctz:
        assuming(LLVM::Core::Cttz != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::Cttz, {Context.Int32Ty},
                                          {stackPop(), LLContext.getFalse()}));
        break;
      case OpCode::I64__ctz:
        assuming(LLVM::Core::Cttz != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::Cttz, {Context.Int64Ty},
                                          {stackPop(), LLContext.getFalse()}));
        break;
      case OpCode::I32__popcnt:
      case OpCode::I64__popcnt:
        assuming(LLVM::Core::Ctpop != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Ctpop, stackPop()));
        break;
      case OpCode::F32__abs:
      case OpCode::F64__abs:
        assuming(LLVM::Core::Fabs != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Fabs, stackPop()));
        break;
      case OpCode::F32__neg:
      case OpCode::F64__neg:
        stackPush(Builder.createFNeg(stackPop()));
        break;
      case OpCode::F32__ceil:
      case OpCode::F64__ceil:
        assuming(LLVM::Core::Ceil != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Ceil, stackPop()));
        break;
      case OpCode::F32__floor:
      case OpCode::F64__floor:
        assuming(LLVM::Core::Floor != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Floor, stackPop()));
        break;
      case OpCode::F32__trunc:
      case OpCode::F64__trunc:
        assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Trunc, stackPop()));
        break;
      case OpCode::F32__nearest:
      case OpCode::F64__nearest: {
        const bool IsFloat = Instr.getOpCode() == OpCode::F32__nearest;
        LLVM::Value Value = stackPop();

#if LLVM_VERSION_MAJOR >= 12
        assuming(LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic);
        if (LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic) {
          stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Roundeven, Value));
          break;
        }
#endif

        // The VectorSize is only used when SSE4_1 or NEON is supported.
        [[maybe_unused]] const uint32_t VectorSize = IsFloat ? 4 : 2;
#if defined(__x86_64__)
        if (Context.SupportSSE4_1) {
          auto Zero = LLContext.getInt64(0);
          auto VectorTy =
              LLVM::Type::getVectorType(Value.getType(), VectorSize);
          LLVM::Value Ret = LLVM::Value::getUndef(VectorTy);
          Ret = Builder.createInsertElement(Ret, Value, Zero);
          auto ID = IsFloat ? LLVM::Core::X86SSE41RoundSs
                            : LLVM::Core::X86SSE41RoundSd;
          assuming(ID != LLVM::Core::NotIntrinsic);
          Ret = Builder.createIntrinsic(ID, {},
                                        {Ret, Ret, LLContext.getInt32(8)});
          Ret = Builder.createExtractElement(Ret, Zero);
          stackPush(Ret);
          break;
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON &&
            LLVM::Core::AArch64NeonFRIntN != LLVM::Core::NotIntrinsic) {
          auto Zero = LLContext.getInt64(0);
          auto VectorTy =
              LLVM::Type::getVectorType(Value.getType(), VectorSize);
          LLVM::Value Ret = LLVM::Value::getUndef(VectorTy);
          Ret = Builder.createInsertElement(Ret, Value, Zero);
          Ret =
              Builder.createUnaryIntrinsic(LLVM::Core::AArch64NeonFRIntN, Ret);
          Ret = Builder.createExtractElement(Ret, Zero);
          stackPush(Ret);
          break;
        }
#endif

        // Fallback case.
        // If the SSE4.1 is not supported on the x86_64 platform or
        // the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        assuming(LLVM::Core::Nearbyint != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Nearbyint, Value));
        break;
      }
      case OpCode::F32__sqrt:
      case OpCode::F64__sqrt:
        assuming(LLVM::Core::Sqrt != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Sqrt, stackPop()));
        break;
      case OpCode::I32__wrap_i64:
        stackPush(Builder.createTrunc(stackPop(), Context.Int32Ty));
        break;
      case OpCode::I32__trunc_f32_s:
        compileSignedTrunc(Context.Int32Ty);
        break;
      case OpCode::I32__trunc_f64_s:
        compileSignedTrunc(Context.Int32Ty);
        break;
      case OpCode::I32__trunc_f32_u:
        compileUnsignedTrunc(Context.Int32Ty);
        break;
      case OpCode::I32__trunc_f64_u:
        compileUnsignedTrunc(Context.Int32Ty);
        break;
      case OpCode::I64__extend_i32_s:
        stackPush(Builder.createSExt(stackPop(), Context.Int64Ty));
        break;
      case OpCode::I64__extend_i32_u:
        stackPush(Builder.createZExt(stackPop(), Context.Int64Ty));
        break;
      case OpCode::I64__trunc_f32_s:
        compileSignedTrunc(Context.Int64Ty);
        break;
      case OpCode::I64__trunc_f64_s:
        compileSignedTrunc(Context.Int64Ty);
        break;
      case OpCode::I64__trunc_f32_u:
        compileUnsignedTrunc(Context.Int64Ty);
        break;
      case OpCode::I64__trunc_f64_u:
        compileUnsignedTrunc(Context.Int64Ty);
        break;
      case OpCode::F32__convert_i32_s:
      case OpCode::F32__convert_i64_s:
        stackPush(Builder.createSIToFP(stackPop(), Context.FloatTy));
        break;
      case OpCode::F32__convert_i32_u:
      case OpCode::F32__convert_i64_u:
        stackPush(Builder.createUIToFP(stackPop(), Context.FloatTy));
        break;
      case OpCode::F64__convert_i32_s:
      case OpCode::F64__convert_i64_s:
        stackPush(Builder.createSIToFP(stackPop(), Context.DoubleTy));
        break;
      case OpCode::F64__convert_i32_u:
      case OpCode::F64__convert_i64_u:
        stackPush(Builder.createUIToFP(stackPop(), Context.DoubleTy));
        break;
      case OpCode::F32__demote_f64:
        stackPush(Builder.createFPTrunc(stackPop(), Context.FloatTy));
        break;
      case OpCode::F64__promote_f32:
        stackPush(Builder.createFPExt(stackPop(), Context.DoubleTy));
        break;
      case OpCode::I32__reinterpret_f32:
        stackPush(Builder.createBitCast(stackPop(), Context.Int32Ty));
        break;
      case OpCode::I64__reinterpret_f64:
        stackPush(Builder.createBitCast(stackPop(), Context.Int64Ty));
        break;
      case OpCode::F32__reinterpret_i32:
        stackPush(Builder.createBitCast(stackPop(), Context.FloatTy));
        break;
      case OpCode::F64__reinterpret_i64:
        stackPush(Builder.createBitCast(stackPop(), Context.DoubleTy));
        break;
      case OpCode::I32__extend8_s:
        stackPush(Builder.createSExt(
            Builder.createTrunc(stackPop(), Context.Int8Ty), Context.Int32Ty));
        break;
      case OpCode::I32__extend16_s:
        stackPush(Builder.createSExt(
            Builder.createTrunc(stackPop(), Context.Int16Ty), Context.Int32Ty));
        break;
      case OpCode::I64__extend8_s:
        stackPush(Builder.createSExt(
            Builder.createTrunc(stackPop(), Context.Int8Ty), Context.Int64Ty));
        break;
      case OpCode::I64__extend16_s:
        stackPush(Builder.createSExt(
            Builder.createTrunc(stackPop(), Context.Int16Ty), Context.Int64Ty));
        break;
      case OpCode::I64__extend32_s:
        stackPush(Builder.createSExt(
            Builder.createTrunc(stackPop(), Context.Int32Ty), Context.Int64Ty));
        break;
      case OpCode::I32__trunc_sat_f32_s:
        compileSignedTruncSat(Context.Int32Ty);
        break;
      case OpCode::I32__trunc_sat_f32_u:
        compileUnsignedTruncSat(Context.Int32Ty);
        break;
      case OpCode::I32__trunc_sat_f64_s:
        compileSignedTruncSat(Context.Int32Ty);
        break;
      case OpCode::I32__trunc_sat_f64_u:
        compileUnsignedTruncSat(Context.Int32Ty);
        break;
      case OpCode::I64__trunc_sat_f32_s:
        compileSignedTruncSat(Context.Int64Ty);
        break;
      case OpCode::I64__trunc_sat_f32_u:
        compileUnsignedTruncSat(Context.Int64Ty);
        break;
      case OpCode::I64__trunc_sat_f64_s:
        compileSignedTruncSat(Context.Int64Ty);
        break;
      case OpCode::I64__trunc_sat_f64_u:
        compileUnsignedTruncSat(Context.Int64Ty);
        break;
      case OpCode::I32__eq:
      case OpCode::I64__eq: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpEQ(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__ne:
      case OpCode::I64__ne: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpNE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__lt_s:
      case OpCode::I64__lt_s: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpSLT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__lt_u:
      case OpCode::I64__lt_u: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpULT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__gt_s:
      case OpCode::I64__gt_s: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpSGT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__gt_u:
      case OpCode::I64__gt_u: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpUGT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__le_s:
      case OpCode::I64__le_s: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpSLE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__le_u:
      case OpCode::I64__le_u: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpULE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__ge_s:
      case OpCode::I64__ge_s: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpSGE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__ge_u:
      case OpCode::I64__ge_u: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createICmpUGE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__eq:
      case OpCode::F64__eq: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createFCmpOEQ(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__ne:
      case OpCode::F64__ne: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createFCmpUNE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__lt:
      case OpCode::F64__lt: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createFCmpOLT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__gt:
      case OpCode::F64__gt: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createFCmpOGT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__le:
      case OpCode::F64__le: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createFCmpOLE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__ge:
      case OpCode::F64__ge: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createZExt(Builder.createFCmpOGE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__add:
      case OpCode::I64__add: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createAdd(LHS, RHS));
        break;
      }
      case OpCode::I32__sub:
      case OpCode::I64__sub: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();

        stackPush(Builder.createSub(LHS, RHS));
        break;
      }
      case OpCode::I32__mul:
      case OpCode::I64__mul: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createMul(LHS, RHS));
        break;
      }
      case OpCode::I32__div_s:
      case OpCode::I64__div_s: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        if constexpr (kForceDivCheck) {
          const bool Is32 = Instr.getOpCode() == OpCode::I32__div_s;
          LLVM::Value IntZero =
              Is32 ? LLContext.getInt32(0) : LLContext.getInt64(0);
          LLVM::Value IntMinusOne =
              Is32 ? LLContext.getInt32(static_cast<uint32_t>(INT32_C(-1)))
                   : LLContext.getInt64(static_cast<uint64_t>(INT64_C(-1)));
          LLVM::Value IntMin = Is32 ? LLContext.getInt32(static_cast<uint32_t>(
                                          std::numeric_limits<int32_t>::min()))
                                    : LLContext.getInt64(static_cast<uint64_t>(
                                          std::numeric_limits<int64_t>::min()));

          auto NoZeroBB =
              LLVM::BasicBlock::create(LLContext, F.Fn, "div.nozero");
          auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "div.ok");

          auto IsNotZero =
              Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
          Builder.createCondBr(IsNotZero, NoZeroBB,
                               getTrapBB(ErrCode::Value::DivideByZero));

          Builder.positionAtEnd(NoZeroBB);
          auto NotOverflow = Builder.createLikely(
              Builder.createOr(Builder.createICmpNE(LHS, IntMin),
                               Builder.createICmpNE(RHS, IntMinusOne)));
          Builder.createCondBr(NotOverflow, OkBB,
                               getTrapBB(ErrCode::Value::IntegerOverflow));

          Builder.positionAtEnd(OkBB);
        }
        stackPush(Builder.createSDiv(LHS, RHS));
        break;
      }
      case OpCode::I32__div_u:
      case OpCode::I64__div_u: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        if constexpr (kForceDivCheck) {
          const bool Is32 = Instr.getOpCode() == OpCode::I32__div_u;
          LLVM::Value IntZero =
              Is32 ? LLContext.getInt32(0) : LLContext.getInt64(0);
          auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "div.ok");

          auto IsNotZero =
              Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
          Builder.createCondBr(IsNotZero, OkBB,
                               getTrapBB(ErrCode::Value::DivideByZero));
          Builder.positionAtEnd(OkBB);
        }
        stackPush(Builder.createUDiv(LHS, RHS));
        break;
      }
      case OpCode::I32__rem_s:
      case OpCode::I64__rem_s: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        // handle INT32_MIN % -1
        const bool Is32 = Instr.getOpCode() == OpCode::I32__rem_s;
        LLVM::Value IntMinusOne =
            Is32 ? LLContext.getInt32(static_cast<uint32_t>(INT32_C(-1)))
                 : LLContext.getInt64(static_cast<uint64_t>(INT64_C(-1)));
        LLVM::Value IntMin = Is32 ? LLContext.getInt32(static_cast<uint32_t>(
                                        std::numeric_limits<int32_t>::min()))
                                  : LLContext.getInt64(static_cast<uint64_t>(
                                        std::numeric_limits<int64_t>::min()));
        LLVM::Value IntZero =
            Is32 ? LLContext.getInt32(0) : LLContext.getInt64(0);

        auto NoOverflowBB =
            LLVM::BasicBlock::create(LLContext, F.Fn, "no.overflow");
        auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "end.overflow");

        if constexpr (kForceDivCheck) {
          auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "rem.ok");

          auto IsNotZero =
              Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
          Builder.createCondBr(IsNotZero, OkBB,
                               getTrapBB(ErrCode::Value::DivideByZero));
          Builder.positionAtEnd(OkBB);
        }

        auto CurrBB = Builder.getInsertBlock();

        auto NotOverflow = Builder.createLikely(
            Builder.createOr(Builder.createICmpNE(LHS, IntMin),
                             Builder.createICmpNE(RHS, IntMinusOne)));
        Builder.createCondBr(NotOverflow, NoOverflowBB, EndBB);

        Builder.positionAtEnd(NoOverflowBB);
        auto Ret1 = Builder.createSRem(LHS, RHS);
        Builder.createBr(EndBB);

        Builder.positionAtEnd(EndBB);
        auto Ret = Builder.createPHI(Ret1.getType());
        Ret.addIncoming(Ret1, NoOverflowBB);
        Ret.addIncoming(IntZero, CurrBB);

        stackPush(Ret);
        break;
      }
      case OpCode::I32__rem_u:
      case OpCode::I64__rem_u: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        if constexpr (kForceDivCheck) {
          LLVM::Value IntZero = Instr.getOpCode() == OpCode::I32__rem_u
                                    ? LLContext.getInt32(0)
                                    : LLContext.getInt64(0);
          auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "rem.ok");

          auto IsNotZero =
              Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
          Builder.createCondBr(IsNotZero, OkBB,
                               getTrapBB(ErrCode::Value::DivideByZero));
          Builder.positionAtEnd(OkBB);
        }
        stackPush(Builder.createURem(LHS, RHS));
        break;
      }
      case OpCode::I32__and:
      case OpCode::I64__and: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createAnd(LHS, RHS));
        break;
      }
      case OpCode::I32__or:
      case OpCode::I64__or: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createOr(LHS, RHS));
        break;
      }
      case OpCode::I32__xor:
      case OpCode::I64__xor: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createXor(LHS, RHS));
        break;
      }
      case OpCode::I32__shl:
      case OpCode::I64__shl: {
        LLVM::Value Mask = Instr.getOpCode() == OpCode::I32__shl
                               ? LLContext.getInt32(31)
                               : LLContext.getInt64(63);
        LLVM::Value RHS = Builder.createAnd(stackPop(), Mask);
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createShl(LHS, RHS));
        break;
      }
      case OpCode::I32__shr_s:
      case OpCode::I64__shr_s: {
        LLVM::Value Mask = Instr.getOpCode() == OpCode::I32__shr_s
                               ? LLContext.getInt32(31)
                               : LLContext.getInt64(63);
        LLVM::Value RHS = Builder.createAnd(stackPop(), Mask);
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createAShr(LHS, RHS));
        break;
      }
      case OpCode::I32__shr_u:
      case OpCode::I64__shr_u: {
        LLVM::Value Mask = Instr.getOpCode() == OpCode::I32__shr_u
                               ? LLContext.getInt32(31)
                               : LLContext.getInt64(63);
        LLVM::Value RHS = Builder.createAnd(stackPop(), Mask);
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createLShr(LHS, RHS));
        break;
      }
      case OpCode::I32__rotl: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        assuming(LLVM::Core::FShl != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::FShl, {Context.Int32Ty},
                                          {LHS, LHS, RHS}));
        break;
      }
      case OpCode::I32__rotr: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        assuming(LLVM::Core::FShr != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::FShr, {Context.Int32Ty},
                                          {LHS, LHS, RHS}));
        break;
      }
      case OpCode::I64__rotl: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        assuming(LLVM::Core::FShl != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::FShl, {Context.Int64Ty},
                                          {LHS, LHS, RHS}));
        break;
      }
      case OpCode::I64__rotr: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        assuming(LLVM::Core::FShr != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::FShr, {Context.Int64Ty},
                                          {LHS, LHS, RHS}));
        break;
      }
      case OpCode::F32__add:
      case OpCode::F64__add: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createFAdd(LHS, RHS));
        break;
      }
      case OpCode::F32__sub:
      case OpCode::F64__sub: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createFSub(LHS, RHS));
        break;
      }
      case OpCode::F32__mul:
      case OpCode::F64__mul: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createFMul(LHS, RHS));
        break;
      }
      case OpCode::F32__div:
      case OpCode::F64__div: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        stackPush(Builder.createFDiv(LHS, RHS));
        break;
      }
      case OpCode::F32__min:
      case OpCode::F64__min: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        auto FpTy = Instr.getOpCode() == OpCode::F32__min ? Context.FloatTy
                                                          : Context.DoubleTy;
        auto IntTy = Instr.getOpCode() == OpCode::F32__min ? Context.Int32Ty
                                                           : Context.Int64Ty;

        auto UEQ = Builder.createFCmpUEQ(LHS, RHS);
        auto UNO = Builder.createFCmpUNO(LHS, RHS);

        auto LHSInt = Builder.createBitCast(LHS, IntTy);
        auto RHSInt = Builder.createBitCast(RHS, IntTy);
        auto OrInt = Builder.createOr(LHSInt, RHSInt);
        auto OrFp = Builder.createBitCast(OrInt, FpTy);

        auto AddFp = Builder.createFAdd(LHS, RHS);

        assuming(LLVM::Core::MinNum != LLVM::Core::NotIntrinsic);
        auto MinFp = Builder.createIntrinsic(LLVM::Core::MinNum,
                                             {LHS.getType()}, {LHS, RHS});

        auto Ret = Builder.createSelect(
            UEQ, Builder.createSelect(UNO, AddFp, OrFp), MinFp);
        stackPush(Ret);
        break;
      }
      case OpCode::F32__max:
      case OpCode::F64__max: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        auto FpTy = Instr.getOpCode() == OpCode::F32__max ? Context.FloatTy
                                                          : Context.DoubleTy;
        auto IntTy = Instr.getOpCode() == OpCode::F32__max ? Context.Int32Ty
                                                           : Context.Int64Ty;

        auto UEQ = Builder.createFCmpUEQ(LHS, RHS);
        auto UNO = Builder.createFCmpUNO(LHS, RHS);

        auto LHSInt = Builder.createBitCast(LHS, IntTy);
        auto RHSInt = Builder.createBitCast(RHS, IntTy);
        auto AndInt = Builder.createAnd(LHSInt, RHSInt);
        auto AndFp = Builder.createBitCast(AndInt, FpTy);

        auto AddFp = Builder.createFAdd(LHS, RHS);

        assuming(LLVM::Core::MaxNum != LLVM::Core::NotIntrinsic);
        auto MaxFp = Builder.createIntrinsic(LLVM::Core::MaxNum,
                                             {LHS.getType()}, {LHS, RHS});

        auto Ret = Builder.createSelect(
            UEQ, Builder.createSelect(UNO, AddFp, AndFp), MaxFp);
        stackPush(Ret);
        break;
      }
      case OpCode::F32__copysign:
      case OpCode::F64__copysign: {
        LLVM::Value RHS = stackPop();
        LLVM::Value LHS = stackPop();
        assuming(LLVM::Core::CopySign != LLVM::Core::NotIntrinsic);
        stackPush(Builder.createIntrinsic(LLVM::Core::CopySign, {LHS.getType()},
                                          {LHS, RHS}));
        break;
      }
      case OpCode::V128__load:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(), Context.Int128x1Ty);
        break;
      case OpCode::V128__load8x8_s:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            LLVM::Type::getVectorType(Context.Int8Ty, 8),
                            Context.Int16x8Ty, true);
        break;
      case OpCode::V128__load8x8_u:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            LLVM::Type::getVectorType(Context.Int8Ty, 8),
                            Context.Int16x8Ty, false);
        break;
      case OpCode::V128__load16x4_s:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            LLVM::Type::getVectorType(Context.Int16Ty, 4),
                            Context.Int32x4Ty, true);
        break;
      case OpCode::V128__load16x4_u:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            LLVM::Type::getVectorType(Context.Int16Ty, 4),
                            Context.Int32x4Ty, false);
        break;
      case OpCode::V128__load32x2_s:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            LLVM::Type::getVectorType(Context.Int32Ty, 2),
                            Context.Int64x2Ty, true);
        break;
      case OpCode::V128__load32x2_u:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            LLVM::Type::getVectorType(Context.Int32Ty, 2),
                            Context.Int64x2Ty, false);
        break;
      case OpCode::V128__load8_splat:
        compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Context.Int8Ty,
                           Context.Int8x16Ty);
        break;
      case OpCode::V128__load16_splat:
        compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Context.Int16Ty,
                           Context.Int16x8Ty);
        break;
      case OpCode::V128__load32_splat:
        compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Context.Int32Ty,
                           Context.Int32x4Ty);
        break;
      case OpCode::V128__load64_splat:
        compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Context.Int64Ty,
                           Context.Int64x2Ty);
        break;
      case OpCode::V128__load32_zero:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(), Context.Int32Ty,
                            Context.Int128Ty, false);
        break;
      case OpCode::V128__load64_zero:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(), Context.Int64Ty,
                            Context.Int128Ty, false);
        break;
      case OpCode::V128__store:
        compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int128x1Ty, false, true);
        break;
      case OpCode::V128__load8_lane:
        compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                          Instr.getMemoryAlign(), Instr.getMemoryLane(),
                          Context.Int8Ty, Context.Int8x16Ty);
        break;
      case OpCode::V128__load16_lane:
        compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                          Instr.getMemoryAlign(), Instr.getMemoryLane(),
                          Context.Int16Ty, Context.Int16x8Ty);
        break;
      case OpCode::V128__load32_lane:
        compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                          Instr.getMemoryAlign(), Instr.getMemoryLane(),
                          Context.Int32Ty, Context.Int32x4Ty);
        break;
      case OpCode::V128__load64_lane:
        compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                          Instr.getMemoryAlign(), Instr.getMemoryLane(),
                          Context.Int64Ty, Context.Int64x2Ty);
        break;
      case OpCode::V128__store8_lane:
        compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Instr.getMemoryLane(),
                           Context.Int8Ty, Context.Int8x16Ty);
        break;
      case OpCode::V128__store16_lane:
        compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Instr.getMemoryLane(),
                           Context.Int16Ty, Context.Int16x8Ty);
        break;
      case OpCode::V128__store32_lane:
        compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Instr.getMemoryLane(),
                           Context.Int32Ty, Context.Int32x4Ty);
        break;
      case OpCode::V128__store64_lane:
        compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                           Instr.getMemoryAlign(), Instr.getMemoryLane(),
                           Context.Int64Ty, Context.Int64x2Ty);
        break;
      case OpCode::V128__const: {
        const auto Value = Instr.getNum().get<uint64x2_t>();
        auto Vector =
            LLVM::Value::getConstVector64(LLContext, {Value[0], Value[1]});
        stackPush(Builder.createBitCast(Vector, Context.Int64x2Ty));
        break;
      }
      case OpCode::I8x16__shuffle: {
        auto V2 = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
        auto V1 = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
        const auto V3 = Instr.getNum().get<uint128_t>();
        std::array<uint8_t, 16> Mask;
        for (size_t I = 0; I < 16; ++I) {
          Mask[I] = static_cast<uint8_t>(V3 >> (I * 8));
        }
        stackPush(Builder.createBitCast(
            Builder.createShuffleVector(
                V1, V2, LLVM::Value::getConstVector8(LLContext, Mask)),
            Context.Int64x2Ty));
        break;
      }
      case OpCode::I8x16__extract_lane_s:
        compileExtractLaneOp(Context.Int8x16Ty, Instr.getMemoryLane(),
                             Context.Int32Ty, true);
        break;
      case OpCode::I8x16__extract_lane_u:
        compileExtractLaneOp(Context.Int8x16Ty, Instr.getMemoryLane(),
                             Context.Int32Ty, false);
        break;
      case OpCode::I8x16__replace_lane:
        compileReplaceLaneOp(Context.Int8x16Ty, Instr.getMemoryLane());
        break;
      case OpCode::I16x8__extract_lane_s:
        compileExtractLaneOp(Context.Int16x8Ty, Instr.getMemoryLane(),
                             Context.Int32Ty, true);
        break;
      case OpCode::I16x8__extract_lane_u:
        compileExtractLaneOp(Context.Int16x8Ty, Instr.getMemoryLane(),
                             Context.Int32Ty, false);
        break;
      case OpCode::I16x8__replace_lane:
        compileReplaceLaneOp(Context.Int16x8Ty, Instr.getMemoryLane());
        break;
      case OpCode::I32x4__extract_lane:
        compileExtractLaneOp(Context.Int32x4Ty, Instr.getMemoryLane());
        break;
      case OpCode::I32x4__replace_lane:
        compileReplaceLaneOp(Context.Int32x4Ty, Instr.getMemoryLane());
        break;
      case OpCode::I64x2__extract_lane:
        compileExtractLaneOp(Context.Int64x2Ty, Instr.getMemoryLane());
        break;
      case OpCode::I64x2__replace_lane:
        compileReplaceLaneOp(Context.Int64x2Ty, Instr.getMemoryLane());
        break;
      case OpCode::F32x4__extract_lane:
        compileExtractLaneOp(Context.Floatx4Ty, Instr.getMemoryLane());
        break;
      case OpCode::F32x4__replace_lane:
        compileReplaceLaneOp(Context.Floatx4Ty, Instr.getMemoryLane());
        break;
      case OpCode::F64x2__extract_lane:
        compileExtractLaneOp(Context.Doublex2Ty, Instr.getMemoryLane());
        break;
      case OpCode::F64x2__replace_lane:
        compileReplaceLaneOp(Context.Doublex2Ty, Instr.getMemoryLane());
        break;
      case OpCode::I8x16__swizzle: {
        auto Index = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
        auto Vector = Builder.createBitCast(stackPop(), Context.Int8x16Ty);

#if defined(__x86_64__)
        if (Context.SupportSSSE3) {
          auto Magic = Builder.createVectorSplat(16, LLContext.getInt8(112));
          auto Added = Builder.createAdd(Index, Magic);
          auto NewIndex = Builder.createSelect(
              Builder.createICmpUGT(Index, Added),
              LLVM::Value::getConstAllOnes(Context.Int8x16Ty), Added);
          assuming(LLVM::Core::X86SSSE3PShufB128 != LLVM::Core::NotIntrinsic);
          stackPush(Builder.createBitCast(
              Builder.createIntrinsic(LLVM::Core::X86SSSE3PShufB128, {},
                                      {Vector, NewIndex}),
              Context.Int64x2Ty));
          break;
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON) {
          assuming(LLVM::Core::AArch64NeonTbl1 != LLVM::Core::NotIntrinsic);
          stackPush(Builder.createBitCast(
              Builder.createBinaryIntrinsic(LLVM::Core::AArch64NeonTbl1, Vector,
                                            Index),
              Context.Int64x2Ty));
          break;
        }
#endif

        // Fallback case.
        // If the SSSE3 is not supported on the x86_64 platform or
        // the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        auto Mask = Builder.createVectorSplat(16, LLContext.getInt8(15));
        auto Zero = Builder.createVectorSplat(16, LLContext.getInt8(0));
        auto IsOver = Builder.createICmpUGT(Index, Mask);
        auto InboundIndex = Builder.createAnd(Index, Mask);
        auto Array =
            Builder.createArrayAlloca(Context.Int8Ty, LLContext.getInt64(16));
        for (size_t I = 0; I < 16; ++I) {
          Builder.createStore(
              Builder.createExtractElement(Vector, LLContext.getInt64(I)),
              Builder.createInBoundsGEP1(Context.Int8Ty, Array,
                                         LLContext.getInt64(I)));
        }
        LLVM::Value Ret = LLVM::Value::getUndef(Context.Int8x16Ty);
        for (size_t I = 0; I < 16; ++I) {
          auto Idx =
              Builder.createExtractElement(InboundIndex, LLContext.getInt64(I));
          auto Value = Builder.createLoad(
              Context.Int8Ty,
              Builder.createInBoundsGEP1(Context.Int8Ty, Array, Idx));
          Ret = Builder.createInsertElement(Ret, Value, LLContext.getInt64(I));
        }
        Ret = Builder.createSelect(IsOver, Zero, Ret);
        stackPush(Builder.createBitCast(Ret, Context.Int64x2Ty));
        break;
      }
      case OpCode::I8x16__splat:
        compileSplatOp(Context.Int8x16Ty);
        break;
      case OpCode::I16x8__splat:
        compileSplatOp(Context.Int16x8Ty);
        break;
      case OpCode::I32x4__splat:
        compileSplatOp(Context.Int32x4Ty);
        break;
      case OpCode::I64x2__splat:
        compileSplatOp(Context.Int64x2Ty);
        break;
      case OpCode::F32x4__splat:
        compileSplatOp(Context.Floatx4Ty);
        break;
      case OpCode::F64x2__splat:
        compileSplatOp(Context.Doublex2Ty);
        break;
      case OpCode::I8x16__eq:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntEQ);
        break;
      case OpCode::I8x16__ne:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntNE);
        break;
      case OpCode::I8x16__lt_s:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSLT);
        break;
      case OpCode::I8x16__lt_u:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntULT);
        break;
      case OpCode::I8x16__gt_s:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSGT);
        break;
      case OpCode::I8x16__gt_u:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntUGT);
        break;
      case OpCode::I8x16__le_s:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSLE);
        break;
      case OpCode::I8x16__le_u:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntULE);
        break;
      case OpCode::I8x16__ge_s:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSGE);
        break;
      case OpCode::I8x16__ge_u:
        compileVectorCompareOp(Context.Int8x16Ty, LLVMIntUGE);
        break;
      case OpCode::I16x8__eq:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntEQ);
        break;
      case OpCode::I16x8__ne:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntNE);
        break;
      case OpCode::I16x8__lt_s:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSLT);
        break;
      case OpCode::I16x8__lt_u:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntULT);
        break;
      case OpCode::I16x8__gt_s:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSGT);
        break;
      case OpCode::I16x8__gt_u:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntUGT);
        break;
      case OpCode::I16x8__le_s:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSLE);
        break;
      case OpCode::I16x8__le_u:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntULE);
        break;
      case OpCode::I16x8__ge_s:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSGE);
        break;
      case OpCode::I16x8__ge_u:
        compileVectorCompareOp(Context.Int16x8Ty, LLVMIntUGE);
        break;
      case OpCode::I32x4__eq:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntEQ);
        break;
      case OpCode::I32x4__ne:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntNE);
        break;
      case OpCode::I32x4__lt_s:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSLT);
        break;
      case OpCode::I32x4__lt_u:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntULT);
        break;
      case OpCode::I32x4__gt_s:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSGT);
        break;
      case OpCode::I32x4__gt_u:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntUGT);
        break;
      case OpCode::I32x4__le_s:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSLE);
        break;
      case OpCode::I32x4__le_u:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntULE);
        break;
      case OpCode::I32x4__ge_s:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSGE);
        break;
      case OpCode::I32x4__ge_u:
        compileVectorCompareOp(Context.Int32x4Ty, LLVMIntUGE);
        break;
      case OpCode::I64x2__eq:
        compileVectorCompareOp(Context.Int64x2Ty, LLVMIntEQ);
        break;
      case OpCode::I64x2__ne:
        compileVectorCompareOp(Context.Int64x2Ty, LLVMIntNE);
        break;
      case OpCode::I64x2__lt_s:
        compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSLT);
        break;
      case OpCode::I64x2__gt_s:
        compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSGT);
        break;
      case OpCode::I64x2__le_s:
        compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSLE);
        break;
      case OpCode::I64x2__ge_s:
        compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSGE);
        break;
      case OpCode::F32x4__eq:
        compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOEQ,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__ne:
        compileVectorCompareOp(Context.Floatx4Ty, LLVMRealUNE,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__lt:
        compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOLT,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__gt:
        compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOGT,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__le:
        compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOLE,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__ge:
        compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOGE,
                               Context.Int32x4Ty);
        break;
      case OpCode::F64x2__eq:
        compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOEQ,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__ne:
        compileVectorCompareOp(Context.Doublex2Ty, LLVMRealUNE,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__lt:
        compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOLT,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__gt:
        compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOGT,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__le:
        compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOLE,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__ge:
        compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOGE,
                               Context.Int64x2Ty);
        break;
      case OpCode::V128__not:
        Stack.back() = Builder.createNot(Stack.back());
        break;
      case OpCode::V128__and: {
        auto RHS = stackPop();
        auto LHS = stackPop();
        stackPush(Builder.createAnd(LHS, RHS));
        break;
      }
      case OpCode::V128__andnot: {
        auto RHS = stackPop();
        auto LHS = stackPop();
        stackPush(Builder.createAnd(LHS, Builder.createNot(RHS)));
        break;
      }
      case OpCode::V128__or: {
        auto RHS = stackPop();
        auto LHS = stackPop();
        stackPush(Builder.createOr(LHS, RHS));
        break;
      }
      case OpCode::V128__xor: {
        auto RHS = stackPop();
        auto LHS = stackPop();
        stackPush(Builder.createXor(LHS, RHS));
        break;
      }
      case OpCode::V128__bitselect: {
        auto C = stackPop();
        auto V2 = stackPop();
        auto V1 = stackPop();
        stackPush(Builder.createXor(
            Builder.createAnd(Builder.createXor(V1, V2), C), V2));
        break;
      }
      case OpCode::V128__any_true:
        compileVectorAnyTrue();
        break;
      case OpCode::I8x16__abs:
        compileVectorAbs(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__neg:
        compileVectorNeg(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__popcnt:
        compileVectorPopcnt();
        break;
      case OpCode::I8x16__all_true:
        compileVectorAllTrue(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__bitmask:
        compileVectorBitMask(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__narrow_i16x8_s:
        compileVectorNarrow(Context.Int16x8Ty, true);
        break;
      case OpCode::I8x16__narrow_i16x8_u:
        compileVectorNarrow(Context.Int16x8Ty, false);
        break;
      case OpCode::I8x16__shl:
        compileVectorShl(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__shr_s:
        compileVectorAShr(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__shr_u:
        compileVectorLShr(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__add:
        compileVectorVectorAdd(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__add_sat_s:
        compileVectorVectorAddSat(Context.Int8x16Ty, true);
        break;
      case OpCode::I8x16__add_sat_u:
        compileVectorVectorAddSat(Context.Int8x16Ty, false);
        break;
      case OpCode::I8x16__sub:
        compileVectorVectorSub(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__sub_sat_s:
        compileVectorVectorSubSat(Context.Int8x16Ty, true);
        break;
      case OpCode::I8x16__sub_sat_u:
        compileVectorVectorSubSat(Context.Int8x16Ty, false);
        break;
      case OpCode::I8x16__min_s:
        compileVectorVectorSMin(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__min_u:
        compileVectorVectorUMin(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__max_s:
        compileVectorVectorSMax(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__max_u:
        compileVectorVectorUMax(Context.Int8x16Ty);
        break;
      case OpCode::I8x16__avgr_u:
        compileVectorVectorUAvgr(Context.Int8x16Ty);
        break;
      case OpCode::I16x8__abs:
        compileVectorAbs(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__neg:
        compileVectorNeg(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__all_true:
        compileVectorAllTrue(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__bitmask:
        compileVectorBitMask(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__narrow_i32x4_s:
        compileVectorNarrow(Context.Int32x4Ty, true);
        break;
      case OpCode::I16x8__narrow_i32x4_u:
        compileVectorNarrow(Context.Int32x4Ty, false);
        break;
      case OpCode::I16x8__extend_low_i8x16_s:
        compileVectorExtend(Context.Int8x16Ty, true, true);
        break;
      case OpCode::I16x8__extend_high_i8x16_s:
        compileVectorExtend(Context.Int8x16Ty, true, false);
        break;
      case OpCode::I16x8__extend_low_i8x16_u:
        compileVectorExtend(Context.Int8x16Ty, false, true);
        break;
      case OpCode::I16x8__extend_high_i8x16_u:
        compileVectorExtend(Context.Int8x16Ty, false, false);
        break;
      case OpCode::I16x8__shl:
        compileVectorShl(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__shr_s:
        compileVectorAShr(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__shr_u:
        compileVectorLShr(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__add:
        compileVectorVectorAdd(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__add_sat_s:
        compileVectorVectorAddSat(Context.Int16x8Ty, true);
        break;
      case OpCode::I16x8__add_sat_u:
        compileVectorVectorAddSat(Context.Int16x8Ty, false);
        break;
      case OpCode::I16x8__sub:
        compileVectorVectorSub(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__sub_sat_s:
        compileVectorVectorSubSat(Context.Int16x8Ty, true);
        break;
      case OpCode::I16x8__sub_sat_u:
        compileVectorVectorSubSat(Context.Int16x8Ty, false);
        break;
      case OpCode::I16x8__mul:
        compileVectorVectorMul(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__min_s:
        compileVectorVectorSMin(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__min_u:
        compileVectorVectorUMin(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__max_s:
        compileVectorVectorSMax(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__max_u:
        compileVectorVectorUMax(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__avgr_u:
        compileVectorVectorUAvgr(Context.Int16x8Ty);
        break;
      case OpCode::I16x8__extmul_low_i8x16_s:
        compileVectorExtMul(Context.Int8x16Ty, true, true);
        break;
      case OpCode::I16x8__extmul_high_i8x16_s:
        compileVectorExtMul(Context.Int8x16Ty, true, false);
        break;
      case OpCode::I16x8__extmul_low_i8x16_u:
        compileVectorExtMul(Context.Int8x16Ty, false, true);
        break;
      case OpCode::I16x8__extmul_high_i8x16_u:
        compileVectorExtMul(Context.Int8x16Ty, false, false);
        break;
      case OpCode::I16x8__q15mulr_sat_s:
        compileVectorVectorQ15MulSat();
        break;
      case OpCode::I16x8__extadd_pairwise_i8x16_s:
        compileVectorExtAddPairwise(Context.Int8x16Ty, true);
        break;
      case OpCode::I16x8__extadd_pairwise_i8x16_u:
        compileVectorExtAddPairwise(Context.Int8x16Ty, false);
        break;
      case OpCode::I32x4__abs:
        compileVectorAbs(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__neg:
        compileVectorNeg(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__all_true:
        compileVectorAllTrue(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__bitmask:
        compileVectorBitMask(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__extend_low_i16x8_s:
        compileVectorExtend(Context.Int16x8Ty, true, true);
        break;
      case OpCode::I32x4__extend_high_i16x8_s:
        compileVectorExtend(Context.Int16x8Ty, true, false);
        break;
      case OpCode::I32x4__extend_low_i16x8_u:
        compileVectorExtend(Context.Int16x8Ty, false, true);
        break;
      case OpCode::I32x4__extend_high_i16x8_u:
        compileVectorExtend(Context.Int16x8Ty, false, false);
        break;
      case OpCode::I32x4__shl:
        compileVectorShl(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__shr_s:
        compileVectorAShr(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__shr_u:
        compileVectorLShr(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__add:
        compileVectorVectorAdd(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__sub:
        compileVectorVectorSub(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__mul:
        compileVectorVectorMul(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__min_s:
        compileVectorVectorSMin(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__min_u:
        compileVectorVectorUMin(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__max_s:
        compileVectorVectorSMax(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__max_u:
        compileVectorVectorUMax(Context.Int32x4Ty);
        break;
      case OpCode::I32x4__extmul_low_i16x8_s:
        compileVectorExtMul(Context.Int16x8Ty, true, true);
        break;
      case OpCode::I32x4__extmul_high_i16x8_s:
        compileVectorExtMul(Context.Int16x8Ty, true, false);
        break;
      case OpCode::I32x4__extmul_low_i16x8_u:
        compileVectorExtMul(Context.Int16x8Ty, false, true);
        break;
      case OpCode::I32x4__extmul_high_i16x8_u:
        compileVectorExtMul(Context.Int16x8Ty, false, false);
        break;
      case OpCode::I32x4__extadd_pairwise_i16x8_s:
        compileVectorExtAddPairwise(Context.Int16x8Ty, true);
        break;
      case OpCode::I32x4__extadd_pairwise_i16x8_u:
        compileVectorExtAddPairwise(Context.Int16x8Ty, false);
        break;
      case OpCode::I32x4__dot_i16x8_s: {
        auto ExtendTy = Context.Int16x8Ty.getExtendedElementVectorType();
        auto Undef = LLVM::Value::getUndef(ExtendTy);
        auto LHS = Builder.createSExt(
            Builder.createBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
        auto RHS = Builder.createSExt(
            Builder.createBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
        auto M = Builder.createMul(LHS, RHS);
        auto L = Builder.createShuffleVector(
            M, Undef,
            LLVM::Value::getConstVector32(LLContext, {0U, 2U, 4U, 6U}));
        auto R = Builder.createShuffleVector(
            M, Undef,
            LLVM::Value::getConstVector32(LLContext, {1U, 3U, 5U, 7U}));
        auto V = Builder.createAdd(L, R);
        stackPush(Builder.createBitCast(V, Context.Int64x2Ty));
        break;
      }
      case OpCode::I64x2__abs:
        compileVectorAbs(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__neg:
        compileVectorNeg(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__all_true:
        compileVectorAllTrue(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__bitmask:
        compileVectorBitMask(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__extend_low_i32x4_s:
        compileVectorExtend(Context.Int32x4Ty, true, true);
        break;
      case OpCode::I64x2__extend_high_i32x4_s:
        compileVectorExtend(Context.Int32x4Ty, true, false);
        break;
      case OpCode::I64x2__extend_low_i32x4_u:
        compileVectorExtend(Context.Int32x4Ty, false, true);
        break;
      case OpCode::I64x2__extend_high_i32x4_u:
        compileVectorExtend(Context.Int32x4Ty, false, false);
        break;
      case OpCode::I64x2__shl:
        compileVectorShl(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__shr_s:
        compileVectorAShr(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__shr_u:
        compileVectorLShr(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__add:
        compileVectorVectorAdd(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__sub:
        compileVectorVectorSub(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__mul:
        compileVectorVectorMul(Context.Int64x2Ty);
        break;
      case OpCode::I64x2__extmul_low_i32x4_s:
        compileVectorExtMul(Context.Int32x4Ty, true, true);
        break;
      case OpCode::I64x2__extmul_high_i32x4_s:
        compileVectorExtMul(Context.Int32x4Ty, true, false);
        break;
      case OpCode::I64x2__extmul_low_i32x4_u:
        compileVectorExtMul(Context.Int32x4Ty, false, true);
        break;
      case OpCode::I64x2__extmul_high_i32x4_u:
        compileVectorExtMul(Context.Int32x4Ty, false, false);
        break;
      case OpCode::F32x4__abs:
        compileVectorFAbs(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__neg:
        compileVectorFNeg(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__sqrt:
        compileVectorFSqrt(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__add:
        compileVectorVectorFAdd(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__sub:
        compileVectorVectorFSub(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__mul:
        compileVectorVectorFMul(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__div:
        compileVectorVectorFDiv(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__min:
        compileVectorVectorFMin(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__max:
        compileVectorVectorFMax(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__pmin:
        compileVectorVectorFPMin(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__pmax:
        compileVectorVectorFPMax(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__ceil:
        compileVectorFCeil(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__floor:
        compileVectorFFloor(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__trunc:
        compileVectorFTrunc(Context.Floatx4Ty);
        break;
      case OpCode::F32x4__nearest:
        compileVectorFNearest(Context.Floatx4Ty);
        break;
      case OpCode::F64x2__abs:
        compileVectorFAbs(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__neg:
        compileVectorFNeg(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__sqrt:
        compileVectorFSqrt(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__add:
        compileVectorVectorFAdd(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__sub:
        compileVectorVectorFSub(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__mul:
        compileVectorVectorFMul(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__div:
        compileVectorVectorFDiv(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__min:
        compileVectorVectorFMin(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__max:
        compileVectorVectorFMax(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__pmin:
        compileVectorVectorFPMin(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__pmax:
        compileVectorVectorFPMax(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__ceil:
        compileVectorFCeil(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__floor:
        compileVectorFFloor(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__trunc:
        compileVectorFTrunc(Context.Doublex2Ty);
        break;
      case OpCode::F64x2__nearest:
        compileVectorFNearest(Context.Doublex2Ty);
        break;
      case OpCode::I32x4__trunc_sat_f32x4_s:
        compileVectorTruncSatS32(Context.Floatx4Ty, false);
        break;
      case OpCode::I32x4__trunc_sat_f32x4_u:
        compileVectorTruncSatU32(Context.Floatx4Ty, false);
        break;
      case OpCode::F32x4__convert_i32x4_s:
        compileVectorConvertS(Context.Int32x4Ty, Context.Floatx4Ty, false);
        break;
      case OpCode::F32x4__convert_i32x4_u:
        compileVectorConvertU(Context.Int32x4Ty, Context.Floatx4Ty, false);
        break;
      case OpCode::I32x4__trunc_sat_f64x2_s_zero:
        compileVectorTruncSatS32(Context.Doublex2Ty, true);
        break;
      case OpCode::I32x4__trunc_sat_f64x2_u_zero:
        compileVectorTruncSatU32(Context.Doublex2Ty, true);
        break;
      case OpCode::F64x2__convert_low_i32x4_s:
        compileVectorConvertS(Context.Int32x4Ty, Context.Doublex2Ty, true);
        break;
      case OpCode::F64x2__convert_low_i32x4_u:
        compileVectorConvertU(Context.Int32x4Ty, Context.Doublex2Ty, true);
        break;
      case OpCode::F32x4__demote_f64x2_zero:
        compileVectorDemote();
        break;
      case OpCode::F64x2__promote_low_f32x4:
        compileVectorPromote();
        break;
      case OpCode::Atomic__fence:
        return compileMemoryFence();
      case OpCode::Memory__atomic__notify:
        return compileAtomicNotify(Instr.getTargetIndex(),
                                   Instr.getMemoryOffset());
      case OpCode::Memory__atomic__wait32:
        return compileAtomicWait(Instr.getTargetIndex(),
                                 Instr.getMemoryOffset(), Context.Int32Ty, 32);
      case OpCode::Memory__atomic__wait64:
        return compileAtomicWait(Instr.getTargetIndex(),
                                 Instr.getMemoryOffset(), Context.Int64Ty, 64);

      case OpCode::I32__atomic__load:
        return compileAtomicLoad(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__load:
        return compileAtomicLoad(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__load8_u:
        return compileAtomicLoad(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__load16_u:
        return compileAtomicLoad(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__load8_u:
        return compileAtomicLoad(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__load16_u:
        return compileAtomicLoad(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__load32_u:
        return compileAtomicLoad(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__store:
        return compileAtomicStore(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__store:
        return compileAtomicStore(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__store8:
        return compileAtomicStore(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int8Ty, true);
      case OpCode::I32__atomic__store16:
        return compileAtomicStore(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int16Ty, true);
      case OpCode::I64__atomic__store8:
        return compileAtomicStore(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int8Ty, true);
      case OpCode::I64__atomic__store16:
        return compileAtomicStore(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int16Ty, true);
      case OpCode::I64__atomic__store32:
        return compileAtomicStore(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int32Ty, true);
      case OpCode::I32__atomic__rmw__add:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                                  Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__add:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                                  Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__add_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                                  Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__add_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                                  Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__add_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                                  Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__add_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                                  Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__add_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                                  Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__sub:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                                  Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__sub:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                                  Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__sub_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                                  Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__sub_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                                  Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__sub_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                                  Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__sub_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                                  Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__sub_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                                  Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__and:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                                  Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__and:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                                  Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__and_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                                  Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__and_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                                  Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__and_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                                  Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__and_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                                  Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__and_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                                  Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__or:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                                  Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__or:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                                  Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__or_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                                  Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__or_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                                  Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__or_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                                  Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__or_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                                  Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__or_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                                  Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__xor:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                                  Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__xor:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                                  Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__xor_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                                  Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__xor_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                                  Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__xor_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                                  Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__xor_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                                  Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__xor_u:
        return compileAtomicRMWOp(Instr.getTargetIndex(),
                                  Instr.getMemoryOffset(),
                                  Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                                  Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__xchg:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg, Context.Int32Ty,
            Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__xchg:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg, Context.Int64Ty,
            Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg, Context.Int32Ty,
            Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg, Context.Int32Ty,
            Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg, Context.Int64Ty,
            Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg, Context.Int64Ty,
            Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg, Context.Int64Ty,
            Context.Int32Ty);
      case OpCode::I32__atomic__rmw__cmpxchg:
        return compileAtomicCompareExchange(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__cmpxchg:
        return compileAtomicCompareExchange(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__cmpxchg_u:
        return compileAtomicCompareExchange(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__cmpxchg_u:
        return compileAtomicCompareExchange(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__cmpxchg_u:
        return compileAtomicCompareExchange(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__cmpxchg_u:
        return compileAtomicCompareExchange(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__cmpxchg_u:
        return compileAtomicCompareExchange(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), Context.Int64Ty, Context.Int32Ty);

      default:
        assumingUnreachable();
      }
      return;
    };
    for (const auto &Instr : Instrs) {
      // Update instruction count
      if (LocalInstrCount) {
        Builder.createStore(
            Builder.createAdd(
                Builder.createLoad(Context.Int64Ty, LocalInstrCount),
                LLContext.getInt64(1)),
            LocalInstrCount);
      }
      if (LocalGas) {
        auto NewGas = Builder.createAdd(
            Builder.createLoad(Context.Int64Ty, LocalGas),
            Builder.createLoad(
                Context.Int64Ty,
                Builder.createConstInBoundsGEP2_64(
                    LLVM::Type::getArrayType(Context.Int64Ty, UINT16_MAX + 1),
                    Context.getCostTable(Builder, ExecCtx), 0,
                    uint16_t(Instr.getOpCode()))));
        Builder.createStore(NewGas, LocalGas);
      }

      // Make the instruction node according to Code.
      Dispatch(Instr);
    }
  }
  void compileSignedTrunc(LLVM::Type IntType) noexcept {
    auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "strunc.norm");
    auto NotMinBB = LLVM::BasicBlock::create(LLContext, F.Fn, "strunc.notmin");
    auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "strunc.notmax");
    auto Value = stackPop();
    const auto [Precise, MinFp, MaxFp] =
        [IntType, Value]() -> std::tuple<bool, LLVM::Value, LLVM::Value> {
      const auto BitWidth = IntType.getIntegerBitWidth();
      const auto [Min, Max] = [BitWidth]() -> std::tuple<int64_t, int64_t> {
        switch (BitWidth) {
        case 32:
          return {std::numeric_limits<int32_t>::min(),
                  std::numeric_limits<int32_t>::max()};
        case 64:
          return {std::numeric_limits<int64_t>::min(),
                  std::numeric_limits<int64_t>::max()};
        default:
          assumingUnreachable();
        }
      }();
      auto FPType = Value.getType();
      assuming(FPType.isFloatTy() || FPType.isDoubleTy());
      const auto FPWidth = FPType.getFPMantissaWidth();
      return {BitWidth <= FPWidth, LLVM::Value::getConstReal(FPType, Min),
              LLVM::Value::getConstReal(FPType, Max)};
    }();

    auto IsNotNan = Builder.createLikely(Builder.createFCmpORD(Value, Value));
    Builder.createCondBr(IsNotNan, NormBB,
                         getTrapBB(ErrCode::Value::InvalidConvToInt));

    Builder.positionAtEnd(NormBB);
    assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
    auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
    auto IsNotUnderflow =
        Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
    Builder.createCondBr(IsNotUnderflow, NotMinBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.positionAtEnd(NotMinBB);
    auto IsNotOverflow = Builder.createLikely(
        Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
    Builder.createCondBr(IsNotOverflow, NotMaxBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.positionAtEnd(NotMaxBB);
    stackPush(Builder.createFPToSI(Trunc, IntType));
  }
  void compileSignedTruncSat(LLVM::Type IntType) noexcept {
    auto CurrBB = Builder.getInsertBlock();
    auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.norm");
    auto NotMinBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.notmin");
    auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.notmax");
    auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.end");
    auto Value = stackPop();
    const auto [Precise, MinInt, MaxInt, MinFp, MaxFp] = [IntType, Value]()
        -> std::tuple<bool, uint64_t, uint64_t, LLVM::Value, LLVM::Value> {
      const auto BitWidth = IntType.getIntegerBitWidth();
      const auto [Min, Max] = [BitWidth]() -> std::tuple<int64_t, int64_t> {
        switch (BitWidth) {
        case 32:
          return {std::numeric_limits<int32_t>::min(),
                  std::numeric_limits<int32_t>::max()};
        case 64:
          return {std::numeric_limits<int64_t>::min(),
                  std::numeric_limits<int64_t>::max()};
        default:
          assumingUnreachable();
        }
      }();
      auto FPType = Value.getType();
      assuming(FPType.isFloatTy() || FPType.isDoubleTy());
      const auto FPWidth = FPType.getFPMantissaWidth();
      return {BitWidth <= FPWidth, static_cast<uint64_t>(Min),
              static_cast<uint64_t>(Max),
              LLVM::Value::getConstReal(FPType, Min),
              LLVM::Value::getConstReal(FPType, Max)};
    }();

    auto IsNotNan = Builder.createLikely(Builder.createFCmpORD(Value, Value));
    Builder.createCondBr(IsNotNan, NormBB, EndBB);

    Builder.positionAtEnd(NormBB);
    assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
    auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
    auto IsNotUnderflow =
        Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
    Builder.createCondBr(IsNotUnderflow, NotMinBB, EndBB);

    Builder.positionAtEnd(NotMinBB);
    auto IsNotOverflow = Builder.createLikely(
        Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
    Builder.createCondBr(IsNotOverflow, NotMaxBB, EndBB);

    Builder.positionAtEnd(NotMaxBB);
    auto IntValue = Builder.createFPToSI(Trunc, IntType);
    Builder.createBr(EndBB);

    Builder.positionAtEnd(EndBB);
    auto PHIRet = Builder.createPHI(IntType);
    PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, 0, true), CurrBB);
    PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MinInt, true), NormBB);
    PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MaxInt, true),
                       NotMinBB);
    PHIRet.addIncoming(IntValue, NotMaxBB);

    stackPush(PHIRet);
  }
  void compileUnsignedTrunc(LLVM::Type IntType) noexcept {
    auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "utrunc.norm");
    auto NotMinBB = LLVM::BasicBlock::create(LLContext, F.Fn, "utrunc.notmin");
    auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "utrunc.notmax");
    auto Value = stackPop();
    const auto [Precise, MinFp, MaxFp] =
        [IntType, Value]() -> std::tuple<bool, LLVM::Value, LLVM::Value> {
      const auto BitWidth = IntType.getIntegerBitWidth();
      const auto [Min, Max] = [BitWidth]() -> std::tuple<uint64_t, uint64_t> {
        switch (BitWidth) {
        case 32:
          return {std::numeric_limits<uint32_t>::min(),
                  std::numeric_limits<uint32_t>::max()};
        case 64:
          return {std::numeric_limits<uint64_t>::min(),
                  std::numeric_limits<uint64_t>::max()};
        default:
          assumingUnreachable();
        }
      }();
      auto FPType = Value.getType();
      assuming(FPType.isFloatTy() || FPType.isDoubleTy());
      const auto FPWidth = FPType.getFPMantissaWidth();
      return {BitWidth <= FPWidth, LLVM::Value::getConstReal(FPType, Min),
              LLVM::Value::getConstReal(FPType, Max)};
    }();

    auto IsNotNan = Builder.createLikely(Builder.createFCmpORD(Value, Value));
    Builder.createCondBr(IsNotNan, NormBB,
                         getTrapBB(ErrCode::Value::InvalidConvToInt));

    Builder.positionAtEnd(NormBB);
    assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
    auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
    auto IsNotUnderflow =
        Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
    Builder.createCondBr(IsNotUnderflow, NotMinBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.positionAtEnd(NotMinBB);
    auto IsNotOverflow = Builder.createLikely(
        Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
    Builder.createCondBr(IsNotOverflow, NotMaxBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.positionAtEnd(NotMaxBB);
    stackPush(Builder.createFPToUI(Trunc, IntType));
  }
  void compileUnsignedTruncSat(LLVM::Type IntType) noexcept {
    auto CurrBB = Builder.getInsertBlock();
    auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "usat.norm");
    auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "usat.notmax");
    auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "usat.end");
    auto Value = stackPop();
    const auto [Precise, MinInt, MaxInt, MinFp, MaxFp] = [IntType, Value]()
        -> std::tuple<bool, uint64_t, uint64_t, LLVM::Value, LLVM::Value> {
      const auto BitWidth = IntType.getIntegerBitWidth();
      const auto [Min, Max] = [BitWidth]() -> std::tuple<uint64_t, uint64_t> {
        switch (BitWidth) {
        case 32:
          return {std::numeric_limits<uint32_t>::min(),
                  std::numeric_limits<uint32_t>::max()};
        case 64:
          return {std::numeric_limits<uint64_t>::min(),
                  std::numeric_limits<uint64_t>::max()};
        default:
          assumingUnreachable();
        }
      }();
      auto FPType = Value.getType();
      assuming(FPType.isFloatTy() || FPType.isDoubleTy());
      const auto FPWidth = FPType.getFPMantissaWidth();
      return {BitWidth <= FPWidth, Min, Max,
              LLVM::Value::getConstReal(FPType, Min),
              LLVM::Value::getConstReal(FPType, Max)};
    }();

    assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
    auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
    auto IsNotUnderflow =
        Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
    Builder.createCondBr(IsNotUnderflow, NormBB, EndBB);

    Builder.positionAtEnd(NormBB);
    auto IsNotOverflow = Builder.createLikely(
        Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
    Builder.createCondBr(IsNotOverflow, NotMaxBB, EndBB);

    Builder.positionAtEnd(NotMaxBB);
    auto IntValue = Builder.createFPToUI(Trunc, IntType);
    Builder.createBr(EndBB);

    Builder.positionAtEnd(EndBB);
    auto PHIRet = Builder.createPHI(IntType);
    PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MinInt), CurrBB);
    PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MaxInt), NormBB);
    PHIRet.addIncoming(IntValue, NotMaxBB);

    stackPush(PHIRet);
  }

  void compileAtomicCheckOffsetAlignment(LLVM::Value Offset,
                                         LLVM::Type IntType) noexcept {
    const auto BitWidth = IntType.getIntegerBitWidth();
    auto BWMask = LLContext.getInt64((BitWidth >> 3) - 1);
    auto Value = Builder.createAnd(Offset, BWMask);
    auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "address_align_ok");
    auto IsAddressAligned = Builder.createLikely(
        Builder.createICmpEQ(Value, LLContext.getInt64(0)));
    Builder.createCondBr(IsAddressAligned, OkBB,
                         getTrapBB(ErrCode::Value::UnalignedAtomicAccess));

    Builder.positionAtEnd(OkBB);
  }

  void compileMemoryFence() noexcept {
    Builder.createFence(LLVMAtomicOrderingSequentiallyConsistent);
  }
  void compileAtomicNotify(unsigned MemoryIndex,
                           unsigned MemoryOffset) noexcept {
    auto Count = stackPop();
    auto Addr = Builder.createZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Addr = Builder.createAdd(Addr, LLContext.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Addr, Context.Int32Ty);
    auto Offset = stackPop();

    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder, AST::Module::Intrinsics::kMemoryAtomicNotify,
            LLVM::Type::getFunctionType(
                Context.Int32Ty,
                {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
        {LLContext.getInt32(MemoryIndex), Offset, Count}));
  }
  void compileAtomicWait(unsigned MemoryIndex, unsigned MemoryOffset,
                         LLVM::Type TargetType, uint32_t BitWidth) noexcept {
    auto Timeout = stackPop();
    auto ExpectedValue = Builder.createZExtOrTrunc(stackPop(), Context.Int64Ty);
    auto Addr = Builder.createZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Addr = Builder.createAdd(Addr, LLContext.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Addr, TargetType);
    auto Offset = stackPop();

    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder, AST::Module::Intrinsics::kMemoryAtomicWait,
            LLVM::Type::getFunctionType(Context.Int32Ty,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int64Ty, Context.Int64Ty,
                                         Context.Int32Ty},
                                        false)),
        {LLContext.getInt32(MemoryIndex), Offset, ExpectedValue, Timeout,
         LLContext.getInt32(BitWidth)}));
  }
  void compileAtomicLoad(unsigned MemoryIndex, unsigned MemoryOffset,
                         unsigned Alignment, LLVM::Type IntType,
                         LLVM::Type TargetType, bool Signed = false) noexcept {

    auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto VPtr = Builder.createInBoundsGEP1(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);

    auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());
    auto Load = Builder.createLoad(TargetType, Ptr, OptNone);
    Load.setAlignment(1 << Alignment);
    Load.setOrdering(LLVMAtomicOrderingSequentiallyConsistent);

    if (Signed) {
      Stack.back() = Builder.createSExt(Load, IntType);
    } else {
      Stack.back() = Builder.createZExt(Load, IntType);
    }
  }
  void compileAtomicStore(unsigned MemoryIndex, unsigned MemoryOffset,
                          unsigned Alignment, LLVM::Type, LLVM::Type TargetType,
                          bool Signed = false) noexcept {
    auto V = stackPop();

    if (Signed) {
      V = Builder.createSExtOrTrunc(V, TargetType);
    } else {
      V = Builder.createZExtOrTrunc(V, TargetType);
    }
    auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto VPtr = Builder.createInBoundsGEP1(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);
    auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());
    auto Store = Builder.createStore(V, Ptr, OptNone);
    Store.setAlignment(1 << Alignment);
    Store.setOrdering(LLVMAtomicOrderingSequentiallyConsistent);
  }

  void compileAtomicRMWOp(unsigned MemoryIndex, unsigned MemoryOffset,
                          [[maybe_unused]] unsigned Alignment,
                          LLVMAtomicRMWBinOp BinOp, LLVM::Type IntType,
                          LLVM::Type TargetType, bool Signed = false) noexcept {
    auto Value = Builder.createSExtOrTrunc(stackPop(), TargetType);
    auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto VPtr = Builder.createInBoundsGEP1(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);
    auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());

    auto Ret = Builder.createAtomicRMW(
        BinOp, Ptr, Value, LLVMAtomicOrderingSequentiallyConsistent);
#if LLVM_VERSION_MAJOR >= 13
    Ret.setAlignment(1 << Alignment);
#endif
    if (Signed) {
      Stack.back() = Builder.createSExt(Ret, IntType);
    } else {
      Stack.back() = Builder.createZExt(Ret, IntType);
    }
  }
  void compileAtomicCompareExchange(unsigned MemoryIndex, unsigned MemoryOffset,
                                    [[maybe_unused]] unsigned Alignment,
                                    LLVM::Type IntType, LLVM::Type TargetType,
                                    bool Signed = false) noexcept {

    auto Replacement = Builder.createSExtOrTrunc(stackPop(), TargetType);
    auto Expected = Builder.createSExtOrTrunc(stackPop(), TargetType);
    auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto VPtr = Builder.createInBoundsGEP1(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);
    auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());

    auto Ret = Builder.createAtomicCmpXchg(
        Ptr, Expected, Replacement, LLVMAtomicOrderingSequentiallyConsistent,
        LLVMAtomicOrderingSequentiallyConsistent);
#if LLVM_VERSION_MAJOR >= 13
    Ret.setAlignment(1 << Alignment);
#endif
    auto OldVal = Builder.createExtractValue(Ret, 0);
    if (Signed) {
      Stack.back() = Builder.createSExt(OldVal, IntType);
    } else {
      Stack.back() = Builder.createZExt(OldVal, IntType);
    }
  }

  void compileReturn() noexcept {
    updateInstrCount();
    updateGas();
    auto Ty = F.Ty.getReturnType();
    if (Ty.isVoidTy()) {
      Builder.createRetVoid();
    } else if (Ty.isStructTy()) {
      const auto Count = Ty.getStructNumElements();
      std::vector<LLVM::Value> Ret(Count);
      for (unsigned I = 0; I < Count; ++I) {
        const unsigned J = Count - 1 - I;
        Ret[J] = stackPop();
      }
      Builder.createAggregateRet(Ret);
    } else {
      Builder.createRet(stackPop());
    }
  }

  void updateInstrCount() noexcept {
    if (LocalInstrCount) {
      auto Store [[maybe_unused]] = Builder.createAtomicRMW(
          LLVMAtomicRMWBinOpAdd, Context.getInstrCount(Builder, ExecCtx),
          Builder.createLoad(Context.Int64Ty, LocalInstrCount),
          LLVMAtomicOrderingMonotonic);
#if LLVM_VERSION_MAJOR >= 13
      Store.setAlignment(8);
#endif
      Builder.createStore(LLContext.getInt64(0), LocalInstrCount);
    }
  }

  void updateGas() noexcept {
    if (LocalGas) {
      auto CurrBB = Builder.getInsertBlock();
      auto CheckBB = LLVM::BasicBlock::create(LLContext, F.Fn, "gas_check");
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "gas_ok");
      auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "gas_end");

      auto Cost = Builder.createLoad(Context.Int64Ty, LocalGas);
      Cost.setAlignment(64);
      auto GasPtr = Context.getGas(Builder, ExecCtx);
      auto GasLimit = Context.getGasLimit(Builder, ExecCtx);
      auto Gas = Builder.createLoad(Context.Int64Ty, GasPtr);
      Gas.setAlignment(64);
      Gas.setOrdering(LLVMAtomicOrderingMonotonic);
      Builder.createBr(CheckBB);
      Builder.positionAtEnd(CheckBB);

      auto PHIOldGas = Builder.createPHI(Context.Int64Ty);
      auto NewGas = Builder.createAdd(PHIOldGas, Cost);
      auto IsGasRemain =
          Builder.createLikely(Builder.createICmpULE(NewGas, GasLimit));
      Builder.createCondBr(IsGasRemain, OkBB,
                           getTrapBB(ErrCode::Value::CostLimitExceeded));
      Builder.positionAtEnd(OkBB);

      auto RGasAndSucceed = Builder.createAtomicCmpXchg(
          GasPtr, PHIOldGas, NewGas, LLVMAtomicOrderingMonotonic,
          LLVMAtomicOrderingMonotonic);
#if LLVM_VERSION_MAJOR >= 13
      RGasAndSucceed.setAlignment(8);
#endif
      RGasAndSucceed.setWeak(true);
      auto RGas = Builder.createExtractValue(RGasAndSucceed, 0);
      auto Succeed = Builder.createExtractValue(RGasAndSucceed, 1);
      Builder.createCondBr(Builder.createLikely(Succeed), EndBB, CheckBB);
      Builder.positionAtEnd(EndBB);

      Builder.createStore(LLContext.getInt64(0), LocalGas);

      PHIOldGas.addIncoming(Gas, CurrBB);
      PHIOldGas.addIncoming(RGas, OkBB);
    }
  }

  void updateGasAtTrap() noexcept {
    if (LocalGas) {
      auto Update [[maybe_unused]] = Builder.createAtomicRMW(
          LLVMAtomicRMWBinOpAdd, Context.getGas(Builder, ExecCtx),
          Builder.createLoad(Context.Int64Ty, LocalGas),
          LLVMAtomicOrderingMonotonic);
#if LLVM_VERSION_MAJOR >= 13
      Update.setAlignment(8);
#endif
    }
  }

private:
  void compileCallOp(const unsigned int FuncIndex) noexcept {
    const auto &FuncType =
        *Context.FunctionTypes[std::get<0>(Context.Functions[FuncIndex])];
    const auto &Function = std::get<1>(Context.Functions[FuncIndex]);
    const auto &ParamTypes = FuncType.getParamTypes();

    std::vector<LLVM::Value> Args(ParamTypes.size() + 1);
    Args[0] = F.Fn.getFirstParam();
    for (size_t I = 0; I < ParamTypes.size(); ++I) {
      const size_t J = ParamTypes.size() - 1 - I;
      Args[J + 1] = stackPop();
    }

    auto Ret = Builder.createCall(Function, Args);
    auto Ty = Ret.getType();
    if (Ty.isVoidTy()) {
      // nothing to do
    } else if (Ty.isStructTy()) {
      for (auto Val : unpackStruct(Builder, Ret)) {
        stackPush(Val);
      }
    } else {
      stackPush(Ret);
    }
  }

  void compileIndirectCallOp(const uint32_t TableIndex,
                             const uint32_t FuncTypeIndex) noexcept {
    auto NotNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.not_null");
    auto IsNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.is_null");
    auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.end");

    LLVM::Value FuncIndex = stackPop();
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    auto FTy = toLLVMType(Context.LLContext, Context.ExecCtxPtrTy, FuncType);
    auto RTy = FTy.getReturnType();

    const size_t ArgSize = FuncType.getParamTypes().size();
    const size_t RetSize =
        RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();
    std::vector<LLVM::Value> ArgsVec(ArgSize + 1, nullptr);
    ArgsVec[0] = F.Fn.getFirstParam();
    for (size_t I = 0; I < ArgSize; ++I) {
      const size_t J = ArgSize - I;
      ArgsVec[J] = stackPop();
    }

    std::vector<LLVM::Value> FPtrRetsVec;
    FPtrRetsVec.reserve(RetSize);
    {
      auto FPtr = Builder.createCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kPtrFunc,
              LLVM::Type::getFunctionType(
                  FTy.getPointerTo(),
                  {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
          {LLContext.getInt32(TableIndex), LLContext.getInt32(FuncTypeIndex),
           FuncIndex});
      Builder.createCondBr(
          Builder.createLikely(Builder.createNot(Builder.createIsNull(FPtr))),
          NotNullBB, IsNullBB);
      Builder.positionAtEnd(NotNullBB);

      auto FPtrRet =
          Builder.createCall(LLVM::FunctionCallee{FTy, FPtr}, ArgsVec);
      if (RetSize == 0) {
        // nothing to do
      } else if (RetSize == 1) {
        FPtrRetsVec.push_back(FPtrRet);
      } else {
        for (auto Val : unpackStruct(Builder, FPtrRet)) {
          FPtrRetsVec.push_back(Val);
        }
      }
    }

    Builder.createBr(EndBB);
    Builder.positionAtEnd(IsNullBB);

    std::vector<LLVM::Value> RetsVec(RetSize);
    {
      LLVM::Value Args;
      if (ArgSize == 0) {
        Args = LLVM::Value::getConstPointerNull(Context.Int8PtrTy);
      } else {
        auto Alloca = Builder.createArrayAlloca(
            Context.Int8Ty, LLContext.getInt64(ArgSize * kValSize));
        Alloca.setAlignment(kValSize);
        Args = Alloca;
      }

      LLVM::Value Rets;
      if (RetSize == 0) {
        Rets = LLVM::Value::getConstPointerNull(Context.Int8PtrTy);
      } else {
        auto Alloca = Builder.createArrayAlloca(
            Context.Int8Ty, LLContext.getInt64(RetSize * kValSize));
        Alloca.setAlignment(kValSize);
        Rets = Alloca;
      }

      for (size_t I = 0; I < ArgSize; ++I) {
        auto Ptr = Builder.createConstInBoundsGEP1_64(Context.Int8Ty, Args,
                                                      I * kValSize);
        auto Arg = ArgsVec[I + 1];
        Builder.createStore(
            Arg, Builder.createBitCast(Ptr, Arg.getType().getPointerTo()));
      }

      Builder.createCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kCallIndirect,
              LLVM::Type::getFunctionType(Context.VoidTy,
                                          {Context.Int32Ty, Context.Int32Ty,
                                           Context.Int32Ty, Context.Int8PtrTy,
                                           Context.Int8PtrTy},
                                          false)),
          {LLContext.getInt32(TableIndex), LLContext.getInt32(FuncTypeIndex),
           FuncIndex, Args, Rets});

      if (RetSize == 0) {
        // nothing to do
      } else if (RetSize == 1) {
        auto VPtr = Builder.createConstInBoundsGEP1_64(Context.Int8Ty, Rets, 0);
        auto Ptr = Builder.createBitCast(VPtr, RTy.getPointerTo());
        RetsVec[0] = Builder.createLoad(RTy, Ptr);
      } else {
        for (unsigned I = 0; I < RetSize; ++I) {
          auto VPtr = Builder.createConstInBoundsGEP1_64(Context.Int8Ty, Rets,
                                                         I * kValSize);
          auto Ptr = Builder.createBitCast(
              VPtr, RTy.getStructElementType(I).getPointerTo());
          RetsVec[I] = Builder.createLoad(RTy.getStructElementType(I), Ptr);
        }
      }
      Builder.createBr(EndBB);
      Builder.positionAtEnd(EndBB);
    }

    for (unsigned I = 0; I < RetSize; ++I) {
      auto PHIRet = Builder.createPHI(FPtrRetsVec[I].getType());
      PHIRet.addIncoming(FPtrRetsVec[I], NotNullBB);
      PHIRet.addIncoming(RetsVec[I], IsNullBB);
      stackPush(PHIRet);
    }
  }

  void compileReturnCallOp(const unsigned int FuncIndex) noexcept {
    const auto &FuncType =
        *Context.FunctionTypes[std::get<0>(Context.Functions[FuncIndex])];
    const auto &Function = std::get<1>(Context.Functions[FuncIndex]);
    const auto &ParamTypes = FuncType.getParamTypes();

    std::vector<LLVM::Value> Args(ParamTypes.size() + 1);
    Args[0] = F.Fn.getFirstParam();
    for (size_t I = 0; I < ParamTypes.size(); ++I) {
      const size_t J = ParamTypes.size() - 1 - I;
      Args[J + 1] = stackPop();
    }

    auto Ret = Builder.createCall(Function, Args);
    auto Ty = Ret.getType();
    if (Ty.isVoidTy()) {
      Builder.createRetVoid();
    } else {
      Builder.createRet(Ret);
    }
  }

  void compileReturnIndirectCallOp(const uint32_t TableIndex,
                                   const uint32_t FuncTypeIndex) noexcept {
    auto NotNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.not_null");
    auto IsNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.is_null");

    LLVM::Value FuncIndex = stackPop();
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    auto FTy = toLLVMType(Context.LLContext, Context.ExecCtxPtrTy, FuncType);
    auto RTy = FTy.getReturnType();

    const size_t ArgSize = FuncType.getParamTypes().size();
    const size_t RetSize =
        RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();
    std::vector<LLVM::Value> ArgsVec(ArgSize + 1, nullptr);
    ArgsVec[0] = F.Fn.getFirstParam();
    for (size_t I = 0; I < ArgSize; ++I) {
      const size_t J = ArgSize - I;
      ArgsVec[J] = stackPop();
    }

    {
      auto FPtr = Builder.createCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kPtrFunc,
              LLVM::Type::getFunctionType(
                  FTy.getPointerTo(),
                  {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
          {LLContext.getInt32(TableIndex), LLContext.getInt32(FuncTypeIndex),
           FuncIndex});
      Builder.createCondBr(
          Builder.createLikely(Builder.createNot(Builder.createIsNull(FPtr))),
          NotNullBB, IsNullBB);
      Builder.positionAtEnd(NotNullBB);

      auto FPtrRet =
          Builder.createCall(LLVM::FunctionCallee(FTy, FPtr), ArgsVec);
      if (RetSize == 0) {
        Builder.createRetVoid();
      } else {
        Builder.createRet(FPtrRet);
      }
    }

    Builder.positionAtEnd(IsNullBB);

    {
      LLVM::Value Args;
      if (ArgSize == 0) {
        Args = LLVM::Value::getConstPointerNull(Context.Int8PtrTy);
      } else {
        auto Alloca = Builder.createArrayAlloca(
            Context.Int8Ty, LLContext.getInt64(ArgSize * kValSize));
        Alloca.setAlignment(kValSize);
        Args = Alloca;
      }

      LLVM::Value Rets;
      if (RetSize == 0) {
        Rets = LLVM::Value::getConstPointerNull(Context.Int8PtrTy);
      } else {
        auto Alloca = Builder.createArrayAlloca(
            Context.Int8Ty, LLContext.getInt64(RetSize * kValSize));
        Alloca.setAlignment(kValSize);
        Rets = Alloca;
      }

      for (size_t I = 0; I < ArgSize; ++I) {
        auto Ptr = Builder.createConstInBoundsGEP1_64(Context.Int8Ty, Args,
                                                      I * kValSize);
        auto Arg = ArgsVec[I + 1];
        Builder.createStore(
            Arg, Builder.createBitCast(Ptr, Arg.getType().getPointerTo()));
      }

      Builder.createCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kCallIndirect,
              LLVM::Type::getFunctionType(Context.VoidTy,
                                          {Context.Int32Ty, Context.Int32Ty,
                                           Context.Int32Ty, Context.Int8PtrTy,
                                           Context.Int8PtrTy},
                                          false)),
          {LLContext.getInt32(TableIndex), LLContext.getInt32(FuncTypeIndex),
           FuncIndex, Args, Rets});

      if (RetSize == 0) {
        Builder.createRetVoid();
      } else if (RetSize == 1) {
        auto VPtr = Builder.createConstInBoundsGEP1_64(Context.Int8Ty, Rets, 0);
        auto Ptr = Builder.createBitCast(VPtr, RTy.getPointerTo());
        Builder.createRet(Builder.createLoad(RTy, Ptr));
      } else {
        std::vector<LLVM::Value> Ret(RetSize);
        for (unsigned I = 0; I < RetSize; ++I) {
          auto VPtr = Builder.createConstInBoundsGEP1_64(Context.Int8Ty, Rets,
                                                         I * kValSize);
          auto Ptr = Builder.createBitCast(
              VPtr, RTy.getStructElementType(I).getPointerTo());
          Ret[I] = Builder.createLoad(RTy.getStructElementType(I), Ptr);
        }
        Builder.createAggregateRet(Ret);
      }
    }
  }

  void compileLoadOp(unsigned MemoryIndex, unsigned Offset, unsigned Alignment,
                     LLVM::Type LoadTy) noexcept {
    if constexpr (kForceUnalignment) {
      Alignment = 0;
    }
    auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
    if (Offset != 0) {
      Off = Builder.createAdd(Off, LLContext.getInt64(Offset));
    }

    auto VPtr = Builder.createInBoundsGEP1(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Off);
    auto Ptr = Builder.createBitCast(VPtr, LoadTy.getPointerTo());
    auto LoadInst = Builder.createLoad(LoadTy, Ptr, OptNone);
    LoadInst.setAlignment(1 << Alignment);
    stackPush(LoadInst);
  }
  void compileLoadOp(unsigned MemoryIndex, unsigned Offset, unsigned Alignment,
                     LLVM::Type LoadTy, LLVM::Type ExtendTy,
                     bool Signed) noexcept {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    if (Signed) {
      Stack.back() = Builder.createSExt(Stack.back(), ExtendTy);
    } else {
      Stack.back() = Builder.createZExt(Stack.back(), ExtendTy);
    }
  }
  void compileVectorLoadOp(unsigned MemoryIndex, unsigned Offset,
                           unsigned Alignment, LLVM::Type LoadTy) noexcept {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    Stack.back() = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
  }
  void compileVectorLoadOp(unsigned MemoryIndex, unsigned Offset,
                           unsigned Alignment, LLVM::Type LoadTy,
                           LLVM::Type ExtendTy, bool Signed) noexcept {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy, ExtendTy, Signed);
    Stack.back() = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
  }
  void compileSplatLoadOp(unsigned MemoryIndex, unsigned Offset,
                          unsigned Alignment, LLVM::Type LoadTy,
                          LLVM::Type VectorTy) noexcept {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    compileSplatOp(VectorTy);
  }
  void compileLoadLaneOp(unsigned MemoryIndex, unsigned Offset,
                         unsigned Alignment, unsigned Index, LLVM::Type LoadTy,
                         LLVM::Type VectorTy) noexcept {
    auto Vector = stackPop();
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    auto Value = Stack.back();
    Stack.back() = Builder.createBitCast(
        Builder.createInsertElement(Builder.createBitCast(Vector, VectorTy),
                                    Value, LLContext.getInt64(Index)),
        Context.Int64x2Ty);
  }
  void compileStoreLaneOp(unsigned MemoryIndex, unsigned Offset,
                          unsigned Alignment, unsigned Index, LLVM::Type LoadTy,
                          LLVM::Type VectorTy) noexcept {
    auto Vector = Stack.back();
    Stack.back() = Builder.createExtractElement(
        Builder.createBitCast(Vector, VectorTy), LLContext.getInt64(Index));
    compileStoreOp(MemoryIndex, Offset, Alignment, LoadTy);
  }
  void compileStoreOp(unsigned MemoryIndex, unsigned Offset, unsigned Alignment,
                      LLVM::Type LoadTy, bool Trunc = false,
                      bool BitCast = false) noexcept {
    if constexpr (kForceUnalignment) {
      Alignment = 0;
    }
    auto V = stackPop();
    auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
    if (Offset != 0) {
      Off = Builder.createAdd(Off, LLContext.getInt64(Offset));
    }

    if (Trunc) {
      V = Builder.createTrunc(V, LoadTy);
    }
    if (BitCast) {
      V = Builder.createBitCast(V, LoadTy);
    }
    auto VPtr = Builder.createInBoundsGEP1(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Off);
    auto Ptr = Builder.createBitCast(VPtr, LoadTy.getPointerTo());
    auto StoreInst = Builder.createStore(V, Ptr, OptNone);
    StoreInst.setAlignment(1 << Alignment);
  }
  void compileSplatOp(LLVM::Type VectorTy) noexcept {
    auto Undef = LLVM::Value::getUndef(VectorTy);
    auto Zeros = LLVM::Value::getConstNull(
        LLVM::Type::getVectorType(Context.Int32Ty, VectorTy.getVectorSize()));
    auto Value = Builder.createTrunc(Stack.back(), VectorTy.getElementType());
    auto Vector =
        Builder.createInsertElement(Undef, Value, LLContext.getInt64(0));
    Vector = Builder.createShuffleVector(Vector, Undef, Zeros);

    Stack.back() = Builder.createBitCast(Vector, Context.Int64x2Ty);
  }
  void compileExtractLaneOp(LLVM::Type VectorTy, unsigned Index) noexcept {
    auto Vector = Builder.createBitCast(Stack.back(), VectorTy);
    Stack.back() =
        Builder.createExtractElement(Vector, LLContext.getInt64(Index));
  }
  void compileExtractLaneOp(LLVM::Type VectorTy, unsigned Index,
                            LLVM::Type ExtendTy, bool Signed) noexcept {
    compileExtractLaneOp(VectorTy, Index);
    if (Signed) {
      Stack.back() = Builder.createSExt(Stack.back(), ExtendTy);
    } else {
      Stack.back() = Builder.createZExt(Stack.back(), ExtendTy);
    }
  }
  void compileReplaceLaneOp(LLVM::Type VectorTy, unsigned Index) noexcept {
    auto Value = Builder.createTrunc(stackPop(), VectorTy.getElementType());
    auto Vector = Stack.back();
    Stack.back() = Builder.createBitCast(
        Builder.createInsertElement(Builder.createBitCast(Vector, VectorTy),
                                    Value, LLContext.getInt64(Index)),
        Context.Int64x2Ty);
  }
  void compileVectorCompareOp(LLVM::Type VectorTy,
                              LLVMIntPredicate Predicate) noexcept {
    auto RHS = stackPop();
    auto LHS = stackPop();
    auto Result = Builder.createSExt(
        Builder.createICmp(Predicate, Builder.createBitCast(LHS, VectorTy),
                           Builder.createBitCast(RHS, VectorTy)),
        VectorTy);
    stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
  }
  void compileVectorCompareOp(LLVM::Type VectorTy, LLVMRealPredicate Predicate,
                              LLVM::Type ResultTy) noexcept {
    auto RHS = stackPop();
    auto LHS = stackPop();
    auto Result = Builder.createSExt(
        Builder.createFCmp(Predicate, Builder.createBitCast(LHS, VectorTy),
                           Builder.createBitCast(RHS, VectorTy)),
        ResultTy);
    stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
  }
  template <typename Func>
  void compileVectorOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    auto V = Builder.createBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.createBitCast(Op(V), Context.Int64x2Ty);
  }
  void compileVectorAbs(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy, [this, VectorTy](auto V) noexcept {
      auto Zero = LLVM::Value::getConstNull(VectorTy);
      auto C = Builder.createICmpSLT(V, Zero);
      return Builder.createSelect(C, Builder.createNeg(V), V);
    });
  }
  void compileVectorNeg(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy,
                    [this](auto V) noexcept { return Builder.createNeg(V); });
  }
  void compileVectorPopcnt() noexcept {
    compileVectorOp(Context.Int8x16Ty, [this](auto V) noexcept {
      assuming(LLVM::Core::Ctpop != LLVM::Core::NotIntrinsic);
      return Builder.createUnaryIntrinsic(LLVM::Core::Ctpop, V);
    });
  }
  template <typename Func>
  void compileVectorReduceIOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    auto V = Builder.createBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.createZExt(Op(V), Context.Int32Ty);
  }
  void compileVectorAnyTrue() noexcept {
    compileVectorReduceIOp(Context.Int128x1Ty, [this](auto V) noexcept {
      auto Zero = LLVM::Value::getConstNull(Context.Int128x1Ty);
      return Builder.createBitCast(Builder.createICmpNE(V, Zero),
                                   LLContext.getInt1Ty());
    });
  }
  void compileVectorAllTrue(LLVM::Type VectorTy) noexcept {
    compileVectorReduceIOp(VectorTy, [this, VectorTy](auto V) noexcept {
      const auto Size = VectorTy.getVectorSize();
      auto IntType = LLContext.getIntNTy(Size);
      auto Zero = LLVM::Value::getConstNull(VectorTy);
      auto Cmp = Builder.createBitCast(Builder.createICmpEQ(V, Zero), IntType);
      auto CmpZero = LLVM::Value::getConstInt(IntType, 0);
      return Builder.createICmpEQ(Cmp, CmpZero);
    });
  }
  void compileVectorBitMask(LLVM::Type VectorTy) noexcept {
    compileVectorReduceIOp(VectorTy, [this, VectorTy](auto V) noexcept {
      const auto Size = VectorTy.getVectorSize();
      auto IntType = LLContext.getIntNTy(Size);
      auto Zero = LLVM::Value::getConstNull(VectorTy);
      return Builder.createBitCast(Builder.createICmpSLT(V, Zero), IntType);
    });
  }
  template <typename Func>
  void compileVectorShiftOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    const bool Trunc = VectorTy.getElementType().getIntegerBitWidth() < 32;
    const uint32_t Mask = VectorTy.getElementType().getIntegerBitWidth() - 1;
    auto N = Builder.createAnd(stackPop(), LLContext.getInt32(Mask));
    auto RHS = Builder.createVectorSplat(
        VectorTy.getVectorSize(),
        Trunc ? Builder.createTrunc(N, VectorTy.getElementType())
              : Builder.createZExtOrTrunc(N, VectorTy.getElementType()));
    auto LHS = Builder.createBitCast(stackPop(), VectorTy);
    stackPush(Builder.createBitCast(Op(LHS, RHS), Context.Int64x2Ty));
  }
  void compileVectorShl(LLVM::Type VectorTy) noexcept {
    compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createShl(LHS, RHS);
    });
  }
  void compileVectorLShr(LLVM::Type VectorTy) noexcept {
    compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createLShr(LHS, RHS);
    });
  }
  void compileVectorAShr(LLVM::Type VectorTy) noexcept {
    compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createAShr(LHS, RHS);
    });
  }
  template <typename Func>
  void compileVectorVectorOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    auto RHS = Builder.createBitCast(stackPop(), VectorTy);
    auto LHS = Builder.createBitCast(stackPop(), VectorTy);
    stackPush(Builder.createBitCast(Op(LHS, RHS), Context.Int64x2Ty));
  }
  void compileVectorVectorAdd(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createAdd(LHS, RHS);
    });
  }
  void compileVectorVectorAddSat(LLVM::Type VectorTy, bool Signed) noexcept {
    auto ID = Signed ? LLVM::Core::SAddSat : LLVM::Core::UAddSat;
    assuming(ID != LLVM::Core::NotIntrinsic);
    compileVectorVectorOp(VectorTy, [this, ID](auto LHS, auto RHS) noexcept {
      return Builder.createBinaryIntrinsic(ID, LHS, RHS);
    });
  }
  void compileVectorVectorSub(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createSub(LHS, RHS);
    });
  }
  void compileVectorVectorSubSat(LLVM::Type VectorTy, bool Signed) noexcept {
    auto ID = Signed ? LLVM::Core::SSubSat : LLVM::Core::USubSat;
    assuming(ID != LLVM::Core::NotIntrinsic);
    compileVectorVectorOp(VectorTy, [this, ID](auto LHS, auto RHS) noexcept {
      return Builder.createBinaryIntrinsic(ID, LHS, RHS);
    });
  }
  void compileVectorVectorMul(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createMul(LHS, RHS);
    });
  }
  void compileVectorVectorQ15MulSat() noexcept {
    compileVectorVectorOp(
        Context.Int16x8Ty, [this](auto LHS, auto RHS) noexcept -> LLVM::Value {
#if defined(__x86_64__)
          if (Context.SupportSSSE3) {
            assuming(LLVM::Core::X86SSSE3PMulHrSw128 !=
                     LLVM::Core::NotIntrinsic);
            auto Result = Builder.createIntrinsic(
                LLVM::Core::X86SSSE3PMulHrSw128, {}, {LHS, RHS});
            auto IntMaxV = Builder.createVectorSplat(
                8, LLContext.getInt16(UINT16_C(0x8000)));
            auto NotOver = Builder.createSExt(
                Builder.createICmpEQ(Result, IntMaxV), Context.Int16x8Ty);
            return Builder.createXor(Result, NotOver);
          }
#endif

#if defined(__aarch64__)
          if (Context.SupportNEON) {
            assuming(LLVM::Core::AArch64NeonSQRDMulH !=
                     LLVM::Core::NotIntrinsic);
            return Builder.createBinaryIntrinsic(
                LLVM::Core::AArch64NeonSQRDMulH, LHS, RHS);
          }
#endif

          // Fallback case.
          // If the SSSE3 is not supported on the x86_64 platform or
          // the NEON is not supported on the aarch64 platform,
          // then fallback to this.
          auto ExtTy = Context.Int16x8Ty.getExtendedElementVectorType();
          auto Offset = Builder.createVectorSplat(
              8, LLContext.getInt32(UINT32_C(0x4000)));
          auto Shift =
              Builder.createVectorSplat(8, LLContext.getInt32(UINT32_C(15)));
          auto ExtLHS = Builder.createSExt(LHS, ExtTy);
          auto ExtRHS = Builder.createSExt(RHS, ExtTy);
          auto Result = Builder.createTrunc(
              Builder.createAShr(
                  Builder.createAdd(Builder.createMul(ExtLHS, ExtRHS), Offset),
                  Shift),
              Context.Int16x8Ty);
          auto IntMaxV = Builder.createVectorSplat(
              8, LLContext.getInt16(UINT16_C(0x8000)));
          auto NotOver = Builder.createSExt(
              Builder.createICmpEQ(Result, IntMaxV), Context.Int16x8Ty);
          return Builder.createXor(Result, NotOver);
        });
  }
  void compileVectorVectorSMin(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto C = Builder.createICmpSLE(LHS, RHS);
      return Builder.createSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorUMin(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto C = Builder.createICmpULE(LHS, RHS);
      return Builder.createSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorSMax(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto C = Builder.createICmpSGE(LHS, RHS);
      return Builder.createSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorUMax(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto C = Builder.createICmpUGE(LHS, RHS);
      return Builder.createSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorUAvgr(LLVM::Type VectorTy) noexcept {
    auto ExtendTy = VectorTy.getExtendedElementVectorType();
    compileVectorVectorOp(
        VectorTy,
        [this, VectorTy, ExtendTy](auto LHS, auto RHS) noexcept -> LLVM::Value {
#if defined(__x86_64__)
          if (Context.SupportSSE2) {
            const auto ID = [VectorTy]() noexcept {
              switch (VectorTy.getElementType().getIntegerBitWidth()) {
              case 8:
                return LLVM::Core::X86SSE2PAvgB;
              case 16:
                return LLVM::Core::X86SSE2PAvgW;
              default:
                assumingUnreachable();
              }
            }();
            assuming(ID != LLVM::Core::NotIntrinsic);
            return Builder.createIntrinsic(ID, {}, {LHS, RHS});
          }
#endif

#if defined(__aarch64__)
          if (Context.SupportNEON) {
            assuming(LLVM::Core::AArch64NeonURHAdd != LLVM::Core::NotIntrinsic);
            return Builder.createBinaryIntrinsic(LLVM::Core::AArch64NeonURHAdd,
                                                 LHS, RHS);
          }
#endif

          // Fallback case.
          // If the SSE2 is not supported on the x86_64 platform or
          // the NEON is not supported on the aarch64 platform,
          // then fallback to this.
          auto EL = Builder.createZExt(LHS, ExtendTy);
          auto ER = Builder.createZExt(RHS, ExtendTy);
          auto One = Builder.createZExt(
              Builder.createVectorSplat(ExtendTy.getVectorSize(),
                                        LLContext.getTrue()),
              ExtendTy);
          return Builder.createTrunc(
              Builder.createLShr(
                  Builder.createAdd(Builder.createAdd(EL, ER), One), One),
              VectorTy);
        });
  }
  void compileVectorNarrow(LLVM::Type FromTy, bool Signed) noexcept {
    auto [MinInt,
          MaxInt] = [&]() noexcept -> std::tuple<LLVM::Value, LLVM::Value> {
      switch (FromTy.getElementType().getIntegerBitWidth()) {
      case 16: {
        const auto Min =
            static_cast<int16_t>(Signed ? std::numeric_limits<int8_t>::min()
                                        : std::numeric_limits<uint8_t>::min());
        const auto Max =
            static_cast<int16_t>(Signed ? std::numeric_limits<int8_t>::max()
                                        : std::numeric_limits<uint8_t>::max());
        return {LLContext.getInt16(static_cast<uint16_t>(Min)),
                LLContext.getInt16(static_cast<uint16_t>(Max))};
      }
      case 32: {
        const auto Min =
            static_cast<int32_t>(Signed ? std::numeric_limits<int16_t>::min()
                                        : std::numeric_limits<uint16_t>::min());
        const auto Max =
            static_cast<int32_t>(Signed ? std::numeric_limits<int16_t>::max()
                                        : std::numeric_limits<uint16_t>::max());
        return {LLContext.getInt32(static_cast<uint32_t>(Min)),
                LLContext.getInt32(static_cast<uint32_t>(Max))};
      }
      default:
        assumingUnreachable();
      }
    }();
    const auto Count = FromTy.getVectorSize();
    auto VMin = Builder.createVectorSplat(Count, MinInt);
    auto VMax = Builder.createVectorSplat(Count, MaxInt);

    auto TruncTy = FromTy.getTruncatedElementVectorType();

    auto F2 = Builder.createBitCast(stackPop(), FromTy);
    F2 = Builder.createSelect(Builder.createICmpSLT(F2, VMin), VMin, F2);
    F2 = Builder.createSelect(Builder.createICmpSGT(F2, VMax), VMax, F2);
    F2 = Builder.createTrunc(F2, TruncTy);

    auto F1 = Builder.createBitCast(stackPop(), FromTy);
    F1 = Builder.createSelect(Builder.createICmpSLT(F1, VMin), VMin, F1);
    F1 = Builder.createSelect(Builder.createICmpSGT(F1, VMax), VMax, F1);
    F1 = Builder.createTrunc(F1, TruncTy);

    std::vector<uint32_t> Mask(Count * 2);
    std::iota(Mask.begin(), Mask.end(), 0);
    stackPush(Builder.createBitCast(
        Builder.createShuffleVector(
            F1, F2, LLVM::Value::getConstVector32(LLContext, Mask)),
        Context.Int64x2Ty));
  }
  void compileVectorExtend(LLVM::Type FromTy, bool Signed, bool Low) noexcept {
    auto ExtTy = FromTy.getExtendedElementVectorType();
    const auto Count = FromTy.getVectorSize();
    std::vector<uint32_t> Mask(Count / 2);
    std::iota(Mask.begin(), Mask.end(), Low ? 0 : Count / 2);
    auto R = Builder.createBitCast(Stack.back(), FromTy);
    if (Signed) {
      R = Builder.createSExt(R, ExtTy);
    } else {
      R = Builder.createZExt(R, ExtTy);
    }
    R = Builder.createShuffleVector(
        R, LLVM::Value::getUndef(ExtTy),
        LLVM::Value::getConstVector32(LLContext, Mask));
    Stack.back() = Builder.createBitCast(R, Context.Int64x2Ty);
  }
  void compileVectorExtMul(LLVM::Type FromTy, bool Signed, bool Low) noexcept {
    auto ExtTy = FromTy.getExtendedElementVectorType();
    const auto Count = FromTy.getVectorSize();
    std::vector<uint32_t> Mask(Count / 2);
    std::iota(Mask.begin(), Mask.end(), Low ? 0 : Count / 2);
    auto Extend = [this, FromTy, Signed, ExtTy, &Mask](LLVM::Value R) noexcept {
      R = Builder.createBitCast(R, FromTy);
      if (Signed) {
        R = Builder.createSExt(R, ExtTy);
      } else {
        R = Builder.createZExt(R, ExtTy);
      }
      return Builder.createShuffleVector(
          R, LLVM::Value::getUndef(ExtTy),
          LLVM::Value::getConstVector32(LLContext, Mask));
    };
    auto RHS = Extend(stackPop());
    auto LHS = Extend(stackPop());
    stackPush(
        Builder.createBitCast(Builder.createMul(RHS, LHS), Context.Int64x2Ty));
  }
  void compileVectorExtAddPairwise(LLVM::Type VectorTy, bool Signed) noexcept {
    compileVectorOp(
        VectorTy, [this, VectorTy, Signed](auto V) noexcept -> LLVM::Value {
          auto ExtTy = VectorTy.getExtendedElementVectorType()
                           .getHalfElementsVectorType();
#if defined(__x86_64__)
          const auto Count = VectorTy.getVectorSize();
          if (Context.SupportXOP) {
            const auto ID = [Count, Signed]() noexcept {
              switch (Count) {
              case 8:
                return Signed ? LLVM::Core::X86XOpVPHAddWD
                              : LLVM::Core::X86XOpVPHAddUWD;
              case 16:
                return Signed ? LLVM::Core::X86XOpVPHAddBW
                              : LLVM::Core::X86XOpVPHAddUBW;
              default:
                assumingUnreachable();
              }
            }();
            assuming(ID != LLVM::Core::NotIntrinsic);
            return Builder.createUnaryIntrinsic(ID, V);
          }
          if (Context.SupportSSSE3 && Count == 16) {
            assuming(LLVM::Core::X86SSSE3PMAddUbSw128 !=
                     LLVM::Core::NotIntrinsic);
            if (Signed) {
              return Builder.createIntrinsic(
                  LLVM::Core::X86SSSE3PMAddUbSw128, {},
                  {Builder.createVectorSplat(16, LLContext.getInt8(1)), V});
            } else {
              return Builder.createIntrinsic(
                  LLVM::Core::X86SSSE3PMAddUbSw128, {},
                  {V, Builder.createVectorSplat(16, LLContext.getInt8(1))});
            }
          }
          if (Context.SupportSSE2 && Count == 8) {
            assuming(LLVM::Core::X86SSE2PMAddWd != LLVM::Core::NotIntrinsic);
            if (Signed) {
              return Builder.createIntrinsic(
                  LLVM::Core::X86SSE2PMAddWd, {},
                  {V, Builder.createVectorSplat(8, LLContext.getInt16(1))});
            } else {
              V = Builder.createXor(
                  V, Builder.createVectorSplat(8, LLContext.getInt16(0x8000)));
              V = Builder.createIntrinsic(
                  LLVM::Core::X86SSE2PMAddWd, {},
                  {V, Builder.createVectorSplat(8, LLContext.getInt16(1))});
              return Builder.createAdd(
                  V, Builder.createVectorSplat(4, LLContext.getInt32(0x10000)));
            }
          }
#endif

#if defined(__aarch64__)
          if (Context.SupportNEON) {
            const auto ID = Signed ? LLVM::Core::AArch64NeonSAddLP
                                   : LLVM::Core::AArch64NeonUAddLP;
            assuming(ID != LLVM::Core::NotIntrinsic);
            return Builder.createIntrinsic(ID, {ExtTy, VectorTy}, {V});
          }
#endif

          // Fallback case.
          // If the XOP, SSSE3, or SSE2 is not supported on the x86_64 platform
          // or the NEON is not supported on the aarch64 platform,
          // then fallback to this.
          const auto Width = LLContext.getInt32(
              VectorTy.getElementType().getIntegerBitWidth());
          auto EV = Builder.createBitCast(V, ExtTy);
          LLVM::Value L, R;
          if (Signed) {
            L = Builder.createAShr(EV, Width);
            R = Builder.createAShr(Builder.createShl(EV, Width), Width);
          } else {
            L = Builder.createLShr(EV, Width);
            R = Builder.createLShr(Builder.createShl(EV, Width), Width);
          }
          return Builder.createAdd(L, R);
        });
  }
  void compileVectorFAbs(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy, [this](auto V) noexcept {
      assuming(LLVM::Core::Fabs != LLVM::Core::NotIntrinsic);
      return Builder.createUnaryIntrinsic(LLVM::Core::Fabs, V);
    });
  }
  void compileVectorFNeg(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy,
                    [this](auto V) noexcept { return Builder.createFNeg(V); });
  }
  void compileVectorFSqrt(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy, [this](auto V) noexcept {
      assuming(LLVM::Core::Sqrt != LLVM::Core::NotIntrinsic);
      return Builder.createUnaryIntrinsic(LLVM::Core::Sqrt, V);
    });
  }
  void compileVectorFCeil(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy, [this](auto V) noexcept {
      assuming(LLVM::Core::Ceil != LLVM::Core::NotIntrinsic);
      return Builder.createUnaryIntrinsic(LLVM::Core::Ceil, V);
    });
  }
  void compileVectorFFloor(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy, [this](auto V) noexcept {
      assuming(LLVM::Core::Floor != LLVM::Core::NotIntrinsic);
      return Builder.createUnaryIntrinsic(LLVM::Core::Floor, V);
    });
  }
  void compileVectorFTrunc(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy, [this](auto V) noexcept {
      assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
      return Builder.createUnaryIntrinsic(LLVM::Core::Trunc, V);
    });
  }
  void compileVectorFNearest(LLVM::Type VectorTy) noexcept {
    compileVectorOp(VectorTy, [&](auto V) noexcept {
#if LLVM_VERSION_MAJOR >= 12
      assuming(LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic);
      if (LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic) {
        return Builder.createUnaryIntrinsic(LLVM::Core::Roundeven, V);
      }
#endif

#if defined(__x86_64__)
      if (Context.SupportSSE4_1) {
        const bool IsFloat = VectorTy.getElementType().isFloatTy();
        auto ID =
            IsFloat ? LLVM::Core::X86SSE41RoundPs : LLVM::Core::X86SSE41RoundPd;
        assuming(ID != LLVM::Core::NotIntrinsic);
        return Builder.createIntrinsic(ID, {}, {V, LLContext.getInt32(8)});
      }
#endif

#if defined(__aarch64__)
      if (Context.SupportNEON &&
          LLVM::Core::AArch64NeonFRIntN != LLVM::Core::NotIntrinsic) {
        return Builder.createUnaryIntrinsic(LLVM::Core::AArch64NeonFRIntN, V);
      }
#endif

      // Fallback case.
      // If the SSE4.1 is not supported on the x86_64 platform or
      // the NEON is not supported on the aarch64 platform,
      // then fallback to this.
      assuming(LLVM::Core::Nearbyint != LLVM::Core::NotIntrinsic);
      return Builder.createUnaryIntrinsic(LLVM::Core::Nearbyint, V);
    });
  }
  void compileVectorVectorFAdd(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createFAdd(LHS, RHS);
    });
  }
  void compileVectorVectorFSub(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createFSub(LHS, RHS);
    });
  }
  void compileVectorVectorFMul(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createFMul(LHS, RHS);
    });
  }
  void compileVectorVectorFDiv(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      return Builder.createFDiv(LHS, RHS);
    });
  }
  void compileVectorVectorFMin(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto LNaN = Builder.createFCmpUNO(LHS, LHS);
      auto RNaN = Builder.createFCmpUNO(RHS, RHS);
      auto OLT = Builder.createFCmpOLT(LHS, RHS);
      auto OGT = Builder.createFCmpOGT(LHS, RHS);
      auto Ret = Builder.createBitCast(
          Builder.createOr(Builder.createBitCast(LHS, Context.Int64x2Ty),
                           Builder.createBitCast(RHS, Context.Int64x2Ty)),
          LHS.getType());
      Ret = Builder.createSelect(OGT, RHS, Ret);
      Ret = Builder.createSelect(OLT, LHS, Ret);
      Ret = Builder.createSelect(RNaN, RHS, Ret);
      Ret = Builder.createSelect(LNaN, LHS, Ret);
      return Ret;
    });
  }
  void compileVectorVectorFMax(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto LNaN = Builder.createFCmpUNO(LHS, LHS);
      auto RNaN = Builder.createFCmpUNO(RHS, RHS);
      auto OLT = Builder.createFCmpOLT(LHS, RHS);
      auto OGT = Builder.createFCmpOGT(LHS, RHS);
      auto Ret = Builder.createBitCast(
          Builder.createAnd(Builder.createBitCast(LHS, Context.Int64x2Ty),
                            Builder.createBitCast(RHS, Context.Int64x2Ty)),
          LHS.getType());
      Ret = Builder.createSelect(OLT, RHS, Ret);
      Ret = Builder.createSelect(OGT, LHS, Ret);
      Ret = Builder.createSelect(RNaN, RHS, Ret);
      Ret = Builder.createSelect(LNaN, LHS, Ret);
      return Ret;
    });
  }
  void compileVectorVectorFPMin(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto Cmp = Builder.createFCmpOLT(RHS, LHS);
      return Builder.createSelect(Cmp, RHS, LHS);
    });
  }
  void compileVectorVectorFPMax(LLVM::Type VectorTy) noexcept {
    compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
      auto Cmp = Builder.createFCmpOGT(RHS, LHS);
      return Builder.createSelect(Cmp, RHS, LHS);
    });
  }
  void compileVectorTruncSatS32(LLVM::Type VectorTy, bool PadZero) noexcept {
    compileVectorOp(VectorTy, [this, VectorTy, PadZero](auto V) noexcept {
      const auto Size = VectorTy.getVectorSize();
      auto FPTy = VectorTy.getElementType();
      auto IntMin = LLContext.getInt32(
          static_cast<uint32_t>(std::numeric_limits<int32_t>::min()));
      auto IntMax = LLContext.getInt32(
          static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));
      auto IntMinV = Builder.createVectorSplat(Size, IntMin);
      auto IntMaxV = Builder.createVectorSplat(Size, IntMax);
      auto IntZeroV = LLVM::Value::getConstNull(IntMinV.getType());
      auto FPMin = Builder.createSIToFP(IntMin, FPTy);
      auto FPMax = Builder.createSIToFP(IntMax, FPTy);
      auto FPMinV = Builder.createVectorSplat(Size, FPMin);
      auto FPMaxV = Builder.createVectorSplat(Size, FPMax);

      auto Normal = Builder.createFCmpORD(V, V);
      auto NotUnder = Builder.createFCmpUGE(V, FPMinV);
      auto NotOver = Builder.createFCmpULT(V, FPMaxV);
      V = Builder.createFPToSI(
          V, LLVM::Type::getVectorType(LLContext.getInt32Ty(), Size));
      V = Builder.createSelect(Normal, V, IntZeroV);
      V = Builder.createSelect(NotUnder, V, IntMinV);
      V = Builder.createSelect(NotOver, V, IntMaxV);
      if (PadZero) {
        std::vector<uint32_t> Mask(Size * 2);
        std::iota(Mask.begin(), Mask.end(), 0);
        V = Builder.createShuffleVector(
            V, IntZeroV, LLVM::Value::getConstVector32(LLContext, Mask));
      }
      return V;
    });
  }
  void compileVectorTruncSatU32(LLVM::Type VectorTy, bool PadZero) noexcept {
    compileVectorOp(VectorTy, [this, VectorTy, PadZero](auto V) noexcept {
      const auto Size = VectorTy.getVectorSize();
      auto FPTy = VectorTy.getElementType();
      auto IntMin = LLContext.getInt32(std::numeric_limits<uint32_t>::min());
      auto IntMax = LLContext.getInt32(std::numeric_limits<uint32_t>::max());
      auto IntMinV = Builder.createVectorSplat(Size, IntMin);
      auto IntMaxV = Builder.createVectorSplat(Size, IntMax);
      auto FPMin = Builder.createUIToFP(IntMin, FPTy);
      auto FPMax = Builder.createUIToFP(IntMax, FPTy);
      auto FPMinV = Builder.createVectorSplat(Size, FPMin);
      auto FPMaxV = Builder.createVectorSplat(Size, FPMax);

      auto NotUnder = Builder.createFCmpOGE(V, FPMinV);
      auto NotOver = Builder.createFCmpULT(V, FPMaxV);
      V = Builder.createFPToUI(
          V, LLVM::Type::getVectorType(LLContext.getInt32Ty(), Size));
      V = Builder.createSelect(NotUnder, V, IntMinV);
      V = Builder.createSelect(NotOver, V, IntMaxV);
      if (PadZero) {
        auto IntZeroV = LLVM::Value::getConstNull(IntMinV.getType());
        std::vector<uint32_t> Mask(Size * 2);
        std::iota(Mask.begin(), Mask.end(), 0);
        V = Builder.createShuffleVector(
            V, IntZeroV, LLVM::Value::getConstVector32(LLContext, Mask));
      }
      return V;
    });
  }
  void compileVectorConvertS(LLVM::Type VectorTy, LLVM::Type FPVectorTy,
                             bool Low) noexcept {
    compileVectorOp(VectorTy,
                    [this, VectorTy, FPVectorTy, Low](auto V) noexcept {
                      if (Low) {
                        const auto Size = VectorTy.getVectorSize() / 2;
                        std::vector<uint32_t> Mask(Size);
                        std::iota(Mask.begin(), Mask.end(), 0);
                        V = Builder.createShuffleVector(
                            V, LLVM::Value::getUndef(VectorTy),
                            LLVM::Value::getConstVector32(LLContext, Mask));
                      }
                      return Builder.createSIToFP(V, FPVectorTy);
                    });
  }
  void compileVectorConvertU(LLVM::Type VectorTy, LLVM::Type FPVectorTy,
                             bool Low) noexcept {
    compileVectorOp(VectorTy,
                    [this, VectorTy, FPVectorTy, Low](auto V) noexcept {
                      if (Low) {
                        const auto Size = VectorTy.getVectorSize() / 2;
                        std::vector<uint32_t> Mask(Size);
                        std::iota(Mask.begin(), Mask.end(), 0);
                        V = Builder.createShuffleVector(
                            V, LLVM::Value::getUndef(VectorTy),
                            LLVM::Value::getConstVector32(LLContext, Mask));
                      }
                      return Builder.createUIToFP(V, FPVectorTy);
                    });
  }
  void compileVectorDemote() noexcept {
    compileVectorOp(Context.Doublex2Ty, [this](auto V) noexcept {
      auto Demoted = Builder.createFPTrunc(
          V, LLVM::Type::getVectorType(Context.FloatTy, 2));
      auto ZeroV = LLVM::Value::getConstNull(Demoted.getType());
      return Builder.createShuffleVector(
          Demoted, ZeroV,
          LLVM::Value::getConstVector32(LLContext, {0u, 1u, 2u, 3u}));
    });
  }
  void compileVectorPromote() noexcept {
    compileVectorOp(Context.Floatx4Ty, [this](auto V) noexcept {
      auto UndefV = LLVM::Value::getUndef(V.getType());
      auto Low = Builder.createShuffleVector(
          V, UndefV, LLVM::Value::getConstVector32(LLContext, {0u, 1u}));
      return Builder.createFPExt(
          Low, LLVM::Type::getVectorType(Context.DoubleTy, 2));
    });
  }

  void
  enterBlock(LLVM::BasicBlock JumpBlock, LLVM::BasicBlock NextBlock,
             LLVM::BasicBlock ElseBlock, std::vector<LLVM::Value> Args,
             std::pair<std::vector<ValType>, std::vector<ValType>> Type,
             std::vector<std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
                 ReturnPHI = {}) noexcept {
    assuming(Type.first.size() == Args.size());
    for (auto &Value : Args) {
      stackPush(Value);
    }
    const auto Unreachable = isUnreachable();
    ControlStack.emplace_back(Stack.size() - Args.size(), Unreachable,
                              JumpBlock, NextBlock, ElseBlock, std::move(Args),
                              std::move(Type), std::move(ReturnPHI));
  }

  Control leaveBlock() noexcept {
    Control Entry = std::move(ControlStack.back());
    ControlStack.pop_back();

    auto NextBlock = Entry.NextBlock ? Entry.NextBlock : Entry.JumpBlock;
    if (!Entry.Unreachable) {
      const auto &ReturnType = Entry.Type.second;
      if (!ReturnType.empty()) {
        std::vector<LLVM::Value> Rets(ReturnType.size());
        for (size_t I = 0; I < Rets.size(); ++I) {
          const size_t J = Rets.size() - 1 - I;
          Rets[J] = stackPop();
        }
        Entry.ReturnPHI.emplace_back(std::move(Rets), Builder.getInsertBlock());
      }
      Builder.createBr(NextBlock);
    } else {
      Builder.createUnreachable();
    }
    Builder.positionAtEnd(NextBlock);
    Stack.erase(Stack.begin() + static_cast<int64_t>(Entry.StackSize),
                Stack.end());
    return Entry;
  }

  void checkStop() noexcept {
    if (!Interruptible) {
      return;
    }
    auto NotStopBB = LLVM::BasicBlock::create(LLContext, F.Fn, "NotStop");
    auto StopToken = Builder.createAtomicRMW(
        LLVMAtomicRMWBinOpXchg, Context.getStopToken(Builder, ExecCtx),
        LLContext.getInt32(0), LLVMAtomicOrderingMonotonic);
#if LLVM_VERSION_MAJOR >= 13
    StopToken.setAlignment(32);
#endif
    auto NotStop = Builder.createLikely(
        Builder.createICmpEQ(StopToken, LLContext.getInt32(0)));
    Builder.createCondBr(NotStop, NotStopBB,
                         getTrapBB(ErrCode::Value::Interrupted));

    Builder.positionAtEnd(NotStopBB);
  }

  void setUnreachable() noexcept {
    if (ControlStack.empty()) {
      IsUnreachable = true;
    } else {
      ControlStack.back().Unreachable = true;
    }
  }

  bool isUnreachable() const noexcept {
    if (ControlStack.empty()) {
      return IsUnreachable;
    } else {
      return ControlStack.back().Unreachable;
    }
  }

  void
  buildPHI(Span<const ValType> RetType,
           Span<const std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
               Incomings) noexcept {
    if (isVoidReturn(RetType)) {
      return;
    }
    std::vector<LLVM::Value> Nodes;
    if (Incomings.size() == 0) {
      const auto &Types = toLLVMTypeVector(LLContext, RetType);
      Nodes.reserve(Types.size());
      for (LLVM::Type Type : Types) {
        Nodes.push_back(LLVM::Value::getUndef(Type));
      }
    } else if (Incomings.size() == 1) {
      Nodes = std::move(std::get<0>(Incomings.front()));
    } else {
      const auto &Types = toLLVMTypeVector(LLContext, RetType);
      Nodes.reserve(Types.size());
      for (size_t I = 0; I < Types.size(); ++I) {
        auto PHIRet = Builder.createPHI(Types[I]);
        for (auto &[Value, BB] : Incomings) {
          assuming(Value.size() == Types.size());
          PHIRet.addIncoming(Value[I], BB);
        }
        Nodes.push_back(PHIRet);
      }
    }
    for (auto &Val : Nodes) {
      stackPush(Val);
    }
  }

  void setLableJumpPHI(unsigned int Index) noexcept {
    assuming(Index < ControlStack.size());
    auto &Entry = *(ControlStack.rbegin() + Index);
    if (Entry.NextBlock) { // is loop
      std::vector<LLVM::Value> Args(Entry.Type.first.size());
      for (size_t I = 0; I < Args.size(); ++I) {
        const size_t J = Args.size() - 1 - I;
        Args[J] = stackPop();
      }
      for (size_t I = 0; I < Args.size(); ++I) {
        Entry.Args[I].addIncoming(Args[I], Builder.getInsertBlock());
        stackPush(Args[I]);
      }
    } else if (!Entry.Type.second.empty()) { // has return value
      std::vector<LLVM::Value> Rets(Entry.Type.second.size());
      for (size_t I = 0; I < Rets.size(); ++I) {
        const size_t J = Rets.size() - 1 - I;
        Rets[J] = stackPop();
      }
      for (size_t I = 0; I < Rets.size(); ++I) {
        stackPush(Rets[I]);
      }
      Entry.ReturnPHI.emplace_back(std::move(Rets), Builder.getInsertBlock());
    }
  }

  LLVM::BasicBlock getLabel(unsigned int Index) const noexcept {
    return (ControlStack.rbegin() + Index)->JumpBlock;
  }

  void stackPush(LLVM::Value Value) noexcept { Stack.push_back(Value); }
  LLVM::Value stackPop() noexcept {
    assuming(!ControlStack.empty() || !Stack.empty());
    assuming(ControlStack.empty() ||
             Stack.size() > ControlStack.back().StackSize);
    auto Value = Stack.back();
    Stack.pop_back();
    return Value;
  }

  AOT::Compiler::CompileContext &Context;
  LLVM::Context &LLContext;
  std::vector<std::pair<LLVM::Type, LLVM::Value>> Local;
  std::vector<LLVM::Value> Stack;
  LLVM::Value LocalInstrCount = nullptr;
  LLVM::Value LocalGas = nullptr;
  std::unordered_map<ErrCode::Value, LLVM::BasicBlock> TrapBB;
  bool IsUnreachable = false;
  bool Interruptible = false;
  bool OptNone = false;
  struct Control {
    size_t StackSize;
    bool Unreachable;
    LLVM::BasicBlock JumpBlock;
    LLVM::BasicBlock NextBlock;
    LLVM::BasicBlock ElseBlock;
    std::vector<LLVM::Value> Args;
    std::pair<std::vector<ValType>, std::vector<ValType>> Type;
    std::vector<std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
        ReturnPHI;
    Control(size_t S, bool U, LLVM::BasicBlock J, LLVM::BasicBlock N,
            LLVM::BasicBlock E, std::vector<LLVM::Value> A,
            std::pair<std::vector<ValType>, std::vector<ValType>> T,
            std::vector<std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
                R) noexcept
        : StackSize(S), Unreachable(U), JumpBlock(J), NextBlock(N),
          ElseBlock(E), Args(std::move(A)), Type(std::move(T)),
          ReturnPHI(std::move(R)) {}
    Control(const Control &) = default;
    Control(Control &&) = default;
    Control &operator=(const Control &) = default;
    Control &operator=(Control &&) = default;
  };
  std::vector<Control> ControlStack;
  LLVM::FunctionCallee F;
  LLVM::Value ExecCtx;
  LLVM::Builder Builder;
};

std::vector<LLVM::Value> unpackStruct(LLVM::Builder &Builder,
                                      LLVM::Value Struct) noexcept {
  const auto N = Struct.getType().getStructNumElements();
  std::vector<LLVM::Value> Ret;
  Ret.reserve(N);
  for (unsigned I = 0; I < N; ++I) {
    Ret.push_back(Builder.createExtractValue(Struct, I));
  }
  return Ret;
}

// Write output object and link
Expect<void> outputNativeLibrary(const std::filesystem::path &OutputPath,
                                 const LLVM::MemoryBuffer &OSVec) noexcept {
  spdlog::info("output start");
  std::filesystem::path ObjectName;
  {
    // tempfile
    std::filesystem::path OPath(OutputPath);
#if WASMEDGE_OS_WINDOWS
    OPath.replace_extension("%%%%%%%%%%.obj"sv);
#else
    OPath.replace_extension("%%%%%%%%%%.o"sv);
#endif
    ObjectName = createTemp(OPath);
    if (ObjectName.empty()) {
      // TODO:return error
      spdlog::error("so file creation failed:{}", OPath.u8string());
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
    }
    std::ofstream OS(ObjectName, std::ios_base::binary);
    OS.write(OSVec.data(), static_cast<std::streamsize>(OSVec.size()));
    OS.close();
  }

  // link
  bool LinkResult = false;
#if WASMEDGE_OS_MACOS
#if LLVM_VERSION_MAJOR >= 14
  // LLVM 14 replaces the older mach_o lld implementation with the new one.
  // So we need to change the namespace after LLVM 14.x released.
  // Reference: https://reviews.llvm.org/D114842
  LinkResult = lld::macho::link(
#else
  LinkResult = lld::mach_o::link(
#endif
      std::initializer_list<const char *> {
        "lld", "-arch",
#if defined(__x86_64__)
            "x86_64",
#elif defined(__aarch64__)
            "arm64",
#else
#error Unsupported architecture on the MacOS!
#endif
#if LLVM_VERSION_MAJOR >= 14
            // LLVM 14 replaces the older mach_o lld implementation with the new
            // one. And it require -arch and -platform_version to always be
            // specified. Reference: https://reviews.llvm.org/D97799
            "-platform_version", "macos", "10.0", "11.0",
#else
            "-sdk_version", "11.3",
#endif
            "-dylib", "-demangle", "-macosx_version_min", "10.0.0",
            "-syslibroot",
            "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
            ObjectName.u8string().c_str(), "-o", OutputPath.u8string().c_str(),
            "-lSystem"
      },
#elif WASMEDGE_OS_LINUX
  LinkResult = lld::elf::link(
      std::initializer_list<const char *>{"ld.lld", "--shared", "--gc-sections",
                                          "--discard-all", ObjectName.c_str(),
                                          "-o", OutputPath.u8string().c_str()},
#elif WASMEDGE_OS_WINDOWS
  LinkResult = lld::coff::link(
      std::initializer_list<const char *>{
          "lld-link", "-dll", "-base:0", "-nologo",
          ObjectName.u8string().c_str(),
          ("-out:" + OutputPath.u8string()).c_str()},
#endif

#if LLVM_VERSION_MAJOR >= 14
      llvm::outs(), llvm::errs(), false, false
#elif LLVM_VERSION_MAJOR >= 10
      false, llvm::outs(), llvm::errs()
#else
      false, llvm::errs()
#endif
  );

#if LLVM_VERSION_MAJOR >= 14
  lld::CommonLinkerContext::destroy();
#endif

  if (LinkResult) {
    std::error_code Error;
    std::filesystem::remove(ObjectName, Error);
#if WASMEDGE_OS_WINDOWS
    std::filesystem::path LibPath(OutputPath);
    LibPath.replace_extension(".lib"sv);
    std::filesystem::remove(LibPath, Error);
#endif

    spdlog::info("compile done");
  } else {
    spdlog::error("link error");
  }

#if WASMEDGE_OS_MACOS
  // codesign
  if (LinkResult) {
    pid_t PID = ::fork();
    if (PID == -1) {
      spdlog::error("codesign error on fork:{}", std::strerror(errno));
    } else if (PID == 0) {
      execlp("/usr/bin/codesign", "codesign", "-s", "-",
             OutputPath.u8string().c_str(), nullptr);
      std::exit(256);
    } else {
      int ChildStat;
      waitpid(PID, &ChildStat, 0);
      if (const int Status = WEXITSTATUS(ChildStat); Status != 0) {
        spdlog::error("codesign exited with status {}", Status);
      }
    }
  }
#endif

  return {};
}

Expect<void> outputWasmLibrary(const std::filesystem::path &OutputPath,
                               Span<const Byte> Data,
                               const LLVM::MemoryBuffer &OSVec) noexcept {
  std::filesystem::path SharedObjectName;
  {
    // tempfile
    std::filesystem::path SOPath(OutputPath);
    SOPath.replace_extension("%%%%%%%%%%" WASMEDGE_LIB_EXTENSION);
    SharedObjectName = createTemp(SOPath);
    if (SharedObjectName.empty()) {
      // TODO:return error
      spdlog::error("so file creation failed:{}", SOPath.u8string());
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
    }
    std::ofstream OS(SharedObjectName, std::ios_base::binary);
    OS.write(OSVec.data(), static_cast<std::streamsize>(OSVec.size()));
    OS.close();
  }

  if (auto Res = outputNativeLibrary(SharedObjectName, OSVec); unlikely(!Res)) {
    return Unexpect(Res);
  }

  LLVM::MemoryBuffer SOFile;
  if (auto [Res, ErrorMessage] =
          LLVM::MemoryBuffer::getFile(SharedObjectName.u8string().c_str());
      unlikely(ErrorMessage)) {
    spdlog::error("object file open error:{}", ErrorMessage.string_view());
    return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
  } else {
    SOFile = std::move(Res);
  }

  LLVM::Context Context;
  LLVM::Binary ObjFile;
  if (auto [Res, ErrorMessage] = LLVM::Binary::create(SOFile, Context);
      unlikely(ErrorMessage)) {
    spdlog::error("object file parse error:{}", ErrorMessage.string_view());
    return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
  } else {
    ObjFile = std::move(Res);
  }

  std::string OSCustomSecVec;
  {
    std::ostringstream OS;
    WriteName(OS, "wasmedge"sv);
    WriteU32(OS, WasmEdge::AOT::kBinaryVersion);

#if WASMEDGE_OS_LINUX
    WriteByte(OS, UINT8_C(1));
#elif WASMEDGE_OS_MACOS
    WriteByte(OS, UINT8_C(2));
#elif WASMEDGE_OS_WINDOWS
    WriteByte(OS, UINT8_C(3));
#else
#error Unsupported operating system!
#endif

#if defined(__x86_64__)
    WriteByte(OS, UINT8_C(1));
#elif defined(__aarch64__)
    WriteByte(OS, UINT8_C(2));
#elif defined(__riscv) && __riscv_xlen == 64
    WriteByte(OS, UINT8_C(3));
#else
#error Unsupported hardware architecture!
#endif

    std::vector<std::pair<std::string, uint64_t>> SymbolTable;
#if !WASMEDGE_OS_WINDOWS
    for (auto Symbol = ObjFile.symbols();
         Symbol && !ObjFile.isSymbolEnd(Symbol); Symbol.next()) {
      SymbolTable.emplace_back(Symbol.getName(), Symbol.getAddress());
    }
#else
    for (auto &Symbol :
         llvm::object::unwrap<llvm::object::COFFObjectFile>(ObjFile.unwrap())
             ->export_directories()) {
      llvm::StringRef Name;
      if (auto Error = Symbol.getSymbolName(Name); unlikely(!!Error)) {
        continue;
      } else if (Name.empty()) {
        continue;
      }
      uint32_t Offset = 0;
      if (auto Error = Symbol.getExportRVA(Offset); unlikely(!!Error)) {
        continue;
      }
      SymbolTable.emplace_back(Name.str(), Offset);
    }
#endif
    uint64_t VersionAddress = 0, IntrinsicsAddress = 0;
    std::vector<uint64_t> Types;
    std::vector<uint64_t> Codes;
    uint64_t CodesMin = std::numeric_limits<uint64_t>::max();
    for (const auto &[Name, Address] : SymbolTable) {
      if (Name == SYMBOL("version"sv)) {
        VersionAddress = Address;
      } else if (Name == SYMBOL("intrinsics"sv)) {
        IntrinsicsAddress = Address;
      } else if (startsWith(Name, SYMBOL("t"sv))) {
        uint64_t Index = 0;
        std::from_chars(Name.data() + SYMBOL("t"sv).size(),
                        Name.data() + Name.size(), Index);
        if (Types.size() < Index + 1) {
          Types.resize(Index + 1);
        }
        Types[Index] = Address;
      } else if (startsWith(Name, SYMBOL("f"sv))) {
        uint64_t Index = 0;
        std::from_chars(Name.data() + SYMBOL("f"sv).size(),
                        Name.data() + Name.size(), Index);
        if (Codes.size() < Index + 1) {
          Codes.resize(Index + 1);
        }
        CodesMin = std::min(CodesMin, Index);
        Codes[Index] = Address;
      }
    }
    if (CodesMin != std::numeric_limits<uint64_t>::max()) {
      Codes.erase(Codes.begin(),
                  Codes.begin() + static_cast<int64_t>(CodesMin));
    }
    WriteU64(OS, VersionAddress);
    WriteU64(OS, IntrinsicsAddress);
    WriteU64(OS, Types.size());
    for (const uint64_t TypeAddress : Types) {
      WriteU64(OS, TypeAddress);
    }
    WriteU64(OS, Codes.size());
    for (const uint64_t CodeAddress : Codes) {
      WriteU64(OS, CodeAddress);
    }

    uint32_t SectionCount = 0;
    for (auto Section = ObjFile.sections(); !ObjFile.isSectionEnd(Section);
         Section.next()) {
      if (Section.getSize() == 0) {
        continue;
      }
      if (!Section.isText() && !Section.isData() && !Section.isBSS()) {
        continue;
      }
      ++SectionCount;
    }
    WriteU32(OS, SectionCount);

    for (auto Section = ObjFile.sections(); !ObjFile.isSectionEnd(Section);
         Section.next()) {
      if (Section.getSize() == 0) {
        continue;
      }
      std::vector<char> Content;
      if (auto Res = Section.getContents(); unlikely(Res.empty())) {
        assumingUnreachable();
      } else {
        Content.assign(Res.begin(), Res.end());
      }
      if (Section.isPData()) {
        WriteByte(OS, UINT8_C(4));
      } else if (Section.isText()) {
        WriteByte(OS, UINT8_C(1));
      } else if (Section.isData()) {
        WriteByte(OS, UINT8_C(2));
      } else if (Section.isBSS()) {
        WriteByte(OS, UINT8_C(3));
      } else {
        continue;
      }

      WriteU64(OS, Section.getAddress());
      WriteU64(OS, Content.size());
      WriteName(OS, std::string_view(Content.data(), Content.size()));
    }
    OSCustomSecVec = OS.str();
  }

  spdlog::info("output start");

  std::ofstream OS(OutputPath, std::ios_base::binary);
  if (!OS) {
    spdlog::error("output failed.");
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  OS.write(reinterpret_cast<const char *>(Data.data()),
           static_cast<std::streamsize>(Data.size()));
  // Custom section id
  WriteByte(OS, UINT8_C(0x00));
  WriteName(OS, std::string_view(OSCustomSecVec.data(), OSCustomSecVec.size()));

  std::error_code Error;
  std::filesystem::remove(SharedObjectName, Error);
  return {};
}

} // namespace

namespace WasmEdge {
namespace AOT {

Expect<void> Compiler::compile(Span<const Byte> Data, const AST::Module &Module,
                               std::filesystem::path OutputPath) noexcept {
  // Check the module is validated.
  if (unlikely(!Module.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    return Unexpect(ErrCode::Value::NotValidated);
  }

  std::unique_lock Lock(Mutex);
  spdlog::info("compile start");
  std::filesystem::path LLPath(OutputPath);
  LLPath.replace_extension("ll"sv);

  LLVM::Core::init();

  LLVM::Context LLContext;
  LLVM::Module LLModule(LLContext, LLPath.u8string().c_str());
  LLModule.setTarget(LLVM::getDefaultTargetTriple().unwrap());
  LLModule.addFlag(LLVMModuleFlagBehaviorError, "PIC Level"sv, 2);
  CompileContext NewContext(LLContext, LLModule,
                            Conf.getCompilerConfigure().isGenericBinary(),
                            Conf.getCompilerConfigure().getOutputFormat() ==
                                CompilerConfigure::OutputFormat::Wasm);
  struct RAIICleanup {
    RAIICleanup(CompileContext *&Context, CompileContext &NewContext)
        : Context(Context) {
      Context = &NewContext;
    }
    ~RAIICleanup() { Context = nullptr; }
    CompileContext *&Context;
  };
  RAIICleanup Cleanup(Context, NewContext);

  // Compile Function Types
  compile(Module.getTypeSection());
  // Compile ImportSection
  compile(Module.getImportSection());
  // Compile GlobalSection
  compile(Module.getGlobalSection());
  // Compile MemorySection (MemorySec, DataSec)
  compile(Module.getMemorySection(), Module.getDataSection());
  // Compile TableSection (TableSec, ElemSec)
  compile(Module.getTableSection(), Module.getElementSection());
  // compile Functions in module. (FunctionSec, CodeSec)
  compile(Module.getFunctionSection(), Module.getCodeSection());
  // Compile ExportSection
  compile(Module.getExportSection());
  // StartSection is not required to compile

#if WASMEDGE_OS_WINDOWS
  {
    // create dummy dllmain function
    auto FTy = LLVM::Type::getFunctionType(Context->LLContext.getInt32Ty(), {});
    auto F = Context->LLModule.addFunction(FTy, LLVMExternalLinkage,
                                           "_DllMainCRTStartup");
    F.setVisibility(LLVMProtectedVisibility);
    F.setDSOLocal(true);
    F.addFnAttr(Context->NoStackArgProbe);
    F.addFnAttr(Context->StrictFP);
    F.addFnAttr(Context->NoReturn);
    LLVM::Builder Builder(Context->LLContext);
    Builder.positionAtEnd(
        LLVM::BasicBlock::create(Context->LLContext, F, "entry"));
    Builder.createRet(Context->LLContext.getInt32(1u));

    auto A = LLModule.addAlias(F.getType(), F, "_fltused");
    A.setLinkage(LLVMExternalLinkage);
    A.setVisibility(LLVMProtectedVisibility);
    A.setDSOLocal(true);
  }
#endif

  if (!Context->IsCustomSection) {
    // create wasm.code and wasm.size
    auto Int32Ty = Context->Int32Ty;
    auto Content = LLVM::Value::getConstString(
        LLContext, {reinterpret_cast<const char *>(Data.data()), Data.size()},
        true);
    LLModule.addGlobal(Content.getType(), true, LLVMExternalLinkage, Content,
                       "wasm.code");
    LLModule.addGlobal(Int32Ty, true, LLVMExternalLinkage,
                       LLVM::Value::getConstInt(Int32Ty, Data.size()),
                       "wasm.size");
  }

  // set dllexport
  for (auto GV = LLModule.getFirstGlobal(); GV; GV = GV.getNextGlobal()) {
    if (GV.getLinkage() == LLVMExternalLinkage) {
      GV.setVisibility(LLVMProtectedVisibility);
      GV.setDSOLocal(true);
      GV.setDLLStorageClass(LLVMDLLExportStorageClass);
    }
  }

  if (Conf.getCompilerConfigure().isDumpIR()) {
    if (auto ErrorMessage = LLModule.printModuleToFile("wasm.ll");
        unlikely(ErrorMessage)) {
      spdlog::error("wasm.ll open error:{}", ErrorMessage.string_view());
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
    }
  }

  spdlog::info("verify start");
  LLModule.verify(LLVMPrintMessageAction);
  spdlog::info("optimize start");

  // optimize + codegen
  auto Triple = LLModule.getTarget();
  {
    LLVM::TargetMachine TM;
    {
      auto [TheTarget, ErrorMessage] = LLVM::Target::getFromTriple(Triple);
      if (ErrorMessage) {
        // TODO:return error
        spdlog::error("lookupTarget failed:{}", ErrorMessage.string_view());
        return Unexpect(ErrCode::Value::IllegalPath);
      }

      std::string CPUName;
#if defined(__riscv) && __riscv_xlen == 64
      CPUName = "generic-rv64"s;
#else
      if (!Conf.getCompilerConfigure().isGenericBinary()) {
        CPUName = LLVM::getHostCPUName().string_view();
      } else {
        CPUName = "generic"s;
      }
#endif

      TM = LLVM::TargetMachine::create(
          TheTarget, Triple, CPUName.c_str(),
          Context->SubtargetFeatures.unwrap(),
          toLLVMCodeGenLevel(
              Conf.getCompilerConfigure().getOptimizationLevel()),
          LLVMRelocPIC, LLVMCodeModelDefault);
    }

    {
      auto FP = LLVM::PassManager::createForModule(LLModule);
      auto MP = LLVM::PassManager::create();

      TM.addAnalysisPasses(MP);
      TM.addAnalysisPasses(FP);
      if (Conf.getCompilerConfigure().getOptimizationLevel() ==
          CompilerConfigure::OptimizationLevel::O0) {
        FP.addTailCallEliminationPass();
        MP.addAlwaysInlinerPass();
      } else {
        auto PMB = LLVM::PassManagerBuilder::create();
        auto [OptLevel, SizeLevel] =
            toLLVMLevel(Conf.getCompilerConfigure().getOptimizationLevel());
        PMB.setOptLevel(OptLevel);
        PMB.setSizeLevel(SizeLevel);
        PMB.populateFunctionPassManager(FP);
        PMB.populateModulePassManager(MP);
      }

      FP.initializeFunctionPassManager();
      for (auto Fn = LLModule.getFirstFunction(); Fn;
           Fn = Fn.getNextFunction()) {
        FP.runFunctionPassManager(Fn);
      }
      FP.finalizeFunctionPassManager();
      MP.runPassManager(LLModule);
    }

    // Set initializer for constant value
    if (auto IntrinsicsTable = LLModule.getNamedGlobal("intrinsics")) {
      IntrinsicsTable.setInitializer(
          LLVM::Value::getConstNull(IntrinsicsTable.getType()));
      IntrinsicsTable.setGlobalConstant(false);
    }

    if (Conf.getCompilerConfigure().isDumpIR()) {
      if (auto ErrorMessage = LLModule.printModuleToFile("wasm-opt.ll")) {
        // TODO:return error
        spdlog::error("printModuleToFile failed");
        return Unexpect(ErrCode::Value::IllegalPath);
      }
    }

    spdlog::info("codegen start");
    auto [OSVec, ErrorMessage] =
        TM.emitToMemoryBuffer(LLModule, LLVMObjectFile);
    if (ErrorMessage) {
      // TODO:return error
      spdlog::error("addPassesToEmitFile failed");
      return Unexpect(ErrCode::Value::IllegalPath);
    }

    if (Context->IsCustomSection) {
      if (auto Res = outputWasmLibrary(OutputPath, Data, OSVec);
          unlikely(!Res)) {
        return Unexpect(Res);
      }
    } else {
      if (auto Res = outputNativeLibrary(OutputPath, OSVec); unlikely(!Res)) {
        return Unexpect(Res);
      }
    }
  }

  return {};
}

void Compiler::compile(const AST::TypeSection &TypeSec) noexcept {
  auto WrapperTy =
      LLVM::Type::getFunctionType(Context->VoidTy,
                                  {Context->ExecCtxPtrTy, Context->Int8PtrTy,
                                   Context->Int8PtrTy, Context->Int8PtrTy},
                                  false);
  const auto &FuncTypes = TypeSec.getContent();
  const auto Size = FuncTypes.size();
  if (Size == 0) {
    return;
  }
  Context->FunctionTypes.reserve(Size);
  Context->FunctionWrappers.reserve(Size);

  // Iterate and compile types.
  for (size_t I = 0; I < Size; ++I) {
    const auto &FuncType = FuncTypes[I];
    const auto Name = fmt::format("t{}"sv, Context->FunctionTypes.size());

    // Check function type is unique
    {
      bool Unique = true;
      for (size_t J = 0; J < I; ++J) {
        const auto &OldFuncType = *Context->FunctionTypes[J];
        if (OldFuncType == FuncType) {
          Unique = false;
          Context->FunctionTypes.push_back(&OldFuncType);
          auto F = Context->FunctionWrappers[J];
          Context->FunctionWrappers.push_back(F);
          auto A = Context->LLModule.addAlias(F.getType(), F, Name.c_str());
          A.setLinkage(LLVMExternalLinkage);
          A.setVisibility(LLVMProtectedVisibility);
          A.setDSOLocal(true);
          A.setDLLStorageClass(LLVMDLLExportStorageClass);
          break;
        }
      }
      if (!Unique) {
        continue;
      }
    }

    // Create Wrapper
    auto F = Context->LLModule.addFunction(WrapperTy, LLVMExternalLinkage,
                                           Name.c_str());
    {
      F.setVisibility(LLVMProtectedVisibility);
      F.setDSOLocal(true);
      F.setDLLStorageClass(LLVMDLLExportStorageClass);
      F.addFnAttr(Context->NoStackArgProbe);
      F.addFnAttr(Context->StrictFP);
      F.addParamAttr(0, Context->ReadOnly);
      F.addParamAttr(0, Context->NoAlias);
      F.addParamAttr(1, Context->NoAlias);
      F.addParamAttr(2, Context->NoAlias);
      F.addParamAttr(3, Context->NoAlias);

      LLVM::Builder Builder(Context->LLContext);
      Builder.positionAtEnd(
          LLVM::BasicBlock::create(Context->LLContext, F, "entry"));

      auto FTy =
          toLLVMType(Context->LLContext, Context->ExecCtxPtrTy, FuncType);
      auto RTy = FTy.getReturnType();
      std::vector<LLVM::Type> FPTy(FTy.getNumParams());
      FTy.getParamTypes(FPTy);

      const size_t ArgCount = FPTy.size() - 1;
      const size_t RetCount =
          RTy.isVoidTy() ? 0
                         : (RTy.isStructTy() ? RTy.getStructNumElements() : 1);
      auto ExecCtxPtr = F.getFirstParam();
      auto RawFunc = LLVM::FunctionCallee{
          FTy,
          Builder.createBitCast(ExecCtxPtr.getNextParam(), FTy.getPointerTo())};
      auto RawArgs = ExecCtxPtr.getNextParam().getNextParam();
      auto RawRets = RawArgs.getNextParam();

      std::vector<LLVM::Value> Args;
      Args.reserve(FTy.getNumParams());
      Args.push_back(ExecCtxPtr);
      for (size_t J = 0; J < ArgCount; ++J) {
        auto ArgTy = FPTy[J + 1];
        auto VPtr = Builder.createConstInBoundsGEP1_64(Context->Int8Ty, RawArgs,
                                                       J * kValSize);
        auto Ptr = Builder.createBitCast(VPtr, ArgTy.getPointerTo());
        Args.push_back(Builder.createLoad(ArgTy, Ptr));
      }

      auto Ret = Builder.createCall(RawFunc, Args);
      if (RTy.isVoidTy()) {
        // nothing to do
      } else if (RTy.isStructTy()) {
        auto Rets = unpackStruct(Builder, Ret);
        for (size_t J = 0; J < RetCount; ++J) {
          auto VPtr = Builder.createConstInBoundsGEP1_64(Context->Int8Ty,
                                                         RawRets, J * kValSize);
          auto Ptr =
              Builder.createBitCast(VPtr, Rets[J].getType().getPointerTo());
          Builder.createStore(Rets[J], Ptr);
        }
      } else {
        auto VPtr =
            Builder.createConstInBoundsGEP1_64(Context->Int8Ty, RawRets, 0);
        auto Ptr = Builder.createBitCast(VPtr, Ret.getType().getPointerTo());
        Builder.createStore(Ret, Ptr);
      }
      Builder.createRetVoid();
    }
    // Copy wrapper, param and return lists to module instance.
    Context->FunctionTypes.push_back(&FuncType);
    Context->FunctionWrappers.push_back(F);
  }
}

void Compiler::compile(const AST::ImportSection &ImportSec) noexcept {
  // Iterate and compile import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    // Get data from import description.
    const auto &ExtType = ImpDesc.getExternalType();

    // Add the imports into module instance.
    switch (ExtType) {
    case ExternalType::Function: // Function type index
    {
      const auto FuncID = static_cast<uint32_t>(Context->Functions.size());
      // Get the function type index in module.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      assuming(TypeIdx < Context->FunctionTypes.size());
      const auto &FuncType = *Context->FunctionTypes[TypeIdx];
      auto FTy =
          toLLVMType(Context->LLContext, Context->ExecCtxPtrTy, FuncType);
      auto RTy = FTy.getReturnType();
      const auto Linkage =
          Context->IsCustomSection ? LLVMInternalLinkage : LLVMExternalLinkage;
      auto F = LLVM::FunctionCallee{
          FTy, Context->LLModule.addFunction(
                   FTy, Linkage, fmt::format("f{}"sv, FuncID).c_str())};
      F.Fn.setVisibility(LLVMProtectedVisibility);
      F.Fn.setDSOLocal(true);
      if (!Context->IsCustomSection) {
        F.Fn.setDLLStorageClass(LLVMDLLExportStorageClass);
      }
      F.Fn.addFnAttr(Context->NoStackArgProbe);
      F.Fn.addFnAttr(Context->StrictFP);
      F.Fn.addParamAttr(0, Context->ReadOnly);
      F.Fn.addParamAttr(0, Context->NoAlias);

      LLVM::Builder Builder(Context->LLContext);
      Builder.positionAtEnd(
          LLVM::BasicBlock::create(Context->LLContext, F.Fn, "entry"));

      const auto ArgSize = FuncType.getParamTypes().size();
      const auto RetSize =
          RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();

      LLVM::Value Args;
      if (ArgSize == 0) {
        Args = LLVM::Value::getConstNull(Context->Int8PtrTy);
      } else {
        auto Alloca = Builder.createArrayAlloca(
            Context->Int8Ty, Context->LLContext.getInt64(ArgSize * kValSize));
        Alloca.setAlignment(kValSize);
        Args = Alloca;
      }

      LLVM::Value Rets;
      if (RetSize == 0) {
        Rets = LLVM::Value::getConstNull(Context->Int8PtrTy);
      } else {
        auto Alloca = Builder.createArrayAlloca(
            Context->Int8Ty, Context->LLContext.getInt64(RetSize * kValSize));
        Alloca.setAlignment(kValSize);
        Rets = Alloca;
      }

      auto Arg = F.Fn.getFirstParam();
      for (unsigned I = 0; I < ArgSize; ++I) {
        Arg = Arg.getNextParam();
        LLVM::Value Ptr = Builder.createConstInBoundsGEP1_64(
            Context->Int8Ty, Args, I * kValSize);
        Builder.createStore(
            Arg, Builder.createBitCast(Ptr, Arg.getType().getPointerTo()));
      }

      Builder.createCall(
          Context->getIntrinsic(
              Builder, AST::Module::Intrinsics::kCall,
              LLVM::Type::getFunctionType(
                  Context->VoidTy,
                  {Context->Int32Ty, Context->Int8PtrTy, Context->Int8PtrTy},
                  false)),
          {Context->LLContext.getInt32(FuncID), Args, Rets});

      if (RetSize == 0) {
        Builder.createRetVoid();
      } else if (RetSize == 1) {
        LLVM::Value VPtr =
            Builder.createConstInBoundsGEP1_64(Context->Int8Ty, Rets, 0);
        LLVM::Value Ptr = Builder.createBitCast(VPtr, RTy.getPointerTo());
        Builder.createRet(Builder.createLoad(RTy, Ptr));
      } else {
        std::vector<LLVM::Value> Ret;
        Ret.reserve(RetSize);
        for (unsigned I = 0; I < RetSize; ++I) {
          LLVM::Value VPtr = Builder.createConstInBoundsGEP1_64(
              Context->Int8Ty, Rets, I * kValSize);
          LLVM::Value Ptr = Builder.createBitCast(
              VPtr, RTy.getStructElementType(I).getPointerTo());
          Ret.push_back(Builder.createLoad(RTy.getStructElementType(I), Ptr));
        }
        Builder.createAggregateRet(Ret);
      }

      Context->Functions.emplace_back(TypeIdx, F, nullptr);
      break;
    }
    case ExternalType::Table: // Table type
    {
      // Nothing to do.
      break;
    }
    case ExternalType::Memory: // Memory type
    {
      // Nothing to do.
      break;
    }
    case ExternalType::Global: // Global type
    {
      // Get global type. External type checked in validation.
      const auto &GlobType = ImpDesc.getExternalGlobalType();
      const auto &ValType = GlobType.getValType();
      auto Type = toLLVMType(Context->LLContext, ValType);
      Context->Globals.push_back(Type);
      break;
    }
    default:
      break;
    }
  }
}

void Compiler::compile(const AST::ExportSection &) noexcept {}

void Compiler::compile(const AST::GlobalSection &GlobalSec) noexcept {
  for (const auto &GlobalSeg : GlobalSec.getContent()) {
    const auto &ValType = GlobalSeg.getGlobalType().getValType();
    auto Type = toLLVMType(Context->LLContext, ValType);
    Context->Globals.push_back(Type);
  }
}

void Compiler::compile(const AST::MemorySection &,
                       const AST::DataSection &) noexcept {}

void Compiler::compile(const AST::TableSection &,
                       const AST::ElementSection &) noexcept {}

void Compiler::compile(const AST::FunctionSection &FuncSec,
                       const AST::CodeSection &CodeSec) noexcept {
  const auto &TypeIdxs = FuncSec.getContent();
  const auto &CodeSegs = CodeSec.getContent();
  if (TypeIdxs.size() == 0 || CodeSegs.size() == 0) {
    return;
  }

  for (size_t I = 0; I < TypeIdxs.size() && I < CodeSegs.size(); ++I) {
    const auto &TypeIdx = TypeIdxs[I];
    const auto &Code = CodeSegs[I];
    assuming(TypeIdx < Context->FunctionTypes.size());
    const auto &FuncType = *Context->FunctionTypes[TypeIdx];
    const auto FuncID = Context->Functions.size();
    auto FTy = toLLVMType(Context->LLContext, Context->ExecCtxPtrTy, FuncType);
    LLVM::FunctionCallee F = {FTy, Context->LLModule.addFunction(
                                       FTy, LLVMExternalLinkage,
                                       fmt::format("f{}"sv, FuncID).c_str())};
    F.Fn.setVisibility(LLVMProtectedVisibility);
    F.Fn.setDSOLocal(true);
    F.Fn.setDLLStorageClass(LLVMDLLExportStorageClass);
    F.Fn.addFnAttr(Context->NoStackArgProbe);
    F.Fn.addFnAttr(Context->StrictFP);
    F.Fn.addParamAttr(0, Context->ReadOnly);
    F.Fn.addParamAttr(0, Context->NoAlias);

    Context->Functions.emplace_back(TypeIdx, F, &Code);
  }

  for (auto [T, F, Code] : Context->Functions) {
    if (!Code) {
      continue;
    }

    std::vector<ValType> Locals;
    for (const auto &Local : Code->getLocals()) {
      for (unsigned I = 0; I < Local.first; ++I) {
        Locals.push_back(Local.second);
      }
    }
    FunctionCompiler FC(*Context, F, Locals,
                        Conf.getCompilerConfigure().isInterruptible(),
                        Conf.getStatisticsConfigure().isInstructionCounting(),
                        Conf.getStatisticsConfigure().isCostMeasuring(),
                        Conf.getCompilerConfigure().getOptimizationLevel() ==
                            CompilerConfigure::OptimizationLevel::O0);
    auto Type = Context->resolveBlockType(T);
    FC.compile(*Code, std::move(Type));
    F.Fn.eliminateUnreachableBlocks();
  }
}

} // namespace AOT
} // namespace WasmEdge
