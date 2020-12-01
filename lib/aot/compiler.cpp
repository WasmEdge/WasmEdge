// SPDX-License-Identifier: Apache-2.0
#include "aot/compiler.h"
#include "common/filesystem.h"
#include "common/log.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/table.h"
#include <lld/Common/Driver.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <numeric>

#if LLVM_VERSION_MAJOR >= 10
#include <llvm/IR/IntrinsicsAArch64.h>
#include <llvm/IR/IntrinsicsX86.h>
#include <llvm/Support/Alignment.h>
#endif

namespace {

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

static bool isVoidReturn(SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::Type *toLLVMType(llvm::LLVMContext &LLContext,
                              const SSVM::ValType &ValType);
static std::vector<llvm::Type *>
toLLVMArgsType(llvm::PointerType *ExecCtxPtrTy,
               SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::Type *toLLVMRetsType(llvm::LLVMContext &LLContext,
                                  SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::FunctionType *toLLVMType(llvm::PointerType *ExecCtxPtrTy,
                                      const SSVM::AST::FunctionType &FuncType);
static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &LLContext,
                                          const SSVM::ValType &ValType);
static std::vector<llvm::Value *> unpackStruct(llvm::IRBuilder<> &Builder,
                                               llvm::Value *Struct);
static llvm::Value *createLikely(llvm::IRBuilder<> &Builder,
                                 llvm::Value *Value);
class FunctionCompiler;

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/// XXX: Misalignment handler not implemented yet, forcing unalignment
/// force unalignment load/store
static inline constexpr const bool kForceUnalignment = true;

/// force checking div/rem on zero
static inline constexpr const bool kForceDivCheck = true;

/// Size of a ValVariant
static inline constexpr const uint32_t kValSize = sizeof(SSVM::ValVariant);

/// Translate Compiler::OptimizationLevel to llvm::PassBuilder version
static inline llvm::PassBuilder::OptimizationLevel
toLLVMLevel(SSVM::AOT::Compiler::OptimizationLevel Level) {
  using OL = SSVM::AOT::Compiler::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return llvm::PassBuilder::OptimizationLevel::O0;
  case OL::O1:
    return llvm::PassBuilder::OptimizationLevel::O1;
  case OL::O2:
    return llvm::PassBuilder::OptimizationLevel::O2;
  case OL::O3:
    return llvm::PassBuilder::OptimizationLevel::O3;
  case OL::Os:
    return llvm::PassBuilder::OptimizationLevel::Os;
  case OL::Oz:
    return llvm::PassBuilder::OptimizationLevel::Oz;
  default:
    assert(false);
    __builtin_unreachable();
  }
}
} // namespace

struct SSVM::AOT::Compiler::CompileContext {
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
  llvm::SubtargetFeatures SubtargetFeatures;

#if LLVM_VERSION_MAJOR < 11
  bool SupportRoundeven =
#if (defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||           \
     defined(_M_X64)) &&                                                       \
    (defined(__AVX512F__) || defined(__AVX__) || defined(__SSE4_1__))
      true;
#elif (defined(__arm__) || defined(__aarch64__)) &&                            \
    (defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(__ARM_NEON_FP))
      true;
#else
      false;
#endif
#endif

  bool SupportShuffle =
#if (defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||           \
     defined(_M_X64)) &&                                                       \
    (defined(__AVX512F__) || defined(__AVX__) || defined(__SSE4_1__) ||        \
     defined(__SSE3__))
      true;
#elif (defined(__arm__) || defined(__aarch64__)) &&                            \
    (defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(__ARM_NEON_FP))
      true;
#else
      false;
#endif

  std::vector<const AST::FunctionType *> FunctionTypes;
  std::vector<llvm::Function *> FunctionWrappers;
  std::vector<std::tuple<uint32_t, llvm::Function *, SSVM::AST::CodeSegment *>>
      Functions;
  std::vector<llvm::Type *> Globals;
  llvm::GlobalVariable *IntrinsicsTable;
  llvm::Function *Trap;
  uint32_t MemMin = 1, MemMax = 65536;
  CompileContext(llvm::Module &M)
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
            /// Memory
            Int8PtrTy,
            /// Globals
            Int128PtrTy->getPointerTo(),
            /// InstrCount
            Int64PtrTy,
            /// CostTable
            llvm::ArrayType::get(Int64Ty, UINT16_MAX + 1)->getPointerTo(),
            /// Gas
            Int64PtrTy)),
        ExecCtxPtrTy(ExecCtxTy->getPointerTo()),
        IntrinsicsTable(new llvm::GlobalVariable(
            LLModule,
            llvm::ArrayType::get(
                Int8PtrTy, uint32_t(AST::Module::Intrinsics::kIntrinsicMax)),
            true, llvm::GlobalVariable::ExternalLinkage, nullptr,
            "intrinsics")),
        Trap(llvm::Function::Create(
            llvm::FunctionType::get(VoidTy, {Int8Ty}, false),
            llvm::Function::PrivateLinkage, "trap", LLModule)) {
    Trap->addFnAttr(llvm::Attribute::StrictFP);
    Trap->addFnAttr(llvm::Attribute::NoReturn);

    new llvm::GlobalVariable(
        LLModule, Int32Ty, true, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(Int32Ty, kVersion), "version");

    {
      llvm::StringMap<bool> FeatureMap;
      llvm::sys::getHostCPUFeatures(FeatureMap);
      for (auto &Feature : FeatureMap) {

#if LLVM_VERSION_MAJOR < 11
        if (!SupportRoundeven && Feature.second) {
          if (llvm::StringSwitch<bool>(Feature.first())
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||            \
    defined(_M_X64)
                  .Cases("avx512f", "avx", "sse4.1", true)
#endif
#if defined(__arm__) || defined(__aarch64__)
                  .Case("neon", true)
#endif
                  .Default(false)) {
            SupportRoundeven = true;
          }
        }
#endif

        if (!SupportShuffle && Feature.second) {
          if (llvm::StringSwitch<bool>(Feature.first())
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||            \
    defined(_M_X64)
                  .Cases("avx512f", "avx", "sse4.1", "sse3", true)
#endif
#if defined(__arm__) || defined(__aarch64__)
                  .Case("neon", true)
#endif
                  .Default(false)) {
            SupportShuffle = true;
          }
        }

        SubtargetFeatures.AddFeature(Feature.first(), Feature.second);
      }
    }

    {
      /// create trap
      llvm::IRBuilder<> Builder(
          llvm::BasicBlock::Create(LLContext, "entry", Trap));
      auto *CallTrap = Builder.CreateCall(
          getIntrinsic(Builder, AST::Module::Intrinsics::kTrap,
                       llvm::FunctionType::get(VoidTy, {Int8Ty}, false)),
          {Trap->arg_begin()});
      CallTrap->setDoesNotReturn();
      Builder.CreateUnreachable();
    }
  }
  llvm::Value *getMemory(llvm::IRBuilder<> &Builder, llvm::LoadInst *ExecCtx) {
    return Builder.CreateExtractValue(ExecCtx, {0});
  }
  llvm::Value *getGlobals(llvm::IRBuilder<> &Builder, llvm::LoadInst *ExecCtx,
                          uint32_t Index, llvm::Type *Type) {
    auto *Array = Builder.CreateExtractValue(ExecCtx, {1});
    auto *VPtr =
        Builder.CreateLoad(Builder.CreateConstInBoundsGEP1_64(Array, Index));
    auto *Ptr = Builder.CreateBitCast(VPtr, Type);
    return Ptr;
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
  llvm::FunctionCallee getIntrinsic(llvm::IRBuilder<> &Builder,
                                    AST::Module::Intrinsics Index,
                                    llvm::FunctionType *Ty) {
    auto *VPtr = llvm::ConstantExpr::getInBoundsGetElementPtr(
        nullptr, IntrinsicsTable,
        std::array<llvm::Constant *, 2>{
            llvm::ConstantInt::get(Int64Ty, 0),
            llvm::ConstantInt::get(Int64Ty, uint32_t(Index))});
    return llvm::FunctionCallee(
        Ty, Builder.CreateLoad(llvm::ConstantExpr::getBitCast(
                VPtr, Ty->getPointerTo()->getPointerTo())));
  }
};

namespace {

using namespace SSVM;

static bool isVoidReturn(Span<const SSVM::ValType> ValTypes) {
  return ValTypes.empty() ||
         (ValTypes.size() == 1 && ValTypes.front() == ValType::None);
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
    assert(false);
    __builtin_unreachable();
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
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(LLContext), 0.0f);
  case ValType::F64:
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(LLContext), 0.0);
  default:
    assert(false);
    __builtin_unreachable();
  }
}

class FunctionCompiler {
public:
  FunctionCompiler(AOT::Compiler::CompileContext &Context, llvm::Function *F,
                   Span<const ValType> Locals, bool InstructionCounting,
                   bool GasMeasuring, bool OptNone)
      : Context(Context), LLContext(Context.LLContext), OptNone(OptNone), F(F),
        Builder(llvm::BasicBlock::Create(LLContext, "entry", F)) {
    if (F) {
      setIsFPConstrained(Builder);
      ExecCtx = Builder.CreateLoad(F->arg_begin());

      if (InstructionCounting) {
        LocalInstrCount = Builder.CreateAlloca(Context.Int64Ty);
        Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
      }

      if (GasMeasuring) {
        LocalGas = Builder.CreateAlloca(Context.Int64Ty);
        readGas();
      }

      for (llvm::Argument *Arg = F->arg_begin() + 1; Arg != F->arg_end();
           ++Arg) {
        llvm::Value *ArgPtr = Builder.CreateAlloca(Arg->getType());
        Builder.CreateStore(Arg, ArgPtr);
        Local.push_back(ArgPtr);
      }

      for (const auto &Type : Locals) {
        llvm::Value *ArgPtr = Builder.CreateAlloca(toLLVMType(LLContext, Type));
        Builder.CreateStore(toLLVMConstantZero(LLContext, Type), ArgPtr);
        Local.push_back(ArgPtr);
      }
    }
  }

  explicit FunctionCompiler(AOT::Compiler::CompileContext &Context)
      : Context(Context), LLContext(Context.LLContext), F(nullptr),
        Builder(llvm::BasicBlock::Create(LLContext, "entry", F)) {}

  ~FunctionCompiler() noexcept {
    if (!F) {
      delete Builder.GetInsertBlock();
    }
  }

  llvm::BasicBlock *getTrapBB(ErrCode Error) {
    if (auto Iter = TrapBB.find(Error); Iter != TrapBB.end()) {
      return Iter->second;
    }
    auto *BB = llvm::BasicBlock::Create(LLContext, "trap", F);
    TrapBB.emplace(Error, BB);
    return BB;
  }

  Expect<void> compile(const AST::CodeSegment &Code,
                       Span<const ValType> Returns) {
    auto *RetBB = llvm::BasicBlock::Create(LLContext, "ret", F);
    ControlStack.emplace_back(Stack.size(), RetBB,
                              std::vector(Returns.begin(), Returns.end()), 0,
                              std::nullopt);

    if (auto Status = compile(Code.getInstrs()); !Status) {
      return Unexpect(Status);
    }

    buildPHI(Returns, leaveBlock(RetBB));
    compileReturn();

    for (auto &[Error, BB] : TrapBB) {
      Builder.SetInsertPoint(BB);
      updateInstrCount();
      writeGas();
      Builder.CreateCall(Context.Trap, {Builder.getInt8(uint8_t(Error))});
      Builder.CreateUnreachable();
    }

    return {};
  }

  Expect<void> compile(const AST::InstrVec &Instrs) {
    for (const auto &Instr : Instrs) {
      if (isUnreachable()) {
        return {};
      }
      auto Res = AST::dispatchInstruction(
          Instr->getOpCode(), [this, &Instr](const auto &&Arg) -> Expect<void> {
            using InstrT = typename std::decay_t<decltype(Arg)>::type;
            if constexpr (std::is_void_v<InstrT>) {
              /// OpCode was checked in validator
              __builtin_unreachable();
              return Unexpect(ErrCode::InvalidOpCode);
            } else {
              /// Update instruction count
              if (LocalInstrCount) {
                Builder.CreateStore(
                    Builder.CreateAdd(Builder.CreateLoad(LocalInstrCount),
                                      Builder.getInt64(1)),
                    LocalInstrCount);
              }
              if (LocalGas) {
                auto *NewGas = Builder.CreateAdd(
                    Builder.CreateLoad(LocalGas),
                    Builder.CreateLoad(Builder.CreateConstInBoundsGEP2_64(
                        Context.getCostTable(Builder, ExecCtx), 0,
                        uint16_t(Instr->getOpCode()))));
                Builder.CreateStore(NewGas, LocalGas);
              }

              /// Make the instruction node according to Code.
              if (auto Status =
                      compile(*static_cast<const InstrT *>(Instr.get()));
                  !Status) {
                return Unexpect(Status);
              }
              return {};
            }
          });
      if (!Res) {
        return Unexpect(Res);
      }
    }
    return {};
  }
  Expect<void> compile(const AST::ControlInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::Unreachable: {
      Builder.CreateBr(getTrapBB(ErrCode::Unreachable));
      setUnreachable();
      break;
    }
    case OpCode::Nop:
    case OpCode::End:
      break;
    case OpCode::Return: {
      compileReturn();
      setUnreachable();
      break;
    }
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::BlockControlInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::Block: {
      auto *Block = llvm::BasicBlock::Create(LLContext, "block", F);
      auto *EndBlock = llvm::BasicBlock::Create(LLContext, "block.end", F);
      Builder.CreateBr(Block);

      enterBlock(EndBlock, Instr.getBlockType());
      Builder.SetInsertPoint(Block);
      compile(Instr.getBody());
      buildPHI(resolveBlockType(Instr.getBlockType()).second,
               leaveBlock(EndBlock));
      break;
    }
    case OpCode::Loop: {
      auto *Curr = Builder.GetInsertBlock();
      auto *Loop = llvm::BasicBlock::Create(LLContext, "loop", F);
      auto *EndLoop = llvm::BasicBlock::Create(LLContext, "loop.end", F);
      Builder.CreateBr(Loop);

      Builder.SetInsertPoint(Loop);
      std::vector<llvm::PHINode *> PHIArgs;
      if (auto Type = resolveBlockType(Instr.getBlockType());
          !Type.first.empty()) {
        PHIArgs.resize(Type.first.size());
        for (size_t I = 0; I < PHIArgs.size(); ++I) {
          const size_t J = PHIArgs.size() - 1 - I;
          auto *Value = stackPop();
          PHIArgs[J] = Builder.CreatePHI(Value->getType(), 2);
          PHIArgs[J]->addIncoming(Value, Curr);
        }
        for (auto *PHI : PHIArgs) {
          stackPush(PHI);
        }
      }

      enterBlock(Loop, Instr.getBlockType(), std::move(PHIArgs));
      compile(Instr.getBody());
      buildPHI(resolveBlockType(Instr.getBlockType()).second,
               leaveBlock(EndLoop));
      break;
    }
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::IfElseControlInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::If: {
      auto *Cond = Builder.CreateICmpNE(stackPop(), Builder.getInt32(0));

      auto *Then = llvm::BasicBlock::Create(LLContext, "then", F);
      auto *Else = llvm::BasicBlock::Create(LLContext, "else", F);
      auto *EndIf = llvm::BasicBlock::Create(LLContext, "if.end", F);

      auto BlockType = resolveBlockType(Instr.getBlockType());
      const auto Arity = BlockType.first.size();
      std::vector<llvm::Value *> Args(Arity);
      for (size_t I = 0; I < Arity; ++I) {
        const size_t J = Arity - 1 - I;
        Args[J] = stackPop();
      }

      Builder.CreateCondBr(Cond, Then, Else);

      for (auto *Value : Args) {
        stackPush(Value);
      }
      enterBlock(EndIf, Instr.getBlockType());
      Builder.SetInsertPoint(Then);
      compile(Instr.getIfStatement());
      auto IfResult = leaveBlock(EndIf);

      for (auto *Value : Args) {
        stackPush(Value);
      }
      enterBlock(EndIf, Instr.getBlockType());
      Builder.SetInsertPoint(Else);
      compile(Instr.getElseStatement());
      auto ElseResult = leaveBlock(EndIf);

      IfResult.reserve(IfResult.size() + ElseResult.size());
      IfResult.insert(IfResult.end(), ElseResult.begin(), ElseResult.end());

      buildPHI(resolveBlockType(Instr.getBlockType()).second, IfResult);

      break;
    }
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::BrControlInstruction &Instr) {
    const auto Label = Instr.getLabelIndex();
    switch (Instr.getOpCode()) {
    case OpCode::Br: {
      if (!setLableJumpPHI(Label)) {
        return Unexpect(ErrCode::InvalidLabelIdx);
      }
      Builder.CreateBr(getLabel(Label));
      setUnreachable();
      break;
    }
    case OpCode::Br_if: {
      auto *Cond = Builder.CreateICmpNE(stackPop(), Builder.getInt32(0));
      if (!setLableJumpPHI(Label)) {
        return Unexpect(ErrCode::InvalidLabelIdx);
      }
      auto *Next = llvm::BasicBlock::Create(LLContext, "br_if.end", F);
      Builder.CreateCondBr(Cond, getLabel(Label), Next);
      Builder.SetInsertPoint(Next);
      break;
    }
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::BrTableControlInstruction &Instr) {
    const auto LabelTable = Instr.getLabelList();
    switch (Instr.getOpCode()) {
    case OpCode::Br_table: {
      auto *Value = stackPop();
      if (!setLableJumpPHI(Instr.getLabelIndex())) {
        return Unexpect(ErrCode::InvalidLabelIdx);
      }
      auto *Switch = Builder.CreateSwitch(
          Value, getLabel(Instr.getLabelIndex()), LabelTable.size());
      for (size_t I = 0; I < LabelTable.size(); ++I) {
        if (!setLableJumpPHI(LabelTable[I])) {
          return Unexpect(ErrCode::InvalidLabelIdx);
        }
        Switch->addCase(Builder.getInt32(I), getLabel(LabelTable[I]));
      }
      setUnreachable();
      break;
    }
    default:
      __builtin_unreachable();
    }

    return {};
  }
  Expect<void> compile(const AST::CallControlInstruction &Instr) {
    updateInstrCount();
    writeGas();
    switch (Instr.getOpCode()) {
    case OpCode::Call:
      return compileCallOp(Instr.getTargetIndex());
    case OpCode::Call_indirect: {
      return compileIndirectCallOp(Instr.getTableIndex(),
                                   Instr.getTargetIndex());
    }
    default:
      __builtin_unreachable();
    }
  }
  Expect<void> compile(const AST::ReferenceInstruction &Instr) {
    switch (Instr.getOpCode()) {
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
                               llvm::FunctionType::get(
                                   Context.Int64Ty, {Context.Int32Ty}, false)),
          {Builder.getInt32(Instr.getTargetIndex())}));
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::ParametricInstruction &Instr) {
    switch (Instr.getOpCode()) {
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
    default:
      __builtin_unreachable();
    }

    return {};
  }
  Expect<void> compile(const AST::VariableInstruction &Instr) {
    /// Get variable index.
    const auto Index = Instr.getVariableIndex();

    /// Check OpCode and run the specific instruction.
    switch (Instr.getOpCode()) {
    case OpCode::Local__get:
      stackPush(Builder.CreateLoad(Local[Index]));
      break;
    case OpCode::Local__set:
      Builder.CreateStore(stackPop(), Local[Index]);
      break;
    case OpCode::Local__tee:
      Builder.CreateStore(Stack.back(), Local[Index]);
      break;
    case OpCode::Global__get:
      assert(Index < Context.Globals.size());
      stackPush(Builder.CreateLoad(
          Context.getGlobals(Builder, ExecCtx, Index, Context.Globals[Index])));
      break;
    case OpCode::Global__set:
      assert(Index < Context.Globals.size());
      Builder.CreateStore(
          stackPop(),
          Context.getGlobals(Builder, ExecCtx, Index, Context.Globals[Index]));
      break;
    default:
      __builtin_unreachable();
    }

    return {};
  }
  Expect<void> compile(const AST::TableInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::Table__get: {
      auto *Idx = stackPop();
      stackPush(Builder.CreateCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kTableGet,
              llvm::FunctionType::get(
                  Context.Int64Ty, {Context.Int32Ty, Context.Int32Ty}, false)),
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
                  {Context.Int32Ty, Context.Int32Ty, Context.Int64Ty}, false)),
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
           Builder.getInt32(Instr.getElemIndex()), Dst, Src, Len});
      break;
    }
    case OpCode::Elem__drop: {
      Builder.CreateCall(
          Context.getIntrinsic(Builder, AST::Module::Intrinsics::kElemDrop,
                               llvm::FunctionType::get(
                                   Context.VoidTy, {Context.Int32Ty}, false)),
          {Builder.getInt32(Instr.getElemIndex())});
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
          {Builder.getInt32(Instr.getSourceIndex()),
           Builder.getInt32(Instr.getTargetIndex()), Dst, Src, Len});
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
                  {Context.Int32Ty, Context.Int64Ty, Context.Int32Ty}, false)),
          {Builder.getInt32(Instr.getTargetIndex()), Val, NewSize}));
      break;
    }
    case OpCode::Table__size: {
      stackPush(Builder.CreateCall(
          Context.getIntrinsic(Builder, AST::Module::Intrinsics::kTableSize,
                               llvm::FunctionType::get(
                                   Context.Int32Ty, {Context.Int32Ty}, false)),
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
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::MemoryInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I32__load:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int32Ty);
    case OpCode::I64__load:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int64Ty);
    case OpCode::F32__load:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.FloatTy);
    case OpCode::F64__load:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.DoubleTy);
    case OpCode::I32__load8_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int8Ty, Context.Int32Ty, true);
    case OpCode::I32__load8_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int8Ty, Context.Int32Ty, false);
    case OpCode::I32__load16_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int16Ty, Context.Int32Ty, true);
    case OpCode::I32__load16_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int16Ty, Context.Int32Ty, false);
    case OpCode::I64__load8_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int8Ty, Context.Int64Ty, true);
    case OpCode::I64__load8_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int8Ty, Context.Int64Ty, false);
    case OpCode::I64__load16_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int16Ty, Context.Int64Ty, true);
    case OpCode::I64__load16_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int16Ty, Context.Int64Ty, false);
    case OpCode::I64__load32_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int32Ty, Context.Int64Ty, true);
    case OpCode::I64__load32_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Context.Int32Ty, Context.Int64Ty, false);

    case OpCode::I32__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.Int32Ty);
    case OpCode::I64__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.Int64Ty);
    case OpCode::F32__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.FloatTy);
    case OpCode::F64__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.DoubleTy);
    case OpCode::I32__store8:
    case OpCode::I64__store8:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.Int8Ty, true);
    case OpCode::I32__store16:
    case OpCode::I64__store16:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.Int16Ty, true);
    case OpCode::I64__store32:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.Int32Ty, true);
    case OpCode::Memory__size:
      stackPush(Builder.CreateCall(Context.getIntrinsic(
          Builder, AST::Module::Intrinsics::kMemSize,
          llvm::FunctionType::get(Context.Int32Ty, false))));
      break;
    case OpCode::Memory__grow: {
      auto *Diff = stackPop();
      stackPush(Builder.CreateCall(
          Context.getIntrinsic(Builder, AST::Module::Intrinsics::kMemGrow,
                               llvm::FunctionType::get(
                                   Context.Int32Ty, {Context.Int32Ty}, false)),
          {Diff}));
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
                                       Context.Int32Ty, Context.Int32Ty},
                                      false)),
          {Builder.getInt32(Instr.getDataIndex()), Dst, Src, Len});
      break;
    }
    case OpCode::Data__drop: {
      Builder.CreateCall(
          Context.getIntrinsic(Builder, AST::Module::Intrinsics::kDataDrop,
                               llvm::FunctionType::get(
                                   Context.VoidTy, {Context.Int32Ty}, false)),
          {Builder.getInt32(Instr.getDataIndex())});
      break;
    }
    case OpCode::Memory__copy: {
      auto *Len = stackPop();
      auto *Src = stackPop();
      auto *Dst = stackPop();
      Builder.CreateCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kMemCopy,
              llvm::FunctionType::get(
                  Context.VoidTy,
                  {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
          {Dst, Src, Len});
      break;
    }
    case OpCode::Memory__fill: {
      auto *Len = stackPop();
      auto *Val = Builder.CreateTrunc(stackPop(), Context.Int8Ty);
      auto *Off = stackPop();
      Builder.CreateCall(
          Context.getIntrinsic(
              Builder, AST::Module::Intrinsics::kMemFill,
              llvm::FunctionType::get(
                  Context.VoidTy,
                  {Context.Int32Ty, Context.Int8Ty, Context.Int32Ty}, false)),
          {Off, Val, Len});
      break;
    }
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::ConstInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I32__const:
      stackPush(Builder.getInt32(std::get<uint32_t>(Instr.getConstValue())));
      break;
    case OpCode::I64__const:
      stackPush(Builder.getInt64(std::get<uint64_t>(Instr.getConstValue())));
      break;
    case OpCode::F32__const:
      stackPush(llvm::ConstantFP::get(
          Context.FloatTy,
          llvm::APFloat(std::get<float>(Instr.getConstValue()))));
      break;
    case OpCode::F64__const:
      stackPush(llvm::ConstantFP::get(
          Context.DoubleTy,
          llvm::APFloat(std::get<double>(Instr.getConstValue()))));
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::UnaryNumericInstruction &Instr) {
    switch (Instr.getOpCode()) {
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
      stackPush(Builder.CreateBinaryIntrinsic(llvm::Intrinsic::ctlz, stackPop(),
                                              Builder.getFalse()));
      break;
    case OpCode::I32__ctz:
    case OpCode::I64__ctz:
      stackPush(Builder.CreateBinaryIntrinsic(llvm::Intrinsic::cttz, stackPop(),
                                              Builder.getFalse()));
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
#if LLVM_VERSION_MAJOR >= 11
      stackPush(
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::roundeven, stackPop()));
#else
      const bool IsFloat = Instr.getOpCode() == OpCode::F32__nearest;
      const uint32_t VectorSize = IsFloat ? 4 : 2;
      llvm::Value *Value = stackPop();

#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||            \
    defined(_M_X64)
      if (Context.SupportRoundeven) {
        const uint64_t kZero = 0;
        auto *VectorTy =
            llvm::VectorType::get(Value->getType(), VectorSize, false);
        llvm::Value *Ret = llvm::UndefValue::get(VectorTy);
        Ret = Builder.CreateInsertElement(Ret, Value, kZero);
        auto ID = IsFloat ? llvm::Intrinsic::x86_sse41_round_ss
                          : llvm::Intrinsic::x86_sse41_round_sd;
        Ret = Builder.CreateIntrinsic(ID, {}, {Ret, Ret, Builder.getInt32(8)});
        Ret = Builder.CreateExtractElement(Ret, kZero);
        stackPush(Ret);
        break;
      }
#endif

#if defined(__arm__) || defined(__aarch64__)
      if (Context.SupportRoundeven) {
        const uint64_t kZero = 0;
        auto *VectorTy = llvm::VectorType::get(Value->getType(), VectorSize);
        llvm::Value *Ret = llvm::UndefValue::get(VectorTy);
        Ret = Builder.CreateInsertElement(Ret, Value, kZero);
        Ret = Builder.CreateBinaryIntrinsic(
            llvm::Intrinsic::aarch64_neon_frintn, Ret, Ret);
        Ret = Builder.CreateExtractElement(Ret, kZero);
        stackPush(Ret);
        break;
      }
#endif

      stackPush(
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::nearbyint, Value));
#endif
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
    case OpCode::F32__demote_f64:
#if LLVM_VERSION_MAJOR >= 10
      stackPush(Builder.CreateFPTrunc(stackPop(), Context.FloatTy));
#else
    {
      /// llvm 9 didn't add constrains on fptrunc, do it manually.
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
    }
#endif
      break;
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
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::BinaryNumericInstruction &Instr) {
    llvm::Value *RHS = stackPop();
    llvm::Value *LHS = stackPop();

    switch (Instr.getOpCode()) {
    case OpCode::I32__eq:
    case OpCode::I64__eq:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpEQ(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__ne:
    case OpCode::I64__ne:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpNE(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__lt_s:
    case OpCode::I64__lt_s:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpSLT(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__lt_u:
    case OpCode::I64__lt_u:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpULT(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__gt_s:
    case OpCode::I64__gt_s:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpSGT(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__gt_u:
    case OpCode::I64__gt_u:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpUGT(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__le_s:
    case OpCode::I64__le_s:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpSLE(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__le_u:
    case OpCode::I64__le_u:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpULE(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__ge_s:
    case OpCode::I64__ge_s:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpSGE(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::I32__ge_u:
    case OpCode::I64__ge_u:
      stackPush(
          Builder.CreateZExt(Builder.CreateICmpUGE(LHS, RHS), Context.Int32Ty));
      break;

    case OpCode::F32__eq:
    case OpCode::F64__eq:
      stackPush(
          Builder.CreateZExt(Builder.CreateFCmpOEQ(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::F32__ne:
    case OpCode::F64__ne:
      stackPush(
          Builder.CreateZExt(Builder.CreateFCmpUNE(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::F32__lt:
    case OpCode::F64__lt:
      stackPush(
          Builder.CreateZExt(Builder.CreateFCmpOLT(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::F32__gt:
    case OpCode::F64__gt:
      stackPush(
          Builder.CreateZExt(Builder.CreateFCmpOGT(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::F32__le:
    case OpCode::F64__le:
      stackPush(
          Builder.CreateZExt(Builder.CreateFCmpOLE(LHS, RHS), Context.Int32Ty));
      break;
    case OpCode::F32__ge:
    case OpCode::F64__ge:
      stackPush(
          Builder.CreateZExt(Builder.CreateFCmpOGE(LHS, RHS), Context.Int32Ty));
      break;

    case OpCode::I32__add:
    case OpCode::I64__add:
      stackPush(Builder.CreateAdd(LHS, RHS));
      break;
    case OpCode::I32__sub:
    case OpCode::I64__sub:
      stackPush(Builder.CreateSub(LHS, RHS));
      break;
    case OpCode::I32__mul:
    case OpCode::I64__mul:
      stackPush(Builder.CreateMul(LHS, RHS));
      break;
    case OpCode::I32__div_s:
    case OpCode::I64__div_s:
      if constexpr (kForceDivCheck) {
        const bool Is32 = Instr.getOpCode() == OpCode::I32__div_s;
        llvm::ConstantInt *IntZero =
            Is32 ? Builder.getInt32(0) : Builder.getInt64(0);
        llvm::ConstantInt *IntMinusOne = Is32 ? Builder.getInt32(int32_t(-1))
                                              : Builder.getInt64(int64_t(-1));
        llvm::ConstantInt *IntMin =
            Is32 ? Builder.getInt32(std::numeric_limits<int32_t>::min())
                 : Builder.getInt64(std::numeric_limits<int64_t>::min());

        auto *NoZeroBB = llvm::BasicBlock::Create(LLContext, "div.nozero", F);
        auto *OkBB = llvm::BasicBlock::Create(LLContext, "div.ok", F);

        auto *IsNotZero =
            createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
        Builder.CreateCondBr(IsNotZero, NoZeroBB,
                             getTrapBB(ErrCode::DivideByZero));

        Builder.SetInsertPoint(NoZeroBB);
        auto *NotOverflow = createLikely(
            Builder, Builder.CreateOr(Builder.CreateICmpNE(LHS, IntMin),
                                      Builder.CreateICmpNE(RHS, IntMinusOne)));
        Builder.CreateCondBr(NotOverflow, OkBB,
                             getTrapBB(ErrCode::IntegerOverflow));

        Builder.SetInsertPoint(OkBB);
      }
      stackPush(Builder.CreateSDiv(LHS, RHS));
      break;
    case OpCode::I32__div_u:
    case OpCode::I64__div_u:
      if constexpr (kForceDivCheck) {
        const bool Is32 = Instr.getOpCode() == OpCode::I32__div_u;
        llvm::ConstantInt *IntZero =
            Is32 ? Builder.getInt32(0) : Builder.getInt64(0);
        auto *OkBB = llvm::BasicBlock::Create(LLContext, "div.ok", F);

        auto *IsNotZero =
            createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
        Builder.CreateCondBr(IsNotZero, OkBB, getTrapBB(ErrCode::DivideByZero));
        Builder.SetInsertPoint(OkBB);
      }
      stackPush(Builder.CreateUDiv(LHS, RHS));
      break;
    case OpCode::I32__rem_s:
    case OpCode::I64__rem_s: {
      // handle INT32_MIN % -1
      const bool Is32 = Instr.getOpCode() == OpCode::I32__rem_s;
      llvm::ConstantInt *IntMinusOne =
          Is32 ? Builder.getInt32(int32_t(-1)) : Builder.getInt64(int64_t(-1));
      llvm::ConstantInt *IntMin =
          Is32 ? Builder.getInt32(std::numeric_limits<int32_t>::min())
               : Builder.getInt64(std::numeric_limits<int64_t>::min());
      llvm::ConstantInt *IntZero =
          Is32 ? Builder.getInt32(0) : Builder.getInt64(0);

      auto *NoOverflowBB =
          llvm::BasicBlock::Create(LLContext, "no.overflow", F);
      auto *EndBB = llvm::BasicBlock::Create(LLContext, "end.overflow", F);

      if constexpr (kForceDivCheck) {
        auto *OkBB = llvm::BasicBlock::Create(LLContext, "rem.ok", F);

        auto *IsNotZero =
            createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
        Builder.CreateCondBr(IsNotZero, OkBB, getTrapBB(ErrCode::DivideByZero));
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
    case OpCode::I64__rem_u:
      if constexpr (kForceDivCheck) {
        llvm::ConstantInt *IntZero = Instr.getOpCode() == OpCode::I32__rem_u
                                         ? Builder.getInt32(0)
                                         : Builder.getInt64(0);
        auto *OkBB = llvm::BasicBlock::Create(LLContext, "rem.ok", F);

        auto *IsNotZero =
            createLikely(Builder, Builder.CreateICmpNE(RHS, IntZero));
        Builder.CreateCondBr(IsNotZero, OkBB, getTrapBB(ErrCode::DivideByZero));
        Builder.SetInsertPoint(OkBB);
      }
      stackPush(Builder.CreateURem(LHS, RHS));
      break;
    case OpCode::I32__and:
    case OpCode::I64__and:
      stackPush(Builder.CreateAnd(LHS, RHS));
      break;
    case OpCode::I32__or:
    case OpCode::I64__or:
      stackPush(Builder.CreateOr(LHS, RHS));
      break;
    case OpCode::I32__xor:
    case OpCode::I64__xor:
      stackPush(Builder.CreateXor(LHS, RHS));
      break;
    case OpCode::I32__shl:
    case OpCode::I64__shl:
      stackPush(Builder.CreateShl(LHS, RHS));
      break;
    case OpCode::I32__shr_s:
    case OpCode::I64__shr_s:
      stackPush(Builder.CreateAShr(LHS, RHS));
      break;
    case OpCode::I32__shr_u:
    case OpCode::I64__shr_u:
      stackPush(Builder.CreateLShr(LHS, RHS));
      break;
    case OpCode::I32__rotl:
      stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshl,
                                        {Context.Int32Ty}, {LHS, LHS, RHS}));
      break;
    case OpCode::I32__rotr:
      stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshr,
                                        {Context.Int32Ty}, {LHS, LHS, RHS}));
      break;
    case OpCode::I64__rotl:
      stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshl,
                                        {Context.Int64Ty}, {LHS, LHS, RHS}));
      break;
    case OpCode::I64__rotr:
      stackPush(Builder.CreateIntrinsic(llvm::Intrinsic::fshr,
                                        {Context.Int64Ty}, {LHS, LHS, RHS}));
      break;

    case OpCode::F32__add:
    case OpCode::F64__add:
      stackPush(Builder.CreateFAdd(LHS, RHS));
      break;
    case OpCode::F32__sub:
    case OpCode::F64__sub:
      stackPush(Builder.CreateFSub(LHS, RHS));
      break;
    case OpCode::F32__mul:
    case OpCode::F64__mul:
      stackPush(Builder.CreateFMul(LHS, RHS));
      break;
    case OpCode::F32__div:
    case OpCode::F64__div:
      stackPush(Builder.CreateFDiv(LHS, RHS));
      break;
    case OpCode::F32__min:
    case OpCode::F64__min: {
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
    case OpCode::F64__copysign:
      stackPush(
          Builder.CreateBinaryIntrinsic(llvm::Intrinsic::copysign, LHS, RHS));
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::SIMDMemoryInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::V128__load:
      return compileVectorLoadOp(Instr.getMemoryOffset(),
                                 Instr.getMemoryAlign(), Context.Int128x1Ty);
    case OpCode::I16x8__load8x8_s:
      return compileVectorLoadOp(
          Instr.getMemoryOffset(), Instr.getMemoryAlign(),
          llvm::VectorType::get(Context.Int8Ty, 8, false), Context.Int16x8Ty,
          true);
    case OpCode::I16x8__load8x8_u:
      return compileVectorLoadOp(
          Instr.getMemoryOffset(), Instr.getMemoryAlign(),
          llvm::VectorType::get(Context.Int8Ty, 8, false), Context.Int16x8Ty,
          false);
    case OpCode::I32x4__load16x4_s:
      return compileVectorLoadOp(
          Instr.getMemoryOffset(), Instr.getMemoryAlign(),
          llvm::VectorType::get(Context.Int16Ty, 4, false), Context.Int32x4Ty,
          true);
    case OpCode::I32x4__load16x4_u:
      return compileVectorLoadOp(
          Instr.getMemoryOffset(), Instr.getMemoryAlign(),
          llvm::VectorType::get(Context.Int16Ty, 4, false), Context.Int32x4Ty,
          false);
    case OpCode::I64x2__load32x2_s:
      return compileVectorLoadOp(
          Instr.getMemoryOffset(), Instr.getMemoryAlign(),
          llvm::VectorType::get(Context.Int32Ty, 2, false), Context.Int64x2Ty,
          true);
    case OpCode::I64x2__load32x2_u:
      return compileVectorLoadOp(
          Instr.getMemoryOffset(), Instr.getMemoryAlign(),
          llvm::VectorType::get(Context.Int32Ty, 2, false), Context.Int64x2Ty,
          false);
    case OpCode::I8x16__load_splat:
      return compileSplatLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                                Context.Int8Ty, Context.Int8x16Ty);
    case OpCode::I16x8__load_splat:
      return compileSplatLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                                Context.Int16Ty, Context.Int16x8Ty);
    case OpCode::I32x4__load_splat:
      return compileSplatLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                                Context.Int32Ty, Context.Int32x4Ty);
    case OpCode::I64x2__load_splat:
      return compileSplatLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                                Context.Int64Ty, Context.Int64x2Ty);
    case OpCode::V128__load32_zero:
      return compileVectorLoadOp(Instr.getMemoryOffset(),
                                 Instr.getMemoryAlign(), Context.Int32Ty,
                                 Context.Int128Ty, false);
    case OpCode::V128__load64_zero:
      return compileVectorLoadOp(Instr.getMemoryOffset(),
                                 Instr.getMemoryAlign(), Context.Int64Ty,
                                 Context.Int128Ty, false);
    case OpCode::V128__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Context.Int128x1Ty, false, true);
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::SIMDConstInstruction &Instr) {
    const auto Value = Instr.getConstValue();
    uint64_t Lower = uint64_t(Value);
    uint64_t High = uint64_t(Value >> 64);
    auto *Vector = llvm::ConstantVector::get(
        {Builder.getInt64(Lower), Builder.getInt64(High)});
    stackPush(Builder.CreateBitCast(Vector, Context.Int64x2Ty));
    return {};
  }
  Expect<void> compile(const AST::SIMDShuffleInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I8x16__shuffle: {
      auto *V2 = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);
      auto *V1 = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);
      const auto V3 = Instr.getShuffleValue();
      std::array<ShuffleElement, 16> Mask;
      for (size_t I = 0; I < 16; ++I) {
        Mask[I] = static_cast<uint8_t>(V3 >> (I * 8));
      }
      stackPush(Builder.CreateBitCast(Builder.CreateShuffleVector(V1, V2, Mask),
                                      Context.Int64x2Ty));
      return {};
    }
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::SIMDLaneInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I8x16__extract_lane_s:
      return compileExtractLaneOp(Context.Int8x16Ty, Instr.getLaneIndex(),
                                  Context.Int32Ty, true);
    case OpCode::I8x16__extract_lane_u:
      return compileExtractLaneOp(Context.Int8x16Ty, Instr.getLaneIndex(),
                                  Context.Int32Ty, false);
    case OpCode::I8x16__replace_lane:
      return compileReplaceLaneOp(Context.Int8x16Ty, Instr.getLaneIndex());
    case OpCode::I16x8__extract_lane_s:
      return compileExtractLaneOp(Context.Int16x8Ty, Instr.getLaneIndex(),
                                  Context.Int32Ty, true);
    case OpCode::I16x8__extract_lane_u:
      return compileExtractLaneOp(Context.Int16x8Ty, Instr.getLaneIndex(),
                                  Context.Int32Ty, false);
    case OpCode::I16x8__replace_lane:
      return compileReplaceLaneOp(Context.Int16x8Ty, Instr.getLaneIndex());
    case OpCode::I32x4__extract_lane:
      return compileExtractLaneOp(Context.Int32x4Ty, Instr.getLaneIndex());
    case OpCode::I32x4__replace_lane:
      return compileReplaceLaneOp(Context.Int32x4Ty, Instr.getLaneIndex());
    case OpCode::I64x2__extract_lane:
      return compileExtractLaneOp(Context.Int64x2Ty, Instr.getLaneIndex());
    case OpCode::I64x2__replace_lane:
      return compileReplaceLaneOp(Context.Int64x2Ty, Instr.getLaneIndex());
    case OpCode::F32x4__extract_lane:
      return compileExtractLaneOp(Context.Floatx4Ty, Instr.getLaneIndex());
    case OpCode::F32x4__replace_lane:
      return compileReplaceLaneOp(Context.Floatx4Ty, Instr.getLaneIndex());
    case OpCode::F64x2__extract_lane:
      return compileExtractLaneOp(Context.Doublex2Ty, Instr.getLaneIndex());
    case OpCode::F64x2__replace_lane:
      return compileReplaceLaneOp(Context.Doublex2Ty, Instr.getLaneIndex());
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::SIMDNumericInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I8x16__swizzle: {
      auto *Index = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);
      auto *Vector = Builder.CreateBitCast(stackPop(), Context.Int8x16Ty);

#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||            \
    defined(_M_X64)
      if (Context.SupportShuffle) {
        auto *Magic = Builder.CreateVectorSplat(16, Builder.getInt8(112));
        auto *Added = Builder.CreateAdd(Index, Magic);
        auto *NewIndex = Builder.CreateSelect(
            Builder.CreateICmpUGT(Index, Added),
            llvm::Constant::getAllOnesValue(Context.Int8x16Ty), Added);
        stackPush(Builder.CreateBitCast(
            Builder.CreateIntrinsic(llvm::Intrinsic::x86_ssse3_pshuf_b_128, {},
                                    {Vector, NewIndex}),
            Context.Int64x2Ty));
        return {};
      }
#endif

#if defined(__arm__) || defined(__aarch64__)
      if (Context.SupportShuffle) {
        stackPush(Builder.CreateBitCast(
            Builder.CreateIntrinsic(llvm::Intrinsic::aarch64_neon_tbl1, {},
                                    {Vector, Index}),
            Context.Int64x2Ty));
        return {};
      }
#endif

      auto *Mask = Builder.CreateVectorSplat(16, Builder.getInt8(15));
      auto *Zero = Builder.CreateVectorSplat(16, Builder.getInt8(0));
      auto *IsOver = Builder.CreateICmpUGT(Index, Mask);
      auto *InboundIndex = Builder.CreateAnd(Index, Mask);
      auto *Array = Builder.CreateAlloca(Context.Int8Ty, Builder.getInt64(16));
      for (size_t I = 0; I < 16; ++I) {
        Builder.CreateStore(Builder.CreateExtractElement(Vector, I),
                            Builder.CreateConstInBoundsGEP1_64(Array, I));
      }
      llvm::Value *Ret = llvm::UndefValue::get(Context.Int8x16Ty);
      for (size_t I = 0; I < 16; ++I) {
        auto *Idx = Builder.CreateExtractElement(InboundIndex, I);
        auto *Value =
            Builder.CreateLoad(Builder.CreateInBoundsGEP(Array, {Idx}));
        Ret = Builder.CreateInsertElement(Ret, Value, I);
      }
      Ret = Builder.CreateSelect(IsOver, Zero, Ret);
      stackPush(Builder.CreateBitCast(Ret, Context.Int64x2Ty));
      return {};
    }
    case OpCode::I8x16__splat:
      return compileSplatOp(Context.Int8x16Ty);
    case OpCode::I16x8__splat:
      return compileSplatOp(Context.Int16x8Ty);
    case OpCode::I32x4__splat:
      return compileSplatOp(Context.Int32x4Ty);
    case OpCode::I64x2__splat:
      return compileSplatOp(Context.Int64x2Ty);
    case OpCode::F32x4__splat:
      return compileSplatOp(Context.Floatx4Ty);
    case OpCode::F64x2__splat:
      return compileSplatOp(Context.Doublex2Ty);

    case OpCode::I8x16__eq:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_EQ);
    case OpCode::I8x16__ne:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_NE);
    case OpCode::I8x16__lt_s:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_SLT);
    case OpCode::I8x16__lt_u:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_ULT);
    case OpCode::I8x16__gt_s:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_SGT);
    case OpCode::I8x16__gt_u:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_UGT);
    case OpCode::I8x16__le_s:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_SLE);
    case OpCode::I8x16__le_u:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_ULE);
    case OpCode::I8x16__ge_s:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_SGE);
    case OpCode::I8x16__ge_u:
      return compileVectorCompareOp(Context.Int8x16Ty,
                                    llvm::CmpInst::Predicate::ICMP_UGE);

    case OpCode::I16x8__eq:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_EQ);
    case OpCode::I16x8__ne:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_NE);
    case OpCode::I16x8__lt_s:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_SLT);
    case OpCode::I16x8__lt_u:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_ULT);
    case OpCode::I16x8__gt_s:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_SGT);
    case OpCode::I16x8__gt_u:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_UGT);
    case OpCode::I16x8__le_s:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_SLE);
    case OpCode::I16x8__le_u:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_ULE);
    case OpCode::I16x8__ge_s:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_SGE);
    case OpCode::I16x8__ge_u:
      return compileVectorCompareOp(Context.Int16x8Ty,
                                    llvm::CmpInst::Predicate::ICMP_UGE);

    case OpCode::I32x4__eq:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_EQ);
    case OpCode::I32x4__ne:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_NE);
    case OpCode::I32x4__lt_s:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_SLT);
    case OpCode::I32x4__lt_u:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_ULT);
    case OpCode::I32x4__gt_s:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_SGT);
    case OpCode::I32x4__gt_u:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_UGT);
    case OpCode::I32x4__le_s:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_SLE);
    case OpCode::I32x4__le_u:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_ULE);
    case OpCode::I32x4__ge_s:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_SGE);
    case OpCode::I32x4__ge_u:
      return compileVectorCompareOp(Context.Int32x4Ty,
                                    llvm::CmpInst::Predicate::ICMP_UGE);

    case OpCode::F32x4__eq:
      return compileVectorCompareOp(Context.Floatx4Ty,
                                    llvm::CmpInst::Predicate::FCMP_OEQ,
                                    Context.Int32x4Ty);
    case OpCode::F32x4__ne:
      return compileVectorCompareOp(Context.Floatx4Ty,
                                    llvm::CmpInst::Predicate::FCMP_UNE,
                                    Context.Int32x4Ty);
    case OpCode::F32x4__lt:
      return compileVectorCompareOp(Context.Floatx4Ty,
                                    llvm::CmpInst::Predicate::FCMP_OLT,
                                    Context.Int32x4Ty);
    case OpCode::F32x4__gt:
      return compileVectorCompareOp(Context.Floatx4Ty,
                                    llvm::CmpInst::Predicate::FCMP_OGT,
                                    Context.Int32x4Ty);
    case OpCode::F32x4__le:
      return compileVectorCompareOp(Context.Floatx4Ty,
                                    llvm::CmpInst::Predicate::FCMP_OLE,
                                    Context.Int32x4Ty);
    case OpCode::F32x4__ge:
      return compileVectorCompareOp(Context.Floatx4Ty,
                                    llvm::CmpInst::Predicate::FCMP_OGE,
                                    Context.Int32x4Ty);

    case OpCode::F64x2__eq:
      return compileVectorCompareOp(Context.Doublex2Ty,
                                    llvm::CmpInst::Predicate::FCMP_OEQ,
                                    Context.Int64x2Ty);
    case OpCode::F64x2__ne:
      return compileVectorCompareOp(Context.Doublex2Ty,
                                    llvm::CmpInst::Predicate::FCMP_UNE,
                                    Context.Int64x2Ty);
    case OpCode::F64x2__lt:
      return compileVectorCompareOp(Context.Doublex2Ty,
                                    llvm::CmpInst::Predicate::FCMP_OLT,
                                    Context.Int64x2Ty);
    case OpCode::F64x2__gt:
      return compileVectorCompareOp(Context.Doublex2Ty,
                                    llvm::CmpInst::Predicate::FCMP_OGT,
                                    Context.Int64x2Ty);
    case OpCode::F64x2__le:
      return compileVectorCompareOp(Context.Doublex2Ty,
                                    llvm::CmpInst::Predicate::FCMP_OLE,
                                    Context.Int64x2Ty);
    case OpCode::F64x2__ge:
      return compileVectorCompareOp(Context.Doublex2Ty,
                                    llvm::CmpInst::Predicate::FCMP_OGE,
                                    Context.Int64x2Ty);

    case OpCode::V128__not:
      Stack.back() = Builder.CreateNot(Stack.back());
      return {};
    case OpCode::V128__and: {
      auto *RHS = stackPop();
      auto *LHS = stackPop();
      stackPush(Builder.CreateAnd(LHS, RHS));
      return {};
    }
    case OpCode::V128__andnot: {
      auto *RHS = stackPop();
      auto *LHS = stackPop();
      stackPush(Builder.CreateAnd(LHS, Builder.CreateNot(RHS)));
      return {};
    }
    case OpCode::V128__or: {
      auto *RHS = stackPop();
      auto *LHS = stackPop();
      stackPush(Builder.CreateOr(LHS, RHS));
      return {};
    }
    case OpCode::V128__xor: {
      auto *RHS = stackPop();
      auto *LHS = stackPop();
      stackPush(Builder.CreateXor(LHS, RHS));
      return {};
    }
    case OpCode::V128__bitselect: {
      auto *C = stackPop();
      auto *V2 = stackPop();
      auto *V1 = stackPop();
      stackPush(Builder.CreateXor(
          Builder.CreateAnd(Builder.CreateXor(V1, V2), C), V2));
      return {};
    }

    case OpCode::I8x16__abs:
      return compileVectorAbs(Context.Int8x16Ty);
    case OpCode::I8x16__neg:
      return compileVectorNeg(Context.Int8x16Ty);
    case OpCode::I8x16__any_true:
      return compileVectorAnyTrue(Context.Int8x16Ty);
    case OpCode::I8x16__all_true:
      return compileVectorAllTrue(Context.Int8x16Ty);
    case OpCode::I8x16__bitmask:
      return compileVectorBitMask(Context.Int8x16Ty);
    case OpCode::I8x16__narrow_i16x8_s:
      return compileVectorNarrow(Context.Int16x8Ty, true);
    case OpCode::I8x16__narrow_i16x8_u:
      return compileVectorNarrow(Context.Int16x8Ty, false);
    case OpCode::I8x16__shl:
      return compileVectorShl(Context.Int8x16Ty);
    case OpCode::I8x16__shr_s:
      return compileVectorAShr(Context.Int8x16Ty);
    case OpCode::I8x16__shr_u:
      return compileVectorLShr(Context.Int8x16Ty);
    case OpCode::I8x16__add:
      return compileVectorVectorAdd(Context.Int8x16Ty);
    case OpCode::I8x16__add_sat_s:
      return compileVectorVectorAddSat(Context.Int8x16Ty, true);
    case OpCode::I8x16__add_sat_u:
      return compileVectorVectorAddSat(Context.Int8x16Ty, false);
    case OpCode::I8x16__sub:
      return compileVectorVectorSub(Context.Int8x16Ty);
    case OpCode::I8x16__sub_sat_s:
      return compileVectorVectorSubSat(Context.Int8x16Ty, true);
    case OpCode::I8x16__sub_sat_u:
      return compileVectorVectorSubSat(Context.Int8x16Ty, false);
    case OpCode::I8x16__mul:
      return compileVectorVectorMul(Context.Int8x16Ty);
    case OpCode::I8x16__min_s:
      return compileVectorVectorSMin(Context.Int8x16Ty);
    case OpCode::I8x16__min_u:
      return compileVectorVectorUMin(Context.Int8x16Ty);
    case OpCode::I8x16__max_s:
      return compileVectorVectorSMax(Context.Int8x16Ty);
    case OpCode::I8x16__max_u:
      return compileVectorVectorUMax(Context.Int8x16Ty);
    case OpCode::I8x16__avgr_u:
      return compileVectorVectorUAvgr(Context.Int8x16Ty);

    case OpCode::I16x8__abs:
      return compileVectorAbs(Context.Int16x8Ty);
    case OpCode::I16x8__neg:
      return compileVectorNeg(Context.Int16x8Ty);
    case OpCode::I16x8__any_true:
      return compileVectorAnyTrue(Context.Int16x8Ty);
    case OpCode::I16x8__all_true:
      return compileVectorAllTrue(Context.Int16x8Ty);
    case OpCode::I16x8__bitmask:
      return compileVectorBitMask(Context.Int16x8Ty);
    case OpCode::I16x8__narrow_i32x4_s:
      return compileVectorNarrow(Context.Int32x4Ty, true);
    case OpCode::I16x8__narrow_i32x4_u:
      return compileVectorNarrow(Context.Int32x4Ty, false);
    case OpCode::I16x8__widen_low_i8x16_s:
      return compileVectorWiden(Context.Int8x16Ty, true, true);
    case OpCode::I16x8__widen_high_i8x16_s:
      return compileVectorWiden(Context.Int8x16Ty, true, false);
    case OpCode::I16x8__widen_low_i8x16_u:
      return compileVectorWiden(Context.Int8x16Ty, false, true);
    case OpCode::I16x8__widen_high_i8x16_u:
      return compileVectorWiden(Context.Int8x16Ty, false, false);
    case OpCode::I16x8__shl:
      return compileVectorShl(Context.Int16x8Ty);
    case OpCode::I16x8__shr_s:
      return compileVectorAShr(Context.Int16x8Ty);
    case OpCode::I16x8__shr_u:
      return compileVectorLShr(Context.Int16x8Ty);
    case OpCode::I16x8__add:
      return compileVectorVectorAdd(Context.Int16x8Ty);
    case OpCode::I16x8__add_sat_s:
      return compileVectorVectorAddSat(Context.Int16x8Ty, true);
    case OpCode::I16x8__add_sat_u:
      return compileVectorVectorAddSat(Context.Int16x8Ty, false);
    case OpCode::I16x8__sub:
      return compileVectorVectorSub(Context.Int16x8Ty);
    case OpCode::I16x8__sub_sat_s:
      return compileVectorVectorSubSat(Context.Int16x8Ty, true);
    case OpCode::I16x8__sub_sat_u:
      return compileVectorVectorSubSat(Context.Int16x8Ty, false);
    case OpCode::I16x8__mul:
      return compileVectorVectorMul(Context.Int16x8Ty);
    case OpCode::I16x8__min_s:
      return compileVectorVectorSMin(Context.Int16x8Ty);
    case OpCode::I16x8__min_u:
      return compileVectorVectorUMin(Context.Int16x8Ty);
    case OpCode::I16x8__max_s:
      return compileVectorVectorSMax(Context.Int16x8Ty);
    case OpCode::I16x8__max_u:
      return compileVectorVectorUMax(Context.Int16x8Ty);
    case OpCode::I16x8__avgr_u:
      return compileVectorVectorUAvgr(Context.Int16x8Ty);

    case OpCode::I32x4__abs:
      return compileVectorAbs(Context.Int32x4Ty);
    case OpCode::I32x4__neg:
      return compileVectorNeg(Context.Int32x4Ty);
    case OpCode::I32x4__any_true:
      return compileVectorAnyTrue(Context.Int32x4Ty);
    case OpCode::I32x4__all_true:
      return compileVectorAllTrue(Context.Int32x4Ty);
    case OpCode::I32x4__bitmask:
      return compileVectorBitMask(Context.Int32x4Ty);
    case OpCode::I32x4__widen_low_i16x8_s:
      return compileVectorWiden(Context.Int16x8Ty, true, true);
    case OpCode::I32x4__widen_high_i16x8_s:
      return compileVectorWiden(Context.Int16x8Ty, true, false);
    case OpCode::I32x4__widen_low_i16x8_u:
      return compileVectorWiden(Context.Int16x8Ty, false, true);
    case OpCode::I32x4__widen_high_i16x8_u:
      return compileVectorWiden(Context.Int16x8Ty, false, false);
    case OpCode::I32x4__shl:
      return compileVectorShl(Context.Int32x4Ty);
    case OpCode::I32x4__shr_s:
      return compileVectorAShr(Context.Int32x4Ty);
    case OpCode::I32x4__shr_u:
      return compileVectorLShr(Context.Int32x4Ty);
    case OpCode::I32x4__add:
      return compileVectorVectorAdd(Context.Int32x4Ty);
    case OpCode::I32x4__sub:
      return compileVectorVectorSub(Context.Int32x4Ty);
    case OpCode::I32x4__mul:
      return compileVectorVectorMul(Context.Int32x4Ty);
    case OpCode::I32x4__min_s:
      return compileVectorVectorSMin(Context.Int32x4Ty);
    case OpCode::I32x4__min_u:
      return compileVectorVectorUMin(Context.Int32x4Ty);
    case OpCode::I32x4__max_s:
      return compileVectorVectorSMax(Context.Int32x4Ty);
    case OpCode::I32x4__max_u:
      return compileVectorVectorUMax(Context.Int32x4Ty);
    case OpCode::I32x4__dot_i16x8_s: {
      /// XXX: Not in testsuite yet
      auto *ExtendTy =
          llvm::VectorType::getExtendedElementVectorType(Context.Int16x8Ty);
      auto *Undef = llvm::UndefValue::get(ExtendTy);
      auto *LHS = Builder.CreateSExt(
          Builder.CreateBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
      auto *RHS = Builder.CreateSExt(
          Builder.CreateBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
      auto *M = Builder.CreateMul(LHS, RHS);
      auto *L = Builder.CreateShuffleVector(
          M, Undef, std::array<ShuffleElement, 4>{0, 2, 4, 6});
      auto *R = Builder.CreateShuffleVector(
          M, Undef, std::array<ShuffleElement, 4>{1, 3, 5, 7});
      auto *V = Builder.CreateAdd(L, R);
      stackPush(Builder.CreateBitCast(V, Context.Int64x2Ty));
      return {};
    }

    case OpCode::I64x2__neg:
      return compileVectorNeg(Context.Int64x2Ty);
    case OpCode::I64x2__any_true:
      return compileVectorAnyTrue(Context.Int64x2Ty);
    case OpCode::I64x2__all_true:
      return compileVectorAllTrue(Context.Int64x2Ty);
    case OpCode::I64x2__shl:
      return compileVectorShl(Context.Int64x2Ty);
    case OpCode::I64x2__shr_s:
      return compileVectorAShr(Context.Int64x2Ty);
    case OpCode::I64x2__shr_u:
      return compileVectorLShr(Context.Int64x2Ty);
    case OpCode::I64x2__add:
      return compileVectorVectorAdd(Context.Int64x2Ty);
    case OpCode::I64x2__sub:
      return compileVectorVectorSub(Context.Int64x2Ty);
    case OpCode::I64x2__mul:
      return compileVectorVectorMul(Context.Int64x2Ty);

    case OpCode::F32x4__abs:
      return compileVectorFAbs(Context.Floatx4Ty);
    case OpCode::F32x4__neg:
      return compileVectorFNeg(Context.Floatx4Ty);
    case OpCode::F32x4__sqrt:
      return compileVectorFSqrt(Context.Floatx4Ty);
    case OpCode::F32x4__add:
      return compileVectorVectorFAdd(Context.Floatx4Ty);
    case OpCode::F32x4__sub:
      return compileVectorVectorFSub(Context.Floatx4Ty);
    case OpCode::F32x4__mul:
      return compileVectorVectorFMul(Context.Floatx4Ty);
    case OpCode::F32x4__div:
      return compileVectorVectorFDiv(Context.Floatx4Ty);
    case OpCode::F32x4__min:
      return compileVectorVectorFMin(Context.Floatx4Ty);
    case OpCode::F32x4__max:
      return compileVectorVectorFMax(Context.Floatx4Ty);
    case OpCode::F32x4__pmin:
      return compileVectorVectorFPMin(Context.Floatx4Ty);
    case OpCode::F32x4__pmax:
      return compileVectorVectorFPMax(Context.Floatx4Ty);
    case OpCode::F32x4__qfma:
      return compileVectorVectorVectorQFMA(Context.Floatx4Ty);
    case OpCode::F32x4__qfms:
      return compileVectorVectorVectorQFMS(Context.Floatx4Ty);
    case OpCode::F32x4__ceil:
      return compileVectorFCeil(Context.Floatx4Ty);
    case OpCode::F32x4__floor:
      return compileVectorFFloor(Context.Floatx4Ty);
    case OpCode::F32x4__trunc:
      return compileVectorFTrunc(Context.Floatx4Ty);
    case OpCode::F32x4__nearest:
      return compileVectorFNearest(Context.Floatx4Ty);

    case OpCode::F64x2__abs:
      return compileVectorFAbs(Context.Doublex2Ty);
    case OpCode::F64x2__neg:
      return compileVectorFNeg(Context.Doublex2Ty);
    case OpCode::F64x2__sqrt:
      return compileVectorFSqrt(Context.Doublex2Ty);
    case OpCode::F64x2__add:
      return compileVectorVectorFAdd(Context.Doublex2Ty);
    case OpCode::F64x2__sub:
      return compileVectorVectorFSub(Context.Doublex2Ty);
    case OpCode::F64x2__mul:
      return compileVectorVectorFMul(Context.Doublex2Ty);
    case OpCode::F64x2__div:
      return compileVectorVectorFDiv(Context.Doublex2Ty);
    case OpCode::F64x2__min:
      return compileVectorVectorFMin(Context.Doublex2Ty);
    case OpCode::F64x2__max:
      return compileVectorVectorFMax(Context.Doublex2Ty);
    case OpCode::F64x2__pmin:
      return compileVectorVectorFPMin(Context.Doublex2Ty);
    case OpCode::F64x2__pmax:
      return compileVectorVectorFPMax(Context.Doublex2Ty);
    case OpCode::F64x2__qfma:
      return compileVectorVectorVectorQFMA(Context.Doublex2Ty);
    case OpCode::F64x2__qfms:
      return compileVectorVectorVectorQFMS(Context.Doublex2Ty);
    case OpCode::F64x2__ceil:
      return compileVectorFCeil(Context.Doublex2Ty);
    case OpCode::F64x2__floor:
      return compileVectorFFloor(Context.Doublex2Ty);
    case OpCode::F64x2__trunc:
      return compileVectorFTrunc(Context.Doublex2Ty);
    case OpCode::F64x2__nearest:
      return compileVectorFNearest(Context.Doublex2Ty);

    case OpCode::I32x4__trunc_sat_f32x4_s:
      return compileVectorTruncSatS(Context.Floatx4Ty, 32);
    case OpCode::I32x4__trunc_sat_f32x4_u:
      return compileVectorTruncSatU(Context.Floatx4Ty, 32);
    case OpCode::F32x4__convert_i32x4_s:
      return compileVectorConvertS(Context.Int32x4Ty, Context.Floatx4Ty);
    case OpCode::F32x4__convert_i32x4_u:
      return compileVectorConvertU(Context.Int32x4Ty, Context.Floatx4Ty);

    case OpCode::I64x2__trunc_sat_f64x2_s:
      return compileVectorTruncSatS(Context.Doublex2Ty, 64);
    case OpCode::I64x2__trunc_sat_f64x2_u:
      return compileVectorTruncSatU(Context.Doublex2Ty, 64);
    case OpCode::F64x2__convert_i64x2_s:
      return compileVectorConvertS(Context.Int64x2Ty, Context.Doublex2Ty);
    case OpCode::F64x2__convert_i64x2_u:
      return compileVectorConvertU(Context.Int64x2Ty, Context.Doublex2Ty);
    default:
      __builtin_unreachable();
    }
    return {};
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
                         getTrapBB(ErrCode::InvalidConvToInt));

    Builder.SetInsertPoint(NormBB);
    auto *Trunc = Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Value);
    auto *IsNotUnderflow =
        createLikely(Builder, Builder.CreateFCmpOGE(Trunc, MinFp));
    Builder.CreateCondBr(IsNotUnderflow, NotMinBB,
                         getTrapBB(ErrCode::IntegerOverflow));

    Builder.SetInsertPoint(NotMinBB);
    auto *IsNotOverflow = createLikely(
        Builder,
        Builder.CreateFCmp(Precise ? llvm::CmpInst::Predicate::FCMP_OLE
                                   : llvm::CmpInst::Predicate::FCMP_OLT,
                           Trunc, MaxFp));
    Builder.CreateCondBr(IsNotOverflow, NotMaxBB,
                         getTrapBB(ErrCode::IntegerOverflow));

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
                         getTrapBB(ErrCode::InvalidConvToInt));

    Builder.SetInsertPoint(NormBB);
    auto *Trunc = Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Value);
    auto *IsNotUnderflow =
        createLikely(Builder, Builder.CreateFCmpOGE(Trunc, MinFp));
    Builder.CreateCondBr(IsNotUnderflow, NotMinBB,
                         getTrapBB(ErrCode::IntegerOverflow));

    Builder.SetInsertPoint(NotMinBB);
    auto *IsNotOverflow = createLikely(
        Builder,
        Builder.CreateFCmp(Precise ? llvm::CmpInst::Predicate::FCMP_OLE
                                   : llvm::CmpInst::Predicate::FCMP_OLT,
                           Trunc, MaxFp));
    Builder.CreateCondBr(IsNotOverflow, NotMaxBB,
                         getTrapBB(ErrCode::IntegerOverflow));

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

  void compileReturn() {
    updateInstrCount();
    writeGas();
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
      auto *Ptr = Context.getInstrCount(Builder, ExecCtx);
      Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(LocalInstrCount),
                                            Builder.CreateLoad(Ptr)),
                          Ptr);
      Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
    }
  }

  void readGas() {
    if (LocalGas) {
      Builder.CreateStore(Builder.CreateLoad(Context.getGas(Builder, ExecCtx)),
                          LocalGas);
    }
  }

  void writeGas() {
    if (LocalGas) {
      Builder.CreateStore(Builder.CreateLoad(LocalGas),
                          Context.getGas(Builder, ExecCtx));
    }
  }

private:
  Expect<void> compileCallOp(const unsigned int FuncIndex) {
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

    readGas();
    return {};
  }

  Expect<void> compileIndirectCallOp(const uint32_t TableIndex,
                                     const uint32_t FuncTypeIndex) {
    llvm::Value *FuncIndex = stackPop();
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    auto *FTy = toLLVMType(Context.ExecCtxPtrTy, FuncType);
    auto *RTy = FTy->getReturnType();

    const auto ArgSize = FuncType.getParamTypes().size();
    const auto RetSize = RTy->isVoidTy() ? 0 : FuncType.getReturnTypes().size();

    llvm::Value *Args;
    if (ArgSize == 0) {
      Args = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
    } else {
      auto *Alloca = Builder.CreateAlloca(Builder.getInt8Ty(),
                                          Builder.getInt64(ArgSize * kValSize));
      Alloca->setAlignment(Align(kValSize));
      Args = Alloca;
    }

    llvm::Value *Rets;
    if (RetSize == 0) {
      Rets = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
    } else {
      auto *Alloca = Builder.CreateAlloca(Builder.getInt8Ty(),
                                          Builder.getInt64(RetSize * kValSize));
      Alloca->setAlignment(Align(kValSize));
      Rets = Alloca;
    }

    for (unsigned I = 0; I < ArgSize; ++I) {
      const unsigned J = ArgSize - 1 - I;
      auto *Arg = stackPop();
      auto *Ptr = Builder.CreateConstInBoundsGEP1_64(Args, J * kValSize);
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
      auto *VPtr = Builder.CreateConstInBoundsGEP1_64(Rets, 0);
      auto *Ptr = Builder.CreateBitCast(VPtr, RTy->getPointerTo());
      stackPush(Builder.CreateLoad(Ptr));
    } else {
      for (unsigned I = 0; I < RetSize; ++I) {
        auto *VPtr = Builder.CreateConstInBoundsGEP1_64(Rets, I * kValSize);
        auto *Ptr = Builder.CreateBitCast(
            VPtr, RTy->getStructElementType(I)->getPointerTo());
        stackPush(Builder.CreateLoad(Ptr));
      }
    }

    readGas();
    return {};
  }

  Expect<void> compileLoadOp(unsigned Offset, unsigned Alignment,
                             llvm::Type *LoadTy) {
    if constexpr (kForceUnalignment) {
      Alignment = 0;
    }
    auto *Off = Builder.CreateZExt(stackPop(), Context.Int64Ty);
    if (Offset != 0) {
      Off = Builder.CreateAdd(Off, Builder.getInt64(Offset));
    }

    auto *VPtr =
        Builder.CreateInBoundsGEP(Context.getMemory(Builder, ExecCtx), {Off});
    auto *Ptr = Builder.CreateBitCast(VPtr, LoadTy->getPointerTo());
    auto *LoadInst = Builder.CreateLoad(Ptr, OptNone);
    LoadInst->setAlignment(Align(UINT64_C(1) << Alignment));
    stackPush(LoadInst);
    return {};
  }
  Expect<void> compileLoadOp(unsigned Offset, unsigned Alignment,
                             llvm::Type *LoadTy, llvm::Type *ExtendTy,
                             bool Signed) {
    if (auto Ret = compileLoadOp(Offset, Alignment, LoadTy); !Ret) {
      return Unexpect(Ret);
    }
    if (Signed) {
      Stack.back() = Builder.CreateSExt(Stack.back(), ExtendTy);
    } else {
      Stack.back() = Builder.CreateZExt(Stack.back(), ExtendTy);
    }
    return {};
  }
  Expect<void> compileVectorLoadOp(unsigned Offset, unsigned Alignment,
                                   llvm::Type *LoadTy) {
    if (auto Ret = compileLoadOp(Offset, Alignment, LoadTy); !Ret) {
      return Unexpect(Ret);
    }
    Stack.back() = Builder.CreateBitCast(Stack.back(), Context.Int64x2Ty);
    return {};
  }
  Expect<void> compileVectorLoadOp(unsigned Offset, unsigned Alignment,
                                   llvm::Type *LoadTy, llvm::Type *ExtendTy,
                                   bool Signed) {
    if (auto Ret = compileLoadOp(Offset, Alignment, LoadTy, ExtendTy, Signed);
        !Ret) {
      return Unexpect(Ret);
    }
    Stack.back() = Builder.CreateBitCast(Stack.back(), Context.Int64x2Ty);
    return {};
  }
  Expect<void> compileSplatLoadOp(unsigned Offset, unsigned Alignment,
                                  llvm::Type *LoadTy,
                                  llvm::VectorType *VectorTy) {
    if (auto Ret = compileLoadOp(Offset, Alignment, LoadTy); !Ret) {
      return Unexpect(Ret);
    }
    return compileSplatOp(VectorTy);
  }
  Expect<void> compileStoreOp(unsigned Offset, unsigned Alignment,
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
    auto *VPtr =
        Builder.CreateInBoundsGEP(Context.getMemory(Builder, ExecCtx), {Off});
    auto *Ptr = Builder.CreateBitCast(VPtr, LoadTy->getPointerTo());
    auto *StoreInst = Builder.CreateStore(V, Ptr, OptNone);
    StoreInst->setAlignment(Align(UINT64_C(1) << Alignment));
    return {};
  }
  Expect<void> compileSplatOp(llvm::VectorType *VectorTy) {
    const uint32_t kZero = 0;
    auto *Undef = llvm::UndefValue::get(VectorTy);
    auto *Zeros = llvm::ConstantAggregateZero::get(llvm::VectorType::get(
        Context.Int32Ty, VectorTy->getElementCount().Min, false));
    auto *Value = Builder.CreateTrunc(Stack.back(), VectorTy->getElementType());
    auto *Vector = Builder.CreateInsertElement(Undef, Value, kZero);
    Vector = Builder.CreateShuffleVector(Vector, Undef, Zeros);

    Stack.back() = Builder.CreateBitCast(Vector, Context.Int64x2Ty);
    return {};
  }
  Expect<void> compileExtractLaneOp(llvm::VectorType *VectorTy,
                                    unsigned Index) {
    auto *Vector = Builder.CreateBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.CreateExtractElement(Vector, Index);
    return {};
  }
  Expect<void> compileExtractLaneOp(llvm::VectorType *VectorTy, unsigned Index,
                                    llvm::Type *ExtendTy, bool Signed) {
    if (auto Ret = compileExtractLaneOp(VectorTy, Index); !Ret) {
      return Unexpect(Ret);
    }
    if (Signed) {
      Stack.back() = Builder.CreateSExt(Stack.back(), ExtendTy);
    } else {
      Stack.back() = Builder.CreateZExt(Stack.back(), ExtendTy);
    }
    return {};
  }
  Expect<void> compileReplaceLaneOp(llvm::VectorType *VectorTy,
                                    unsigned Index) {
    auto *Value = Builder.CreateTrunc(stackPop(), VectorTy->getElementType());
    auto *Vector = Stack.back();
    Stack.back() = Builder.CreateBitCast(
        Builder.CreateInsertElement(Builder.CreateBitCast(Vector, VectorTy),
                                    Value, Index),
        Context.Int64x2Ty);
    return {};
  }
  Expect<void> compileVectorCompareOp(llvm::VectorType *VectorTy,
                                      llvm::CmpInst::Predicate Predicate) {
    auto *RHS = stackPop();
    auto *LHS = stackPop();
    auto *Result = Builder.CreateSExt(
        Builder.CreateICmp(Predicate, Builder.CreateBitCast(LHS, VectorTy),
                           Builder.CreateBitCast(RHS, VectorTy)),
        VectorTy);
    stackPush(Builder.CreateBitCast(Result, Context.Int64x2Ty));
    return {};
  }
  Expect<void> compileVectorCompareOp(llvm::VectorType *VectorTy,
                                      llvm::CmpInst::Predicate Predicate,
                                      llvm::VectorType *ResultTy) {
    auto *RHS = stackPop();
    auto *LHS = stackPop();
    auto *Result = Builder.CreateSExt(
        Builder.CreateFCmp(Predicate, Builder.CreateBitCast(LHS, VectorTy),
                           Builder.CreateBitCast(RHS, VectorTy)),
        ResultTy);
    stackPush(Builder.CreateBitCast(Result, Context.Int64x2Ty));
    return {};
  }
  template <typename Func>
  Expect<void> compileVectorOp(llvm::VectorType *VectorTy, Func &&Op) {
    auto *V = Builder.CreateBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.CreateBitCast(Op(V), Context.Int64x2Ty);
    return {};
  }
  Expect<void> compileVectorAbs(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy, [this, VectorTy](auto *V) {
      auto *Zero = llvm::ConstantAggregateZero::get(VectorTy);
      auto *C = Builder.CreateICmpSLT(V, Zero);
      return Builder.CreateSelect(C, Builder.CreateNeg(V), V);
    });
  }
  Expect<void> compileVectorNeg(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy,
                           [this](auto *V) { return Builder.CreateNeg(V); });
  }
  template <typename Func>
  Expect<void> compileVectorReduceIOp(llvm::VectorType *VectorTy, Func &&Op) {
    auto *V = Builder.CreateBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.CreateZExt(Op(V), Context.Int32Ty);
    return {};
  }
  Expect<void> compileVectorAnyTrue(llvm::VectorType *VectorTy) {
    return compileVectorReduceIOp(VectorTy, [this, VectorTy](auto *V) {
      const auto Size = VectorTy->getElementCount().Min;
      auto *IntType = Builder.getIntNTy(Size);
      auto *Zero = llvm::ConstantAggregateZero::get(VectorTy);
      auto *Cmp = Builder.CreateBitCast(Builder.CreateICmpNE(V, Zero), IntType);
      auto *CmpZero = llvm::ConstantInt::get(IntType, 0);
      return Builder.CreateICmpNE(Cmp, CmpZero);
    });
  }
  Expect<void> compileVectorAllTrue(llvm::VectorType *VectorTy) {
    return compileVectorReduceIOp(VectorTy, [this, VectorTy](auto *V) {
      const auto Size = VectorTy->getElementCount().Min;
      auto *IntType = Builder.getIntNTy(Size);
      auto *Zero = llvm::ConstantAggregateZero::get(VectorTy);
      auto *Cmp = Builder.CreateBitCast(Builder.CreateICmpEQ(V, Zero), IntType);
      auto *CmpZero = llvm::ConstantInt::get(IntType, 0);
      return Builder.CreateICmpEQ(Cmp, CmpZero);
    });
  }
  Expect<void> compileVectorBitMask(llvm::VectorType *VectorTy) {
    return compileVectorReduceIOp(VectorTy, [this, VectorTy](auto *V) {
      const auto Size = VectorTy->getElementCount().Min;
      auto *IntType = Builder.getIntNTy(Size);
      auto *Zero = llvm::ConstantAggregateZero::get(VectorTy);
      return Builder.CreateBitCast(Builder.CreateICmpSLT(V, Zero), IntType);
    });
  }
  template <typename Func>
  Expect<void> compileVectorShiftOp(llvm::VectorType *VectorTy, Func &&Op) {
    const uint64_t Mask = VectorTy->getElementType()->getIntegerBitWidth() - 1;
    auto *N = Builder.CreateAnd(stackPop(), Builder.getInt32(Mask));
    auto *RHS = Builder.CreateVectorSplat(
        VectorTy->getElementCount().Min,
        Builder.CreateZExtOrTrunc(N, VectorTy->getElementType()));
    auto *LHS = Builder.CreateBitCast(stackPop(), VectorTy);
    stackPush(Builder.CreateBitCast(Op(LHS, RHS), Context.Int64x2Ty));
    return {};
  }
  Expect<void> compileVectorShl(llvm::VectorType *VectorTy) {
    return compileVectorShiftOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateShl(LHS, RHS);
    });
  }
  Expect<void> compileVectorLShr(llvm::VectorType *VectorTy) {
    return compileVectorShiftOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateLShr(LHS, RHS);
    });
  }
  Expect<void> compileVectorAShr(llvm::VectorType *VectorTy) {
    return compileVectorShiftOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateAShr(LHS, RHS);
    });
  }
  template <typename Func>
  Expect<void> compileVectorVectorOp(llvm::VectorType *VectorTy, Func &&Op) {
    auto *RHS = Builder.CreateBitCast(stackPop(), VectorTy);
    auto *LHS = Builder.CreateBitCast(stackPop(), VectorTy);
    stackPush(Builder.CreateBitCast(Op(LHS, RHS), Context.Int64x2Ty));
    return {};
  }
  Expect<void> compileVectorVectorAdd(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateAdd(LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorAddSat(llvm::VectorType *VectorTy,
                                         bool Signed) {
    auto ID = Signed ? llvm::Intrinsic::sadd_sat : llvm::Intrinsic::uadd_sat;
    return compileVectorVectorOp(VectorTy, [this, ID](auto *LHS, auto *RHS) {
      return Builder.CreateBinaryIntrinsic(ID, LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorSub(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateSub(LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorSubSat(llvm::VectorType *VectorTy,
                                         bool Signed) {
    auto ID = Signed ? llvm::Intrinsic::ssub_sat : llvm::Intrinsic::usub_sat;
    return compileVectorVectorOp(VectorTy, [this, ID](auto *LHS, auto *RHS) {
      return Builder.CreateBinaryIntrinsic(ID, LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorMul(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateMul(LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorSMin(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpSLE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorUMin(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpULE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorSMax(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpSGE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorUMax(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *C = Builder.CreateICmpUGE(LHS, RHS);
      return Builder.CreateSelect(C, LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorUAvgr(llvm::VectorType *VectorTy) {
    auto *ExtendTy = VectorTy->getExtendedElementVectorType(VectorTy);
    return compileVectorVectorOp(VectorTy, [this, VectorTy,
                                            ExtendTy](auto *LHS, auto *RHS) {
      auto *EL = Builder.CreateZExt(LHS, ExtendTy);
      auto *ER = Builder.CreateZExt(RHS, ExtendTy);
      auto *One = Builder.CreateZExt(
          Builder.CreateVectorSplat(ExtendTy->getElementCount().Min,
                                    Builder.getTrue()),
          ExtendTy);
      return Builder.CreateTrunc(
          Builder.CreateLShr(Builder.CreateAdd(Builder.CreateAdd(EL, ER), One),
                             One),
          VectorTy);
    });
  }
  Expect<void> compileVectorNarrow(llvm::VectorType *FromTy, bool Signed) {
    const auto IntWidth = FromTy->getElementType()->getIntegerBitWidth();
    auto MinInt = Signed ? llvm::APInt::getSignedMinValue(IntWidth / 2)
                         : llvm::APInt::getMinValue(IntWidth / 2);
    MinInt = Signed ? MinInt.sext(IntWidth) : MinInt.zext(IntWidth);
    auto MaxInt = Signed ? llvm::APInt::getSignedMaxValue(IntWidth / 2)
                         : llvm::APInt::getMaxValue(IntWidth / 2);
    MaxInt = Signed ? MaxInt.sext(IntWidth) : MaxInt.zext(IntWidth);

    const auto Count = FromTy->getElementCount().Min;
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
    return {};
  }
  Expect<void> compileVectorWiden(llvm::VectorType *FromTy, bool Signed,
                                  bool Low) {
    auto *ExtTy = llvm::VectorType::getExtendedElementVectorType(FromTy);
    const auto Count = FromTy->getElementCount().Min;
    std::vector<ShuffleElement> Mask(Count / 2);
    std::iota(Mask.begin(), Mask.end(), Low ? 0 : Count / 2);
    auto *F = Builder.CreateBitCast(Stack.back(), FromTy);
    if (Signed) {
      F = Builder.CreateSExt(F, ExtTy);
    } else {
      F = Builder.CreateZExt(F, ExtTy);
    }
    F = Builder.CreateShuffleVector(F, llvm::UndefValue::get(ExtTy), Mask);
    Stack.back() = Builder.CreateBitCast(F, Context.Int64x2Ty);
    return {};
  }
  Expect<void> compileVectorFAbs(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::fabs, V);
    });
  }
  Expect<void> compileVectorFNeg(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy,
                           [this](auto *V) { return Builder.CreateFNeg(V); });
  }
  Expect<void> compileVectorFSqrt(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, V);
    });
  }
  Expect<void> compileVectorFCeil(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ceil, V);
    });
  }
  Expect<void> compileVectorFFloor(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::floor, V);
    });
  }
  Expect<void> compileVectorFTrunc(llvm::VectorType *VectorTy) {
    return compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, V);
    });
  }
  Expect<void> compileVectorFNearest(llvm::VectorType *VectorTy) {
#if LLVM_VERSION_MAJOR >= 11
    return compileVectorOp(VectorTy, [this](auto *V) {
      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::roundeven, V);
    });
#else
    const bool IsFloat = VectorTy->getElementType() == Context.FloatTy;
    return compileVectorOp(VectorTy, [this, IsFloat](auto *V) {
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||            \
    defined(_M_X64)
      if (Context.SupportRoundeven) {
        auto ID = IsFloat ? llvm::Intrinsic::x86_sse41_round_ps
                          : llvm::Intrinsic::x86_sse41_round_pd;
        return Builder.CreateIntrinsic(ID, {}, {V, Builder.getInt32(8)});
      }
#endif

#if defined(__arm__) || defined(__aarch64__)
      if (Context.SupportRoundeven) {
        return Builder.CreateBinaryIntrinsic(
            llvm::Intrinsic::aarch64_neon_frintn, V, V);
      }
#endif

      return Builder.CreateUnaryIntrinsic(llvm::Intrinsic::nearbyint, V);
    });
#endif
  }
  Expect<void> compileVectorVectorFAdd(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFAdd(LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorFSub(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFSub(LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorFMul(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFMul(LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorFDiv(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      return Builder.CreateFDiv(LHS, RHS);
    });
  }
  Expect<void> compileVectorVectorFMin(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
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
  Expect<void> compileVectorVectorFMax(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
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
  Expect<void> compileVectorVectorFPMin(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *Cmp = Builder.CreateFCmpOLT(RHS, LHS);
      return Builder.CreateSelect(Cmp, RHS, LHS);
    });
  }
  Expect<void> compileVectorVectorFPMax(llvm::VectorType *VectorTy) {
    return compileVectorVectorOp(VectorTy, [this](auto *LHS, auto *RHS) {
      auto *Cmp = Builder.CreateFCmpOGT(RHS, LHS);
      return Builder.CreateSelect(Cmp, RHS, LHS);
    });
  }
  Expect<void> compileVectorTruncSatS(llvm::VectorType *VectorTy,
                                      unsigned IntWidth) {
    return compileVectorOp(VectorTy, [this, VectorTy, IntWidth](auto *V) {
      const auto Size = VectorTy->getElementCount().Min;
      auto *FPTy = VectorTy->getElementType();
      auto *IntZero = Builder.getIntN(IntWidth, 0);
      auto *IntMin = Builder.getInt(llvm::APInt::getSignedMinValue(IntWidth));
      auto *IntMax = Builder.getInt(llvm::APInt::getSignedMaxValue(IntWidth));
      auto *IntZeroV = Builder.CreateVectorSplat(Size, IntZero);
      auto *IntMinV = Builder.CreateVectorSplat(Size, IntMin);
      auto *IntMaxV = Builder.CreateVectorSplat(Size, IntMax);
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
      return V;
    });
  }
  Expect<void> compileVectorTruncSatU(llvm::VectorType *VectorTy,
                                      unsigned IntWidth) {
    return compileVectorOp(VectorTy, [this, VectorTy, IntWidth](auto *V) {
      const auto Size = VectorTy->getElementCount().Min;
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
      return V;
    });
  }
  Expect<void> compileVectorConvertS(llvm::VectorType *VectorTy,
                                     llvm::VectorType *FPVectorTy) {
    return compileVectorOp(VectorTy, [this, FPVectorTy](auto *V) {
      return Builder.CreateSIToFP(V, FPVectorTy);
    });
  }
  Expect<void> compileVectorConvertU(llvm::VectorType *VectorTy,
                                     llvm::VectorType *FPVectorTy) {
    return compileVectorOp(VectorTy, [this, FPVectorTy](auto *V) {
      return Builder.CreateUIToFP(V, FPVectorTy);
    });
  }
  template <typename Func>
  Expect<void> compileVectorVectorVectorOp(llvm::VectorType *VectorTy,
                                           Func &&Op) {
    auto *C = Builder.CreateBitCast(stackPop(), VectorTy);
    auto *B = Builder.CreateBitCast(stackPop(), VectorTy);
    auto *A = Builder.CreateBitCast(stackPop(), VectorTy);
    stackPush(Builder.CreateBitCast(Op(A, B, C), Context.Int64x2Ty));
    return {};
  }
  Expect<void> compileVectorVectorVectorQFMA(llvm::VectorType *VectorTy) {
    /// XXX: Not in testsuite yet
    return compileVectorVectorVectorOp(
        VectorTy, [this](auto *A, auto *B, auto *C) {
          return Builder.CreateIntrinsic(llvm::Intrinsic::fmuladd,
                                         {A->getType()}, {A, B, C});
        });
  }
  Expect<void> compileVectorVectorVectorQFMS(llvm::VectorType *VectorTy) {
    /// XXX: Not in testsuite yet
    return compileVectorVectorVectorOp(
        VectorTy, [this](auto *A, auto *B, auto *C) {
          C = Builder.CreateFNeg(C);
          return Builder.CreateIntrinsic(llvm::Intrinsic::fmuladd,
                                         {A->getType()}, {A, B, C});
        });
  }

  std::pair<std::vector<ValType>, std::vector<ValType>>
  resolveBlockType(const BlockType &ResultType) const {
    using VecT = std::vector<ValType>;
    using RetT = std::pair<VecT, VecT>;
    return std::visit(
        overloaded{[](const ValType &Type) -> RetT {
                     if (Type == ValType::None) {
                       return RetT{};
                     }
                     return RetT{{}, {Type}};
                   },
                   [this](const uint32_t &Index) -> RetT {
                     const auto &Type = *Context.FunctionTypes[Index];
                     return RetT{VecT(Type.getParamTypes().begin(),
                                      Type.getParamTypes().end()),
                                 VecT(Type.getReturnTypes().begin(),
                                      Type.getReturnTypes().end())};
                   }},
        ResultType);
  }

  void enterBlock(
      llvm::BasicBlock *JumpTarget, const BlockType &ResultType,
      std::optional<std::vector<llvm::PHINode *>> PHIArgs = std::nullopt) {
    auto Type = resolveBlockType(ResultType);
    ControlStack.emplace_back(Stack.size() - Type.first.size(), JumpTarget,
                              std::move(Type.second), 0, std::move(PHIArgs));
  }

  std::vector<std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>
  leaveBlock(llvm::BasicBlock *NextTarget) {
    auto &Entry = ControlStack.back();
    if (!isUnreachable()) {
      if (const auto &Types = std::get<kReturnType>(Entry); !Types.empty()) {
        std::vector<llvm::Value *> Rets(Types.size());
        for (size_t I = 0; I < Types.size(); ++I) {
          const size_t J = Types.size() - 1 - I;
          Rets[J] = stackPop();
        }
        std::get<kReturnPHI>(Entry).emplace_back(std::move(Rets),
                                                 Builder.GetInsertBlock());
      }
      Builder.CreateBr(NextTarget);
    }
    Builder.SetInsertPoint(NextTarget);

    Stack.erase(Stack.begin() + std::get<kStackSize>(Entry), Stack.end());
    std::vector<std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>
        Result = std::move(std::get<kReturnPHI>(Entry));

    ControlStack.pop_back();
    clearUnreachable();
    return Result;
  }

  void setUnreachable() { IsUnreachable = true; }

  void clearUnreachable() { IsUnreachable = false; }

  bool isUnreachable() { return IsUnreachable; }

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
        auto *PHIRet = Builder.CreatePHI(Types[I], Incomings.size());
        for (auto &[Value, BB] : Incomings) {
          assert(Value.size() == Types.size());
          PHIRet->addIncoming(Value[I], BB);
        }
        Nodes.push_back(PHIRet);
      }
    }
    for (auto *Val : Nodes) {
      stackPush(Val);
    }
  }

  bool setLableJumpPHI(unsigned int Index) {
    if (Index >= ControlStack.size()) {
      return false;
    }
    auto &Entry = *(ControlStack.rbegin() + Index);
    if (auto &MaybePHIArgs = std::get<kPHIArgs>(Entry)) {
      auto &PHIArgs = *MaybePHIArgs;
      std::vector<llvm::Value *> Values(PHIArgs.size());
      for (size_t I = 0; I < Values.size(); ++I) {
        const size_t J = Values.size() - 1 - I;
        Values[J] = stackPop();
      }
      for (size_t I = 0; I < Values.size(); ++I) {
        PHIArgs[I]->addIncoming(Values[I], Builder.GetInsertBlock());
        stackPush(Values[I]);
      }
    } else if (const auto &Types = std::get<kReturnType>(Entry);
               !Types.empty()) {
      std::vector<llvm::Value *> Values(Types.size());
      for (size_t I = 0; I < Values.size(); ++I) {
        const size_t J = Values.size() - 1 - I;
        Values[J] = stackPop();
      }
      for (size_t I = 0; I < Values.size(); ++I) {
        stackPush(Values[I]);
      }
      std::get<kReturnPHI>(Entry).emplace_back(std::move(Values),
                                               Builder.GetInsertBlock());
    }
    return true;
  }

  llvm::BasicBlock *getLabel(unsigned int Index) const {
    return std::get<kJumpBlock>(*(ControlStack.rbegin() + Index));
  }

  void stackPush(llvm::Value *Value) { Stack.push_back(Value); }
  llvm::Value *stackPop() {
    assert(!ControlStack.empty() || !Stack.empty());
    assert(ControlStack.empty() ||
           Stack.size() > std::get<kStackSize>(ControlStack.back()));
    auto *Value = Stack.back();
    Stack.pop_back();
    return Value;
  }

  AOT::Compiler::CompileContext &Context;
  llvm::LLVMContext &LLContext;
  std::vector<llvm::Value *> Local;
  std::vector<llvm::Value *> Stack;
  llvm::Value *LocalInstrCount = nullptr;
  llvm::Value *LocalGas = nullptr;
  std::unordered_map<ErrCode, llvm::BasicBlock *> TrapBB;
  bool IsUnreachable = false;
  bool OptNone = false;
  static inline constexpr size_t kStackSize = 0;
  static inline constexpr size_t kJumpBlock = 1;
  static inline constexpr size_t kReturnType = 2;
  static inline constexpr size_t kReturnPHI = 3;
  static inline constexpr size_t kPHIArgs = 4;
  std::vector<std::tuple<
      size_t, llvm::BasicBlock *, std::vector<ValType>,
      std::vector<std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>,
      std::optional<std::vector<llvm::PHINode *>>>>
      ControlStack;
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

} // namespace

namespace SSVM {
namespace AOT {

Expect<void> Compiler::compile(Span<const Byte> Data, const AST::Module &Module,
                               std::string_view OutputPath) {
  namespace fs = std::filesystem;
  using namespace std::literals;

  LOG(INFO) << "compile start";
  const fs::path Path(fs::u8path(OutputPath));
  fs::path LLPath(Path);
  LLPath.replace_extension("ll"sv);
  std::filesystem::path OPath(Path);
  OPath.replace_extension("%%%%%%%%%%.o"sv);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  llvm::LLVMContext LLContext;
  auto LLModule = std::make_unique<llvm::Module>(LLPath.native(), LLContext);
  LLModule->setTargetTriple(llvm::sys::getProcessTriple());
  LLModule->setPICLevel(llvm::PICLevel::Level::SmallPIC);
  CompileContext NewContext(*LLModule);
  struct RAIICleanup {
    RAIICleanup(CompileContext *&Context, CompileContext &NewContext)
        : Context(Context) {
      Context = &NewContext;
    }
    ~RAIICleanup() { Context = nullptr; }
    CompileContext *&Context;
  };
  RAIICleanup Cleanup(Context, NewContext);

  return Expect<void>()
      .and_then([&]() -> Expect<void> {
        /// Compile Function Types
        if (const AST::TypeSection *TypeSec = Module.getTypeSection()) {
          return compile(*TypeSec);
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// Compile ImportSection
        if (const AST::ImportSection *ImportSec = Module.getImportSection()) {
          return compile(*ImportSec);
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// Compile GlobalSection
        if (const AST::GlobalSection *GlobSec = Module.getGlobalSection()) {
          return compile(*GlobSec);
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// Compile MemorySection (MemorySec, DataSec)
        if (const AST::MemorySection *MemSec = Module.getMemorySection()) {
          if (const AST::DataSection *DataSec = Module.getDataSection()) {
            return compile(*MemSec, *DataSec);
          }
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// Compile TableSection
        if (const AST::TableSection *TabSec = Module.getTableSection()) {
          if (const AST::ElementSection *ElemSec = Module.getElementSection()) {
            return compile(*TabSec, *ElemSec);
          }
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// compile Functions in module. (FunctionSec, CodeSec)
        if (const AST::FunctionSection *FuncSec = Module.getFunctionSection()) {
          if (const AST::CodeSection *CodeSec = Module.getCodeSection()) {
            return compile(*FuncSec, *CodeSec);
          }
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// Compile ExportSection
        if (const AST::ExportSection *ExportSec = Module.getExportSection()) {
          return compile(*ExportSec);
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// Compile StartSection (StartSec)
        if (Module.getStartSection()) {
          ;
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// create wasm.code and wasm.size
        {
          auto *Int32Ty = Context->Int32Ty;
          auto *Content = llvm::ConstantDataArray::getString(
              LLContext,
              llvm::StringRef(reinterpret_cast<const char *>(Data.data()),
                              Data.size()),
              false);
          new llvm::GlobalVariable(Context->LLModule, Content->getType(), false,
                                   llvm::GlobalValue::ExternalLinkage, Content,
                                   "wasm.code");
          new llvm::GlobalVariable(Context->LLModule, Int32Ty, false,
                                   llvm::GlobalValue::ExternalLinkage,
                                   llvm::ConstantInt::get(Int32Ty, Data.size()),
                                   "wasm.size");
        }

        if (DumpIR) {
          int Fd;
          llvm::sys::fs::openFileForWrite("wasm.ll", Fd);
          llvm::raw_fd_ostream OS(Fd, true);
          LLModule->print(OS, nullptr);
        }

        LOG(INFO) << "verify start";
        llvm::verifyModule(*LLModule, &llvm::errs());
        LOG(INFO) << "optimize start";

        // tempfile
        auto Object = llvm::sys::fs::TempFile::create(OPath.native());
        if (!Object) {
          // TODO:return error
          LOG(ERROR) << "so file creation failed:" << OPath.native();
          llvm::consumeError(Object.takeError());
          return {};
        }
        std::error_code EC;
        auto OS = std::make_unique<llvm::raw_fd_ostream>(Object->TmpName, EC);
        if (EC) {
          // TODO:return error
          LOG(ERROR) << "object file creation failed:" << Object->TmpName;
          llvm::consumeError(Object->discard());
          return {};
        }

        // optimize + codegen
        {
          std::string Error;
          std::string Triple = LLModule->getTargetTriple();
          const llvm::Target *TheTarget =
              llvm::TargetRegistry::lookupTarget(Triple, Error);
          if (!TheTarget) {
            // TODO:return error
            LOG(ERROR) << "lookupTarget failed";
            llvm::consumeError(Object->discard());
            return {};
          }

          llvm::TargetOptions Options;
          llvm::Reloc::Model RM = llvm::Reloc::PIC_;
          std::unique_ptr<llvm::TargetMachine> TM(
              TheTarget->createTargetMachine(
                  Triple, llvm::sys::getHostCPUName(),
                  Context->SubtargetFeatures.getString(), Options, RM,
                  llvm::None, llvm::CodeGenOpt::Level::Aggressive));
          LLModule->setDataLayout(TM->createDataLayout());

          llvm::TargetLibraryInfoImpl TLII(
              llvm::Triple(LLModule->getTargetTriple()));

          {
#if LLVM_VERSION_MAJOR >= 9
            llvm::PassBuilder PB(TM.get(), llvm::PipelineTuningOptions(),
                                 llvm::None);
#else
            llvm::PassBuilder PB(TM.get(), llvm::None);
#endif

            llvm::LoopAnalysisManager LAM(false);
            llvm::FunctionAnalysisManager FAM(false);
            llvm::CGSCCAnalysisManager CGAM(false);
            llvm::ModuleAnalysisManager MAM(false);

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

            llvm::ModulePassManager MPM(false);
            if (optNone()) {
              MPM.addPass(llvm::AlwaysInlinerPass(false));
            } else {
              MPM.addPass(PB.buildPerModuleDefaultPipeline(toLLVMLevel(Level)));
            }

            MPM.run(*LLModule, MAM);
          }

          llvm::legacy::PassManager CodeGenPasses;
          CodeGenPasses.add(llvm::createTargetTransformInfoWrapperPass(
              TM->getTargetIRAnalysis()));

          // Add LibraryInfo.
          CodeGenPasses.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

          if (TM->addPassesToEmitFile(CodeGenPasses, *OS, nullptr,
#if LLVM_VERSION_MAJOR >= 10
                                      llvm::CGFT_ObjectFile,
#else
                                      llvm::TargetMachine::CGFT_ObjectFile,
#endif
                                      false)) {
            // TODO:return error
            LOG(ERROR) << "addPassesToEmitFile failed";
            llvm::consumeError(Object->discard());
            return {};
          }

          if (DumpIR) {
            int Fd;
            llvm::sys::fs::openFileForWrite("wasm-opt.ll", Fd);
            llvm::raw_fd_ostream OS(Fd, true);
            LLModule->print(OS, nullptr);
          }
          LOG(INFO) << "codegen start";
          CodeGenPasses.run(*LLModule);
        }

    // link
#ifdef __APPLE__
        using lld::mach_o::link;
#else
        using lld::elf::link;
#endif
        link(std::array{"lld", "--shared", "--gc-sections",
                        Object->TmpName.c_str(), "-o", Path.u8string().c_str()},
             false,
#if LLVM_VERSION_MAJOR >= 10
             llvm::outs(), llvm::errs()
#else
             llvm::errs()
#endif
        );

        llvm::consumeError(Object->discard());
        LOG(INFO) << "compile done";
        return {};
      });
}

Expect<void> Compiler::compile(const AST::TypeSection &TypeSection) {
  auto *WrapperTy =
      llvm::FunctionType::get(Context->VoidTy,
                              {Context->ExecCtxPtrTy, Context->Int8PtrTy,
                               Context->Int8PtrTy, Context->Int8PtrTy},
                              false);
  const auto &FuncTypes = TypeSection.getContent();
  const auto Size = FuncTypes.size();
  std::vector<llvm::Constant *> Types;
  Types.reserve(Size);
  Context->FunctionTypes.reserve(Size);
  Context->FunctionWrappers.reserve(Size);

  /// Iterate and compile types.
  for (size_t I = 0; I < Size; ++I) {
    const auto &FuncType = *FuncTypes[I];

    /// Check function type is unique
    {
      bool Unique = true;
      for (size_t J = 0; J < I; ++J) {
        const auto &OldFuncType = *Context->FunctionTypes[J];
        if (OldFuncType == FuncType) {
          Unique = false;
          Context->FunctionTypes.push_back(&OldFuncType);
          auto *F = Context->FunctionWrappers[J];
          Context->FunctionWrappers.push_back(F);
          Types.push_back(Types[J]);
          break;
        }
      }
      if (!Unique) {
        continue;
      }
    }

    /// Create Wrapper
    auto *F = llvm::Function::Create(
        WrapperTy, llvm::Function::InternalLinkage,
        "t" + std::to_string(Context->FunctionTypes.size()), Context->LLModule);
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
      for (size_t I = 0; I < ArgCount; ++I) {
        auto *ArgTy = FTy->getParamType(I + 1);
        llvm::Value *VPtr =
            Builder.CreateConstInBoundsGEP1_64(RawArgs, I * kValSize);
        llvm::Value *Ptr = Builder.CreateBitCast(VPtr, ArgTy->getPointerTo());
        Args.push_back(Builder.CreateLoad(Ptr));
      }

      auto Ret = Builder.CreateCall(RawFunc, Args);
      if (RTy->isVoidTy()) {
        // nothing to do
      } else if (RTy->isStructTy()) {
        auto Rets = unpackStruct(Builder, Ret);
        for (size_t I = 0; I < RetCount; ++I) {
          llvm::Value *VPtr =
              Builder.CreateConstInBoundsGEP1_64(RawRets, I * kValSize);
          llvm::Value *Ptr =
              Builder.CreateBitCast(VPtr, Rets[I]->getType()->getPointerTo());
          Builder.CreateStore(Rets[I], Ptr);
        }
      } else {
        llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(RawRets, 0);
        llvm::Value *Ptr =
            Builder.CreateBitCast(VPtr, Ret->getType()->getPointerTo());
        Builder.CreateStore(Ret, Ptr);
      }
      Builder.CreateRetVoid();
    }
    /// Copy wrapper, param and return lists to module instance.
    Context->FunctionTypes.push_back(&FuncType);
    Context->FunctionWrappers.push_back(F);
    Types.push_back(llvm::ConstantExpr::getBitCast(F, Context->Int8PtrTy));
  }

  auto *ArrayTy = llvm::ArrayType::get(Context->Int8PtrTy, Size);
  new llvm::GlobalVariable(Context->LLModule, ArrayTy, false,
                           llvm::GlobalValue::ExternalLinkage,
                           llvm::ConstantArray::get(ArrayTy, Types), "types");
  return {};
}

Expect<void> Compiler::compile(const AST::ImportSection &ImportSec) {
  /// Iterate and compile import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    /// Get data from import description.
    const auto &ExtType = ImpDesc->getExternalType();

    /// Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: /// Function type index
    {
      const auto FuncID = Context->Functions.size();
      /// Get the function type index in module.
      unsigned int *TypeIdx = nullptr;
      if (auto Res = ImpDesc->getExternalContent<uint32_t>(); unlikely(!Res)) {
        return Unexpect(ErrCode::InvalidFuncIdx);
      } else {
        TypeIdx = *Res;
      }
      if (*TypeIdx >= Context->FunctionTypes.size()) {
        return Unexpect(ErrCode::InvalidFuncTypeIdx);
      }
      const auto &FuncType = *Context->FunctionTypes[*TypeIdx];

      auto *FTy = toLLVMType(Context->ExecCtxPtrTy, FuncType);
      auto *RTy = FTy->getReturnType();
      auto *F = llvm::Function::Create(FTy, llvm::Function::InternalLinkage,
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
        llvm::Value *Ptr =
            Builder.CreateConstInBoundsGEP1_64(Args, I * kValSize);
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
        llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(Rets, 0);
        llvm::Value *Ptr =
            Builder.CreateBitCast(VPtr, F->getReturnType()->getPointerTo());
        Builder.CreateRet(Builder.CreateLoad(Ptr));
      } else {
        std::vector<llvm::Value *> Ret;
        Ret.reserve(RetSize);
        for (unsigned I = 0; I < RetSize; ++I) {
          llvm::Value *VPtr =
              Builder.CreateConstInBoundsGEP1_64(Rets, I * kValSize);
          llvm::Value *Ptr = Builder.CreateBitCast(
              VPtr, RTy->getStructElementType(I)->getPointerTo());
          Ret.push_back(Builder.CreateLoad(Ptr));
        }
        Builder.CreateAggregateRet(Ret.data(), RetSize);
      }

      Context->Functions.emplace_back(*TypeIdx, F, nullptr);
      break;
    }
    case ExternalType::Table: /// Table type
    {
      /// Nothing to do.
      break;
    }
    case ExternalType::Memory: /// Memory type
    {
      /// Nothing to do.
      break;
    }
    case ExternalType::Global: /// Global type
    {
      /// Get global type. External type checked in validation.
      AST::GlobalType *GlobType =
          *ImpDesc->getExternalContent<AST::GlobalType>();

      const auto &ValType = GlobType->getValueType();
      auto *Type = toLLVMType(Context->LLContext, ValType)->getPointerTo();
      Context->Globals.push_back(Type);
      break;
    }
    default:
      break;
    }
  }
  return {};
}

Expect<void> Compiler::compile(const AST::ExportSection &ExportSec) {
  return {};
}

Expect<void> Compiler::compile(const AST::GlobalSection &GlobalSec) {
  for (const auto &Global : GlobalSec.getContent()) {
    const auto &ValType = Global->getGlobalType()->getValueType();
    auto *Type = toLLVMType(Context->LLContext, ValType)->getPointerTo();
    Context->Globals.push_back(Type);
  }
  return {};
}

Expect<void> Compiler::compile(const AST::MemorySection &MemorySection,
                               const AST::DataSection &DataSec) {
  if (MemorySection.getContent().size() != 1) {
    return Unexpect(ErrCode::MultiMemories);
  }
  const auto &Limit = *MemorySection.getContent().front()->getLimit();
  Context->MemMin = Limit.getMin();
  Context->MemMax = Limit.hasMax() ? Limit.getMax() : 65536;
  return {};
}

Expect<void> Compiler::compile(const AST::TableSection &TableSection,
                               const AST::ElementSection &ElementSection) {
  return {};
}

Expect<void> Compiler::compile(const AST::FunctionSection &FuncSec,
                               const AST::CodeSection &CodeSec) {
  const auto &TypeIdxs = FuncSec.getContent();
  const auto &CodeSegs = CodeSec.getContent();

  std::vector<llvm::Constant *> Codes;
  Codes.reserve(CodeSegs.size());

  for (size_t I = 0; I < TypeIdxs.size() && I < CodeSegs.size(); ++I) {
    const auto &TypeIdx = TypeIdxs[I];
    const auto &Code = CodeSegs[I];
    if (TypeIdx >= Context->FunctionTypes.size()) {
      return Unexpect(ErrCode::InvalidFuncTypeIdx);
    }
    const auto &FuncType = *Context->FunctionTypes[TypeIdx];
    const auto FuncID = Context->Functions.size();
    auto *FTy = toLLVMType(Context->ExecCtxPtrTy, FuncType);
    auto *F =
        llvm::Function::Create(FTy, llvm::Function::InternalLinkage,
                               "f" + std::to_string(FuncID), Context->LLModule);
    F->addFnAttr(llvm::Attribute::StrictFP);
    F->addParamAttr(0, llvm::Attribute::AttrKind::ReadOnly);
    F->addParamAttr(0, llvm::Attribute::AttrKind::NoAlias);

    Context->Functions.emplace_back(TypeIdx, F, Code.get());
    Codes.push_back(llvm::ConstantExpr::getBitCast(F, Context->Int8PtrTy));
  }

  {
    auto *ArrayTy = llvm::ArrayType::get(Context->Int8PtrTy, Codes.size());
    new llvm::GlobalVariable(Context->LLModule, ArrayTy, false,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantArray::get(ArrayTy, Codes), "codes");
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

    const auto &ReturnTypes = Context->FunctionTypes[T]->getReturnTypes();
    FunctionCompiler FC(*Context, F, Locals, InstructionCounting, GasMeasuring,
                        optNone());
    if (auto Status = FC.compile(*Code, ReturnTypes); !Status) {
      return Status;
    }
  }

  return {};
}

} // namespace AOT
} // namespace SSVM
