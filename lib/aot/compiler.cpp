// SPDX-License-Identifier: Apache-2.0
#include "aot/compiler.h"
#include "runtime/instance/memory.h"
#include "support/filesystem.h"
#include "support/log.h"
#include <lld/Common/Driver.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>

#if LLVM_VERSION_MAJOR >= 10
#include <llvm/IR/IntrinsicsAArch64.h>
#include <llvm/IR/IntrinsicsX86.h>
#include <llvm/Support/Alignment.h>
#endif

namespace {

#if LLVM_VERSION_MAJOR >= 10
using RoundingMode = llvm::fp::RoundingMode;
using ExceptionBehavior = llvm::fp::ExceptionBehavior;
using Align = llvm::Align;
#else
using RoundingMode = llvm::ConstrainedFPIntrinsic::RoundingMode;
using ExceptionBehavior = llvm::ConstrainedFPIntrinsic::ExceptionBehavior;
static inline unsigned Align(unsigned Value) noexcept { return Value; }
#endif

static bool isVoidReturn(SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::Type *toLLVMType(llvm::LLVMContext &LLContext,
                              const SSVM::ValType &ValType);
static std::vector<llvm::Type *>
toLLVMArgsType(llvm::LLVMContext &LLContext,
               SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::Type *toLLVMRetsType(llvm::LLVMContext &LLContext,
                                  SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::FunctionType *toLLVMType(llvm::LLVMContext &LLContext,
                                      const SSVM::AST::FunctionType &FuncType);
static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &LLContext,
                                          const SSVM::ValType &ValType);
static std::vector<llvm::Value *> unpackStruct(llvm::IRBuilder<> &Builder,
                                               llvm::Value *Struct);
class FunctionCompiler;

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overloaded(Ts...)->overloaded<Ts...>;

/// force checking div/rem on zero
static inline constexpr bool ForceDivCheck = false;

/// Translate Compiler::OptimizationLevel to llvm::PassBuilder version
static inline llvm::PassBuilder::OptimizationLevel
toLLVMLevel(SSVM::AOT::Compiler::OptimizationLevel Level) {
  using OL = SSVM::AOT::Compiler::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return llvm::PassBuilder::O0;
  case OL::O1:
    return llvm::PassBuilder::O1;
  case OL::O2:
    return llvm::PassBuilder::O2;
  case OL::O3:
    return llvm::PassBuilder::O3;
  case OL::Os:
    return llvm::PassBuilder::Os;
  case OL::Oz:
    return llvm::PassBuilder::Oz;
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
  llvm::Type *FloatTy;
  llvm::Type *DoubleTy;
  llvm::PointerType *Int8PtrTy;
  llvm::PointerType *Int32PtrTy;
  llvm::PointerType *Int64PtrTy;
  llvm::SubtargetFeatures SubtargetFeatures;
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
  std::vector<const AST::FunctionType *> FunctionTypes;
  std::vector<unsigned int> Elements;
  std::vector<
      std::tuple<unsigned int, llvm::Function *, SSVM::AST::CodeSegment *>>
      Functions;
  std::vector<llvm::GlobalVariable *> Globals;
  llvm::GlobalVariable *InstrCount;
  llvm::GlobalVariable *CostTable;
  llvm::GlobalVariable *Gas;
  llvm::GlobalVariable *Call;
  llvm::GlobalVariable *MemGrow;
  llvm::GlobalVariable *Mem;
  llvm::GlobalVariable *TrapCode;
  llvm::Function *Trap;
  llvm::MDNode *Likely;
  uint32_t MemMin = 1, MemMax = 65536;
  CompileContext(llvm::Module &M)
      : LLContext(M.getContext()), LLModule(M),
        VoidTy(llvm::Type::getVoidTy(LLContext)),
        Int8Ty(llvm::Type::getInt8Ty(LLContext)),
        Int16Ty(llvm::Type::getInt16Ty(LLContext)),
        Int32Ty(llvm::Type::getInt32Ty(LLContext)),
        Int64Ty(llvm::Type::getInt64Ty(LLContext)),
        FloatTy(llvm::Type::getFloatTy(LLContext)),
        DoubleTy(llvm::Type::getDoubleTy(LLContext)),
        Int8PtrTy(llvm::Type::getInt8PtrTy(LLContext)),
        Int32PtrTy(llvm::Type::getInt32PtrTy(LLContext)),
        Int64PtrTy(Int64Ty->getPointerTo()),
        InstrCount(new llvm::GlobalVariable(LLModule, Int64PtrTy, true,
                                            llvm::GlobalValue::ExternalLinkage,
                                            nullptr, "instr")),
        CostTable(new llvm::GlobalVariable(LLModule, Int64PtrTy, true,
                                           llvm::GlobalValue::ExternalLinkage,
                                           nullptr, "cost")),
        Gas(new llvm::GlobalVariable(LLModule, Int64PtrTy, true,
                                     llvm::GlobalValue::ExternalLinkage,
                                     nullptr, "gas")),
        Call(new llvm::GlobalVariable(
            LLModule,
            llvm::FunctionType::get(VoidTy, {Int32Ty, Int8PtrTy, Int8PtrTy},
                                    false)
                ->getPointerTo(),
            true, llvm::GlobalVariable::ExternalLinkage, nullptr, "call")),
        MemGrow(new llvm::GlobalVariable(
            LLModule,
            llvm::FunctionType::get(Int32Ty, {Int32Ty}, false)->getPointerTo(),
            true, llvm::GlobalVariable::ExternalLinkage, nullptr, "memgrow")),
        Mem(new llvm::GlobalVariable(LLModule, Int8PtrTy, true,
                                     llvm::GlobalValue::ExternalLinkage,
                                     nullptr, "mem")),
        TrapCode(new llvm::GlobalVariable(LLModule, Int32PtrTy, false,
                                          llvm::GlobalValue::ExternalLinkage,
                                          nullptr, "code")),
        Trap(llvm::Function::Create(
            llvm::FunctionType::get(VoidTy, {Int32Ty}, false),
            llvm::Function::PrivateLinkage, "trap", LLModule)),
        Likely(llvm::MDTuple::getDistinct(
            LLContext, {llvm::MDString::get(LLContext, "branch_weights"),
                        llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
                            LLContext, llvm::APInt(32, 2000))),
                        llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
                            LLContext, llvm::APInt(32, 0)))})) {
    Trap->addFnAttr(llvm::Attribute::NoReturn);
    TrapCode->setInitializer(llvm::ConstantPointerNull::get(Int32PtrTy));

    new llvm::GlobalVariable(
        LLModule, Int32Ty, true, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(Int32Ty, kVersion), "version");

    {
      llvm::StringMap<bool> FeatureMap;
      llvm::sys::getHostCPUFeatures(FeatureMap);
      for (auto &Feature : FeatureMap) {
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
        SubtargetFeatures.AddFeature(Feature.first(), Feature.second);
      }
    }

    {
      /// create trap
      llvm::IRBuilder<> Builder(
          llvm::BasicBlock::Create(LLContext, "entry", Trap));
      Builder.CreateStore(Trap->arg_begin(), Builder.CreateLoad(TrapCode));
      Builder.CreateIntrinsic(llvm::Intrinsic::trap, {}, {});
      Builder.CreateUnreachable();
    }
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
    return llvm::Type::getInt64Ty(LLContext);
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

static std::vector<llvm::Type *> toLLVMArgsType(llvm::LLVMContext &LLContext,
                                                Span<const ValType> ValTypes) {
  return toLLVMTypeVector(LLContext, ValTypes);
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

static llvm::FunctionType *toLLVMType(llvm::LLVMContext &LLContext,
                                      const AST::FunctionType &FuncType) {
  auto ArgsTy = toLLVMArgsType(LLContext, FuncType.getParamTypes());
  auto RetTy = toLLVMRetsType(LLContext, FuncType.getReturnTypes());
  return llvm::FunctionType::get(RetTy, ArgsTy, false);
}

static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &LLContext,
                                          const ValType &ValType) {
  switch (ValType) {
  case ValType::I32:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(LLContext), 0);
  case ValType::I64:
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(LLContext), 0);
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
                   bool GasMeasuring)
      : Context(Context), LLContext(Context.LLContext), F(F),
        Builder(llvm::BasicBlock::Create(LLContext, "entry", F)) {
    if (F) {
      Builder.setIsFPConstrained(true);
      Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
      Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);

      if (InstructionCounting) {
        LocalInstrCount = Builder.CreateAlloca(Context.Int64Ty);
        Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
      }

      if (GasMeasuring) {
        LocalCostTable = Builder.CreateAlloca(Context.Int64PtrTy);
        Builder.CreateStore(Builder.CreateLoad(Context.CostTable),
                            LocalCostTable);
        LocalGas = Builder.CreateAlloca(Context.Int64Ty);
        Builder.CreateStore(Builder.CreateLoad(Builder.CreateLoad(Context.Gas)),
                            LocalGas);
      }

      for (llvm::Argument *Arg = F->arg_begin(); Arg != F->arg_end(); ++Arg) {
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
      Builder.CreateCall(Context.Trap, {Builder.getInt32(uint32_t(Error))});
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
            if constexpr (std::is_void_v<
                              typename std::decay_t<decltype(Arg)>::type>) {
              /// If the Code not matched, return null pointer.
              LOG(ERROR) << ErrCode::InstrTypeMismatch;
              LOG(ERROR) << ErrInfo::InfoInstruction(Instr->getOpCode(),
                                                     Instr->getOffset());
              return Unexpect(ErrCode::InstrTypeMismatch);
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
                    Builder.CreateLoad(Builder.CreateInBoundsGEP(
                        Builder.CreateLoad(LocalCostTable),
                        {Builder.getInt64(uint16_t(Instr->getOpCode()))})));
                Builder.CreateStore(NewGas, LocalGas);
              }

              /// Make the instruction node according to Code.
              if (auto Status = compile(
                      *static_cast<
                          const typename std::decay_t<decltype(Arg)>::type *>(
                          Instr.get()));
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
      return compileIndirectCallOp(Instr.getTargetIndex());
    }
    default:
      __builtin_unreachable();
    }
  }
  Expect<void> compile(const AST::ParametricInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::Drop:
      stackPop();
      break;
    case OpCode::Select: {
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
      if (Index >= Context.Globals.size()) {
        return Unexpect(ErrCode::InvalidGlobalIdx);
      }
      stackPush(Builder.CreateLoad(Context.Globals[Index]));
      break;
    case OpCode::Global__set:
      if (Index >= Context.Globals.size()) {
        return Unexpect(ErrCode::InvalidGlobalIdx);
      }
      Builder.CreateStore(stackPop(), Context.Globals[Index]);
      break;
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
      stackPush(Builder.CreateCall(Builder.CreateLoad(Context.MemGrow),
                                   {Builder.getInt32(0)}));
      break;
    case OpCode::Memory__grow: {
      auto *Diff = stackPop();
      auto *Result =
          Builder.CreateCall(Builder.CreateLoad(Context.MemGrow), {Diff});
      stackPush(Result);
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
      const bool IsFloat = Instr.getOpCode() == OpCode::F32__nearest;
      const uint32_t VectorSize = IsFloat ? 4 : 2;
      llvm::Value *Value = stackPop();

#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) ||            \
    defined(_M_X64)

      if (Context.SupportRoundeven) {
        llvm::Value *Ret = llvm::UndefValue::get(
            llvm::VectorType::get(Value->getType(), VectorSize));
        Ret = Builder.CreateInsertElement(Ret, Value, UINT64_C(0));
        if (IsFloat) {
          Ret = Builder.CreateIntrinsic(llvm::Intrinsic::x86_sse41_round_ss, {},
                                        {Ret, Ret, Builder.getInt32(8)});
        } else {
          Ret = Builder.CreateIntrinsic(llvm::Intrinsic::x86_sse41_round_sd, {},
                                        {Ret, Ret, Builder.getInt32(8)});
        }
        Ret = Builder.CreateExtractElement(Ret, UINT64_C(0));
        stackPush(Ret);
        break;
      }
#endif

#if defined(__arm__) || defined(__aarch64__)
      if (Context.SupportRoundeven) {
        llvm::Value *Ret = llvm::UndefValue::get(
            llvm::VectorType::get(Value->getType(), VectorSize));
        Ret = Builder.CreateInsertElement(Ret, Value, UINT64_C(0));
        Ret = Builder.CreateBinaryIntrinsic(
            llvm::Intrinsic::aarch64_neon_frintn, Ret, Ret);
        Ret = Builder.CreateExtractElement(Ret, UINT64_C(0));
        stackPush(Ret);
        break;
      }
#endif

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
      compileSignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+31f)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+31f)),
          Context.Int32Ty);
      break;
    case OpCode::I32__trunc_f64_s:
      compileSignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+31)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+31)),
          Context.Int32Ty);
      break;
    case OpCode::I32__trunc_f32_u:
      compileUnsignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+32f)),
          Context.Int32Ty);
      break;
    case OpCode::I32__trunc_f64_u:
      compileUnsignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+32)),
          Context.Int32Ty);
      break;
    case OpCode::I64__extend_i32_s:
      stackPush(Builder.CreateSExt(stackPop(), Context.Int64Ty));
      break;
    case OpCode::I64__extend_i32_u:
      stackPush(Builder.CreateZExt(stackPop(), Context.Int64Ty));
      break;
    case OpCode::I64__trunc_f32_s:
      compileSignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+63f)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+63f)),
          Context.Int64Ty);
      break;
    case OpCode::I64__trunc_f64_s:
      compileSignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+63)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+63)),
          Context.Int64Ty);
      break;
    case OpCode::I64__trunc_f32_u:
      compileUnsignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+64f)),
          Context.Int64Ty);
      break;
    case OpCode::I64__trunc_f64_u:
      compileUnsignedTrunc(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+64)),
          Context.Int64Ty);
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
      stackPush(Builder.CreateFPTrunc(stackPop(), Context.FloatTy));
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
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+31f)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+31f)),
          Builder.getInt32(INT32_MIN), Builder.getInt32(INT32_MAX));
      break;
    case OpCode::I32__trunc_sat_f32_u:
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+32f)),
          Builder.getInt32(UINT32_MAX));
      break;
    case OpCode::I32__trunc_sat_f64_s:
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+31)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+31)),
          Builder.getInt32(INT32_MIN), Builder.getInt32(INT32_MAX));
      break;
    case OpCode::I32__trunc_sat_f64_u:
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+32)),
          Builder.getInt32(UINT32_MAX));
      break;
    case OpCode::I64__trunc_sat_f32_s:
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+63f)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+63f)),
          Builder.getInt64(INT64_MIN), Builder.getInt64(INT64_MAX));
      break;
    case OpCode::I64__trunc_sat_f32_u:
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+64f)),
          Builder.getInt64(UINT64_MAX));
      break;
    case OpCode::I64__trunc_sat_f64_s:
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+63)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+63)),
          Builder.getInt64(INT64_MIN), Builder.getInt64(INT64_MAX));
      break;
    case OpCode::I64__trunc_sat_f64_u:
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+64)),
          Builder.getInt64(UINT64_MAX));
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
      if constexpr (ForceDivCheck) {
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
        Builder.CreateCondBr(Builder.CreateICmpNE(RHS, IntZero), NoZeroBB,
                             getTrapBB(ErrCode::DivideByZero), Context.Likely);

        Builder.SetInsertPoint(NoZeroBB);
        auto *NotOverflow =
            Builder.CreateOr(Builder.CreateICmpNE(LHS, IntMin),
                             Builder.CreateICmpNE(RHS, IntMinusOne));
        Builder.CreateCondBr(NotOverflow, OkBB,
                             getTrapBB(ErrCode::IntegerOverflow),
                             Context.Likely);

        Builder.SetInsertPoint(OkBB);
      }
      stackPush(Builder.CreateSDiv(LHS, RHS));
      break;
    case OpCode::I32__div_u:
    case OpCode::I64__div_u:
      if constexpr (ForceDivCheck) {
        const bool Is32 = Instr.getOpCode() == OpCode::I32__div_u;
        llvm::ConstantInt *IntZero =
            Is32 ? Builder.getInt32(0) : Builder.getInt64(0);
        auto *OkBB = llvm::BasicBlock::Create(LLContext, "div.ok", F);
        Builder.CreateCondBr(Builder.CreateICmpNE(RHS, IntZero), OkBB,
                             getTrapBB(ErrCode::DivideByZero), Context.Likely);
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

      if constexpr (ForceDivCheck) {
        auto *OkBB = llvm::BasicBlock::Create(LLContext, "rem.ok", F);
        Builder.CreateCondBr(Builder.CreateICmpNE(RHS, IntZero), OkBB,
                             getTrapBB(ErrCode::DivideByZero), Context.Likely);
        Builder.SetInsertPoint(OkBB);
      }

      auto *CurrBB = Builder.GetInsertBlock();
      auto *NotOverflow =
          Builder.CreateOr(Builder.CreateICmpNE(LHS, IntMin),
                           Builder.CreateICmpNE(RHS, IntMinusOne));
      Builder.CreateCondBr(NotOverflow, NoOverflowBB, EndBB, Context.Likely);

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
      if constexpr (ForceDivCheck) {
        llvm::ConstantInt *IntZero = Instr.getOpCode() == OpCode::I32__rem_u
                                         ? Builder.getInt32(0)
                                         : Builder.getInt64(0);
        auto *OkBB = llvm::BasicBlock::Create(LLContext, "rem.ok", F);
        Builder.CreateCondBr(Builder.CreateICmpNE(RHS, IntZero), OkBB,
                             getTrapBB(ErrCode::DivideByZero), Context.Likely);
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
  void compileSignedTrunc(llvm::ConstantFP *MinFp, llvm::ConstantFP *MaxFp,
                          llvm::Type *Type) {
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "strunc.norm", F);
    auto *NotMinBB = llvm::BasicBlock::Create(LLContext, "strunc.notmin", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "strunc.notmax", F);
    auto *Value = stackPop();

    Builder.CreateCondBr(Builder.CreateFCmpORD(Value, Value), NormBB,
                         getTrapBB(ErrCode::InvalidConvToInt), Context.Likely);

    Builder.SetInsertPoint(NormBB);
    Builder.CreateCondBr(Builder.CreateFCmpUGE(Value, MinFp), NotMinBB,
                         getTrapBB(ErrCode::IntegerOverflow), Context.Likely);

    Builder.SetInsertPoint(NotMinBB);
    Builder.CreateCondBr(Builder.CreateFCmpULT(Value, MaxFp), NotMaxBB,
                         getTrapBB(ErrCode::IntegerOverflow), Context.Likely);

    Builder.SetInsertPoint(NotMaxBB);
    stackPush(Builder.CreateFPToSI(Value, Type));
  }
  void compileSignedTruncSat(llvm::ConstantFP *MinFp, llvm::ConstantFP *MaxFp,
                             llvm::ConstantInt *MinInt,
                             llvm::ConstantInt *MaxInt) {
    auto *CurrBB = Builder.GetInsertBlock();
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "ssat.norm", F);
    auto *NotMinBB = llvm::BasicBlock::Create(LLContext, "ssat.notmin", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "ssat.notmax", F);
    auto *EndBB = llvm::BasicBlock::Create(LLContext, "ssat.end", F);
    auto *Value = stackPop();

    Builder.CreateCondBr(Builder.CreateFCmpORD(Value, Value), NormBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NormBB);
    Builder.CreateCondBr(Builder.CreateFCmpUGT(Value, MinFp), NotMinBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NotMinBB);
    Builder.CreateCondBr(Builder.CreateFCmpULT(Value, MaxFp), NotMaxBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NotMaxBB);
    auto *Ty = MaxInt->getType();
    auto *IntValue = Builder.CreateFPToSI(Value, Ty);
    Builder.CreateBr(EndBB);

    Builder.SetInsertPoint(EndBB);
    auto *PHIRet = Builder.CreatePHI(Ty, 4);
    PHIRet->addIncoming(llvm::ConstantInt::get(Ty, 0), CurrBB);
    PHIRet->addIncoming(MinInt, NormBB);
    PHIRet->addIncoming(MaxInt, NotMinBB);
    PHIRet->addIncoming(IntValue, NotMaxBB);

    stackPush(PHIRet);
  }
  void compileUnsignedTrunc(llvm::ConstantFP *MaxFp, llvm::Type *Type) {
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "utrunc.norm", F);
    auto *NotMinBB = llvm::BasicBlock::Create(LLContext, "utrunc.notmin", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "utrunc.notmax", F);
    auto *Value = stackPop();

    Builder.CreateCondBr(Builder.CreateFCmpORD(Value, Value), NormBB,
                         getTrapBB(ErrCode::InvalidConvToInt), Context.Likely);

    Builder.SetInsertPoint(NormBB);
    Builder.CreateCondBr(
        Builder.CreateFCmpOGT(Value,
                              llvm::ConstantFP::get(Value->getType(), -1.0)),
        NotMinBB, getTrapBB(ErrCode::IntegerOverflow), Context.Likely);

    Builder.SetInsertPoint(NotMinBB);
    Builder.CreateCondBr(Builder.CreateFCmpOLT(Value, MaxFp), NotMaxBB,
                         getTrapBB(ErrCode::IntegerOverflow), Context.Likely);

    Builder.SetInsertPoint(NotMaxBB);
    stackPush(Builder.CreateFPToUI(Value, Type));
  }
  void compileUnsignedTruncSat(llvm::ConstantFP *MaxFp,
                               llvm::ConstantInt *MaxInt) {
    auto *CurrBB = Builder.GetInsertBlock();
    auto *NormBB = llvm::BasicBlock::Create(LLContext, "usat.norm", F);
    auto *NotMaxBB = llvm::BasicBlock::Create(LLContext, "usat.notmax", F);
    auto *EndBB = llvm::BasicBlock::Create(LLContext, "usat.end", F);
    auto *Value = stackPop();

    Builder.CreateCondBr(
        Builder.CreateFCmpOGT(Value,
                              llvm::ConstantFP::get(Value->getType(), 0.0)),
        NormBB, EndBB, Context.Likely);

    Builder.SetInsertPoint(NormBB);
    Builder.CreateCondBr(Builder.CreateFCmpOLT(Value, MaxFp), NotMaxBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NotMaxBB);
    auto *Ty = MaxInt->getType();
    auto *IntValue = Builder.CreateFPToUI(Value, Ty);
    Builder.CreateBr(EndBB);

    Builder.SetInsertPoint(EndBB);
    auto *PHIRet = Builder.CreatePHI(Ty, 3);
    PHIRet->addIncoming(llvm::ConstantInt::get(Ty, 0), CurrBB);
    PHIRet->addIncoming(MaxInt, NormBB);
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
      auto *Ptr = Builder.CreateLoad(Context.InstrCount);
      Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(LocalInstrCount),
                                            Builder.CreateLoad(Ptr)),
                          Ptr);
      Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
    }
  }

  void readGas() {
    if (LocalGas) {
      Builder.CreateStore(Builder.CreateLoad(Builder.CreateLoad(Context.Gas)),
                          LocalGas);
    }
  }

  void writeGas() {
    if (LocalGas) {
      Builder.CreateStore(Builder.CreateLoad(LocalGas),
                          Builder.CreateLoad(Context.Gas));
    }
  }

  static Expect<llvm::Constant *>
  evaluate(const AST::InstrVec &Instrs,
           AOT::Compiler::CompileContext &Context) {
    // XXX: assuming Instrs contains only one constant value instruction
    FunctionCompiler FC(Context);
    FC.compile(Instrs);
    if (FC.Stack.size() != 1) {
      return Unexpect(ErrCode::ConstExprRequired);
    }
    auto *Result = llvm::cast_or_null<llvm::Constant>(FC.Stack.back());
    if (!Result) {
      return Unexpect(ErrCode::ConstExprRequired);
    }
    return Result;
  }

private:
  Expect<void> compileCallOp(const unsigned int FuncIndex) {
    const auto &FuncType =
        *Context.FunctionTypes[std::get<0>(Context.Functions[FuncIndex])];
    const auto &Function = std::get<1>(Context.Functions[FuncIndex]);
    const auto &ParamTypes = FuncType.getParamTypes();

    std::vector<llvm::Value *> Args(ParamTypes.size());
    for (size_t I = 0; I < ParamTypes.size(); ++I) {
      const size_t J = ParamTypes.size() - 1 - I;
      Args[J] = stackPop();
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

  Expect<void> compileIndirectCallOp(const unsigned int FuncTypeIndex) {
    llvm::Value *Value = stackPop();
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    const auto &ParamTypes = FuncType.getParamTypes();
    std::vector<llvm::Value *> Args(ParamTypes.size());
    for (size_t I = 0; I < ParamTypes.size(); ++I) {
      const size_t J = ParamTypes.size() - 1 - I;
      Args[J] = stackPop();
    }

    std::vector<std::pair<size_t, llvm::Function *>> Table;
    for (uint32_t I = 0; I < Context.Elements.size(); ++I) {
      const auto FuncIdx = Context.Elements[I];
      const auto FuncTypeIndex2 = std::get<0>(Context.Functions[FuncIdx]);
      const auto &FuncType2 = *Context.FunctionTypes[FuncTypeIndex2];
      if (FuncTypeIndex == FuncTypeIndex2 || FuncType == FuncType2) {
        Table.emplace_back(I, std::get<1>(Context.Functions[FuncIdx]));
      }
    }
    auto *OKBB = llvm::BasicBlock::Create(LLContext, "call_indirect.end", F);
    auto *Switch = Builder.CreateSwitch(
        Value, getTrapBB(ErrCode::UndefinedElement), Table.size());

    const bool HasReturnValue = !isVoidReturn(FuncType.getReturnTypes());
    const bool HasMultipleValule = FuncType.getReturnTypes().size() > 1;
    std::vector<std::tuple<std::vector<llvm::Value *>, llvm::BasicBlock *>>
        ReturnValues;
    if (HasReturnValue) {
      ReturnValues.reserve(Table.size());
    }

    for (const auto &[Value, Function] : Table) {
      auto *Entry = llvm::BasicBlock::Create(
          LLContext, "call_indirect." + std::to_string(Value), F);
      Builder.SetInsertPoint(Entry);

      auto *Ret = Builder.CreateCall(Function, Args);
      if (HasMultipleValule) {
        ReturnValues.emplace_back(unpackStruct(Builder, Ret), Entry);
      } else if (HasReturnValue) {
        ReturnValues.emplace_back(std::vector<llvm::Value *>{Ret}, Entry);
      }
      Builder.CreateBr(OKBB);
      Switch->addCase(Builder.getInt32(Value), Entry);
    }

    Builder.SetInsertPoint(OKBB);
    if (HasReturnValue) {
      buildPHI(FuncType.getReturnTypes(), ReturnValues);
    }

    readGas();
    return {};
  }

  Expect<void> compileLoadOp(unsigned Offset, unsigned Alignment,
                             llvm::Type *LoadTy) {
    auto *Off = Builder.CreateZExt(stackPop(), Context.Int64Ty);
    if (Offset != 0) {
      Off = Builder.CreateAdd(Off, Builder.getInt64(Offset));
    }

    auto *VPtr =
        Builder.CreateInBoundsGEP(Builder.CreateLoad(Context.Mem), {Off});
    auto *Ptr = Builder.CreateBitCast(VPtr, LoadTy->getPointerTo());
    auto *LoadInst = Builder.CreateLoad(Ptr);
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
  Expect<void> compileStoreOp(unsigned Offset, unsigned Alignment,
                              llvm::Type *LoadTy, bool Trunc = false) {
    auto *V = stackPop();
    auto *Off = Builder.CreateZExt(stackPop(), Context.Int64Ty);
    if (Offset != 0) {
      Off = Builder.CreateAdd(Off, Builder.getInt64(Offset));
    }

    if (Trunc) {
      V = Builder.CreateTrunc(V, LoadTy);
    }
    auto *VPtr =
        Builder.CreateInBoundsGEP(Builder.CreateLoad(Context.Mem), {Off});
    auto *Ptr = Builder.CreateBitCast(VPtr, LoadTy->getPointerTo());
    auto *StoreInst = Builder.CreateStore(V, Ptr);
    StoreInst->setAlignment(Align(UINT64_C(1) << Alignment));
    return {};
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
  llvm::Value *LocalCostTable = nullptr;
  llvm::Value *LocalGas = nullptr;
  std::unordered_map<ErrCode, llvm::BasicBlock *> TrapBB;
  bool IsUnreachable = false;
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
          llvm::consumeError(Object.takeError());
          return {};
        }
        std::error_code EC;
        auto OS = std::make_unique<llvm::raw_fd_ostream>(Object->TmpName, EC);
        if (EC) {
          // TODO:return error
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
            llvm::errs() << "lookupTarget failed\n";
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

          if (!optNone()) {
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

            MPM.addPass(PB.buildPerModuleDefaultPipeline(toLLVMLevel(Level)));
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
            llvm::errs() << "addPassesToEmitFile failed\n";
            llvm::consumeError(Object->discard());
            return {};
          }

          {
            for (auto Name : {"instr"sv, "cost"sv, "gas"sv, "call"sv,
                              "memgrow"sv, "mem"sv}) {
              if (auto *Var = LLModule->getGlobalVariable(
                      llvm::StringRef(Name.data(), Name.size()))) {
                Var->setInitializer(llvm::ConstantPointerNull::get(
                    llvm::cast<llvm::PointerType>(Var->getValueType())));
                Var->setConstant(false);
              }
            }
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
        lld::mach_o::link(
#else
        lld::elf::link(
#endif
            std::array{"lld", "--shared", "--gc-sections",
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
  /// Iterate and compile types.
  for (const auto &FuncType : TypeSection.getContent()) {
    /// Copy param and return lists to module instance.
    Context->FunctionTypes.push_back(FuncType.get());
  }

  return {};
}

Expect<void> Compiler::compile(const AST::ImportSection &ImportSec) {
  auto &LLContext = Context->LLContext;
  /// Iterate and compile import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    /// Get data from import description.
    const auto &ExtType = ImpDesc->getExternalType();
    const std::string ModName(ImpDesc->getModuleName());
    const std::string ExtName(ImpDesc->getExternalName());
    const std::string FullName =
        AST::Module::toExportName(ModName + '.' + ExtName);

    /// Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: /// Function type index
    {
      const auto FuncIndex = Context->Functions.size();
      /// Get the function type index in module.
      unsigned int *TypeIdx = nullptr;
      if (auto Res = ImpDesc->getExternalContent<uint32_t>()) {
        TypeIdx = *Res;
      } else {
        return Unexpect(ErrCode::InvalidFuncIdx);
      }
      if (*TypeIdx >= Context->FunctionTypes.size()) {
        return Unexpect(ErrCode::InvalidFuncTypeIdx);
      }
      const auto &FuncType = *Context->FunctionTypes[*TypeIdx];

      llvm::FunctionType *FTy = toLLVMType(LLContext, FuncType);
      auto *RTy = FTy->getReturnType();
      auto *F = llvm::Function::Create(FTy, llvm::Function::InternalLinkage,
                                       FullName, Context->LLModule);
      F->addFnAttr(llvm::Attribute::StrictFP);

      auto *Entry = llvm::BasicBlock::Create(LLContext, "entry", F);
      llvm::IRBuilder<> Builder(Entry);
      Builder.setIsFPConstrained(true);
      Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
      Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);

      const auto ArgSize = FuncType.getParamTypes().size();
      const auto RetSize =
          RTy->isVoidTy() ? 0 : FuncType.getReturnTypes().size();

      llvm::Value *Args;
      if (ArgSize == 0) {
        Args = llvm::ConstantPointerNull::get(Context->Int8PtrTy);
      } else {
        Args = Builder.CreateAlloca(Context->Int8Ty,
                                    Builder.getInt64(ArgSize * 8));
      }

      llvm::Value *Rets;
      if (RetSize == 0) {
        Rets = llvm::ConstantPointerNull::get(Context->Int8PtrTy);
      } else {
        Rets = Builder.CreateAlloca(Context->Int8Ty,
                                    Builder.getInt64(RetSize * 8));
      }

      for (unsigned I = 0; I < ArgSize; ++I) {
        llvm::Argument *Arg = F->arg_begin() + I;
        llvm::Value *Ptr = Builder.CreateConstInBoundsGEP1_64(Args, I * 8);
        Builder.CreateStore(
            Arg, Builder.CreateBitCast(Ptr, Arg->getType()->getPointerTo()));
      }

      Builder.CreateCall(Builder.CreateLoad(Context->Call),
                         {Builder.getInt32(FuncIndex), Args, Rets});

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
          llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(Rets, I);
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
      // XXX: unimplemented
      break;
    }
    case ExternalType::Memory: /// Memory type
    {
      // XXX: unimplemented
      break;
    }
    case ExternalType::Global: /// Global type
    {
      // XXX: unimplemented
      break;
    }
    default:
      break;
    }
  }
  return {};
}

Expect<void> Compiler::compile(const AST::ExportSection &ExportSec) {
  for (const auto &ExpDesc : ExportSec.getContent()) {
    switch (ExpDesc->getExternalType()) {
    case ExternalType::Function: {
      auto *Wrapper = llvm::Function::Create(
          llvm::FunctionType::get(
              Context->VoidTy, {Context->Int8PtrTy, Context->Int8PtrTy}, false),
          llvm::GlobalValue::ExternalLinkage,
          AST::Module::toExportName(ExpDesc->getExternalName()),
          Context->LLModule);
      Wrapper->addFnAttr(llvm::Attribute::StrictFP);
      llvm::Argument *RawArgs = Wrapper->arg_begin();
      llvm::Argument *RawRets = Wrapper->arg_begin() + 1;
      llvm::IRBuilder<> Builder(
          llvm::BasicBlock::Create(Wrapper->getContext(), "entry", Wrapper));
      Builder.setIsFPConstrained(true);
      Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
      Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);
      llvm::Function *F =
          std::get<1>(Context->Functions[ExpDesc->getExternalIndex()]);

      auto *RTy = F->getReturnType();
      const size_t ArgCount = F->arg_size();
      const size_t RetCount =
          RTy->isVoidTy()
              ? 0
              : (RTy->isStructTy() ? RTy->getStructNumElements() : 1);
      std::vector<llvm::Value *> Args;
      Args.reserve(F->arg_size());
      for (size_t I = 0; I < ArgCount; ++I) {
        llvm::Argument *Arg = F->arg_begin() + I;
        llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(RawArgs, I * 8);
        llvm::Value *Ptr =
            Builder.CreateBitCast(VPtr, Arg->getType()->getPointerTo());
        Args.push_back(Builder.CreateLoad(Ptr));
      }

      auto Ret = Builder.CreateCall(F, Args);
      if (RTy->isVoidTy()) {
        // nothing to do
      } else if (RTy->isStructTy()) {
        auto Rets = unpackStruct(Builder, Ret);
        for (size_t I = 0; I < RetCount; ++I) {
          llvm::Value *VPtr =
              Builder.CreateConstInBoundsGEP1_64(RawRets, I * 8);
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

      break;
    }
    case ExternalType::Global: {
      llvm::GlobalVariable *G = Context->Globals[ExpDesc->getExternalIndex()];
      G->setLinkage(llvm::GlobalValue::ExternalLinkage);
      G->setName(AST::Module::toExportName(ExpDesc->getExternalName()));
      break;
    }
    case ExternalType::Memory: {
      // XXX: not supported yet
      break;
    }
    case ExternalType::Table: {
      // XXX: not supported yet
      break;
    }
    default:
      break;
    }
  }
  return {};
}

Expect<void> Compiler::compile(const AST::GlobalSection &GlobalSec) {
  for (size_t I = 0; I < GlobalSec.getContent().size(); ++I) {
    auto Temp = FunctionCompiler::evaluate(
        GlobalSec.getContent()[I]->getInstrs(), *Context);
    if (!Temp) {
      return Unexpect(Temp);
    }
    const auto &ValType =
        GlobalSec.getContent()[I]->getGlobalType()->getValueType();
    auto *G = new llvm::GlobalVariable(
        Context->LLModule, toLLVMType(Context->LLContext, ValType), false,
        llvm::GlobalValue::InternalLinkage, *Temp, "g." + std::to_string(I));
    Context->Globals.push_back(G);
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
  if (TableSection.getContent().size() != 1) {
    return Unexpect(ErrCode::MultiTables);
  }
  auto &Elements = Context->Elements;
  for (const auto &Element : ElementSection.getContent()) {
    auto Temp = FunctionCompiler::evaluate(Element->getInstrs(), *Context);
    if (!Temp) {
      return Unexpect(Temp);
    }
    const uint64_t Offset =
        llvm::cast<llvm::ConstantInt>(*Temp)->getZExtValue();
    const auto &FuncIdxes = Element->getFuncIdxes();
    if (Elements.size() < Offset + FuncIdxes.size()) {
      Elements.resize(Offset + FuncIdxes.size());
    }
    std::copy(FuncIdxes.begin(), FuncIdxes.end(), Elements.begin() + Offset);
  }
  return {};
}

Expect<void> Compiler::compile(const AST::FunctionSection &FuncSec,
                               const AST::CodeSection &CodeSec) {
  const auto &TypeIdxs = FuncSec.getContent();
  const auto &CodeSegs = CodeSec.getContent();

  for (size_t I = 0; I < TypeIdxs.size() && I < CodeSegs.size(); ++I) {
    const auto &TypeIdx = TypeIdxs[I];
    const auto &Code = CodeSegs[I];
    if (TypeIdx >= Context->FunctionTypes.size()) {
      return Unexpect(ErrCode::InvalidFuncTypeIdx);
    }
    const auto &FuncType = *Context->FunctionTypes[TypeIdx];
    const auto FuncID = Context->Functions.size();
    auto *FTy = toLLVMType(Context->LLContext, FuncType);
    auto *F =
        llvm::Function::Create(FTy, llvm::Function::InternalLinkage,
                               "f" + std::to_string(FuncID), Context->LLModule);
    F->addFnAttr(llvm::Attribute::StrictFP);

    Context->Functions.emplace_back(TypeIdx, F, Code.get());
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
    FunctionCompiler FC(*Context, F, Locals, InstructionCounting, GasMeasuring);
    if (auto Status = FC.compile(*Code, ReturnTypes); !Status) {
      return Status;
    }
  }

  return {};
}

} // namespace AOT
} // namespace SSVM
