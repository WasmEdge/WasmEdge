// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "aot/compiler.h"

#include "aot/version.h"
#include "common/defines.h"
#include "common/filesystem.h"
#include "common/log.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <lld/Common/Driver.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar/TailRecursionElimination.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <system_error>

#if WASMEDGE_OS_WINDOWS
#include <llvm/Object/COFF.h>
#endif

#if LLVM_VERSION_MAJOR >= 14
#include <llvm/MC/TargetRegistry.h>
#else
#include <llvm/Support/TargetRegistry.h>
#endif

#if LLVM_VERSION_MAJOR >= 12
#include <llvm/Analysis/AliasAnalysis.h>
#endif
#if LLVM_VERSION_MAJOR >= 10
#include <llvm/IR/IntrinsicsAArch64.h>
#include <llvm/IR/IntrinsicsX86.h>
#include <llvm/Support/Alignment.h>
#endif

#if WASMEDGE_OS_MACOS
#include <unistd.h>
#endif

#if WASMEDGE_OS_LINUX
#define SYMBOL(X) X
#elif WASMEDGE_OS_MACOS
#define SYMBOL(X) "_" X
#elif WASMEDGE_OS_WINDOWS
#define SYMBOL(X) X
#endif

namespace {

// is x86_64
#if defined(_M_X64) && !defined(__x86_64__)
#define __x86_64__ 1
#endif

inline void setIsFPConstrained(llvm::IRBuilder<> &Builder) {
  Builder.setIsFPConstrained(true);
#if LLVM_VERSION_MAJOR >= 11
  Builder.setDefaultConstrainedRounding(llvm::RoundingMode::NearestTiesToEven);
  Builder.setDefaultConstrainedExcept(llvm::fp::ExceptionBehavior::ebIgnore);
#elif LLVM_VERSION_MAJOR >= 10
  Builder.setDefaultConstrainedRounding(llvm::fp::RoundingMode::rmToNearest);
  Builder.setDefaultConstrainedExcept(llvm::fp::ExceptionBehavior::ebIgnore);
#else
  Builder.setDefaultConstrainedRounding(
      llvm::ConstrainedFPIntrinsic::RoundingMode::rmToNearest);
  Builder.setDefaultConstrainedExcept(
      llvm::ConstrainedFPIntrinsic::ExceptionBehavior::ebIgnore);
#endif
}

#if LLVM_VERSION_MAJOR >= 11
using ShuffleElement = int;
#else
using ShuffleElement = uint32_t;
#endif

#if LLVM_VERSION_MAJOR >= 10
using Align = llvm::Align;
#else
static inline unsigned Align(unsigned Value) noexcept { return Value; }
#endif
static inline auto elementCount(llvm::VectorType *VectorTy) noexcept {
#if LLVM_VERSION_MAJOR >= 12
  return VectorTy->getElementCount().getKnownMinValue();
#else
  return VectorTy->getElementCount().Min;
#endif
}

static bool isVoidReturn(WasmEdge::Span<const WasmEdge::ValType> ValTypes);
static llvm::Type *toLLVMType(llvm::LLVMContext &LLContext,
                              const WasmEdge::ValType &ValType);
static std::vector<llvm::Type *>
toLLVMArgsType(llvm::PointerType *ExecCtxPtrTy,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes);
static llvm::Type *
toLLVMRetsType(llvm::LLVMContext &LLContext,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes);
static llvm::FunctionType *
toLLVMType(llvm::PointerType *ExecCtxPtrTy,
           const WasmEdge::AST::FunctionType &FuncType);
static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &LLContext,
                                          const WasmEdge::ValType &ValType);
static std::vector<llvm::Value *> unpackStruct(llvm::IRBuilder<> &Builder,
                                               llvm::Value *Struct);
static llvm::Value *createLikely(llvm::IRBuilder<> &Builder,
                                 llvm::Value *Value);
class FunctionCompiler;

// XXX: Misalignment handler not implemented yet, forcing unalignment
// force unalignment load/store
static inline constexpr const bool kForceUnalignment = true;

// force checking div/rem on zero
static inline constexpr const bool kForceDivCheck = true;

// Size of a ValVariant
static inline constexpr const uint32_t kValSize = sizeof(WasmEdge::ValVariant);

#if LLVM_VERSION_MAJOR >= 14
using LLVMOptimizationLevel = llvm::OptimizationLevel;
#else
using LLVMOptimizationLevel = llvm::PassBuilder::OptimizationLevel;
#endif

// Translate Compiler::OptimizationLevel to llvm::PassBuilder version
static inline LLVMOptimizationLevel
toLLVMLevel(WasmEdge::CompilerConfigure::OptimizationLevel Level) {
  using OL = WasmEdge::CompilerConfigure::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return LLVMOptimizationLevel::O0;
  case OL::O1:
    return LLVMOptimizationLevel::O1;
  case OL::O2:
    return LLVMOptimizationLevel::O2;
  case OL::O3:
    return LLVMOptimizationLevel::O3;
  case OL::Os:
    return LLVMOptimizationLevel::Os;
  case OL::Oz:
    return LLVMOptimizationLevel::Oz;
  default:
    assumingUnreachable();
  }
}

WasmEdge::Expect<void> WriteByte(llvm::raw_ostream &OS, uint8_t Data) {
  OS.write(Data);
  return {};
};

WasmEdge::Expect<void> WriteU32(llvm::raw_ostream &OS, uint32_t Data) {
  do {
    uint8_t Byte = static_cast<uint8_t>(Data & UINT32_C(0x7f));
    Data >>= 7;
    if (Data > UINT32_C(0)) {
      Byte |= UINT8_C(0x80);
    }
    WriteByte(OS, Byte);
  } while (Data > UINT32_C(0));
  return {};
};

WasmEdge::Expect<void> WriteU64(llvm::raw_ostream &OS, uint64_t Data) {
  do {
    uint8_t Byte = static_cast<uint8_t>(Data & UINT64_C(0x7f));
    Data >>= 7;
    if (Data > UINT64_C(0)) {
      Byte |= UINT8_C(0x80);
    }
    WriteByte(OS, Byte);
  } while (Data > UINT64_C(0));
  return {};
};

WasmEdge::Expect<void> WriteName(llvm::raw_ostream &OS, std::string_view Data) {
  WriteU32(OS, static_cast<uint32_t>(Data.size()));
  for (const auto C : Data) {
    WriteByte(OS, static_cast<uint8_t>(C));
  }
  return {};
};

inline constexpr bool startsWith(std::string_view Value,
                                 std::string_view Prefix) {
  return Value.size() >= Prefix.size() &&
         Value.substr(0, Prefix.size()) == Prefix;
}

} // namespace

struct WasmEdge::AOT::Compiler::CompileContext {
  llvm::LLVMContext &LLContext;
  llvm::Module &LLModule;
  llvm::Type *VoidTy;
  llvm::IntegerType *Int8Ty;
  llvm::IntegerType *Int16Ty;
  llvm::IntegerType *Int32Ty;
  llvm::IntegerType *Int64Ty;
  llvm::IntegerType *Int128Ty;
  llvm::Type *FloatTy;
  llvm::Type *DoubleTy;
  llvm::VectorType *Int8x16Ty;
  llvm::VectorType *Int16x8Ty;
  llvm::VectorType *Int32x4Ty;
  llvm::VectorType *Floatx4Ty;
  llvm::VectorType *Int64x2Ty;
  llvm::VectorType *Doublex2Ty;
  llvm::VectorType *Int128x1Ty;
  llvm::PointerType *Int8PtrTy;
  llvm::PointerType *Int32PtrTy;
  llvm::PointerType *Int64PtrTy;
  llvm::PointerType *Int128PtrTy;
  llvm::StructType *ExecCtxTy;
  llvm::PointerType *ExecCtxPtrTy;
  llvm::ArrayType *IntrinsicsTableTy;
  llvm::PointerType *IntrinsicsTablePtrTy;
  llvm::SubtargetFeatures SubtargetFeatures;

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
  std::vector<llvm::Function *> FunctionWrappers;
  std::vector<std::tuple<uint32_t, llvm::Function *,
                         const WasmEdge::AST::CodeSegment *>>
      Functions;
  std::vector<llvm::Type *> Globals;
  llvm::GlobalVariable *IntrinsicsTable;
  llvm::Function *Trap;
  CompileContext(llvm::Module &M, bool IsGenericBinary)
      : LLContext(M.getContext()), LLModule(M),
        VoidTy(llvm::Type::getVoidTy(LLContext)),
        Int8Ty(llvm::Type::getInt8Ty(LLContext)),
        Int16Ty(llvm::Type::getInt16Ty(LLContext)),
        Int32Ty(llvm::Type::getInt32Ty(LLContext)),
        Int64Ty(llvm::Type::getInt64Ty(LLContext)),
        Int128Ty(llvm::Type::getInt128Ty(LLContext)),
        FloatTy(llvm::Type::getFloatTy(LLContext)),
        DoubleTy(llvm::Type::getDoubleTy(LLContext)),
        Int8x16Ty(llvm::VectorType::get(Int8Ty, 16, false)),
        Int16x8Ty(llvm::VectorType::get(Int16Ty, 8, false)),
        Int32x4Ty(llvm::VectorType::get(Int32Ty, 4, false)),
        Floatx4Ty(llvm::VectorType::get(FloatTy, 4, false)),
        Int64x2Ty(llvm::VectorType::get(Int64Ty, 2, false)),
        Doublex2Ty(llvm::VectorType::get(DoubleTy, 2, false)),
        Int128x1Ty(llvm::VectorType::get(Int128Ty, 1, false)),
        Int8PtrTy(llvm::Type::getInt8PtrTy(LLContext)),
        Int32PtrTy(llvm::Type::getInt32PtrTy(LLContext)),
        Int64PtrTy(Int64Ty->getPointerTo()),
        Int128PtrTy(Int128Ty->getPointerTo()),
        ExecCtxTy(llvm::StructType::create(
            "ExecCtx",
            // Memory
            Int8PtrTy->getPointerTo(),
            // Globals
            Int128PtrTy->getPointerTo(),
            // InstrCount
            Int64PtrTy,
            // CostTable
            llvm::ArrayType::get(Int64Ty, UINT16_MAX + 1)->getPointerTo(),
            // Gas
            Int64PtrTy,
            // GasLimit
            Int64Ty,
            // StopToken
            llvm::Type::getInt32PtrTy(LLContext))),
        ExecCtxPtrTy(ExecCtxTy->getPointerTo()),
        IntrinsicsTableTy(llvm::ArrayType::get(
            Int8PtrTy, uint32_t(AST::Module::Intrinsics::kIntrinsicMax))),
        IntrinsicsTablePtrTy(IntrinsicsTableTy->getPointerTo()),
        IntrinsicsTable(new llvm::GlobalVariable(
            LLModule, IntrinsicsTablePtrTy, true,
            llvm::GlobalVariable::ExternalLinkage, nullptr, "intrinsics")),
        Trap(llvm::Function::Create(
            llvm::FunctionType::get(VoidTy, {Int32Ty}, false),
            llvm::Function::PrivateLinkage, "trap", LLModule)) {
    Trap->addFnAttr(llvm::Attribute::StrictFP);
    Trap->addFnAttr(llvm::Attribute::NoReturn);
    Trap->addFnAttr(llvm::Attribute::Cold);
    Trap->addFnAttr(llvm::Attribute::NoInline);

    new llvm::GlobalVariable(
        LLModule, Int32Ty, true, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(Int32Ty, kBinaryVersion), "version");

    if (!IsGenericBinary) {
      llvm::StringMap<bool> FeatureMap;
      llvm::sys::getHostCPUFeatures(FeatureMap);
      for (auto &Feature : FeatureMap) {
        if (Feature.second) {
#if defined(__x86_64__)
          if (!SupportXOP && Feature.first() == "xop") {
            SupportXOP = true;
          }
          if (!SupportSSE4_1 && Feature.first() == "sse4.1") {
            SupportSSE4_1 = true;
          }
          if (!SupportSSSE3 && Feature.first() == "ssse3") {
            SupportSSSE3 = true;
          }
          if (!SupportSSE2 && Feature.first() == "sse2") {
            SupportSSE2 = true;
          }
#elif defined(__aarch64__)
          if (!SupportNEON && Feature.first() == "neon") {
            SupportNEON = true;
          }
#endif
        }

        SubtargetFeatures.AddFeature(Feature.first(), Feature.second);
      }
    }

    {
      // create trap
      llvm::IRBuilder<> Builder(
          llvm::BasicBlock::Create(LLContext, "entry", Trap));
      auto *CallTrap = Builder.CreateCall(
          getIntrinsic(Builder, AST::Module::Intrinsics::kTrap,
                       llvm::FunctionType::get(VoidTy, {Int32Ty}, false)),
          {Trap->arg_begin()});
      CallTrap->setDoesNotReturn();
      Builder.CreateUnreachable();
    }
  }
  llvm::Value *getMemory(llvm::IRBuilder<> &Builder, llvm::LoadInst *ExecCtx,
                         uint32_t Index) {
    auto *Array = Builder.CreateExtractValue(ExecCtx, {0});
    auto *VPtr = Builder.CreateLoad(
        Int8PtrTy, Builder.CreateConstInBoundsGEP1_64(Int8PtrTy, Array, Index));
    return Builder.CreateBitCast(VPtr, Int8PtrTy);
  }
  std::pair<llvm::Type *, llvm::Value *> getGlobal(llvm::IRBuilder<> &Builder,
                                                   llvm::LoadInst *ExecCtx,
                                                   uint32_t Index) {
    llvm::Type *Ty = Globals[Index];
    auto *Array = Builder.CreateExtractValue(ExecCtx, {1});
    auto *VPtr = Builder.CreateLoad(
        Int128PtrTy,
        Builder.CreateConstInBoundsGEP1_64(Int128PtrTy, Array, Index));
    auto *Ptr = Builder.CreateBitCast(VPtr, Ty->getPointerTo());
    return {Ty, Ptr};
  }
  llvm::Value *getInstrCount(llvm::IRBuilder<> &Builder,
                             llvm::LoadInst *ExecCtx) {
    return Builder.CreateExtractValue(ExecCtx, {2});
  }
  llvm::Value *getCostTable(llvm::IRBuilder<> &Builder,
                            llvm::LoadInst *ExecCtx) {
    return Builder.CreateExtractValue(ExecCtx, {3});
  }
  llvm::Value *getGas(llvm::IRBuilder<> &Builder, llvm::LoadInst *ExecCtx) {
    return Builder.CreateExtractValue(ExecCtx, {4});
  }
  llvm::Value *getGasLimit(llvm::IRBuilder<> &Builder,
                           llvm::LoadInst *ExecCtx) {
    return Builder.CreateExtractValue(ExecCtx, {5});
  }
  llvm::Value *getStopToken(llvm::IRBuilder<> &Builder,
                            llvm::LoadInst *ExecCtx) {
    return Builder.CreateExtractValue(ExecCtx, {6});
  }
  llvm::FunctionCallee getIntrinsic(llvm::IRBuilder<> &Builder,
                                    AST::Module::Intrinsics Index,
                                    llvm::FunctionType *Ty) {
    const auto Value = static_cast<uint32_t>(Index);
    auto *IT = Builder.CreateLoad(IntrinsicsTablePtrTy, IntrinsicsTable);
    IT->setMetadata(llvm::LLVMContext::MD_invariant_load,
                    llvm::MDNode::get(LLContext, {}));
    auto *VPtr =
        Builder.CreateConstInBoundsGEP2_64(IntrinsicsTableTy, IT, 0, Value);
    auto *Ptr = Builder.CreateBitCast(VPtr, Ty->getPointerTo()->getPointerTo());
    return llvm::FunctionCallee(Ty,
                                Builder.CreateLoad(Ty->getPointerTo(), Ptr));
  }
  std::pair<std::vector<ValType>, std::vector<ValType>>
  resolveBlockType(const BlockType &BType) const {
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

static bool isVoidReturn(Span<const WasmEdge::ValType> ValTypes) {
  return ValTypes.empty();
}

static llvm::Type *toLLVMType(llvm::LLVMContext &LLContext,
                              const ValType &ValType) {
  switch (ValType) {
  case ValType::I32:
    return llvm::Type::getInt32Ty(LLContext);
  case ValType::I64:
  case ValType::FuncRef:
  case ValType::ExternRef:
    return llvm::Type::getInt64Ty(LLContext);
  case ValType::V128:
    return llvm::VectorType::get(llvm::Type::getInt64Ty(LLContext), 2, false);
  case ValType::F32:
    return llvm::Type::getFloatTy(LLContext);
  case ValType::F64:
    return llvm::Type::getDoubleTy(LLContext);
  default:
    assumingUnreachable();
  }
}

static std::vector<llvm::Type *>
toLLVMTypeVector(llvm::LLVMContext &LLContext, Span<const ValType> ValTypes) {
  std::vector<llvm::Type *> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(LLContext, Type));
  }
  return Result;
}

static std::vector<llvm::Type *> toLLVMArgsType(llvm::PointerType *ExecCtxPtrTy,
                                                Span<const ValType> ValTypes) {
  auto Result = toLLVMTypeVector(ExecCtxPtrTy->getContext(), ValTypes);
  Result.insert(Result.begin(), ExecCtxPtrTy);
  return Result;
}

static llvm::Type *toLLVMRetsType(llvm::LLVMContext &LLContext,
                                  Span<const ValType> ValTypes) {
  if (isVoidReturn(ValTypes)) {
    return llvm::Type::getVoidTy(LLContext);
  }
  if (ValTypes.size() == 1) {
    return toLLVMType(LLContext, ValTypes.front());
  }
  std::vector<llvm::Type *> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(LLContext, Type));
  }
  return llvm::StructType::create(Result);
}

static llvm::FunctionType *toLLVMType(llvm::PointerType *ExecCtxPtrTy,
                                      const AST::FunctionType &FuncType) {
  auto ArgsTy = toLLVMArgsType(ExecCtxPtrTy, FuncType.getParamTypes());
  auto RetTy =
      toLLVMRetsType(ExecCtxPtrTy->getContext(), FuncType.getReturnTypes());
  return llvm::FunctionType::get(RetTy, ArgsTy, false);
}

static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &LLContext,
                                          const ValType &ValType) {
  switch (ValType) {
  case ValType::I32:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(LLContext), 0);
  case ValType::I64:
  case ValType::FuncRef:
  case ValType::ExternRef:
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(LLContext), 0);
  case ValType::V128:
    return llvm::ConstantAggregateZero::get(
        llvm::VectorType::get(llvm::Type::getInt64Ty(LLContext), 2, false));
  case ValType::F32:
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(LLContext), 0.0);
  case ValType::F64:
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(LLContext), 0.0);
  default:
    assumingUnreachable();
  }
}

class FunctionCompiler {
  struct Control;

public:
  FunctionCompiler(AOT::Compiler::CompileContext &Context, llvm::Function *F,
                   Span<const ValType> Locals, bool Interruptible,
                   bool InstructionCounting, bool GasMeasuring, bool OptNone)
      : Context(Context), LLContext(Context.LLContext),
        Interruptible(Interruptible), OptNone(OptNone), F(F),
        Builder(llvm::BasicBlock::Create(LLContext, "entry", F)) {
    if (F) {
      setIsFPConstrained(Builder);
      ExecCtx = Builder.CreateLoad(Context.ExecCtxTy, F->arg_begin());

      if (InstructionCounting) {
        LocalInstrCount = Builder.CreateAlloca(Context.Int64Ty);
        Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
      }

      if (GasMeasuring) {
        LocalGas = Builder.CreateAlloca(Context.Int64Ty);
        Builder.CreateStore(Builder.getInt64(0), LocalGas);
      }

      for (llvm::Argument *Arg = F->arg_begin() + 1; Arg != F->arg_end();
           ++Arg) {
        llvm::Type *Ty = Arg->getType();
        llvm::Value *ArgPtr = Builder.CreateAlloca(Ty);
        Builder.CreateStore(Arg, ArgPtr);
        Local.emplace_back(Ty, ArgPtr);
      }

      for (const auto &Type : Locals) {
        llvm::Type *Ty = toLLVMType(LLContext, Type);
        llvm::Value *ArgPtr = Builder.CreateAlloca(Ty);
        Builder.CreateStore(toLLVMConstantZero(LLContext, Type), ArgPtr);
        Local.emplace_back(Ty, ArgPtr);
      }
    }
  }

  ~FunctionCompiler() noexcept {
    if (!F) {
      delete Builder.GetInsertBlock();
    }
  }

  llvm::BasicBlock *getTrapBB(ErrCode::Value Error) {
    if (auto Iter = TrapBB.find(Error); Iter != TrapBB.end()) {
      return Iter->second;
    }
    auto *BB = llvm::BasicBlock::Create(LLContext, "trap", F);
    TrapBB.emplace(Error, BB);
    return BB;
  }

  void compile(const AST::CodeSegment &Code,
               std::pair<std::vector<ValType>, std::vector<ValType>> Type) {
    auto *RetBB = llvm::BasicBlock::Create(LLContext, "ret", F);
    Type.first.clear();
    enterBlock(RetBB, nullptr, nullptr, {}, std::move(Type));
    compile(Code.getExpr().getInstrs());
    assuming(ControlStack.empty());
    compileReturn();

    for (auto &[Error, BB] : TrapBB) {
      Builder.SetInsertPoint(BB);
      updateInstrCount();
      updateGasAtTrap();
      auto *CallTrap = Builder.CreateCall(
          Context.Trap, {Builder.getInt32(static_cast<uint32_t>(Error))});
      CallTrap->setDoesNotReturn();
      Builder.CreateUnreachable();
    }
  }

  void compile(AST::InstrView Instrs) {
    auto Dispatch = [this](const AST::Instruction &Instr) -> void {
      switch (Instr.getOpCode()) {
      case OpCode::Block: {
        auto *Block = llvm::BasicBlock::Create(LLContext, "block", F);
        auto *EndBlock = llvm::BasicBlock::Create(LLContext, "block.end", F);
        Builder.CreateBr(Block);

        Builder.SetInsertPoint(Block);
        auto Type = Context.resolveBlockType(Instr.getBlockType());
        const auto Arity = Type.first.size();
        std::vector<llvm::Value *> Args(Arity);
        if (isUnreachable()) {
          for (size_t I = 0; I < Arity; ++I) {
            auto *Ty = toLLVMType(LLContext, Type.first[I]);
            Args[I] = llvm::UndefValue::get(Ty);
          }
        } else {
          for (size_t I = 0; I < Arity; ++I) {
            const size_t J = Arity - 1 - I;
            Args[J] = stackPop();
          }
        }
        enterBlock(EndBlock, nullptr, nullptr, std::move(Args),
                   std::move(Type));
        checkStop();
        updateGas();
        return;
      }
      case OpCode::Loop: {
        auto *Curr = Builder.GetInsertBlock();
        auto *Loop = llvm::BasicBlock::Create(LLContext, "loop", F);
        auto *EndLoop = llvm::BasicBlock::Create(LLContext, "loop.end", F);
        Builder.CreateBr(Loop);

        Builder.SetInsertPoint(Loop);
        auto Type = Context.resolveBlockType(Instr.getBlockType());
        const auto Arity = Type.first.size();
        std::vector<llvm::Value *> Args(Arity);
        if (isUnreachable()) {
          for (size_t I = 0; I < Arity; ++I) {
            auto *Ty = toLLVMType(LLContext, Type.first[I]);
            auto *Value = llvm::UndefValue::get(Ty);
            auto *PHINode = Builder.CreatePHI(Ty, 2);
            PHINode->addIncoming(Value, Curr);
            Args[I] = PHINode;
          }
        } else {
          for (size_t I = 0; I < Arity; ++I) {
            const size_t J = Arity - 1 - I;
            auto *Value = stackPop();
            auto *PHINode = Builder.CreatePHI(Value->getType(), 2);
            PHINode->addIncoming(Value, Curr);
            Args[J] = PHINode;
          }
        }
        enterBlock(Loop, EndLoop, nullptr, std::move(Args), std::move(Type));
        checkStop();
        updateGas();
        return;
      }
      case OpCode::If: {
        auto *Then = llvm::BasicBlock::Create(LLContext, "then", F);
        auto *Else = llvm::BasicBlock::Create(LLContext, "else", F);
        auto *EndIf = llvm::BasicBlock::Create(LLContext, "if.end", F);
        llvm::Value *Cond;
        if (isUnreachable()) {
          Cond = llvm::UndefValue::get(Builder.getInt1Ty());
        } else {
          Cond = Builder.CreateICmpNE(stackPop(), Builder.getInt32(0));
        }
        Builder.CreateCondBr(Cond, Then, Else);

        Builder.SetInsertPoint(Then);
        auto Type = Context.resolveBlockType(Instr.getBlockType());
        const auto Arity = Type.first.size();
        std::vector<llvm::Value *> Args(Arity);
        if (isUnreachable()) {
          for (size_t I = 0; I < Arity; ++I) {
            auto *Ty = toLLVMType(LLContext, Type.first[I]);
            Args[I] = llvm::UndefValue::get(Ty);
          }
        } else {
          for (size_t I = 0; I < Arity; ++I) {
            const size_t J = Arity - 1 - I;
            Args[J] = stackPop();
          }
        }
        enterBlock(EndIf, nullptr, Else, std::move(Args), std::move(Type));
        return;
      }
      case OpCode::End: {
        auto Entry = leaveBlock();
        if (Entry.ElseBlock) {
          auto *Block = Builder.GetInsertBlock();
          Builder.SetInsertPoint(Entry.ElseBlock);
          enterBlock(Block, nullptr, nullptr, std::move(Entry.Args),
                     std::move(Entry.Type), std::move(Entry.ReturnPHI));
          Entry = leaveBlock();
        }
        buildPHI(Entry.Type.second, Entry.ReturnPHI);
        return;
      }
      case OpCode::Else: {
        auto Entry = leaveBlock();
        Builder.SetInsertPoint(Entry.ElseBlock);
        enterBlock(Entry.JumpBlock, nullptr, nullptr, std::move(Entry.Args),
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
        Builder.CreateBr(getTrapBB(ErrCode::Value::Unreachable));
        setUnreachable();
        Builder.SetInsertPoint(
            llvm::BasicBlock::Create(LLContext, "unreachable.end", F));
        break;
      case OpCode::Nop:
        break;
      case OpCode::Return:
        compileReturn();
        setUnreachable();
        Builder.SetInsertPoint(
            llvm::BasicBlock::Create(LLContext, "ret.end", F));
        break;
      case OpCode::Br: {
        const auto Label = Instr.getJump().TargetIndex;
        setLableJumpPHI(Label);
        Builder.CreateBr(getLabel(Label));
        setUnreachable();
        Builder.SetInsertPoint(
            llvm::BasicBlock::Create(LLContext, "br.end", F));
        break;
      }
      case OpCode::Br_if: {
        const auto Label = Instr.getJump().TargetIndex;
        auto *Cond = Builder.CreateICmpNE(stackPop(), Builder.getInt32(0));
        setLableJumpPHI(Label);
        auto *Next = llvm::BasicBlock::Create(LLContext, "br_if.end", F);
        Builder.CreateCondBr(Cond, getLabel(Label), Next);
        Builder.SetInsertPoint(Next);
        break;
      }
      case OpCode::Br_table: {
        auto LabelTable = Instr.getLabelList();
        assuming(LabelTable.size() <= std::numeric_limits<uint32_t>::max());
        const auto LabelTableSize =
            static_cast<uint32_t>(LabelTable.size() - 1);
        auto *Value = stackPop();
        setLableJumpPHI(LabelTable[LabelTableSize].TargetIndex);
        auto *Switch = Builder.CreateSwitch(
            Value, getLabel(LabelTable[LabelTableSize].TargetIndex),
            LabelTableSize);
        for (uint32_t I = 0; I < LabelTableSize; ++I) {
          setLableJumpPHI(LabelTable[I].TargetIndex);
          Switch->addCase(Builder.getInt32(I),
                          getLabel(LabelTable[I].TargetIndex));
        }
        setUnreachable();
        Builder.SetInsertPoint(
            llvm::BasicBlock::Create(LLContext, "br_table.end", F));
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
        Builder.SetInsertPoint(
            llvm::BasicBlock::Create(LLContext, "ret_call.end", F));
        break;
      case OpCode::Return_call_indirect:
        updateInstrCount();
        updateGas();
        compileReturnIndirectCallOp(Instr.getSourceIndex(),
                                    Instr.getTargetIndex());
        setUnreachable();
        Builder.SetInsertPoint(
            llvm::BasicBlock::Create(LLContext, "ret_call_indir.end", F));
        break;
      case OpCode::Ref__null:
        stackPush(Builder.getInt64(0));
        break;
      case OpCode::Ref__is_null:
        stackPush(Builder.CreateZExt(
            Builder.CreateICmpEQ(stackPop(), Builder.getInt64(0)),
            Context.Int32Ty));
        break;
      case OpCode::Ref__func:
        stackPush(Builder.CreateCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kRefFunc,
                                 llvm::FunctionType::get(Context.Int64Ty,
                                                         {Context.Int32Ty},
                                                         false)),
            {Builder.getInt32(Instr.getTargetIndex())}));
        break;
      case OpCode::Drop:
        stackPop();
        break;
      case OpCode::Select:
      case OpCode::Select_t: {
        auto *Cond = Builder.CreateICmpNE(stackPop(), Builder.getInt32(0));
        auto *False = stackPop();
        auto *True = stackPop();
        stackPush(Builder.CreateSelect(Cond, True, False));
        break;
      }
      case OpCode::Local__get: {
        const auto &L = Local[Instr.getTargetIndex()];
        stackPush(Builder.CreateLoad(L.first, L.second));
        break;
      }
      case OpCode::Local__set:
        Builder.CreateStore(stackPop(), Local[Instr.getTargetIndex()].second);
        break;
      case OpCode::Local__tee:
        Builder.CreateStore(Stack.back(), Local[Instr.getTargetIndex()].second);
        break;
      case OpCode::Global__get: {
        const auto G =
            Context.getGlobal(Builder, ExecCtx, Instr.getTargetIndex());
        stackPush(Builder.CreateLoad(G.first, G.second));
        break;
      }
      case OpCode::Global__set:
        Builder.CreateStore(
            stackPop(),
            Context.getGlobal(Builder, ExecCtx, Instr.getTargetIndex()).second);
        break;
      case OpCode::Table__get: {
        auto *Idx = stackPop();
        stackPush(Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableGet,
                llvm::FunctionType::get(Context.Int64Ty,
                                        {Context.Int32Ty, Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()), Idx}));
        break;
      }
      case OpCode::Table__set: {
        auto *Ref = stackPop();
        auto *Idx = stackPop();
        Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableSet,
                llvm::FunctionType::get(
                    Context.Int64Ty,
                    {Context.Int32Ty, Context.Int32Ty, Context.Int64Ty},
                    false)),
            {Builder.getInt32(Instr.getTargetIndex()), Idx, Ref});
        break;
      }
      case OpCode::Table__init: {
        auto *Len = stackPop();
        auto *Src = stackPop();
        auto *Dst = stackPop();
        Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableInit,
                llvm::FunctionType::get(Context.VoidTy,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()),
             Builder.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Elem__drop: {
        Builder.CreateCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kElemDrop,
                                 llvm::FunctionType::get(
                                     Context.VoidTy, {Context.Int32Ty}, false)),
            {Builder.getInt32(Instr.getTargetIndex())});
        break;
      }
      case OpCode::Table__copy: {
        auto *Len = stackPop();
        auto *Src = stackPop();
        auto *Dst = stackPop();
        Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableCopy,
                llvm::FunctionType::get(Context.VoidTy,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()),
             Builder.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Table__grow: {
        auto *NewSize = stackPop();
        auto *Val = stackPop();
        stackPush(Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableGrow,
                llvm::FunctionType::get(
                    Context.Int32Ty,
                    {Context.Int32Ty, Context.Int64Ty, Context.Int32Ty},
                    false)),
            {Builder.getInt32(Instr.getTargetIndex()), Val, NewSize}));
        break;
      }
      case OpCode::Table__size: {
        stackPush(Builder.CreateCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kTableSize,
                                 llvm::FunctionType::get(Context.Int32Ty,
                                                         {Context.Int32Ty},
                                                         false)),
            {Builder.getInt32(Instr.getTargetIndex())}));
        break;
      }
      case OpCode::Table__fill: {
        auto *Len = stackPop();
        auto *Val = stackPop();
        auto *Off = stackPop();
        Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kTableFill,
                llvm::FunctionType::get(Context.Int32Ty,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int64Ty, Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()), Off, Val, Len});
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
        stackPush(Builder.CreateCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kMemSize,
                                 llvm::FunctionType::get(Context.Int32Ty,
                                                         {Context.Int32Ty},
                                                         false)),
            {Builder.getInt32(Instr.getTargetIndex())}));
        break;
      case OpCode::Memory__grow: {
        auto *Diff = stackPop();
        stackPush(Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemGrow,
                llvm::FunctionType::get(Context.Int32Ty,
                                        {Context.Int32Ty, Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()), Diff}));
        break;
      }
      case OpCode::Memory__init: {
        auto *Len = stackPop();
        auto *Src = stackPop();
        auto *Dst = stackPop();
        Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemInit,
                llvm::FunctionType::get(Context.VoidTy,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()),
             Builder.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Data__drop: {
        Builder.CreateCall(
            Context.getIntrinsic(Builder, AST::Module::Intrinsics::kDataDrop,
                                 llvm::FunctionType::get(
                                     Context.VoidTy, {Context.Int32Ty}, false)),
            {Builder.getInt32(Instr.getTargetIndex())});
        break;
      }
      case OpCode::Memory__copy: {
        auto *Len = stackPop();
        auto *Src = stackPop();
        auto *Dst = stackPop();
        Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemCopy,
                llvm::FunctionType::get(Context.VoidTy,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()),
             Builder.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
        break;
      }
      case OpCode::Memory__fill: {
        auto *Len = stackPop();
        auto *Val = Builder.CreateTrunc(stackPop(), Context.Int8Ty);
        auto *Off = stackPop();
        Builder.CreateCall(
            Context.getIntrinsic(
                Builder, AST::Module::Intrinsics::kMemFill,
                llvm::FunctionType::get(Context.VoidTy,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int8Ty, Context.Int32Ty},
                                        false)),
            {Builder.getInt32(Instr.getTargetIndex()), Off, Val, Len});
        break;
      }
      case OpCode::I32__const:
        stackPush(Builder.getInt32(Instr.getNum().get<uint32_t>()));
        break;
      case OpCode::I64__const:
        stackPush(Builder.getInt64(Instr.getNum().get<uint64_t>()));
        break;
      case OpCode::F32__const:
        stackPush(llvm::ConstantFP::get(
            Context.FloatTy, llvm::APFloat(Instr.getNum().get<float>())));
        break;
      case OpCode::F64__const:
        stackPush(llvm::ConstantFP::get(
            Context.DoubleTy, llvm::APFloat(Instr.getNum().get<double>())));
        break;
      case OpCode::I32__eqz:
        stackPush(Builder.CreateZExt(
            Builder.CreateICmpEQ(stackPop(), Builder.getInt32(0)),
            Context.Int32Ty));
        break;
      case OpCode::I64__eqz:
        stackPush(Builder.CreateZExt(
            Builder.CreateICmpEQ(stackPop(), Builder.getInt64(0)),
            Context.Int32Ty));
        break;
      case OpCode::I32__clz:
      case OpCode::I64__clz:
        stackPush(Builder.CreateBinaryIntrinsic(
            llvm::Intrinsic::ctlz, stackPop(), Builder.getFalse()));
        break;
      case OpCode::I32__ctz:
      case OpCode::I64__ctz:
        stackPush(Builder.CreateBinaryIntrinsic(
            llvm::Intrinsic::cttz, stackPop(), Builder.getFalse()));
        break;
      case OpCode::I32__popcnt:
      case OpCode::I64__popcnt:
        stackPush(
            Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, stackPop()));
        break;
      case OpCode::F32__abs:
      case OpCode::F64__abs:
        stackPush(
            Builder.CreateUnaryIntrinsic(llvm::Intrinsic::fabs, stackPop()));
        break;
      case OpCode::F32__neg:
      case OpCode::F64__neg:
        stackPush(Builder.CreateFNeg(stackPop()));
        break;
      case OpCode::F32__ceil:
      case OpCode::F64__ceil:
        stackPush(
            Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ceil, stackPop()));
        break;
      case OpCode::F32__floor:
      case OpCode::F64__floor:
        stackPush(
            Builder.CreateUnaryIntrinsic(llvm::Intrinsic::floor, stackPop()));
        break;
      case OpCode::F32__trunc:
      case OpCode::F64__trunc:
        stackPush(
            Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, stackPop()));
        break;
      case OpCode::F32__nearest:
      case OpCode::F64__nearest: {
        const bool IsFloat = Instr.getOpCode() == OpCode::F32__nearest;
        // The VectorSize is only used when SSE4_1 or NEON is supported.
        [[maybe_unused]] const uint32_t VectorSize = IsFloat ? 4 : 2;
        llvm::Value *Value = stackPop();

#if defined(__x86_64__)
        if (Context.SupportSSE4_1) {
          const uint64_t kZero = 0;
          auto *VectorTy =
              llvm::VectorType::get(Value->getType(), VectorSize, false);
          llvm::Value *Ret = llvm::UndefValue::get(VectorTy);
          Ret = Builder.CreateInsertElement(Ret, Value, kZero);
          auto ID = IsFloat ? llvm::Intrinsic::x86_sse41_round_ss
                            : llvm::Intrinsic::x86_sse41_round_sd;
          Ret =
              Builder.CreateIntrinsic(ID, {}, {Ret, Ret, Builder.getInt32(8)});
          Ret = Builder.CreateExtractElement(Ret, kZero);
          stackPush(Ret);
          break;
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON) {
          const uint64_t kZero = 0;
          auto *VectorTy =
              llvm::VectorType::get(Value->getType(), VectorSize, false);
          llvm::Value *Ret = llvm::UndefValue::get(VectorTy);
          Ret = Builder.CreateInsertElement(Ret, Value, kZero);
#if LLVM_VERSION_MAJOR >= 13
          Ret = Builder.CreateUnaryIntrinsic(llvm::Intrinsic::roundeven, Ret);
#else
          Ret = Builder.CreateUnaryIntrinsic(
              llvm::Intrinsic::aarch64_neon_frintn, Ret);
#endif
          Ret = Builder.CreateExtractElement(Ret, kZero);
          stackPush(Ret);
          break;
        }
#endif

        // Fallback case.
        // If the SSE4.1 is not supported on the x86_64 platform or
        // the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        stackPush(
            Builder.CreateUnaryIntrinsic(llvm::Intrinsic::nearbyint, Value));
        break;
      }
      case OpCode::F32__sqrt:
      case OpCode::F64__sqrt:
        stackPush(
            Builder.CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, stackPop()));
        break;
      case OpCode::I32__wrap_i64:
        stackPush(Builder.CreateTrunc(stackPop(), Context.Int32Ty));
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
        stackPush(Builder.CreateSExt(stackPop(), Context.Int64Ty));
        break;
      case OpCode::I64__extend_i32_u:
        stackPush(Builder.CreateZExt(stackPop(), Context.Int64Ty));
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
        stackPush(Builder.CreateSIToFP(stackPop(), Context.FloatTy));
        break;
      case OpCode::F32__convert_i32_u:
      case OpCode::F32__convert_i64_u:
        stackPush(Builder.CreateUIToFP(stackPop(), Context.FloatTy));
        break;
      case OpCode::F64__convert_i32_s:
      case OpCode::F64__convert_i64_s:
        stackPush(Builder.CreateSIToFP(stackPop(), Context.DoubleTy));
        break;
      case OpCode::F64__convert_i32_u:
      case OpCode::F64__convert_i64_u:
        stackPush(Builder.CreateUIToFP(stackPop(), Context.DoubleTy));
        break;
      case OpCode::F32__demote_f64: {
#if LLVM_VERSION_MAJOR >= 10
        stackPush(Builder.CreateFPTrunc(stackPop(), Context.FloatTy));
#else
        // llvm 9 didn't add constrains on fptrunc, do it manually.
        auto &LLContext = Context.LLContext;
        auto *Value = stackPop();
        auto ExceptStr = llvm::ConstrainedFPIntrinsic::ExceptionBehaviorToStr(
            Builder.getDefaultConstrainedExcept());
        auto *ExceptMDS = llvm::MDString::get(LLContext, ExceptStr.getValue());
        auto *ExceptV = llvm::MetadataAsValue::get(LLContext, ExceptMDS);
        auto RoundingStr = llvm::ConstrainedFPIntrinsic::RoundingModeToStr(
            Builder.getDefaultConstrainedRounding());
        auto *RoundingMDS =
            llvm::MDString::get(Context.LLContext, RoundingStr.getValue());
        auto *RoundingV = llvm::MetadataAsValue::get(LLContext, RoundingMDS);
        stackPush(Builder.CreateIntrinsic(
            llvm::Intrinsic::experimental_constrained_fptrunc,
            {Context.FloatTy, Value->getType()}, {Value, RoundingV, ExceptV}));
#endif
        break;
      }
      case OpCode::F64__promote_f32:
        stackPush(Builder.CreateFPExt(stackPop(), Context.DoubleTy));
        break;
      case OpCode::I32__reinterpret_f32:
        stackPush(Builder.CreateBitCast(stackPop(), Context.Int32Ty));
        break;
      case OpCode::I64__reinterpret_f64:
        stackPush(Builder.CreateBitCast(stackPop(), Context.Int64Ty));
        break;
      case OpCode::F32__reinterpret_i32:
        stackPush(Builder.CreateBitCast(stackPop(), Context.FloatTy));
        break;
      case OpCode::F64__reinterpret_i64:
        stackPush(Builder.CreateBitCast(stackPop(), Context.DoubleTy));
        break;
      case OpCode::I32__extend8_s:
        stackPush(Builder.CreateSExt(
            Builder.CreateTrunc(stackPop(), Context.Int8Ty), Context.Int32Ty));
        break;
      case OpCode::I32__extend16_s:
        stackPush(Builder.CreateSExt(
            Builder.CreateTrunc(stackPop(), Context.Int16Ty), Context.Int32Ty));
        break;
      case OpCode::I64__extend8_s:
        stackPush(Builder.CreateSExt(
            Builder.CreateTrunc(stackPop(), Context.Int8Ty), Context.Int64Ty));
        break;
      case OpCode::I64__extend16_s:
        stackPush(Builder.CreateSExt(
            Builder.CreateTrunc(stackPop(), Context.Int16Ty), Context.Int64Ty));
        break;
      case OpCode::I64__extend32_s:
        stackPush(Builder.CreateSExt(
            Builder.CreateTrunc(stackPop(), Context.Int32Ty), Context.Int64Ty));
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
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpEQ(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__ne:
      case OpCode::I64__ne: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpNE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__lt_s:
      case OpCode::I64__lt_s: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpSLT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__lt_u:
      case OpCode::I64__lt_u: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpULT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__gt_s:
      case OpCode::I64__gt_s: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpSGT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__gt_u:
      case OpCode::I64__gt_u: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpUGT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__le_s:
      case OpCode::I64__le_s: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpSLE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__le_u:
      case OpCode::I64__le_u: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpULE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__ge_s:
      case OpCode::I64__ge_s: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpSGE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__ge_u:
      case OpCode::I64__ge_u: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateICmpUGE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__eq:
      case OpCode::F64__eq: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateFCmpOEQ(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__ne:
      case OpCode::F64__ne: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateFCmpUNE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__lt:
      case OpCode::F64__lt: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateFCmpOLT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__gt:
      case OpCode::F64__gt: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateFCmpOGT(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__le:
      case OpCode::F64__le: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateFCmpOLE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::F32__ge:
      case OpCode::F64__ge: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateZExt(Builder.CreateFCmpOGE(LHS, RHS),
                                     Context.Int32Ty));
        break;
      }
      case OpCode::I32__add:
      case OpCode::I64__add: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateAdd(LHS, RHS));
        break;
      }
      case OpCode::I32__sub:
      case OpCode::I64__sub: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();

        stackPush(Builder.CreateSub(LHS, RHS));
        break;
      }
      case OpCode::I32__mul:
      case OpCode::I64__mul: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateMul(LHS, RHS));
        break;
      }
      case OpCode::I32__div_s:
      case OpCode::I64__div_s: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        if constexpr (kForceDivCheck) {
          const bool Is32 = Instr.getOpCode() == OpCode::I32__div_s;
          llvm::ConstantInt *IntZero =
              Is32 ? Builder.getInt32(0) : Builder.getInt64(0);
          llvm::ConstantInt *IntMinusOne =
              Is32 ? Builder.getInt32(static_cast<uint32_t>(INT32_C(-1)))
                   : Builder.getInt64(static_cast<uint64_t>(INT64_C(-1)));
          llvm::ConstantInt *IntMin =
              Is32 ? Builder.getInt32(static_cast<uint32_t>(
                         std::numeric_limits<int32_t>::min()))
                   : Builder.getInt64(static_cast<uint64_t>(
                         std::numeric_limits<int64_t>::min()));

          auto *NoZeroBB = llvm::BasicBlock::Create(LLContext, "div.nozero", F);
          auto *OkBB = llvm::BasicBlock::Create(LLContext, "div.ok", F);

          auto *IsNotZero =
              createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
          Builder.CreateCondBr(IsNotZero, NoZeroBB,
                               getTrapBB(ErrCode::Value::DivideByZero));

          Builder.SetInsertPoint(NoZeroBB);
          auto *NotOverflow = createLikely(
              Builder,
              Builder.CreateOr(Builder.CreateICmpNE(LHS, IntMin),
                               Builder.CreateICmpNE(RHS, IntMinusOne)));
          Builder.CreateCondBr(NotOverflow, OkBB,
                               getTrapBB(ErrCode::Value::IntegerOverflow));

          Builder.SetInsertPoint(OkBB);
        }
        stackPush(Builder.CreateSDiv(LHS, RHS));
        break;
      }
      case OpCode::I32__div_u:
      case OpCode::I64__div_u: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        if constexpr (kForceDivCheck) {
          const bool Is32 = Instr.getOpCode() == OpCode::I32__div_u;
          llvm::ConstantInt *IntZero =
              Is32 ? Builder.getInt32(0) : Builder.getInt64(0);
          auto *OkBB = llvm::BasicBlock::Create(LLContext, "div.ok", F);

          auto *IsNotZero =
              createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
          Builder.CreateCondBr(IsNotZero, OkBB,
                               getTrapBB(ErrCode::Value::DivideByZero));
          Builder.SetInsertPoint(OkBB);
        }
        stackPush(Builder.CreateUDiv(LHS, RHS));
        break;
      }
      case OpCode::I32__rem_s:
      case OpCode::I64__rem_s: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        // handle INT32_MIN % -1
        const bool Is32 = Instr.getOpCode() == OpCode::I32__rem_s;
        llvm::ConstantInt *IntMinusOne =
            Is32 ? Builder.getInt32(static_cast<uint32_t>(INT32_C(-1)))
                 : Builder.getInt64(static_cast<uint64_t>(INT64_C(-1)));
        llvm::ConstantInt *IntMin =
            Is32 ? Builder.getInt32(static_cast<uint32_t>(
                       std::numeric_limits<int32_t>::min()))
                 : Builder.getInt64(static_cast<uint64_t>(
                       std::numeric_limits<int64_t>::min()));
        llvm::ConstantInt *IntZero =
            Is32 ? Builder.getInt32(0) : Builder.getInt64(0);

        auto *NoOverflowBB =
            llvm::BasicBlock::Create(LLContext, "no.overflow", F);
        auto *EndBB = llvm::BasicBlock::Create(LLContext, "end.overflow", F);

        if constexpr (kForceDivCheck) {
          auto *OkBB = llvm::BasicBlock::Create(LLContext, "rem.ok", F);

          auto *IsNotZero =
              createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
          Builder.CreateCondBr(IsNotZero, OkBB,
                               getTrapBB(ErrCode::Value::DivideByZero));
          Builder.SetInsertPoint(OkBB);
        }

        auto *CurrBB = Builder.GetInsertBlock();

        auto *NotOverflow = createLikely(
            Builder, Builder.CreateOr(Builder.CreateICmpNE(LHS, IntMin),
                                      Builder.CreateICmpNE(RHS, IntMinusOne)));
        Builder.CreateCondBr(NotOverflow, NoOverflowBB, EndBB);

        Builder.SetInsertPoint(NoOverflowBB);
        auto *Ret1 = Builder.CreateSRem(LHS, RHS);
        Builder.CreateBr(EndBB);

        Builder.SetInsertPoint(EndBB);
        auto *Ret = Builder.CreatePHI(Ret1->getType(), 2);
        Ret->addIncoming(Ret1, NoOverflowBB);
        Ret->addIncoming(IntZero, CurrBB);

        stackPush(Ret);
        break;
      }
      case OpCode::I32__rem_u:
      case OpCode::I64__rem_u: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        if constexpr (kForceDivCheck) {
          llvm::ConstantInt *IntZero = Instr.getOpCode() == OpCode::I32__rem_u
                                           ? Builder.getInt32(0)
                                           : Builder.getInt64(0);
          auto *OkBB = llvm::BasicBlock::Create(LLContext, "rem.ok", F);

          auto *IsNotZero =
              createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
          Builder.CreateCondBr(IsNotZero, OkBB,
                               getTrapBB(ErrCode::Value::DivideByZero));
          Builder.SetInsertPoint(OkBB);
        }
        stackPush(Builder.CreateURem(LHS, RHS));
        break;
      }
      case OpCode::I32__and:
      case OpCode::I64__and: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateAnd(LHS, RHS));
        break;
      }
      case OpCode::I32__or:
      case OpCode::I64__or: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateOr(LHS, RHS));
        break;
      }
      case OpCode::I32__xor:
      case OpCode::I64__xor: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateXor(LHS, RHS));
        break;
      }
      case OpCode::I32__shl:
      case OpCode::I64__shl: {
        llvm::ConstantInt *Mask = Instr.getOpCode() == OpCode::I32__shl
                                      ? Builder.getInt32(31)
                                      : Builder.getInt64(63);
        llvm::Value *RHS = Builder.CreateAnd(stackPop(), Mask);
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateShl(LHS, RHS));
        break;
      }
      case OpCode::I32__shr_s:
      case OpCode::I64__shr_s: {
        llvm::ConstantInt *Mask = Instr.getOpCode() == OpCode::I32__shr_s
                                      ? Builder.getInt32(31)
                                      : Builder.getInt64(63);
        llvm::Value *RHS = Builder.CreateAnd(stackPop(), Mask);
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateAShr(LHS, RHS));
        break;
      }
      case OpCode::I32__shr_u:
      case OpCode::I64__shr_u: {
        llvm::ConstantInt *Mask = Instr.getOpCode() == OpCode::I32__shr_u
                                      ? Builder.getInt32(31)
                                      : Builder.getInt64(63);
        llvm::Value *RHS = Builder.CreateAnd(stackPop(), Mask);
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateLShr(LHS, RHS));
        break;
      }
      case OpCode::I32__rotl: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshl,
                                          {Context.Int32Ty}, {LHS, LHS, RHS}));
        break;
      }
      case OpCode::I32__rotr: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshr,
                                          {Context.Int32Ty}, {LHS, LHS, RHS}));
        break;
      }
      case OpCode::I64__rotl: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshl,
                                          {Context.Int64Ty}, {LHS, LHS, RHS}));
        break;
      }
      case OpCode::I64__rotr: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshr,
                                          {Context.Int64Ty}, {LHS, LHS, RHS}));
        break;
      }
      case OpCode::F32__add:
      case OpCode::F64__add: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateFAdd(LHS, RHS));
        break;
      }
      case OpCode::F32__sub:
      case OpCode::F64__sub: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateFSub(LHS, RHS));
        break;
      }
      case OpCode::F32__mul:
      case OpCode::F64__mul: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateFMul(LHS, RHS));
        break;
      }
      case OpCode::F32__div:
      case OpCode::F64__div: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(Builder.CreateFDiv(LHS, RHS));
        break;
      }
      case OpCode::F32__min:
      case OpCode::F64__min: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        auto *FpTy = Instr.getOpCode() == OpCode::F32__min ? Context.FloatTy
                                                           : Context.DoubleTy;
        auto *IntTy = Instr.getOpCode() == OpCode::F32__min ? Context.Int32Ty
                                                            : Context.Int64Ty;

        auto *UEQ = Builder.CreateFCmpUEQ(LHS, RHS);
        auto *UNO = Builder.CreateFCmpUNO(LHS, RHS);

        auto *LHSInt = Builder.CreateBitCast(LHS, IntTy);
        auto *RHSInt = Builder.CreateBitCast(RHS, IntTy);
        auto *OrInt = Builder.CreateOr(LHSInt, RHSInt);
        auto *OrFp = Builder.CreateBitCast(OrInt, FpTy);

        auto *AddFp = Builder.CreateFAdd(LHS, RHS);

        auto *MinFp =
            Builder.CreateBinaryIntrinsic(llvm::Intrinsic::minnum, LHS, RHS);
        MinFp->setHasNoNaNs(true);

        auto *Ret = Builder.CreateSelect(
            UEQ, Builder.CreateSelect(UNO, AddFp, OrFp), MinFp);
        stackPush(Ret);
        break;
      }
      case OpCode::F32__max:
      case OpCode::F64__max: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        auto *FpTy = Instr.getOpCode() == OpCode::F32__max ? Context.FloatTy
                                                           : Context.DoubleTy;
        auto *IntTy = Instr.getOpCode() == OpCode::F32__max ? Context.Int32Ty
                                                            : Context.Int64Ty;

        auto *UEQ = Builder.CreateFCmpUEQ(LHS, RHS);
        auto *UNO = Builder.CreateFCmpUNO(LHS, RHS);

        auto *LHSInt = Builder.CreateBitCast(LHS, IntTy);
        auto *RHSInt = Builder.CreateBitCast(RHS, IntTy);
        auto *AndInt = Builder.CreateAnd(LHSInt, RHSInt);
        auto *AndFp = Builder.CreateBitCast(AndInt, FpTy);

        auto *AddFp = Builder.CreateFAdd(LHS, RHS);

        auto *MaxFp =
            Builder.CreateBinaryIntrinsic(llvm::Intrinsic::maxnum, LHS, RHS);
        MaxFp->setHasNoNaNs(true);

        auto *Ret = Builder.CreateSelect(
            UEQ, Builder.CreateSelect(UNO, AddFp, AndFp), MaxFp);
        stackPush(Ret);
        break;
      }
      case OpCode::F32__copysign:
      case OpCode::F64__copysign: {
        llvm::Value *RHS = stackPop();
        llvm::Value *LHS = stackPop();
        stackPush(
            Builder.CreateBinaryIntrinsic(llvm::Intrinsic::copysign, LHS, RHS));
        break;
      }
      case OpCode::V128__load:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(), Context.Int128x1Ty);
        break;
      case OpCode::V128__load8x8_s:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            llvm::VectorType::get(Context.Int8Ty, 8, false),
                            Context.Int16x8Ty, true);
        break;
      case OpCode::V128__load8x8_u:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            llvm::VectorType::get(Context.Int8Ty, 8, false),
                            Context.Int16x8Ty, false);
        break;
      case OpCode::V128__load16x4_s:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            llvm::VectorType::get(Context.Int16Ty, 4, false),
                            Context.Int32x4Ty, true);
        break;
      case OpCode::V128__load16x4_u:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            llvm::VectorType::get(Context.Int16Ty, 4, false),
                            Context.Int32x4Ty, false);
        break;
      case OpCode::V128__load32x2_s:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            llvm::VectorType::get(Context.Int32Ty, 2, false),
                            Context.Int64x2Ty, true);
        break;
      case OpCode::V128__load32x2_u:
        compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                            Instr.getMemoryAlign(),
                            llvm::VectorType::get(Context.Int32Ty, 2, false),
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
        auto *Vector = llvm::ConstantVector::get(
            {Builder.getInt64(Value[0]), Builder.getInt64(Value[1])});
        stackPush(Builder.CreateBitCast(Vector, Context.Int64x2Ty));
        break;
      }
      case OpCode::I8x16__shuffle: {
        auto *V2 = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);
        auto *V1 = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);
        const auto V3 = Instr.getNum().get<uint128_t>();
        std::array<ShuffleElement, 16> Mask;
        for (size_t I = 0; I < 16; ++I) {
          Mask[I] = static_cast<uint8_t>(V3 >> (I * 8));
        }
        stackPush(Builder.CreateBitCast(
            Builder.CreateShuffleVector(V1, V2, Mask), Context.Int64x2Ty));
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
        auto *Index = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);
        auto *Vector = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);

#if defined(__x86_64__)
        if (Context.SupportSSSE3) {
          auto *Magic = Builder.CreateVectorSplat(16, Builder.getInt8(112));
          auto *Added = Builder.CreateAdd(Index, Magic);
          auto *NewIndex = Builder.CreateSelect(
              Builder.CreateICmpUGT(Index, Added),
              llvm::Constant::getAllOnesValue(Context.Int8x16Ty), Added);
          stackPush(Builder.CreateBitCast(
              Builder.CreateIntrinsic(llvm::Intrinsic::x86_ssse3_pshuf_b_128,
                                      {}, {Vector, NewIndex}),
              Context.Int64x2Ty));
          break;
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON) {
          stackPush(Builder.CreateBitCast(
              Builder.CreateBinaryIntrinsic(llvm::Intrinsic::aarch64_neon_tbl1,
                                            Vector, Index),
              Context.Int64x2Ty));
          break;
        }
#endif

        // Fallback case.
        // If the SSSE3 is not supported on the x86_64 platform or
        // the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        auto *Mask = Builder.CreateVectorSplat(16, Builder.getInt8(15));
        auto *Zero = Builder.CreateVectorSplat(16, Builder.getInt8(0));
        auto *IsOver = Builder.CreateICmpUGT(Index, Mask);
        auto *InboundIndex = Builder.CreateAnd(Index, Mask);
        auto *Array =
            Builder.CreateAlloca(Context.Int8Ty, Builder.getInt64(16));
        for (size_t I = 0; I < 16; ++I) {
          Builder.CreateStore(
              Builder.CreateExtractElement(Vector, I),
              Builder.CreateConstInBoundsGEP1_64(Context.Int8Ty, Array, I));
        }
        llvm::Value *Ret = llvm::UndefValue::get(Context.Int8x16Ty);
        for (size_t I = 0; I < 16; ++I) {
          auto *Idx = Builder.CreateExtractElement(InboundIndex, I);
          auto *Value = Builder.CreateLoad(
              Context.Int8Ty,
              Builder.CreateInBoundsGEP(Context.Int8PtrTy, Array, Idx));
          Ret = Builder.CreateInsertElement(Ret, Value, I);
        }
        Ret = Builder.CreateSelect(IsOver, Zero, Ret);
        stackPush(Builder.CreateBitCast(Ret, Context.Int64x2Ty));
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
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_EQ);
        break;
      case OpCode::I8x16__ne:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_NE);
        break;
      case OpCode::I8x16__lt_s:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_SLT);
        break;
      case OpCode::I8x16__lt_u:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_ULT);
        break;
      case OpCode::I8x16__gt_s:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_SGT);
        break;
      case OpCode::I8x16__gt_u:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_UGT);
        break;
      case OpCode::I8x16__le_s:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_SLE);
        break;
      case OpCode::I8x16__le_u:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_ULE);
        break;
      case OpCode::I8x16__ge_s:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_SGE);
        break;
      case OpCode::I8x16__ge_u:
        compileVectorCompareOp(Context.Int8x16Ty,
                               llvm::CmpInst::Predicate::ICMP_UGE);
        break;
      case OpCode::I16x8__eq:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_EQ);
        break;
      case OpCode::I16x8__ne:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_NE);
        break;
      case OpCode::I16x8__lt_s:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_SLT);
        break;
      case OpCode::I16x8__lt_u:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_ULT);
        break;
      case OpCode::I16x8__gt_s:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_SGT);
        break;
      case OpCode::I16x8__gt_u:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_UGT);
        break;
      case OpCode::I16x8__le_s:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_SLE);
        break;
      case OpCode::I16x8__le_u:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_ULE);
        break;
      case OpCode::I16x8__ge_s:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_SGE);
        break;
      case OpCode::I16x8__ge_u:
        compileVectorCompareOp(Context.Int16x8Ty,
                               llvm::CmpInst::Predicate::ICMP_UGE);
        break;
      case OpCode::I32x4__eq:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_EQ);
        break;
      case OpCode::I32x4__ne:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_NE);
        break;
      case OpCode::I32x4__lt_s:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_SLT);
        break;
      case OpCode::I32x4__lt_u:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_ULT);
        break;
      case OpCode::I32x4__gt_s:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_SGT);
        break;
      case OpCode::I32x4__gt_u:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_UGT);
        break;
      case OpCode::I32x4__le_s:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_SLE);
        break;
      case OpCode::I32x4__le_u:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_ULE);
        break;
      case OpCode::I32x4__ge_s:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_SGE);
        break;
      case OpCode::I32x4__ge_u:
        compileVectorCompareOp(Context.Int32x4Ty,
                               llvm::CmpInst::Predicate::ICMP_UGE);
        break;
      case OpCode::I64x2__eq:
        compileVectorCompareOp(Context.Int64x2Ty,
                               llvm::CmpInst::Predicate::ICMP_EQ);
        break;
      case OpCode::I64x2__ne:
        compileVectorCompareOp(Context.Int64x2Ty,
                               llvm::CmpInst::Predicate::ICMP_NE);
        break;
      case OpCode::I64x2__lt_s:
        compileVectorCompareOp(Context.Int64x2Ty,
                               llvm::CmpInst::Predicate::ICMP_SLT);
        break;
      case OpCode::I64x2__gt_s:
        compileVectorCompareOp(Context.Int64x2Ty,
                               llvm::CmpInst::Predicate::ICMP_SGT);
        break;
      case OpCode::I64x2__le_s:
        compileVectorCompareOp(Context.Int64x2Ty,
                               llvm::CmpInst::Predicate::ICMP_SLE);
        break;
      case OpCode::I64x2__ge_s:
        compileVectorCompareOp(Context.Int64x2Ty,
                               llvm::CmpInst::Predicate::ICMP_SGE);
        break;
      case OpCode::F32x4__eq:
        compileVectorCompareOp(Context.Floatx4Ty,
                               llvm::CmpInst::Predicate::FCMP_OEQ,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__ne:
        compileVectorCompareOp(Context.Floatx4Ty,
                               llvm::CmpInst::Predicate::FCMP_UNE,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__lt:
        compileVectorCompareOp(Context.Floatx4Ty,
                               llvm::CmpInst::Predicate::FCMP_OLT,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__gt:
        compileVectorCompareOp(Context.Floatx4Ty,
                               llvm::CmpInst::Predicate::FCMP_OGT,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__le:
        compileVectorCompareOp(Context.Floatx4Ty,
                               llvm::CmpInst::Predicate::FCMP_OLE,
                               Context.Int32x4Ty);
        break;
      case OpCode::F32x4__ge:
        compileVectorCompareOp(Context.Floatx4Ty,
                               llvm::CmpInst::Predicate::FCMP_OGE,
                               Context.Int32x4Ty);
        break;
      case OpCode::F64x2__eq:
        compileVectorCompareOp(Context.Doublex2Ty,
                               llvm::CmpInst::Predicate::FCMP_OEQ,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__ne:
        compileVectorCompareOp(Context.Doublex2Ty,
                               llvm::CmpInst::Predicate::FCMP_UNE,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__lt:
        compileVectorCompareOp(Context.Doublex2Ty,
                               llvm::CmpInst::Predicate::FCMP_OLT,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__gt:
        compileVectorCompareOp(Context.Doublex2Ty,
                               llvm::CmpInst::Predicate::FCMP_OGT,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__le:
        compileVectorCompareOp(Context.Doublex2Ty,
                               llvm::CmpInst::Predicate::FCMP_OLE,
                               Context.Int64x2Ty);
        break;
      case OpCode::F64x2__ge:
        compileVectorCompareOp(Context.Doublex2Ty,
                               llvm::CmpInst::Predicate::FCMP_OGE,
                               Context.Int64x2Ty);
        break;
      case OpCode::V128__not:
        Stack.back() = Builder.CreateNot(Stack.back());
        break;
      case OpCode::V128__and: {
        auto *RHS = stackPop();
        auto *LHS = stackPop();
        stackPush(Builder.CreateAnd(LHS, RHS));
        break;
      }
      case OpCode::V128__andnot: {
        auto *RHS = stackPop();
        auto *LHS = stackPop();
        stackPush(Builder.CreateAnd(LHS, Builder.CreateNot(RHS)));
        break;
      }
      case OpCode::V128__or: {
        auto *RHS = stackPop();
        auto *LHS = stackPop();
        stackPush(Builder.CreateOr(LHS, RHS));
        break;
      }
      case OpCode::V128__xor: {
        auto *RHS = stackPop();
        auto *LHS = stackPop();
        stackPush(Builder.CreateXor(LHS, RHS));
        break;
      }
      case OpCode::V128__bitselect: {
        auto *C = stackPop();
        auto *V2 = stackPop();
        auto *V1 = stackPop();
        stackPush(Builder.CreateXor(
            Builder.CreateAnd(Builder.CreateXor(V1, V2), C), V2));
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
        auto *ExtendTy =
            llvm::VectorType::getExtendedElementVectorType(Context.Int16x8Ty);
        auto *Undef = llvm::UndefValue::get(ExtendTy);
        auto *LHS = Builder.CreateSExt(
            Builder.CreateBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
        auto *RHS = Builder.CreateSExt(
            Builder.CreateBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
        auto *M = Builder.CreateMul(LHS, RHS);
        auto *L = Builder.CreateShuffleVector(
            M, Undef, std::initializer_list<ShuffleElement>{0, 2, 4, 6});
        auto *R = Builder.CreateShuffleVector(
            M, Undef, std::initializer_list<ShuffleElement>{1, 3, 5, 7});
        auto *V = Builder.CreateAdd(L, R);
        stackPush(Builder.CreateBitCast(V, Context.Int64x2Ty));
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
        compileVectorTruncSatS(Context.Floatx4Ty, 32, false);
        break;
      case OpCode::I32x4__trunc_sat_f32x4_u:
        compileVectorTruncSatU(Context.Floatx4Ty, 32, false);
        break;
      case OpCode::F32x4__convert_i32x4_s:
        compileVectorConvertS(Context.Int32x4Ty, Context.Floatx4Ty, false);
        break;
      case OpCode::F32x4__convert_i32x4_u:
        compileVectorConvertU(Context.Int32x4Ty, Context.Floatx4Ty, false);
        break;
      case OpCode::I32x4__trunc_sat_f64x2_s_zero:
        compileVectorTruncSatS(Context.Doublex2Ty, 32, true);
        break;
      case OpCode::I32x4__trunc_sat_f64x2_u_zero:
        compileVectorTruncSatU(Context.Doublex2Ty, 32, true);
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
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Add,
            Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__add:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Add,
            Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__add_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Add,
            Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__add_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Add,
            Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__add_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Add,
            Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__add_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Add,
            Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__add_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Add,
            Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__sub:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Sub,
            Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__sub:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Sub,
            Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__sub_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Sub,
            Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__sub_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Sub,
            Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__sub_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Sub,
            Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__sub_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Sub,
            Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__sub_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Sub,
            Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__and:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::And,
            Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__and:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::And,
            Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__and_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::And,
            Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__and_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::And,
            Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__and_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::And,
            Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__and_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::And,
            Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__and_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::And,
            Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__or:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Or,
            Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__or:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Or,
            Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__or_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Or,
            Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__or_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Or,
            Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__or_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Or,
            Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__or_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Or,
            Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__or_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Or,
            Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__xor:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xor,
            Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__xor:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xor,
            Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__xor_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xor,
            Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__xor_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xor,
            Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__xor_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xor,
            Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__xor_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xor,
            Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__xor_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xor,
            Context.Int64Ty, Context.Int32Ty);
      case OpCode::I32__atomic__rmw__xchg:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xchg,
            Context.Int32Ty, Context.Int32Ty, true);
      case OpCode::I64__atomic__rmw__xchg:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xchg,
            Context.Int64Ty, Context.Int64Ty, true);
      case OpCode::I32__atomic__rmw8__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xchg,
            Context.Int32Ty, Context.Int8Ty);
      case OpCode::I32__atomic__rmw16__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xchg,
            Context.Int32Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw8__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xchg,
            Context.Int64Ty, Context.Int8Ty);
      case OpCode::I64__atomic__rmw16__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xchg,
            Context.Int64Ty, Context.Int16Ty);
      case OpCode::I64__atomic__rmw32__xchg_u:
        return compileAtomicRMWOp(
            Instr.getTargetIndex(), Instr.getMemoryOffset(),
            Instr.getMemoryAlign(), llvm::AtomicRMWInst::BinOp::Xchg,
            Context.Int64Ty, Context.Int32Ty);
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
        Builder.CreateStore(
            Builder.CreateAdd(
                Builder.CreateLoad(Context.Int64Ty, LocalInstrCount),
                Builder.getInt64(1)),
            LocalInstrCount);
      }
      if (LocalGas) {
        auto *NewGas = Builder.CreateAdd(
            Builder.CreateLoad(Context.Int64Ty, LocalGas),
            Builder.CreateLoad(
                Context.Int64Ty,
                Builder.CreateConstInBoundsGEP2_64(
                    llvm::ArrayType::get(Context.Int64Ty, UINT16_MAX + 1),
                    Context.getCostTable(Builder, ExecCtx), 0,
                    uint16_t(Instr.getOpCode()))));
        Builder.CreateStore(NewGas, LocalGas);
      }

      // Make the instruction node according to Code.
      Dispatch(Instr);
    }
  }
  void compileSignedTrunc(llvm::IntegerType *IntType) {
    const auto MinInt = llvm::APInt::getSignedMinValue(IntType->getBitWidth());
    const auto MaxInt = llvm::APInt::getSignedMaxValue(IntType->getBitWidth());
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "strunc.norm", F);
    auto *NotMinBB = llvm::BasicBlock::Create(LLContext, "strunc.notmin", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "strunc.notmax", F);
    auto *Value = stackPop();
    auto *FPType = Value->getType();
    auto *MinFp = llvm::ConstantFP::get(FPType, MinInt.signedRoundToDouble());
    auto *MaxFp = llvm::ConstantFP::get(FPType, MaxInt.signedRoundToDouble());
    const bool Precise = IntType->getBitWidth() < FPType->getScalarSizeInBits();

    auto *IsNotNan = createLikely(Builder, Builder.CreateFCmpORD(Value, Value));
    Builder.CreateCondBr(IsNotNan, NormBB,
                         getTrapBB(ErrCode::Value::InvalidConvToInt));

    Builder.SetInsertPoint(NormBB);
    auto *Trunc = Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Value);
    auto *IsNotUnderflow =
        createLikely(Builder, Builder.CreateFCmpOGE(Trunc, MinFp));
    Builder.CreateCondBr(IsNotUnderflow, NotMinBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.SetInsertPoint(NotMinBB);
    auto *IsNotOverflow = createLikely(
        Builder,
        Builder.CreateFCmp(Precise ? llvm::CmpInst::Predicate::FCMP_OLE
                                   : llvm::CmpInst::Predicate::FCMP_OLT,
                           Trunc, MaxFp));
    Builder.CreateCondBr(IsNotOverflow, NotMaxBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.SetInsertPoint(NotMaxBB);
    stackPush(Builder.CreateFPToSI(Trunc, IntType));
  }
  void compileSignedTruncSat(llvm::IntegerType *IntType) {
    const auto MinInt = llvm::APInt::getSignedMinValue(IntType->getBitWidth());
    const auto MaxInt = llvm::APInt::getSignedMaxValue(IntType->getBitWidth());
    auto *CurrBB = Builder.GetInsertBlock();
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "ssat.norm", F);
    auto *NotMinBB = llvm::BasicBlock::Create(LLContext, "ssat.notmin", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "ssat.notmax", F);
    auto *EndBB = llvm::BasicBlock::Create(LLContext, "ssat.end", F);
    auto *Value = stackPop();
    auto *FPType = Value->getType();
    auto *MinFp = llvm::ConstantFP::get(FPType, MinInt.signedRoundToDouble());
    auto *MaxFp = llvm::ConstantFP::get(FPType, MaxInt.signedRoundToDouble());
    const bool Precise = IntType->getBitWidth() < FPType->getScalarSizeInBits();

    auto *IsNotNan = createLikely(Builder, Builder.CreateFCmpORD(Value, Value));
    Builder.CreateCondBr(IsNotNan, NormBB, EndBB);

    Builder.SetInsertPoint(NormBB);
    auto *Trunc = Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Value);
    auto *IsNotUnderflow =
        createLikely(Builder, Builder.CreateFCmpOGE(Trunc, MinFp));
    Builder.CreateCondBr(IsNotUnderflow, NotMinBB, EndBB);

    Builder.SetInsertPoint(NotMinBB);
    auto *IsNotOverflow = createLikely(
        Builder,
        Builder.CreateFCmp(Precise ? llvm::CmpInst::Predicate::FCMP_OLE
                                   : llvm::CmpInst::Predicate::FCMP_OLT,
                           Trunc, MaxFp));
    Builder.CreateCondBr(IsNotOverflow, NotMaxBB, EndBB);

    Builder.SetInsertPoint(NotMaxBB);
    auto *IntValue = Builder.CreateFPToSI(Trunc, IntType);
    Builder.CreateBr(EndBB);

    Builder.SetInsertPoint(EndBB);
    auto *PHIRet = Builder.CreatePHI(IntType, 4);
    PHIRet->addIncoming(llvm::ConstantInt::get(IntType, 0), CurrBB);
    PHIRet->addIncoming(llvm::ConstantInt::get(IntType, MinInt), NormBB);
    PHIRet->addIncoming(llvm::ConstantInt::get(IntType, MaxInt), NotMinBB);
    PHIRet->addIncoming(IntValue, NotMaxBB);

    stackPush(PHIRet);
  }
  void compileUnsignedTrunc(llvm::IntegerType *IntType) {
    const auto MinInt = llvm::APInt::getMinValue(IntType->getBitWidth());
    const auto MaxInt = llvm::APInt::getMaxValue(IntType->getBitWidth());
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "utrunc.norm", F);
    auto *NotMinBB = llvm::BasicBlock::Create(LLContext, "utrunc.notmin", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "utrunc.notmax", F);
    auto *Value = stackPop();
    auto *FPType = Value->getType();
    auto *MinFp = llvm::ConstantFP::get(FPType, MinInt.roundToDouble());
    auto *MaxFp = llvm::ConstantFP::get(FPType, MaxInt.roundToDouble());
    const bool Precise = IntType->getBitWidth() < FPType->getScalarSizeInBits();

    auto *IsNotNan = createLikely(Builder, Builder.CreateFCmpORD(Value, Value));
    Builder.CreateCondBr(IsNotNan, NormBB,
                         getTrapBB(ErrCode::Value::InvalidConvToInt));

    Builder.SetInsertPoint(NormBB);
    auto *Trunc = Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Value);
    auto *IsNotUnderflow =
        createLikely(Builder, Builder.CreateFCmpOGE(Trunc, MinFp));
    Builder.CreateCondBr(IsNotUnderflow, NotMinBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.SetInsertPoint(NotMinBB);
    auto *IsNotOverflow = createLikely(
        Builder,
        Builder.CreateFCmp(Precise ? llvm::CmpInst::Predicate::FCMP_OLE
                                   : llvm::CmpInst::Predicate::FCMP_OLT,
                           Trunc, MaxFp));
    Builder.CreateCondBr(IsNotOverflow, NotMaxBB,
                         getTrapBB(ErrCode::Value::IntegerOverflow));

    Builder.SetInsertPoint(NotMaxBB);
    stackPush(Builder.CreateFPToUI(Trunc, IntType));
  }
  void compileUnsignedTruncSat(llvm::IntegerType *IntType) {
    const auto MinInt = llvm::APInt::getMinValue(IntType->getBitWidth());
    const auto MaxInt = llvm::APInt::getMaxValue(IntType->getBitWidth());
    auto *CurrBB = Builder.GetInsertBlock();
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "usat.norm", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "usat.notmax", F);
    auto *EndBB = llvm::BasicBlock::Create(LLContext, "usat.end", F);
    auto *Value = stackPop();
    auto *FPType = Value->getType();
    auto *MinFp = llvm::ConstantFP::get(FPType, MinInt.roundToDouble());
    auto *MaxFp = llvm::ConstantFP::get(FPType, MaxInt.roundToDouble());
    const bool Precise = IntType->getBitWidth() < FPType->getScalarSizeInBits();

    auto *Trunc = Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Value);
    auto *IsNotUnderflow =
        createLikely(Builder, Builder.CreateFCmpOGE(Trunc, MinFp));
    Builder.CreateCondBr(IsNotUnderflow, NormBB, EndBB);

    Builder.SetInsertPoint(NormBB);
    auto *IsNotOverflow = createLikely(
        Builder,
        Builder.CreateFCmp(Precise ? llvm::CmpInst::Predicate::FCMP_OLE
                                   : llvm::CmpInst::Predicate::FCMP_OLT,
                           Trunc, MaxFp));
    Builder.CreateCondBr(IsNotOverflow, NotMaxBB, EndBB);

    Builder.SetInsertPoint(NotMaxBB);
    auto *IntValue = Builder.CreateFPToUI(Trunc, IntType);
    Builder.CreateBr(EndBB);

    Builder.SetInsertPoint(EndBB);
    auto *PHIRet = Builder.CreatePHI(IntType, 3);
    PHIRet->addIncoming(llvm::ConstantInt::get(IntType, MinInt), CurrBB);
    PHIRet->addIncoming(llvm::ConstantInt::get(IntType, MaxInt), NormBB);
    PHIRet->addIncoming(IntValue, NotMaxBB);

    stackPush(PHIRet);
  }

  void compileAtomicCheckOffsetAlignment(llvm::Value *Offset,
                                         llvm::IntegerType *IntType) {
    const auto BitWidth = IntType->getBitWidth();
    auto *BWMask = llvm::ConstantInt::get(Context.Int64Ty, (BitWidth >> 3) - 1);
    auto *Value = Builder.CreateAnd(Offset, BWMask);
    auto *OkBB = llvm::BasicBlock::Create(LLContext, "address_align_ok", F);
    auto *IsAddressAligned =
        createLikely(Builder, Builder.CreateICmpEQ(Value, Builder.getInt64(0)));
    Builder.CreateCondBr(IsAddressAligned, OkBB,
                         getTrapBB(ErrCode::Value::UnalignedAtomicAccess));

    Builder.SetInsertPoint(OkBB);
  }

  void compileMemoryFence() {
    Builder.CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
  }
  void compileAtomicNotify(unsigned MemoryIndex, unsigned MemoryOffset) {
    auto *Count = stackPop();
    auto *Addr = Builder.CreateZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Addr = Builder.CreateAdd(Addr, Builder.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Addr, Context.Int32Ty);
    auto *Offset = stackPop();

    stackPush(Builder.CreateCall(
        Context.getIntrinsic(
            Builder, AST::Module::Intrinsics::kMemoryAtomicNotify,
            llvm::FunctionType::get(
                Context.Int32Ty,
                {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
        {Builder.getInt32(MemoryIndex), Offset, Count}));
  }
  void compileAtomicWait(unsigned MemoryIndex, unsigned MemoryOffset,
                         llvm::IntegerType *TargetType, uint32_t BitWidth) {
    auto *Timeout = stackPop();
    auto *ExpectedValue =
        Builder.CreateZExtOrTrunc(stackPop(), Context.Int64Ty);
    auto *Addr = Builder.CreateZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Addr = Builder.CreateAdd(Addr, Builder.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Addr, TargetType);
    auto *Offset = stackPop();

    stackPush(Builder.CreateCall(
        Context.getIntrinsic(
            Builder, AST::Module::Intrinsics::kMemoryAtomicWait,
            llvm::FunctionType::get(Context.Int32Ty,
                                    {Context.Int32Ty, Context.Int32Ty,
                                     Context.Int64Ty, Context.Int64Ty,
                                     Context.Int32Ty},
                                    false)),
        {Builder.getInt32(MemoryIndex), Offset, ExpectedValue, Timeout,
         Builder.getInt32(BitWidth)}));
  }
  void compileAtomicLoad(unsigned MemoryIndex, unsigned MemoryOffset,
                         unsigned Alignment, llvm::IntegerType *IntType,
                         llvm::IntegerType *TargetType, bool Signed = false) {

    auto *Offset = Builder.CreateZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.CreateAdd(Offset, Builder.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto *VPtr = Builder.CreateInBoundsGEP(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);

    auto *Ptr = Builder.CreateBitCast(VPtr, TargetType->getPointerTo());
    auto *Load = Builder.CreateLoad(TargetType, Ptr, OptNone);
    Load->setAlignment(Align(UINT64_C(1) << Alignment));
    Load->setAtomic(llvm::AtomicOrdering::SequentiallyConsistent);

    if (Signed) {
      Stack.back() = Builder.CreateSExt(Load, IntType);
    } else {
      Stack.back() = Builder.CreateZExt(Load, IntType);
    }
  }
  void compileAtomicStore(unsigned MemoryIndex, unsigned MemoryOffset,
                          unsigned Alignment, llvm::IntegerType *,
                          llvm::IntegerType *TargetType, bool Signed = false) {

    auto *V = stackPop();

    if (Signed) {
      V = Builder.CreateSExtOrTrunc(V, TargetType);
    } else {
      V = Builder.CreateZExtOrTrunc(V, TargetType);
    }
    auto *Offset = Builder.CreateZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.CreateAdd(Offset, Builder.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto *VPtr = Builder.CreateInBoundsGEP(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);
    auto *Ptr = Builder.CreateBitCast(VPtr, TargetType->getPointerTo());
    auto *Store = Builder.CreateStore(V, Ptr, OptNone);
    Store->setAlignment(Align(UINT64_C(1) << Alignment));
    Store->setAtomic(llvm::AtomicOrdering::SequentiallyConsistent);
  }

  void compileAtomicRMWOp(unsigned MemoryIndex, unsigned MemoryOffset,
                          [[maybe_unused]] unsigned Alignment,
                          llvm::AtomicRMWInst::BinOp BinOp,
                          llvm::IntegerType *IntType,
                          llvm::IntegerType *TargetType, bool Signed = false) {
    auto *Value = Builder.CreateSExtOrTrunc(stackPop(), TargetType);
    auto *Offset = Builder.CreateZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.CreateAdd(Offset, Builder.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto *VPtr = Builder.CreateInBoundsGEP(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);
    auto *Ptr = Builder.CreateBitCast(VPtr, TargetType->getPointerTo());

    Stack.back() =
        Builder.CreateAtomicRMW(BinOp, Ptr, Value,
#if LLVM_VERSION_MAJOR >= 13
                                Align(UINT64_C(1) << Alignment),
#endif
                                llvm::AtomicOrdering::SequentiallyConsistent);

    if (Signed) {
      Stack.back() = Builder.CreateSExt(Stack.back(), IntType);
    } else {
      Stack.back() = Builder.CreateZExt(Stack.back(), IntType);
    }
  }
  void compileAtomicCompareExchange(unsigned MemoryIndex, unsigned MemoryOffset,
                                    [[maybe_unused]] unsigned Alignment,
                                    llvm::IntegerType *IntType,
                                    llvm::IntegerType *TargetType,
                                    bool Signed = false) {

    auto *Replacement = Builder.CreateSExtOrTrunc(stackPop(), TargetType);
    auto *Expected = Builder.CreateSExtOrTrunc(stackPop(), TargetType);
    auto *Offset = Builder.CreateZExt(Stack.back(), Context.Int64Ty);
    if (MemoryOffset != 0) {
      Offset = Builder.CreateAdd(Offset, Builder.getInt64(MemoryOffset));
    }
    compileAtomicCheckOffsetAlignment(Offset, TargetType);
    auto *VPtr = Builder.CreateInBoundsGEP(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex),
        Offset);
    auto *Ptr = Builder.CreateBitCast(VPtr, TargetType->getPointerTo());

    auto *Ret = Builder.CreateAtomicCmpXchg(
        Ptr, Expected, Replacement,
#if LLVM_VERSION_MAJOR >= 13
        Align(UINT64_C(1) << Alignment),
#endif
        llvm::AtomicOrdering::SequentiallyConsistent,
        llvm::AtomicOrdering::SequentiallyConsistent);

    auto *OldVal = Builder.CreateExtractValue(Ret, 0);
    Stack.back() = OldVal;
    if (Signed) {
      Stack.back() = Builder.CreateSExt(Stack.back(), IntType);
    } else {
      Stack.back() = Builder.CreateZExt(Stack.back(), IntType);
    }
  }

  void compileReturn() {
    updateInstrCount();
    updateGas();
    auto *Ty = F->getReturnType();
    if (Ty->isVoidTy()) {
      Builder.CreateRetVoid();
    } else if (Ty->isStructTy()) {
      const auto Count = Ty->getStructNumElements();
      std::vector<llvm::Value *> Ret(Count);
      for (unsigned I = 0; I < Count; ++I) {
        const unsigned J = Count - 1 - I;
        Ret[J] = stackPop();
      }
      Builder.CreateAggregateRet(Ret.data(), Count);
    } else {
      Builder.CreateRet(stackPop());
    }
  }

  void updateInstrCount() {
    if (LocalInstrCount) {
      Builder.CreateAtomicRMW(
          llvm::AtomicRMWInst::BinOp::Add,
          Context.getInstrCount(Builder, ExecCtx),
          Builder.CreateLoad(Context.Int64Ty, LocalInstrCount),
#if LLVM_VERSION_MAJOR >= 13
          llvm::MaybeAlign(8),
#endif
          llvm::AtomicOrdering::Monotonic);
      Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
    }
  }

  void updateGas() {
    if (LocalGas) {
      auto *CurrBB = Builder.GetInsertBlock();
      auto *CheckBB = llvm::BasicBlock::Create(LLContext, "gas_check", F);
      auto *OkBB = llvm::BasicBlock::Create(LLContext, "gas_ok", F);
      auto *EndBB = llvm::BasicBlock::Create(LLContext, "gas_end", F);

      auto *Cost = Builder.CreateLoad(Context.Int64Ty, LocalGas);
      Cost->setAlignment(Align(8));
      auto *GasPtr = Context.getGas(Builder, ExecCtx);
      auto *GasLimit = Context.getGasLimit(Builder, ExecCtx);
      auto *Gas = Builder.CreateLoad(Context.Int64Ty, GasPtr);
      Gas->setAlignment(Align(8));
      Gas->setAtomic(llvm::AtomicOrdering::Monotonic);
      Builder.CreateBr(CheckBB);
      Builder.SetInsertPoint(CheckBB);

      auto *PHIOldGas = Builder.CreatePHI(Context.Int64Ty, 2);
      auto *NewGas = Builder.CreateAdd(PHIOldGas, Cost);
      auto *IsGasRemain =
          createLikely(Builder, Builder.CreateICmpULE(NewGas, GasLimit));
      Builder.CreateCondBr(IsGasRemain, OkBB,
                           getTrapBB(ErrCode::Value::CostLimitExceeded));
      Builder.SetInsertPoint(OkBB);

      auto *RGasAndSucceed = Builder.CreateAtomicCmpXchg(
          GasPtr, PHIOldGas, NewGas,
#if LLVM_VERSION_MAJOR >= 13
          llvm::MaybeAlign(8),
#endif
          llvm::AtomicOrdering::Monotonic, llvm::AtomicOrdering::Monotonic);
      RGasAndSucceed->setWeak(true);
      auto *RGas = Builder.CreateExtractValue(RGasAndSucceed, {0});
      auto *Succeed = Builder.CreateExtractValue(RGasAndSucceed, {1});
      Builder.CreateCondBr(createLikely(Builder, Succeed), EndBB, CheckBB);
      Builder.SetInsertPoint(EndBB);

      Builder.CreateStore(Builder.getInt64(0), LocalGas);

      PHIOldGas->addIncoming(Gas, CurrBB);
      PHIOldGas->addIncoming(RGas, OkBB);
    }
  }

  void updateGasAtTrap() {
    if (LocalGas) {
      Builder.CreateAtomicRMW(llvm::AtomicRMWInst::BinOp::Add,
                              Context.getGas(Builder, ExecCtx),
                              Builder.CreateLoad(Context.Int64Ty, LocalGas),
#if LLVM_VERSION_MAJOR >= 13
                              llvm::MaybeAlign(8),
#endif
                              llvm::AtomicOrdering::Monotonic);
    }
  }

private:
  void compileCallOp(const unsigned int FuncIndex) {
    const auto &FuncType =
        *Context.FunctionTypes[std::get<0>(Context.Functions[FuncIndex])];
    const auto &Function = std::get<1>(Context.Functions[FuncIndex]);
    const auto &ParamTypes = FuncType.getParamTypes();

    std::vector<llvm::Value *> Args(ParamTypes.size() + 1);
    Args[0] = F->arg_begin();
    for (size_t I = 0; I < ParamTypes.size(); ++I) {
      const size_t J = ParamTypes.size() - 1 - I;
      Args[J + 1] = stackPop();
    }

    auto *Ret = Builder.CreateCall(Function, Args);
    auto *Ty = Ret->getType();
    if (Ty->isVoidTy()) {
      // nothing to do
    } else if (Ty->isStructTy()) {
      for (auto *Val : unpackStruct(Builder, Ret)) {
        stackPush(Val);
      }
    } else {
      stackPush(Ret);
    }
  }

  void compileIndirectCallOp(const uint32_t TableIndex,
                             const uint32_t FuncTypeIndex) {
    auto *NotNullBB = llvm::BasicBlock::Create(LLContext, "c_i.not_null", F);
    auto *IsNullBB = llvm::BasicBlock::Create(LLContext, "c_i.is_null", F);
    auto *EndBB = llvm::BasicBlock::Create(LLContext, "c_i.end", F);

    llvm::Value *FuncIndex = stackPop();
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    auto *FTy = toLLVMType(Context.ExecCtxPtrTy, FuncType);
    auto *RTy = FTy->getReturnType();

    const size_t ArgSize = FuncType.getParamTypes().size();
    const size_t RetSize =
        RTy->isVoidTy() ? 0 : FuncType.getReturnTypes().size();
    std::vector<llvm::Value *> ArgsVec(ArgSize + 1, nullptr);
    ArgsVec[0] = F->arg_begin();
    for (size_t I = 0; I < ArgSize; ++I) {
      const size_t J = ArgSize - I;
      ArgsVec[J] = stackPop();
    }

    std::vector<llvm::Value *> FPtrRetsVec;
    FPtrRetsVec.reserve(RetSize);
    {
      auto *FPtr = Builder.CreateCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kPtrFunc,
              llvm::FunctionType::get(
                  FTy->getPointerTo(),
                  {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
          {Builder.getInt32(TableIndex), Builder.getInt32(FuncTypeIndex),
           FuncIndex});
      Builder.CreateCondBr(
          createLikely(Builder, Builder.CreateNot(Builder.CreateIsNull(FPtr))),
          NotNullBB, IsNullBB);
      Builder.SetInsertPoint(NotNullBB);

      auto *FPtrRet =
          Builder.CreateCall(llvm::FunctionCallee(FTy, FPtr), ArgsVec);
      if (RetSize == 0) {
        // nothing to do
      } else if (RetSize == 1) {
        FPtrRetsVec.push_back(FPtrRet);
      } else {
        for (auto *Val : unpackStruct(Builder, FPtrRet)) {
          FPtrRetsVec.push_back(Val);
        }
      }
    }

    Builder.CreateBr(EndBB);
    Builder.SetInsertPoint(IsNullBB);

    std::vector<llvm::Value *> RetsVec(RetSize);
    {
      llvm::Value *Args;
      if (ArgSize == 0) {
        Args = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
      } else {
        auto *Alloca = Builder.CreateAlloca(
            Builder.getInt8Ty(), Builder.getInt64(ArgSize * kValSize));
        Alloca->setAlignment(Align(kValSize));
        Args = Alloca;
      }

      llvm::Value *Rets;
      if (RetSize == 0) {
        Rets = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
      } else {
        auto *Alloca = Builder.CreateAlloca(
            Builder.getInt8Ty(), Builder.getInt64(RetSize * kValSize));
        Alloca->setAlignment(Align(kValSize));
        Rets = Alloca;
      }

      for (size_t I = 0; I < ArgSize; ++I) {
        auto *Ptr = Builder.CreateConstInBoundsGEP1_64(Context.Int8Ty, Args,
                                                       I * kValSize);
        auto *Arg = ArgsVec[I + 1];
        Builder.CreateStore(
            Arg, Builder.CreateBitCast(Ptr, Arg->getType()->getPointerTo()));
      }

      Builder.CreateCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kCallIndirect,
              llvm::FunctionType::get(Context.VoidTy,
                                      {Context.Int32Ty, Context.Int32Ty,
                                       Context.Int32Ty, Context.Int8PtrTy,
                                       Context.Int8PtrTy},
                                      false)),
          {Builder.getInt32(TableIndex), Builder.getInt32(FuncTypeIndex),
           FuncIndex, Args, Rets});

      if (RetSize == 0) {
        // nothing to do
      } else if (RetSize == 1) {
        auto *VPtr =
            Builder.CreateConstInBoundsGEP1_64(Context.Int8Ty, Rets, 0);
        auto *Ptr = Builder.CreateBitCast(VPtr, RTy->getPointerTo());
        RetsVec[0] = Builder.CreateLoad(RTy, Ptr);
      } else {
        for (unsigned I = 0; I < RetSize; ++I) {
          auto *VPtr = Builder.CreateConstInBoundsGEP1_64(Context.Int8Ty, Rets,
                                                          I * kValSize);
          auto *Ptr = Builder.CreateBitCast(
              VPtr, RTy->getStructElementType(I)->getPointerTo());
          RetsVec[I] = Builder.CreateLoad(RTy->getStructElementType(I), Ptr);
        }
      }
      Builder.CreateBr(EndBB);
      Builder.SetInsertPoint(EndBB);
    }

    for (unsigned I = 0; I < RetSize; ++I) {
      auto *PHIRet = Builder.CreatePHI(FPtrRetsVec[I]->getType(), 2);
      PHIRet->addIncoming(FPtrRetsVec[I], NotNullBB);
      PHIRet->addIncoming(RetsVec[I], IsNullBB);
      stackPush(PHIRet);
    }
  }

  void compileReturnCallOp(const unsigned int FuncIndex) {
    const auto &FuncType =
        *Context.FunctionTypes[std::get<0>(Context.Functions[FuncIndex])];
    const auto &Function = std::get<1>(Context.Functions[FuncIndex]);
    const auto &ParamTypes = FuncType.getParamTypes();

    std::vector<llvm::Value *> Args(ParamTypes.size() + 1);
    Args[0] = F->arg_begin();
    for (size_t I = 0; I < ParamTypes.size(); ++I) {
      const size_t J = ParamTypes.size() - 1 - I;
      Args[J + 1] = stackPop();
    }

    auto *Ret = Builder.CreateCall(Function, Args);
    auto *Ty = Ret->getType();
    if (Ty->isVoidTy()) {
      Builder.CreateRetVoid();
    } else {
      Builder.CreateRet(Ret);
    }
  }

  void compileReturnIndirectCallOp(const uint32_t TableIndex,
                                   const uint32_t FuncTypeIndex) {
    auto *NotNullBB = llvm::BasicBlock::Create(LLContext, "c_i.not_null", F);
    auto *IsNullBB = llvm::BasicBlock::Create(LLContext, "c_i.is_null", F);

    llvm::Value *FuncIndex = stackPop();
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    auto *FTy = toLLVMType(Context.ExecCtxPtrTy, FuncType);
    auto *RTy = FTy->getReturnType();

    const size_t ArgSize = FuncType.getParamTypes().size();
    const size_t RetSize =
        RTy->isVoidTy() ? 0 : FuncType.getReturnTypes().size();
    std::vector<llvm::Value *> ArgsVec(ArgSize + 1, nullptr);
    ArgsVec[0] = F->arg_begin();
    for (size_t I = 0; I < ArgSize; ++I) {
      const size_t J = ArgSize - I;
      ArgsVec[J] = stackPop();
    }

    {
      auto *FPtr = Builder.CreateCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kPtrFunc,
              llvm::FunctionType::get(
                  FTy->getPointerTo(),
                  {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
          {Builder.getInt32(TableIndex), Builder.getInt32(FuncTypeIndex),
           FuncIndex});
      Builder.CreateCondBr(
          createLikely(Builder, Builder.CreateNot(Builder.CreateIsNull(FPtr))),
          NotNullBB, IsNullBB);
      Builder.SetInsertPoint(NotNullBB);

      auto *FPtrRet =
          Builder.CreateCall(llvm::FunctionCallee(FTy, FPtr), ArgsVec);
      if (RetSize == 0) {
        Builder.CreateRetVoid();
      } else {
        Builder.CreateRet(FPtrRet);
      }
    }

    Builder.SetInsertPoint(IsNullBB);

    {
      llvm::Value *Args;
      if (ArgSize == 0) {
        Args = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
      } else {
        auto *Alloca = Builder.CreateAlloca(
            Builder.getInt8Ty(), Builder.getInt64(ArgSize * kValSize));
        Alloca->setAlignment(Align(kValSize));
        Args = Alloca;
      }

      llvm::Value *Rets;
      if (RetSize == 0) {
        Rets = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
      } else {
        auto *Alloca = Builder.CreateAlloca(
            Builder.getInt8Ty(), Builder.getInt64(RetSize * kValSize));
        Alloca->setAlignment(Align(kValSize));
        Rets = Alloca;
      }

      for (size_t I = 0; I < ArgSize; ++I) {
        auto *Ptr = Builder.CreateConstInBoundsGEP1_64(Context.Int8Ty, Args,
                                                       I * kValSize);
        auto *Arg = ArgsVec[I + 1];
        Builder.CreateStore(
            Arg, Builder.CreateBitCast(Ptr, Arg->getType()->getPointerTo()));
      }

      Builder.CreateCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kCallIndirect,
              llvm::FunctionType::get(Context.VoidTy,
                                      {Context.Int32Ty, Context.Int32Ty,
                                       Context.Int32Ty, Context.Int8PtrTy,
                                       Context.Int8PtrTy},
                                      false)),
          {Builder.getInt32(TableIndex), Builder.getInt32(FuncTypeIndex),
           FuncIndex, Args, Rets});

      if (RetSize == 0) {
        Builder.CreateRetVoid();
      } else if (RetSize == 1) {
        auto *VPtr =
            Builder.CreateConstInBoundsGEP1_64(Context.Int8Ty, Rets, 0);
        auto *Ptr = Builder.CreateBitCast(VPtr, RTy->getPointerTo());
        Builder.CreateRet(Builder.CreateLoad(RTy, Ptr));
      } else {
        std::vector<llvm::Value *> Ret(RetSize);
        for (unsigned I = 0; I < RetSize; ++I) {
          auto *VPtr = Builder.CreateConstInBoundsGEP1_64(Context.Int8Ty, Rets,
                                                          I * kValSize);
          auto *Ptr = Builder.CreateBitCast(
              VPtr, RTy->getStructElementType(I)->getPointerTo());
          Ret[I] = Builder.CreateLoad(RTy->getStructElementType(I), Ptr);
        }
        Builder.CreateAggregateRet(Ret.data(), static_cast<uint32_t>(RetSize));
      }
    }
  }

  void compileLoadOp(unsigned MemoryIndex, unsigned Offset, unsigned Alignment,
                     llvm::Type *LoadTy) {
    if constexpr (kForceUnalignment) {
      Alignment = 0;
    }
    auto *Off = Builder.CreateZExt(stackPop(), Context.Int64Ty);
    if (Offset != 0) {
      Off = Builder.CreateAdd(Off, Builder.getInt64(Offset));
    }

    auto *VPtr = Builder.CreateInBoundsGEP(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Off);
    auto *Ptr = Builder.CreateBitCast(VPtr, LoadTy->getPointerTo());
    auto *LoadInst = Builder.CreateLoad(LoadTy, Ptr, OptNone);
    LoadInst->setAlignment(Align(UINT64_C(1) << Alignment));
    stackPush(LoadInst);
  }
  void compileLoadOp(unsigned MemoryIndex, unsigned Offset, unsigned Alignment,
                     llvm::Type *LoadTy, llvm::Type *ExtendTy, bool Signed) {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    if (Signed) {
      Stack.back() = Builder.CreateSExt(Stack.back(), ExtendTy);
    } else {
      Stack.back() = Builder.CreateZExt(Stack.back(), ExtendTy);
    }
  }
  void compileVectorLoadOp(unsigned MemoryIndex, unsigned Offset,
                           unsigned Alignment, llvm::Type *LoadTy) {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    Stack.back() = Builder.CreateBitCast(Stack.back(), Context.Int64x2Ty);
  }
  void compileVectorLoadOp(unsigned MemoryIndex, unsigned Offset,
                           unsigned Alignment, llvm::Type *LoadTy,
                           llvm::Type *ExtendTy, bool Signed) {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy, ExtendTy, Signed);
    Stack.back() = Builder.CreateBitCast(Stack.back(), Context.Int64x2Ty);
  }
  void compileSplatLoadOp(unsigned MemoryIndex, unsigned Offset,
                          unsigned Alignment, llvm::Type *LoadTy,
                          llvm::VectorType *VectorTy) {
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    compileSplatOp(VectorTy);
  }
  void compileLoadLaneOp(unsigned MemoryIndex, unsigned Offset,
                         unsigned Alignment, unsigned Index, llvm::Type *LoadTy,
                         llvm::VectorType *VectorTy) {
    auto *Vector = stackPop();
    compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
    auto *Value = Stack.back();
    Stack.back() = Builder.CreateBitCast(
        Builder.CreateInsertElement(Builder.CreateBitCast(Vector, VectorTy),
                                    Value, Index),
        Context.Int64x2Ty);
  }
  void compileStoreLaneOp(unsigned MemoryIndex, unsigned Offset,
                          unsigned Alignment, unsigned Index,
                          llvm::Type *LoadTy, llvm::VectorType *VectorTy) {
    auto *Vector = Stack.back();
    Stack.back() = Builder.CreateExtractElement(
        Builder.CreateBitCast(Vector, VectorTy), Index);
    compileStoreOp(MemoryIndex, Offset, Alignment, LoadTy);
  }
  void compileStoreOp(unsigned MemoryIndex, unsigned Offset, unsigned Alignment,
                      llvm::Type *LoadTy, bool Trunc = false,
                      bool BitCast = false) {
    if constexpr (kForceUnalignment) {
      Alignment = 0;
    }
    auto *V = stackPop();
    auto *Off = Builder.CreateZExt(stackPop(), Context.Int64Ty);
    if (Offset != 0) {
      Off = Builder.CreateAdd(Off, Builder.getInt64(Offset));
    }

    if (Trunc) {
      V = Builder.CreateTrunc(V, LoadTy);
    }
    if (BitCast) {
      V = Builder.CreateBitCast(V, LoadTy);
    }
    auto *VPtr = Builder.CreateInBoundsGEP(
        Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Off);
    auto *Ptr = Builder.CreateBitCast(VPtr, LoadTy->getPointerTo());
    auto *StoreInst = Builder.CreateStore(V, Ptr, OptNone);
    StoreInst->setAlignment(Align(UINT64_C(1) << Alignment));
  }
  void compileSplatOp(llvm::VectorType *VectorTy) {
    auto *Undef = llvm::UndefValue::get(VectorTy);
    auto *Zeros = llvm::ConstantAggregateZero::get(
        llvm::VectorType::get(Context.Int32Ty, elementCount(VectorTy), false));
    auto *Value = Builder.CreateTrunc(Stack.back(), VectorTy->getElementType());
    auto *Vector =
        Builder.CreateInsertElement(Undef, Value, Builder.getInt64(0));
    Vector = Builder.CreateShuffleVector(Vector, Undef, Zeros);

    Stack.back() = Builder.CreateBitCast(Vector, Context.Int64x2Ty);
  }
  void compileExtractLaneOp(llvm::VectorType *VectorTy, unsigned Index) {
    auto *Vector = Builder.CreateBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.CreateExtractElement(Vector, Index);
  }
  void compileExtractLaneOp(llvm::VectorType *VectorTy, unsigned Index,
                            llvm::Type *ExtendTy, bool Signed) {
    compileExtractLaneOp(VectorTy, Index);
    if (Signed) {
      Stack.back() = Builder.CreateSExt(Stack.back(), ExtendTy);
    } else {
      Stack.back() = Builder.CreateZExt(Stack.back(), ExtendTy);
    }
  }
  void compileReplaceLaneOp(llvm::VectorType *VectorTy, unsigned Index) {
    auto *Value = Builder.CreateTrunc(stackPop(), VectorTy->getElementType());
    auto *Vector = Stack.back();
    Stack.back() = Builder.CreateBitCast(
        Builder.CreateInsertElement(Builder.CreateBitCast(Vector, VectorTy),
                                    Value, Index),
        Context.Int64x2Ty);
  }
  void compileVectorCompareOp(llvm::VectorType *VectorTy,
                              llvm::CmpInst::Predicate Predicate) {
    auto *RHS = stackPop();
    auto *LHS = stackPop();
    auto *Result = Builder.CreateSExt(
        Builder.CreateICmp(Predicate, Builder.CreateBitCast(LHS, VectorTy),
                           Builder.CreateBitCast(RHS, VectorTy)),
        VectorTy);
    stackPush(Builder.CreateBitCast(Result, Context.Int64x2Ty));
  }
  void compileVectorCompareOp(llvm::VectorType *VectorTy,
                              llvm::CmpInst::Predicate Predicate,
                              llvm::VectorType *ResultTy) {
    auto *RHS = stackPop();
    auto *LHS = stackPop();
    auto *Result = Builder.CreateSExt(
        Builder.CreateFCmp(Predicate, Builder.CreateBitCast(LHS, VectorTy),
                           Builder.CreateBitCast(RHS, VectorTy)),
        ResultTy);
    stackPush(Builder.CreateBitCast(Result, Context.Int64x2Ty));
  }
  template <typename Func>
  void compileVectorOp(llvm::VectorType *VectorTy, Func &&Op) {
    auto *V = Builder.CreateBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.CreateBitCast(Op(V), Context.Int64x2Ty);
  }
  void compileVectorAbs(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [this, VectorTy](auto *V) {
      auto *Zero = llvm::ConstantAggregateZero::get(VectorTy);
      auto *C = Builder.CreateICmpSLT(V, Zero);
      return Builder.CreateSelect(C, Builder.CreateNeg(V), V);
    });
  }
  void compileVectorNeg(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [this](auto *V) { return Builder.CreateNeg(V); });
  }
  void compileVectorPopcnt() {
    compileVectorOp(Context.Int8x16Ty, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, V);
    });
  }
  template <typename Func>
  void compileVectorReduceIOp(llvm::VectorType *VectorTy, Func &&Op) {
    auto *V = Builder.CreateBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.CreateZExt(Op(V), Context.Int32Ty);
  }
  void compileVectorAnyTrue() {
    compileVectorReduceIOp(Context.Int128x1Ty, [this](auto *V) {
      auto *Zero = llvm::ConstantAggregateZero::get(Context.Int128x1Ty);
      return Builder.CreateBitCast(Builder.CreateICmpNE(V, Zero),
                                   Builder.getInt1Ty());
    });
  }
  void compileVectorAllTrue(llvm::VectorType *VectorTy) {
    compileVectorReduceIOp(VectorTy, [this, VectorTy](auto *V) {
      const auto Size = elementCount(VectorTy);
      auto *IntType = Builder.getIntNTy(Size);
      auto *Zero = llvm::ConstantAggregateZero::get(VectorTy);
      auto *Cmp = Builder.CreateBitCast(Builder.CreateICmpEQ(V, Zero), IntType);
      auto *CmpZero = llvm::ConstantInt::get(IntType, 0);
      return Builder.CreateICmpEQ(Cmp, CmpZero);
    });
  }
  void compileVectorBitMask(llvm::VectorType *VectorTy) {
    compileVectorReduceIOp(VectorTy, [this, VectorTy](auto *V) {
      const auto Size = elementCount(VectorTy);
      auto *IntType = Builder.getIntNTy(Size);
      auto *Zero = llvm::ConstantAggregateZero::get(VectorTy);
      return Builder.CreateBitCast(Builder.CreateICmpSLT(V, Zero), IntType);
    });
  }
  template <typename Func>
  void compileVectorShiftOp(llvm::VectorType *VectorTy, Func &&Op) {
    const uint32_t Mask = VectorTy->getElementType()->getIntegerBitWidth() - 1;
    auto *N = Builder.CreateAnd(stackPop(), Builder.getInt32(Mask));
    auto *RHS = Builder.CreateVectorSplat(
        elementCount(VectorTy),
        Builder.CreateZExtOrTrunc(N, VectorTy->getElementType()));
    auto *LHS = Builder.CreateBitCast(stackPop(), VectorTy);
    stackPush(Builder.CreateBitCast(Op(LHS, RHS), Context.Int64x2Ty));
  }
  void compileVectorShl(llvm::VectorType *VectorTy) {
    compileVectorShiftOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateShl(LHS, RHS);
    });
  }
  void compileVectorLShr(llvm::VectorType *VectorTy) {
    compileVectorShiftOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateLShr(LHS, RHS);
    });
  }
  void compileVectorAShr(llvm::VectorType *VectorTy) {
    compileVectorShiftOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateAShr(LHS, RHS);
    });
  }
  template <typename Func>
  void compileVectorVectorOp(llvm::VectorType *VectorTy, Func &&Op) {
    auto *RHS = Builder.CreateBitCast(stackPop(), VectorTy);
    auto *LHS = Builder.CreateBitCast(stackPop(), VectorTy);
    stackPush(Builder.CreateBitCast(Op(LHS, RHS), Context.Int64x2Ty));
  }
  void compileVectorVectorAdd(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateAdd(LHS, RHS);
    });
  }
  void compileVectorVectorAddSat(llvm::VectorType *VectorTy, bool Signed) {
    auto ID = Signed ? llvm::Intrinsic::sadd_sat : llvm::Intrinsic::uadd_sat;
    compileVectorVectorOp(VectorTy, [this, ID](auto *LHS, auto *RHS) {
      return Builder.CreateBinaryIntrinsic(ID, LHS, RHS);
    });
  }
  void compileVectorVectorSub(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateSub(LHS, RHS);
    });
  }
  void compileVectorVectorSubSat(llvm::VectorType *VectorTy, bool Signed) {
    auto ID = Signed ? llvm::Intrinsic::ssub_sat : llvm::Intrinsic::usub_sat;
    compileVectorVectorOp(VectorTy, [this, ID](auto *LHS, auto *RHS) {
      return Builder.CreateBinaryIntrinsic(ID, LHS, RHS);
    });
  }
  void compileVectorVectorMul(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateMul(LHS, RHS);
    });
  }
  void compileVectorVectorQ15MulSat() {
    compileVectorVectorOp(
        Context.Int16x8Ty, [this](auto *LHS, auto *RHS) -> llvm::Value * {
#if defined(__x86_64__)
          if (Context.SupportSSSE3) {
            auto *Result = Builder.CreateIntrinsic(
                llvm::Intrinsic::x86_ssse3_pmul_hr_sw_128, {}, {LHS, RHS});
            auto *IntMaxV = Builder.CreateVectorSplat(
                8, Builder.getInt16(UINT16_C(0x8000)));
            auto *NotOver = Builder.CreateSExt(
                Builder.CreateICmpEQ(Result, IntMaxV), Context.Int16x8Ty);
            return Builder.CreateXor(Result, NotOver);
          }
#endif

#if defined(__aarch64__)
          if (Context.SupportNEON) {
            return Builder.CreateBinaryIntrinsic(
                llvm::Intrinsic::aarch64_neon_sqrdmulh, LHS, RHS);
          }
#endif

          // Fallback case.
          // If the SSSE3 is not supported on the x86_64 platform or
          // the NEON is not supported on the aarch64 platform,
          // then fallback to this.
          auto *ExtTy =
              llvm::VectorType::getExtendedElementVectorType(Context.Int16x8Ty);
          auto *Offset =
              Builder.CreateVectorSplat(8, Builder.getInt32(UINT32_C(0x4000)));
          auto *Shift =
              Builder.CreateVectorSplat(8, Builder.getInt32(UINT32_C(15)));
          auto *ExtLHS = Builder.CreateSExt(LHS, ExtTy);
          auto *ExtRHS = Builder.CreateSExt(RHS, ExtTy);
          auto *Result = Builder.CreateTrunc(
              Builder.CreateAShr(
                  Builder.CreateAdd(Builder.CreateMul(ExtLHS, ExtRHS), Offset),
                  Shift),
              Context.Int16x8Ty);
          auto *IntMaxV =
              Builder.CreateVectorSplat(8, Builder.getInt16(UINT16_C(0x8000)));
          auto *NotOver = Builder.CreateSExt(
              Builder.CreateICmpEQ(Result, IntMaxV), Context.Int16x8Ty);
          return Builder.CreateXor(Result, NotOver);
        });
  }
  void compileVectorVectorSMin(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpSLE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorUMin(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpULE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorSMax(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpSGE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorUMax(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpUGE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  void compileVectorVectorUAvgr(llvm::VectorType *VectorTy) {
    auto *ExtendTy = VectorTy->getExtendedElementVectorType(VectorTy);
    compileVectorVectorOp(
        VectorTy,
        [this, VectorTy, ExtendTy](auto *LHS, auto *RHS) -> llvm::Value * {
#if defined(__x86_64__)
          if (Context.SupportSSE2) {
            const auto ID =
                VectorTy->getElementType()->getIntegerBitWidth() == 8
                    ? llvm::Intrinsic::x86_sse2_pavg_b
                    : llvm::Intrinsic::x86_sse2_pavg_w;
            return Builder.CreateIntrinsic(ID, {}, {LHS, RHS});
          }
#endif

#if defined(__aarch64__)
          if (Context.SupportNEON) {
            return Builder.CreateBinaryIntrinsic(
                llvm::Intrinsic::aarch64_neon_urhadd, LHS, RHS);
          }
#endif

          // Fallback case.
          // If the SSE2 is not supported on the x86_64 platform or
          // the NEON is not supported on the aarch64 platform,
          // then fallback to this.
          auto *EL = Builder.CreateZExt(LHS, ExtendTy);
          auto *ER = Builder.CreateZExt(RHS, ExtendTy);
          auto *One =
              Builder.CreateZExt(Builder.CreateVectorSplat(
                                     elementCount(ExtendTy), Builder.getTrue()),
                                 ExtendTy);
          return Builder.CreateTrunc(
              Builder.CreateLShr(
                  Builder.CreateAdd(Builder.CreateAdd(EL, ER), One), One),
              VectorTy);
        });
  }
  void compileVectorNarrow(llvm::VectorType *FromTy, bool Signed) {
    const auto IntWidth = FromTy->getElementType()->getIntegerBitWidth();
    auto MinInt = Signed ? llvm::APInt::getSignedMinValue(IntWidth / 2)
                         : llvm::APInt::getMinValue(IntWidth / 2);
    MinInt = Signed ? MinInt.sext(IntWidth) : MinInt.zext(IntWidth);
    auto MaxInt = Signed ? llvm::APInt::getSignedMaxValue(IntWidth / 2)
                         : llvm::APInt::getMaxValue(IntWidth / 2);
    MaxInt = Signed ? MaxInt.sext(IntWidth) : MaxInt.zext(IntWidth);

    const auto Count = elementCount(FromTy);
    auto *VMin = Builder.CreateVectorSplat(Count, Builder.getInt(MinInt));
    auto *VMax = Builder.CreateVectorSplat(Count, Builder.getInt(MaxInt));

    auto *TruncTy = llvm::VectorType::getTruncatedElementVectorType(FromTy);

    auto *F2 = Builder.CreateBitCast(stackPop(), FromTy);
    F2 = Builder.CreateSelect(Builder.CreateICmpSLT(F2, VMin), VMin, F2);
    F2 = Builder.CreateSelect(Builder.CreateICmpSGT(F2, VMax), VMax, F2);
    F2 = Builder.CreateTrunc(F2, TruncTy);

    auto *F1 = Builder.CreateBitCast(stackPop(), FromTy);
    F1 = Builder.CreateSelect(Builder.CreateICmpSLT(F1, VMin), VMin, F1);
    F1 = Builder.CreateSelect(Builder.CreateICmpSGT(F1, VMax), VMax, F1);
    F1 = Builder.CreateTrunc(F1, TruncTy);

    std::vector<ShuffleElement> Mask(Count * 2);
    std::iota(Mask.begin(), Mask.end(), 0);
    stackPush(Builder.CreateBitCast(Builder.CreateShuffleVector(F1, F2, Mask),
                                    Context.Int64x2Ty));
  }
  void compileVectorExtend(llvm::VectorType *FromTy, bool Signed, bool Low) {
    auto *ExtTy = llvm::VectorType::getExtendedElementVectorType(FromTy);
    const auto Count = elementCount(FromTy);
    std::vector<ShuffleElement> Mask(Count / 2);
    std::iota(Mask.begin(), Mask.end(), Low ? 0 : Count / 2);
    auto *R = Builder.CreateBitCast(Stack.back(), FromTy);
    if (Signed) {
      R = Builder.CreateSExt(R, ExtTy);
    } else {
      R = Builder.CreateZExt(R, ExtTy);
    }
    R = Builder.CreateShuffleVector(R, llvm::UndefValue::get(ExtTy), Mask);
    Stack.back() = Builder.CreateBitCast(R, Context.Int64x2Ty);
  }
  void compileVectorExtMul(llvm::VectorType *FromTy, bool Signed, bool Low) {
    auto *ExtTy = llvm::VectorType::getExtendedElementVectorType(FromTy);
    const auto Count = elementCount(FromTy);
    std::vector<ShuffleElement> Mask(Count / 2);
    std::iota(Mask.begin(), Mask.end(), Low ? 0 : Count / 2);
    auto Extend = [this, FromTy, Signed, ExtTy, &Mask](llvm::Value *R) {
      R = Builder.CreateBitCast(R, FromTy);
      if (Signed) {
        R = Builder.CreateSExt(R, ExtTy);
      } else {
        R = Builder.CreateZExt(R, ExtTy);
      }
      return Builder.CreateShuffleVector(R, llvm::UndefValue::get(ExtTy), Mask);
    };
    auto *RHS = Extend(stackPop());
    auto *LHS = Extend(stackPop());
    stackPush(
        Builder.CreateBitCast(Builder.CreateMul(RHS, LHS), Context.Int64x2Ty));
  }
  void compileVectorExtAddPairwise(llvm::VectorType *VectorTy, bool Signed) {
    compileVectorOp(
        VectorTy, [this, VectorTy, Signed](auto *V) -> llvm::Value * {
          auto *ExtTy = llvm::VectorType::getHalfElementsVectorType(
              llvm::VectorType::getExtendedElementVectorType(VectorTy));
#if defined(__x86_64__)
          const auto Count = elementCount(VectorTy);
          if (Context.SupportXOP) {
            const auto ID = Count == 16
                                ? (Signed ? llvm::Intrinsic::x86_xop_vphaddbw
                                          : llvm::Intrinsic::x86_xop_vphaddubw)
                                : (Signed ? llvm::Intrinsic::x86_xop_vphaddwd
                                          : llvm::Intrinsic::x86_xop_vphadduwd);
            return Builder.CreateUnaryIntrinsic(ID, V);
          }
          if (Context.SupportSSSE3 && Count == 16) {
            if (Signed) {
              return Builder.CreateIntrinsic(
                  llvm::Intrinsic::x86_ssse3_pmadd_ub_sw_128, {},
                  {Builder.CreateVectorSplat(16, Builder.getInt8(1)), V});
            } else {
              return Builder.CreateIntrinsic(
                  llvm::Intrinsic::x86_ssse3_pmadd_ub_sw_128, {},
                  {V, Builder.CreateVectorSplat(16, Builder.getInt8(1))});
            }
          }
          if (Context.SupportSSE2 && Count == 8) {
            if (Signed) {
              return Builder.CreateIntrinsic(
                  llvm::Intrinsic::x86_sse2_pmadd_wd, {},
                  {V, Builder.CreateVectorSplat(8, Builder.getInt16(1))});
            } else {
              V = Builder.CreateXor(
                  V, Builder.CreateVectorSplat(8, Builder.getInt16(0x8000)));
              V = Builder.CreateIntrinsic(
                  llvm::Intrinsic::x86_sse2_pmadd_wd, {},
                  {V, Builder.CreateVectorSplat(8, Builder.getInt16(1))});
              return Builder.CreateAdd(
                  V, Builder.CreateVectorSplat(4, Builder.getInt32(0x10000)));
            }
          }
#endif

#if defined(__aarch64__)
          if (Context.SupportNEON) {
            const auto ID = Signed ? llvm::Intrinsic::aarch64_neon_saddlp
                                   : llvm::Intrinsic::aarch64_neon_uaddlp;
            return Builder.CreateIntrinsic(ID, {ExtTy, VectorTy}, {V});
          }
#endif

          // Fallback case.
          // If the XOP, SSSE3, or SSE2 is not supported on the x86_64 platform
          // or the NEON is not supported on the aarch64 platform,
          // then fallback to this.
          const auto Width = VectorTy->getElementType()->getIntegerBitWidth();
          auto *EV = Builder.CreateBitCast(V, ExtTy);
          llvm::Value *L, *R;
          if (Signed) {
            L = Builder.CreateAShr(EV, Width);
            R = Builder.CreateAShr(Builder.CreateShl(EV, Width), Width);
          } else {
            L = Builder.CreateLShr(EV, Width);
            R = Builder.CreateLShr(Builder.CreateShl(EV, Width), Width);
          }
          return Builder.CreateAdd(L, R);
        });
  }
  void compileVectorFAbs(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::fabs, V);
    });
  }
  void compileVectorFNeg(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy,
                    [this](auto *V) { return Builder.CreateFNeg(V); });
  }
  void compileVectorFSqrt(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, V);
    });
  }
  void compileVectorFCeil(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ceil, V);
    });
  }
  void compileVectorFFloor(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::floor, V);
    });
  }
  void compileVectorFTrunc(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, V);
    });
  }
  void compileVectorFNearest(llvm::VectorType *VectorTy) {
    compileVectorOp(VectorTy, [&](auto *V) {
#if defined(__x86_64__)
      if (Context.SupportSSE4_1) {
        const bool IsFloat = VectorTy->getElementType() == Context.FloatTy;
        auto ID = IsFloat ? llvm::Intrinsic::x86_sse41_round_ps
                          : llvm::Intrinsic::x86_sse41_round_pd;
        return Builder.CreateIntrinsic(ID, {}, {V, Builder.getInt32(8)});
      }
#endif

#if defined(__aarch64__)
      if (Context.SupportNEON) {
#if LLVM_VERSION_MAJOR >= 13
        return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::roundeven, V);
#else
        return Builder.CreateUnaryIntrinsic(
            llvm::Intrinsic::aarch64_neon_frintn, V);
#endif
      }
#endif

      // Fallback case.
      // If the SSE4.1 is not supported on the x86_64 platform or
      // the NEON is not supported on the aarch64 platform,
      // then fallback to this.
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::nearbyint, V);
    });
  }
  void compileVectorVectorFAdd(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFAdd(LHS, RHS);
    });
  }
  void compileVectorVectorFSub(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFSub(LHS, RHS);
    });
  }
  void compileVectorVectorFMul(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFMul(LHS, RHS);
    });
  }
  void compileVectorVectorFDiv(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFDiv(LHS, RHS);
    });
  }
  void compileVectorVectorFMin(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *LNaN = Builder.CreateFCmpUNO(LHS, LHS);
      auto *RNaN = Builder.CreateFCmpUNO(RHS, RHS);
      auto *OLT = Builder.CreateFCmpOLT(LHS, RHS);
      auto *OGT = Builder.CreateFCmpOGT(LHS, RHS);
      llvm::Value *Ret = Builder.CreateBitCast(
          Builder.CreateOr(Builder.CreateBitCast(LHS, Context.Int64x2Ty),
                           Builder.CreateBitCast(RHS, Context.Int64x2Ty)),
          LHS->getType());
      Ret = Builder.CreateSelect(OGT, RHS, Ret);
      Ret = Builder.CreateSelect(OLT, LHS, Ret);
      Ret = Builder.CreateSelect(RNaN, RHS, Ret);
      Ret = Builder.CreateSelect(LNaN, LHS, Ret);
      return Ret;
    });
  }
  void compileVectorVectorFMax(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *LNaN = Builder.CreateFCmpUNO(LHS, LHS);
      auto *RNaN = Builder.CreateFCmpUNO(RHS, RHS);
      auto *OLT = Builder.CreateFCmpOLT(LHS, RHS);
      auto *OGT = Builder.CreateFCmpOGT(LHS, RHS);
      llvm::Value *Ret = Builder.CreateBitCast(
          Builder.CreateAnd(Builder.CreateBitCast(LHS, Context.Int64x2Ty),
                            Builder.CreateBitCast(RHS, Context.Int64x2Ty)),
          LHS->getType());
      Ret = Builder.CreateSelect(OLT, RHS, Ret);
      Ret = Builder.CreateSelect(OGT, LHS, Ret);
      Ret = Builder.CreateSelect(RNaN, RHS, Ret);
      Ret = Builder.CreateSelect(LNaN, LHS, Ret);
      return Ret;
    });
  }
  void compileVectorVectorFPMin(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *Cmp = Builder.CreateFCmpOLT(RHS, LHS);
      return Builder.CreateSelect(Cmp, RHS, LHS);
    });
  }
  void compileVectorVectorFPMax(llvm::VectorType *VectorTy) {
    compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *Cmp = Builder.CreateFCmpOGT(RHS, LHS);
      return Builder.CreateSelect(Cmp, RHS, LHS);
    });
  }
  void compileVectorTruncSatS(llvm::VectorType *VectorTy, unsigned IntWidth,
                              bool PadZero) {
    compileVectorOp(VectorTy, [this, VectorTy, IntWidth, PadZero](auto *V) {
      const auto Size = elementCount(VectorTy);
      auto *FPTy = VectorTy->getElementType();
      auto *IntMin = Builder.getInt(llvm::APInt::getSignedMinValue(IntWidth));
      auto *IntMax = Builder.getInt(llvm::APInt::getSignedMaxValue(IntWidth));
      auto *IntMinV = Builder.CreateVectorSplat(Size, IntMin);
      auto *IntMaxV = Builder.CreateVectorSplat(Size, IntMax);
      auto *IntZeroV = llvm::ConstantAggregateZero::get(IntMinV->getType());
      auto *FPMin = llvm::ConstantExpr::getSIToFP(IntMin, FPTy);
      auto *FPMax = llvm::ConstantExpr::getSIToFP(IntMax, FPTy);
      auto *FPMinV = Builder.CreateVectorSplat(Size, FPMin);
      auto *FPMaxV = Builder.CreateVectorSplat(Size, FPMax);

      auto *Normal = Builder.CreateFCmpORD(V, V);
      auto *NotUnder = Builder.CreateFCmpUGE(V, FPMinV);
      auto *NotOver = Builder.CreateFCmpULT(V, FPMaxV);
      V = Builder.CreateFPToSI(
          V, llvm::VectorType::get(Builder.getIntNTy(IntWidth), Size, false));
      V = Builder.CreateSelect(Normal, V, IntZeroV);
      V = Builder.CreateSelect(NotUnder, V, IntMinV);
      V = Builder.CreateSelect(NotOver, V, IntMaxV);
      if (PadZero) {
        std::vector<ShuffleElement> Mask(Size * 2);
        std::iota(Mask.begin(), Mask.end(), 0);
        V = Builder.CreateShuffleVector(V, IntZeroV, Mask);
      }
      return V;
    });
  }
  void compileVectorTruncSatU(llvm::VectorType *VectorTy, unsigned IntWidth,
                              bool PadZero) {
    compileVectorOp(VectorTy, [this, VectorTy, IntWidth, PadZero](auto *V) {
      const auto Size = elementCount(VectorTy);
      auto *FPTy = VectorTy->getElementType();
      auto *IntMin = Builder.getInt(llvm::APInt::getMinValue(IntWidth));
      auto *IntMax = Builder.getInt(llvm::APInt::getMaxValue(IntWidth));
      auto *IntMinV = Builder.CreateVectorSplat(Size, IntMin);
      auto *IntMaxV = Builder.CreateVectorSplat(Size, IntMax);
      auto *FPMin = llvm::ConstantExpr::getUIToFP(IntMin, FPTy);
      auto *FPMax = llvm::ConstantExpr::getUIToFP(IntMax, FPTy);
      auto *FPMinV = Builder.CreateVectorSplat(Size, FPMin);
      auto *FPMaxV = Builder.CreateVectorSplat(Size, FPMax);

      auto *NotUnder = Builder.CreateFCmpOGE(V, FPMinV);
      auto *NotOver = Builder.CreateFCmpULT(V, FPMaxV);
      V = Builder.CreateFPToUI(
          V, llvm::VectorType::get(Builder.getIntNTy(IntWidth), Size, false));
      V = Builder.CreateSelect(NotUnder, V, IntMinV);
      V = Builder.CreateSelect(NotOver, V, IntMaxV);
      if (PadZero) {
        auto *IntZeroV = llvm::ConstantAggregateZero::get(IntMinV->getType());
        std::vector<ShuffleElement> Mask(Size * 2);
        std::iota(Mask.begin(), Mask.end(), 0);
        V = Builder.CreateShuffleVector(V, IntZeroV, Mask);
      }
      return V;
    });
  }
  void compileVectorConvertS(llvm::VectorType *VectorTy,
                             llvm::VectorType *FPVectorTy, bool Low) {
    compileVectorOp(VectorTy, [this, VectorTy, FPVectorTy, Low](auto *V) {
      if (Low) {
        const auto Size = elementCount(VectorTy) / 2;
        std::vector<ShuffleElement> Mask(Size);
        std::iota(Mask.begin(), Mask.end(), 0);
        V = Builder.CreateShuffleVector(V, llvm::UndefValue::get(VectorTy),
                                        Mask);
      }
      return Builder.CreateSIToFP(V, FPVectorTy);
    });
  }
  void compileVectorConvertU(llvm::VectorType *VectorTy,
                             llvm::VectorType *FPVectorTy, bool Low) {
    compileVectorOp(VectorTy, [this, VectorTy, FPVectorTy, Low](auto *V) {
      if (Low) {
        const auto Size = elementCount(VectorTy) / 2;
        std::vector<ShuffleElement> Mask(Size);
        std::iota(Mask.begin(), Mask.end(), 0);
        V = Builder.CreateShuffleVector(V, llvm::UndefValue::get(VectorTy),
                                        Mask);
      }
      return Builder.CreateUIToFP(V, FPVectorTy);
    });
  }
  void compileVectorDemote() {
    compileVectorOp(Context.Doublex2Ty, [this](auto *V) {
      auto *Demoted = Builder.CreateFPTrunc(
          V, llvm::VectorType::get(Context.FloatTy, 2, false));
      auto *ZeroV = llvm::ConstantAggregateZero::get(Demoted->getType());
      return Builder.CreateShuffleVector(
          Demoted, ZeroV, std::initializer_list<ShuffleElement>{0, 1, 2, 3});
    });
  }
  void compileVectorPromote() {
    compileVectorOp(Context.Floatx4Ty, [this](auto *V) {
      auto *UndefV = llvm::UndefValue::get(V->getType());
      auto *Low = Builder.CreateShuffleVector(
          V, UndefV, std::initializer_list<ShuffleElement>{0, 1});
      return Builder.CreateFPExt(
          Low, llvm::VectorType::get(Context.DoubleTy, 2, false));
    });
  }

  void enterBlock(
      llvm::BasicBlock *JumpBlock, llvm::BasicBlock *NextBlock,
      llvm::BasicBlock *ElseBlock, std::vector<llvm::Value *> Args,
      std::pair<std::vector<ValType>, std::vector<ValType>> Type,
      std::vector<std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>
          ReturnPHI = {}) {
    assuming(Type.first.size() == Args.size());
    for (auto *Value : Args) {
      stackPush(Value);
    }
    const auto Unreachable = isUnreachable();
    ControlStack.emplace_back(Stack.size() - Args.size(), Unreachable,
                              JumpBlock, NextBlock, ElseBlock, std::move(Args),
                              std::move(Type), std::move(ReturnPHI));
  }

  Control leaveBlock() {
    Control Entry = std::move(ControlStack.back());
    ControlStack.pop_back();

    auto *NextBlock = Entry.NextBlock ? Entry.NextBlock : Entry.JumpBlock;
    if (!Entry.Unreachable) {
      const auto &ReturnType = Entry.Type.second;
      if (!ReturnType.empty()) {
        std::vector<llvm::Value *> Rets(ReturnType.size());
        for (size_t I = 0; I < Rets.size(); ++I) {
          const size_t J = Rets.size() - 1 - I;
          Rets[J] = stackPop();
        }
        Entry.ReturnPHI.emplace_back(std::move(Rets), Builder.GetInsertBlock());
      }
      Builder.CreateBr(NextBlock);
    } else {
      Builder.CreateUnreachable();
    }
    Builder.SetInsertPoint(NextBlock);
    Stack.erase(Stack.begin() + static_cast<int64_t>(Entry.StackSize),
                Stack.end());
    return Entry;
  }

  void checkStop() {
    if (!Interruptible) {
      return;
    }
    auto *NotStopBB = llvm::BasicBlock::Create(LLContext, "NotStop", F);
    auto *StopToken = Builder.CreateAtomicRMW(
        llvm::AtomicRMWInst::BinOp::Xchg,
        Context.getStopToken(Builder, ExecCtx), Builder.getInt32(0),
#if LLVM_VERSION_MAJOR >= 13
        llvm::MaybeAlign(4),
#endif
        llvm::AtomicOrdering::Monotonic);
    auto *NotStop = createLikely(
        Builder, Builder.CreateICmpEQ(StopToken, Builder.getInt32(0)));
    Builder.CreateCondBr(NotStop, NotStopBB,
                         getTrapBB(ErrCode::Value::Interrupted));

    Builder.SetInsertPoint(NotStopBB);
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

  void buildPHI(
      Span<const ValType> RetType,
      Span<const std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>
          Incomings) {
    if (isVoidReturn(RetType)) {
      return;
    }
    std::vector<llvm::Value *> Nodes;
    if (Incomings.size() == 0) {
      const auto &Types = toLLVMTypeVector(LLContext, RetType);
      Nodes.reserve(Types.size());
      for (llvm::Type *Type : Types) {
        Nodes.push_back(llvm::UndefValue::get(Type));
      }
    } else if (Incomings.size() == 1) {
      Nodes = std::move(std::get<0>(Incomings.front()));
    } else {
      const auto &Types = toLLVMTypeVector(LLContext, RetType);
      Nodes.reserve(Types.size());
      for (size_t I = 0; I < Types.size(); ++I) {
        auto *PHIRet = Builder.CreatePHI(
            Types[I], static_cast<uint32_t>(Incomings.size()));
        for (auto &[Value, BB] : Incomings) {
          assuming(Value.size() == Types.size());
          PHIRet->addIncoming(Value[I], BB);
        }
        Nodes.push_back(PHIRet);
      }
    }
    for (auto *Val : Nodes) {
      stackPush(Val);
    }
  }

  void setLableJumpPHI(unsigned int Index) {
    assuming(Index < ControlStack.size());
    auto &Entry = *(ControlStack.rbegin() + Index);
    if (Entry.NextBlock) { // is loop
      std::vector<llvm::Value *> Args(Entry.Type.first.size());
      for (size_t I = 0; I < Args.size(); ++I) {
        const size_t J = Args.size() - 1 - I;
        Args[J] = stackPop();
      }
      for (size_t I = 0; I < Args.size(); ++I) {
        llvm::cast<llvm::PHINode>(Entry.Args[I])
            ->addIncoming(Args[I], Builder.GetInsertBlock());
        stackPush(Args[I]);
      }
    } else if (!Entry.Type.second.empty()) { // has return value
      std::vector<llvm::Value *> Rets(Entry.Type.second.size());
      for (size_t I = 0; I < Rets.size(); ++I) {
        const size_t J = Rets.size() - 1 - I;
        Rets[J] = stackPop();
      }
      for (size_t I = 0; I < Rets.size(); ++I) {
        stackPush(Rets[I]);
      }
      Entry.ReturnPHI.emplace_back(std::move(Rets), Builder.GetInsertBlock());
    }
  }

  llvm::BasicBlock *getLabel(unsigned int Index) const {
    return (ControlStack.rbegin() + Index)->JumpBlock;
  }

  void stackPush(llvm::Value *Value) { Stack.push_back(Value); }
  llvm::Value *stackPop() {
    assuming(!ControlStack.empty() || !Stack.empty());
    assuming(ControlStack.empty() ||
             Stack.size() > ControlStack.back().StackSize);
    auto *Value = Stack.back();
    Stack.pop_back();
    return Value;
  }

  AOT::Compiler::CompileContext &Context;
  llvm::LLVMContext &LLContext;
  std::vector<std::pair<llvm::Type *, llvm::Value *>> Local;
  std::vector<llvm::Value *> Stack;
  llvm::Value *LocalInstrCount = nullptr;
  llvm::Value *LocalGas = nullptr;
  std::unordered_map<ErrCode::Value, llvm::BasicBlock *> TrapBB;
  bool IsUnreachable = false;
  bool Interruptible = false;
  bool OptNone = false;
  struct Control {
    size_t StackSize;
    bool Unreachable;
    llvm::BasicBlock *JumpBlock;
    llvm::BasicBlock *NextBlock;
    llvm::BasicBlock *ElseBlock;
    std::vector<llvm::Value *> Args;
    std::pair<std::vector<ValType>, std::vector<ValType>> Type;
    std::vector<std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>
        ReturnPHI;
    Control(
        size_t S, bool U, llvm::BasicBlock *J, llvm::BasicBlock *N,
        llvm::BasicBlock *E, std::vector<llvm::Value *> A,
        std::pair<std::vector<ValType>, std::vector<ValType>> T,
        std::vector<std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>
            R)
        : StackSize(S), Unreachable(U), JumpBlock(J), NextBlock(N),
          ElseBlock(E), Args(std::move(A)), Type(std::move(T)),
          ReturnPHI(std::move(R)) {}
    Control(const Control &) = default;
    Control(Control &&) = default;
    Control &operator=(const Control &) = default;
    Control &operator=(Control &&) = default;
  };
  std::vector<Control> ControlStack;
  llvm::Function *F;
  llvm::LoadInst *ExecCtx;
  llvm::IRBuilder<> Builder;
};

static std::vector<llvm::Value *> unpackStruct(llvm::IRBuilder<> &Builder,
                                               llvm::Value *Struct) {
  const unsigned N = Struct->getType()->getStructNumElements();
  std::vector<llvm::Value *> Ret;
  Ret.reserve(N);
  for (unsigned I = 0; I < N; ++I) {
    Ret.push_back(Builder.CreateExtractValue(Struct, {I}));
  }
  return Ret;
}

static llvm::Value *createLikely(llvm::IRBuilder<> &Builder,
                                 llvm::Value *Value) {
  return Builder.CreateBinaryIntrinsic(llvm::Intrinsic::expect, Value,
                                       Builder.getTrue());
}

// Write output object and link
Expect<void> outputNativeLibrary(const std::filesystem::path &OutputPath,
                                 const llvm::SmallString<0> &OSVec) {
  using namespace std::literals;

  spdlog::info("output start");
  std::string ObjectName;
  {
    // tempfile
    std::filesystem::path OPath(OutputPath);
#if WASMEDGE_OS_WINDOWS
    OPath.replace_extension("%%%%%%%%%%.obj"sv);
#else
    OPath.replace_extension("%%%%%%%%%%.o"sv);
#endif
    auto Object = llvm::sys::fs::TempFile::create(OPath.u8string());
    if (!Object) {
      // TODO:return error
      spdlog::error("so file creation failed:{}", OPath.u8string());
      llvm::consumeError(Object.takeError());
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
    }
    llvm::raw_fd_ostream OS(Object->FD, false);
    OS.write(OSVec.data(), OSVec.size());
#if WASMEDGE_OS_WINDOWS
    OS.flush();
#else
    OS.close();
#endif
    ObjectName = Object->TmpName;
    llvm::consumeError(Object->keep());
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
#error Unsupported architectur on the MacOS!
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
            ObjectName.c_str(), "-o", OutputPath.u8string().c_str(), "-lSystem"
      },
#elif WASMEDGE_OS_LINUX
  LinkResult = lld::elf::link(
      std::initializer_list<const char *>{"ld.lld", "--shared", "--gc-sections",
                                          "--discard-all", ObjectName.c_str(),
                                          "-o", OutputPath.u8string().c_str()},
#elif WASMEDGE_OS_WINDOWS
  LinkResult = lld::coff::link(
      std::initializer_list<const char *>{
          "lld-link", "-dll", "-defaultlib:libcmt", "-base:0", "-nologo",
          ObjectName.c_str(), ("-out:" + OutputPath.u8string()).c_str()},
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
    llvm::sys::fs::remove(ObjectName);
#if WASMEDGE_OS_WINDOWS
    std::filesystem::path LibPath(OutputPath);
    LibPath.replace_extension(".lib"sv);
    llvm::sys::fs::remove(LibPath.u8string());
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
                               const llvm::SmallString<0> &OSVec) {
  using namespace std::literals;

  std::string SharedObjectName;
  {
    // tempfile
    std::filesystem::path SOPath(OutputPath);
    SOPath.replace_extension("%%%%%%%%%%" WASMEDGE_LIB_EXTENSION);
    auto Object = llvm::sys::fs::TempFile::create(SOPath.u8string());
    if (!Object) {
      // TODO:return error
      spdlog::error("so file creation failed:{}", SOPath.u8string());
      llvm::consumeError(Object.takeError());
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
    }
    llvm::raw_fd_ostream OS(Object->FD, false);
    OS.write(OSVec.data(), OSVec.size());
#if WASMEDGE_OS_WINDOWS
    OS.flush();
#else
    OS.close();
#endif
    SharedObjectName = Object->TmpName;
    llvm::consumeError(Object->keep());
  }

  if (auto Res =
          outputNativeLibrary(std::filesystem::u8path(SharedObjectName), OSVec);
      unlikely(!Res)) {
    return Unexpect(Res);
  }
  std::unique_ptr<llvm::MemoryBuffer> SOFile;
  if (auto Res = llvm::MemoryBuffer::getFile(SharedObjectName);
      unlikely(!Res)) {
    spdlog::error("object file open error:{}", Res.getError().message());
    return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
  } else {
    SOFile = std::move(*Res);
  }

  std::unique_ptr<llvm::object::ObjectFile> ObjFile;
  if (auto Res = llvm::object::ObjectFile::createObjectFile(*SOFile);
      unlikely(!Res)) {
    spdlog::error("object file parse error:{}",
                  llvm::toString(Res.takeError()));
    return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
  } else {
    ObjFile = std::move(*Res);
  }

  llvm::SmallString<0> OSCustomSecVec;
  {
    llvm::raw_svector_ostream OS(OSCustomSecVec);

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
#else
#error Unsupported hardware architecture!
#endif

    std::vector<std::pair<std::string, uint64_t>> SymbolTable;
#if !WASMEDGE_OS_WINDOWS
    for (auto &Symbol : ObjFile->symbols()) {
      std::string Name;
      if (auto Res = Symbol.getName(); unlikely(!Res)) {
        continue;
      } else if (Res->empty()) {
        continue;
      } else {
        Name = std::move(*Res);
      }
      uint64_t Address = 0;
      if (auto Res = Symbol.getAddress(); unlikely(!Res)) {
        continue;
      } else {
        Address = *Res;
      }
      SymbolTable.emplace_back(std::move(Name), std::move(Address));
    }
#else
    for (auto &Symbol : llvm::cast<llvm::object::COFFObjectFile>(ObjFile.get())
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
    for (auto &Section : ObjFile->sections()) {
      if (auto Res = Section.getContents(); unlikely(!Res)) {
        continue;
      }
      if (!Section.isText() && !Section.isData() && !Section.isBSS()) {
        continue;
      }
      ++SectionCount;
    }
    WriteU32(OS, SectionCount);

    for (auto &Section : ObjFile->sections()) {
      const uint64_t Address = Section.getAddress();
      const uint64_t Size = Section.getSize();
      std::vector<char> Content;
      if (auto Res = Section.getContents(); unlikely(!Res)) {
        continue;
      } else {
        Content.assign(Res->begin(), Res->end());
      }
      if (Section.isText()) {
        WriteByte(OS, UINT8_C(1));
      } else if (Section.isData()) {
        WriteByte(OS, UINT8_C(2));
      } else if (Section.isBSS()) {
        WriteByte(OS, UINT8_C(3));
      } else {
        continue;
      }
      WriteU64(OS, Address);
      WriteU64(OS, Size);
      WriteName(OS, std::string_view(Content.data(), Content.size()));
    }
  }

  spdlog::info("output start");

  std::error_code EC;
  llvm::raw_fd_ostream OS(OutputPath.u8string(), EC);
  if (EC) {
    spdlog::error("output failed:{}", EC.message());
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  OS.write(reinterpret_cast<const char *>(Data.data()), Data.size());
  // Custom section id
  WriteByte(OS, UINT8_C(0x00));
  WriteName(OS, std::string_view(OSCustomSecVec.data(), OSCustomSecVec.size()));

  llvm::sys::fs::remove(SharedObjectName);
  return {};
}

} // namespace

namespace WasmEdge {
namespace AOT {

Expect<void> Compiler::compile(Span<const Byte> Data, const AST::Module &Module,
                               std::filesystem::path OutputPath) {
  // Check the module is validated.
  if (unlikely(!Module.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    return Unexpect(ErrCode::Value::NotValidated);
  }

  using namespace std::literals;

  std::unique_lock Lock(Mutex);
  spdlog::info("compile start");
  std::filesystem::path LLPath(OutputPath);
  LLPath.replace_extension("ll"sv);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  llvm::LLVMContext LLContext;
  llvm::Module LLModule(LLPath.u8string(), LLContext);
  LLModule.setTargetTriple(llvm::sys::getProcessTriple());
#if WASMEDGE_OS_MACOS
  LLModule.setPICLevel(llvm::PICLevel::Level::BigPIC);
#elif WASMEDGE_OS_LINUX | WASMEDGE_OS_WINDOWS
  LLModule.setPICLevel(llvm::PICLevel::Level::SmallPIC);
#endif
  CompileContext NewContext(LLModule,
                            Conf.getCompilerConfigure().isGenericBinary());
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

  if (Conf.getCompilerConfigure().getOutputFormat() ==
      CompilerConfigure::OutputFormat::Native) {
    // create wasm.code and wasm.size
    auto *Int32Ty = Context->Int32Ty;
    auto *Content = llvm::ConstantDataArray::getString(
        LLContext,
        llvm::StringRef(reinterpret_cast<const char *>(Data.data()),
                        Data.size()),
        false);
    new llvm::GlobalVariable(LLModule, Content->getType(), false,
                             llvm::GlobalValue::ExternalLinkage, Content,
                             "wasm.code");
    new llvm::GlobalVariable(
        LLModule, Int32Ty, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(Int32Ty, Data.size()), "wasm.size");
  }

  // set dllexport
  for (auto &GV : LLModule.global_values()) {
    if (GV.hasExternalLinkage()) {
      GV.setVisibility(llvm::GlobalValue::ProtectedVisibility);
      GV.setDLLStorageClass(llvm::GlobalValue::DLLExportStorageClass);
    }
  }

  if (Conf.getCompilerConfigure().isDumpIR()) {
    int Fd;
    llvm::sys::fs::openFileForWrite("wasm.ll", Fd);
    llvm::raw_fd_ostream OS(Fd, true);
    LLModule.print(OS, nullptr);
  }

  spdlog::info("verify start");
  llvm::verifyModule(LLModule, &llvm::errs());
  spdlog::info("optimize start");

  llvm::SmallString<0> OSVec;

  // optimize + codegen
  llvm::Triple Triple(LLModule.getTargetTriple());
  {
    std::string Error;
    const llvm::Target *TheTarget =
        llvm::TargetRegistry::lookupTarget(Triple.getTriple(), Error);
    if (!TheTarget) {
      // TODO:return error
      spdlog::error("lookupTarget failed:{}", Error);
      return Unexpect(ErrCode::Value::IllegalPath);
    }

    llvm::TargetOptions Options;
    llvm::Reloc::Model RM = llvm::Reloc::PIC_;
    llvm::StringRef CPUName("generic");
    if (!Conf.getCompilerConfigure().isGenericBinary()) {
      CPUName = llvm::sys::getHostCPUName();
    }
    std::unique_ptr<llvm::TargetMachine> TM(TheTarget->createTargetMachine(
        Triple.str(), CPUName, Context->SubtargetFeatures.getString(), Options,
        RM, llvm::None, llvm::CodeGenOpt::Level::Aggressive));
    LLModule.setDataLayout(TM->createDataLayout());

    llvm::TargetLibraryInfoImpl TLII(Triple);

    {
#if LLVM_VERSION_MAJOR == 12
      llvm::PassBuilder PB(false, TM.get());
#else
      llvm::PassBuilder PB(TM.get());
#endif

      llvm::LoopAnalysisManager LAM;
      llvm::FunctionAnalysisManager FAM;
      llvm::CGSCCAnalysisManager CGAM;
      llvm::ModuleAnalysisManager MAM;

      // Register the AA manager first so that our version is the one
      // used.
      FAM.registerPass([&] { return PB.buildDefaultAAPipeline(); });

      // Register the target library analysis directly and give it a
      // customized preset TLI.
      FAM.registerPass([&] { return llvm::TargetLibraryAnalysis(TLII); });
#if LLVM_VERSION_MAJOR <= 9
      MAM.registerPass([&] { return llvm::TargetLibraryAnalysis(TLII); });
#endif

      // Register all the basic analyses with the managers.
      PB.registerModuleAnalyses(MAM);
      PB.registerCGSCCAnalyses(CGAM);
      PB.registerFunctionAnalyses(FAM);
      PB.registerLoopAnalyses(LAM);
      PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

      llvm::ModulePassManager MPM;
      if (Conf.getCompilerConfigure().getOptimizationLevel() ==
          CompilerConfigure::OptimizationLevel::O0) {
        MPM.addPass(
            llvm::createModuleToFunctionPassAdaptor(llvm::TailCallElimPass()));
        MPM.addPass(llvm::AlwaysInlinerPass(false));
      } else {
        MPM.addPass(PB.buildPerModuleDefaultPipeline(
            toLLVMLevel(Conf.getCompilerConfigure().getOptimizationLevel())));
      }

      MPM.run(LLModule, MAM);
    }

    // Set initializer for constant value
    if (auto *IntrinsicsTable = LLModule.getNamedGlobal("intrinsics")) {
      IntrinsicsTable->setInitializer(llvm::ConstantPointerNull::get(
          llvm::cast<llvm::PointerType>(IntrinsicsTable->getValueType())));
      IntrinsicsTable->setConstant(false);
    }

    llvm::legacy::PassManager CodeGenPasses;
    CodeGenPasses.add(
        llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));

    // Add LibraryInfo.
    CodeGenPasses.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

    llvm::raw_svector_ostream OS(OSVec);
#if LLVM_VERSION_MAJOR >= 10
    using llvm::CGFT_ObjectFile;
#else
    const auto CGFT_ObjectFile = llvm::TargetMachine::CGFT_ObjectFile;
#endif
    if (TM->addPassesToEmitFile(CodeGenPasses, OS, nullptr, CGFT_ObjectFile,
                                false)) {
      // TODO:return error
      spdlog::error("addPassesToEmitFile failed");
      return Unexpect(ErrCode::Value::IllegalPath);
    }

    if (Conf.getCompilerConfigure().isDumpIR()) {
      int Fd;
      llvm::sys::fs::openFileForWrite("wasm-opt.ll", Fd);
      llvm::raw_fd_ostream LLOS(Fd, true);
      LLModule.print(LLOS, nullptr);
    }
    spdlog::info("codegen start");
    CodeGenPasses.run(LLModule);
  }

  switch (Conf.getCompilerConfigure().getOutputFormat()) {
  case CompilerConfigure::OutputFormat::Native:
    if (auto Res = outputNativeLibrary(OutputPath, OSVec); unlikely(!Res)) {
      return Unexpect(Res);
    }
    break;
  case CompilerConfigure::OutputFormat::Wasm:
    if (auto Res = outputWasmLibrary(OutputPath, Data, OSVec); unlikely(!Res)) {
      return Unexpect(Res);
    }
    break;
  }

  return {};
}

void Compiler::compile(const AST::TypeSection &TypeSec) {
  auto *WrapperTy =
      llvm::FunctionType::get(Context->VoidTy,
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
    const auto Name = "t" + std::to_string(Context->FunctionTypes.size());

    // Check function type is unique
    {
      bool Unique = true;
      for (size_t J = 0; J < I; ++J) {
        const auto &OldFuncType = *Context->FunctionTypes[J];
        if (OldFuncType == FuncType) {
          Unique = false;
          Context->FunctionTypes.push_back(&OldFuncType);
          auto *F = Context->FunctionWrappers[J];
          Context->FunctionWrappers.push_back(F);
          llvm::GlobalAlias::create(Name, F);
          break;
        }
      }
      if (!Unique) {
        continue;
      }
    }

    // Create Wrapper
    auto *F = llvm::Function::Create(WrapperTy, llvm::Function::ExternalLinkage,
                                     Name, Context->LLModule);
    {
      F->addFnAttr(llvm::Attribute::StrictFP);
      F->addParamAttr(0, llvm::Attribute::AttrKind::ReadOnly);
      F->addParamAttr(0, llvm::Attribute::AttrKind::NoAlias);
      F->addParamAttr(1, llvm::Attribute::AttrKind::NoAlias);
      F->addParamAttr(2, llvm::Attribute::AttrKind::NoAlias);
      F->addParamAttr(3, llvm::Attribute::AttrKind::NoAlias);

      llvm::IRBuilder<> Builder(
          llvm::BasicBlock::Create(F->getContext(), "entry", F));
      setIsFPConstrained(Builder);
      auto *FTy = toLLVMType(Context->ExecCtxPtrTy, FuncType);
      auto *RTy = FTy->getReturnType();
      const size_t ArgCount = FTy->getNumParams() - 1;
      const size_t RetCount =
          RTy->isVoidTy()
              ? 0
              : (RTy->isStructTy() ? RTy->getStructNumElements() : 1);
      auto *ExecCtxPtr = F->arg_begin();
      auto RawFunc = llvm::FunctionCallee(
          FTy, Builder.CreateBitCast(F->arg_begin() + 1, FTy->getPointerTo()));
      auto *RawArgs = F->arg_begin() + 2;
      auto *RawRets = F->arg_begin() + 3;

      std::vector<llvm::Value *> Args;
      Args.reserve(FTy->getNumParams());
      Args.push_back(ExecCtxPtr);
      for (size_t J = 0; J < ArgCount; ++J) {
        auto *ArgTy = FTy->getParamType(static_cast<uint32_t>(J + 1));
        llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(
            Context->Int8Ty, RawArgs, J * kValSize);
        llvm::Value *Ptr = Builder.CreateBitCast(VPtr, ArgTy->getPointerTo());
        Args.push_back(Builder.CreateLoad(ArgTy, Ptr));
      }

      auto Ret = Builder.CreateCall(RawFunc, Args);
      if (RTy->isVoidTy()) {
        // nothing to do
      } else if (RTy->isStructTy()) {
        auto Rets = unpackStruct(Builder, Ret);
        for (size_t J = 0; J < RetCount; ++J) {
          llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(
              Context->Int8Ty, RawRets, J * kValSize);
          llvm::Value *Ptr =
              Builder.CreateBitCast(VPtr, Rets[J]->getType()->getPointerTo());
          Builder.CreateStore(Rets[J], Ptr);
        }
      } else {
        llvm::Value *VPtr =
            Builder.CreateConstInBoundsGEP1_64(Context->Int8Ty, RawRets, 0);
        llvm::Value *Ptr =
            Builder.CreateBitCast(VPtr, Ret->getType()->getPointerTo());
        Builder.CreateStore(Ret, Ptr);
      }
      Builder.CreateRetVoid();
    }
    // Copy wrapper, param and return lists to module instance.
    Context->FunctionTypes.push_back(&FuncType);
    Context->FunctionWrappers.push_back(F);
  }
}

void Compiler::compile(const AST::ImportSection &ImportSec) {
  // Iterate and compile import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    // Get data from import description.
    const auto &ExtType = ImpDesc.getExternalType();

    // Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: // Function type index
    {
      const auto FuncID = static_cast<uint32_t>(Context->Functions.size());
      // Get the function type index in module.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      assuming(TypeIdx < Context->FunctionTypes.size());
      const auto &FuncType = *Context->FunctionTypes[TypeIdx];

      auto *FTy = toLLVMType(Context->ExecCtxPtrTy, FuncType);
      auto *RTy = FTy->getReturnType();
      auto *F = llvm::Function::Create(FTy, llvm::Function::PrivateLinkage,
                                       "f" + std::to_string(FuncID),
                                       Context->LLModule);
      F->addFnAttr(llvm::Attribute::StrictFP);
      F->addParamAttr(0, llvm::Attribute::AttrKind::ReadOnly);
      F->addParamAttr(0, llvm::Attribute::AttrKind::NoAlias);

      auto *Entry = llvm::BasicBlock::Create(Context->LLContext, "entry", F);
      llvm::IRBuilder<> Builder(Entry);
      setIsFPConstrained(Builder);

      const auto ArgSize = FuncType.getParamTypes().size();
      const auto RetSize =
          RTy->isVoidTy() ? 0 : FuncType.getReturnTypes().size();

      llvm::Value *Args;
      if (ArgSize == 0) {
        Args = llvm::ConstantPointerNull::get(Context->Int8PtrTy);
      } else {
        auto *Alloca = Builder.CreateAlloca(
            Context->Int8Ty, Builder.getInt64(ArgSize * kValSize));
        Alloca->setAlignment(Align(kValSize));
        Args = Alloca;
      }

      llvm::Value *Rets;
      if (RetSize == 0) {
        Rets = llvm::ConstantPointerNull::get(Context->Int8PtrTy);
      } else {
        auto *Alloca = Builder.CreateAlloca(
            Context->Int8Ty, Builder.getInt64(RetSize * kValSize));
        Alloca->setAlignment(Align(kValSize));
        Rets = Alloca;
      }

      for (unsigned I = 0; I < ArgSize; ++I) {
        llvm::Argument *Arg = F->arg_begin() + 1 + I;
        llvm::Value *Ptr = Builder.CreateConstInBoundsGEP1_64(
            Context->Int8Ty, Args, I * kValSize);
        Builder.CreateStore(
            Arg, Builder.CreateBitCast(Ptr, Arg->getType()->getPointerTo()));
      }

      Builder.CreateCall(
          Context->getIntrinsic(
              Builder, AST::Module::Intrinsics::kCall,
              llvm::FunctionType::get(
                  Context->VoidTy,
                  {Context->Int32Ty, Context->Int8PtrTy, Context->Int8PtrTy},
                  false)),
          {Builder.getInt32(FuncID), Args, Rets});

      if (RetSize == 0) {
        Builder.CreateRetVoid();
      } else if (RetSize == 1) {
        llvm::Value *VPtr =
            Builder.CreateConstInBoundsGEP1_64(Context->Int8Ty, Rets, 0);
        llvm::Value *Ptr =
            Builder.CreateBitCast(VPtr, F->getReturnType()->getPointerTo());
        Builder.CreateRet(Builder.CreateLoad(F->getReturnType(), Ptr));
      } else {
        std::vector<llvm::Value *> Ret;
        Ret.reserve(RetSize);
        for (unsigned I = 0; I < RetSize; ++I) {
          llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(
              Context->Int8Ty, Rets, I * kValSize);
          llvm::Value *Ptr = Builder.CreateBitCast(
              VPtr, RTy->getStructElementType(I)->getPointerTo());
          Ret.push_back(Builder.CreateLoad(RTy->getStructElementType(I), Ptr));
        }
        Builder.CreateAggregateRet(Ret.data(), static_cast<uint32_t>(RetSize));
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
      auto *Type = toLLVMType(Context->LLContext, ValType);
      Context->Globals.push_back(Type);
      break;
    }
    default:
      break;
    }
  }
}

void Compiler::compile(const AST::ExportSection &) {}

void Compiler::compile(const AST::GlobalSection &GlobalSec) {
  for (const auto &GlobalSeg : GlobalSec.getContent()) {
    const auto &ValType = GlobalSeg.getGlobalType().getValType();
    auto *Type = toLLVMType(Context->LLContext, ValType);
    Context->Globals.push_back(Type);
  }
}

void Compiler::compile(const AST::MemorySection &, const AST::DataSection &) {}

void Compiler::compile(const AST::TableSection &, const AST::ElementSection &) {
}

void Compiler::compile(const AST::FunctionSection &FuncSec,
                       const AST::CodeSection &CodeSec) {
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
    auto *FTy = toLLVMType(Context->ExecCtxPtrTy, FuncType);
    auto *F =
        llvm::Function::Create(FTy, llvm::Function::ExternalLinkage,
                               "f" + std::to_string(FuncID), Context->LLModule);
    F->addFnAttr(llvm::Attribute::StrictFP);
    F->addParamAttr(0, llvm::Attribute::AttrKind::ReadOnly);
    F->addParamAttr(0, llvm::Attribute::AttrKind::NoAlias);

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
    llvm::EliminateUnreachableBlocks(*F);
  }
}

} // namespace AOT
} // namespace WasmEdge
