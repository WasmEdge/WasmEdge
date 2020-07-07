// SPDX-License-Identifier: Apache-2.0
#include "aot/compiler.h"
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
#include <llvm/Transforms/IPO/AlwaysInliner.h>

#if LLVM_VERSION_MAJOR >= 10
#include <llvm/IR/IntrinsicsX86.h>
#include <llvm/IR/IntrinsicsAArch64.h>
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
static llvm::Type *toLLVMType(llvm::LLVMContext &Context,
                              const SSVM::ValType &ValType);
static std::vector<llvm::Type *>
toLLVMArgsType(llvm::LLVMContext &Context,
               SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::Type *toLLVMRetsType(llvm::LLVMContext &Context,
                                  SSVM::Span<const SSVM::ValType> ValTypes);
static llvm::FunctionType *toLLVMType(llvm::LLVMContext &Context,
                                      const SSVM::AST::FunctionType &FuncType);
static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &Context,
                                          const SSVM::ValType &ValType);
static std::vector<llvm::Value *> getUndefValue(llvm::Type *Ty);
static llvm::Value *packStruct(llvm::IRBuilder<> &Builder, llvm::Value *Vals,
                               unsigned N);
static std::vector<llvm::Value *> unpackStruct(llvm::IRBuilder<> &Builder,
                                               llvm::Value *Struct);
class FunctionCompiler;

} // namespace

struct SSVM::AOT::Compiler::CompileContext {
  llvm::LLVMContext &Context;
  llvm::Module &Module;
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
  std::vector<llvm::Function *> Ctors;
  llvm::GlobalVariable *Trap;
  llvm::GlobalVariable *Call;
  llvm::GlobalVariable *MemGrow;
  llvm::GlobalVariable *MemSize;
  llvm::GlobalVariable *Mem;
  llvm::GlobalVariable *InstrCount;
  llvm::MDNode *Likely;
  CompileContext(llvm::Module &M)
      : Context(M.getContext()), Module(M),
        Trap(new llvm::GlobalVariable(
            Module,
            llvm::FunctionType::get(llvm::Type::getVoidTy(Context),
                                    {llvm::Type::getInt8PtrTy(Context),
                                     llvm::Type::getInt32Ty(Context)},
                                    false)
                ->getPointerTo(),
            false, llvm::GlobalVariable::InternalLinkage, nullptr, "trap")),
        Call(new llvm::GlobalVariable(
            Module,
            llvm::FunctionType::get(llvm::Type::getVoidTy(Context),
                                    {llvm::Type::getInt8PtrTy(Context),
                                     llvm::Type::getInt32Ty(Context),
                                     llvm::Type::getInt8PtrTy(Context),
                                     llvm::Type::getInt8PtrTy(Context)},
                                    false)
                ->getPointerTo(),
            false, llvm::GlobalVariable::InternalLinkage, nullptr, "call")),
        MemGrow(new llvm::GlobalVariable(
            Module,
            llvm::FunctionType::get(llvm::Type::getInt32Ty(Context),
                                    {llvm::Type::getInt8PtrTy(Context),
                                     llvm::Type::getInt32Ty(Context)},
                                    false)
                ->getPointerTo(),
            false, llvm::GlobalVariable::InternalLinkage, nullptr, "memgrow")),
        MemSize(new llvm::GlobalVariable(
            Module,
            llvm::FunctionType::get(llvm::Type::getInt32Ty(Context),
                                    {llvm::Type::getInt8PtrTy(Context)}, false)
                ->getPointerTo(),
            false, llvm::GlobalVariable::InternalLinkage, nullptr, "memsize")),
        Mem(new llvm::GlobalVariable(Module, llvm::Type::getInt8PtrTy(Context),
                                     false, llvm::GlobalValue::ExternalLinkage,
                                     nullptr, "mem")),
        InstrCount(new llvm::GlobalVariable(
            Module, llvm::Type::getInt64Ty(Context), false,
            llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), 0),
            "instr")),
        Likely(llvm::MDTuple::getDistinct(
            Context, {llvm::MDString::get(Context, "branch_weights"),
                      llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
                          Context, llvm::APInt(32, 2000))),
                      llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
                          Context, llvm::APInt(32, 0)))})) {
    Trap->setInitializer(
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(
            Trap->getType()->getPointerElementType())));
    Call->setInitializer(
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(
            Call->getType()->getPointerElementType())));
    MemGrow->setInitializer(
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(
            MemGrow->getType()->getPointerElementType())));
    MemSize->setInitializer(
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(
            MemSize->getType()->getPointerElementType())));
    Mem->setInitializer(
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(
            Mem->getType()->getPointerElementType())));

    Trap->addAttribute(llvm::Attribute::NoReturn);

    new llvm::GlobalVariable(
        Module, llvm::Type::getInt32Ty(Context), true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), kVersion),
        "version");

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
  }
  void callTrap(llvm::IRBuilder<> &Builder, llvm::Value *Ctx,
                llvm::Value *Status) {
    auto *TrapFunc = Builder.CreateLoad(Trap);
    Builder.CreateCall(TrapFunc, {Ctx, Status});
  }
  void callCall(llvm::IRBuilder<> &Builder, llvm::Value *Ctx,
                llvm::Value *FuncIdx, llvm::Value *Args, llvm::Value *Rets) {
    auto *CallFunc = Builder.CreateLoad(Call);
    Builder.CreateCall(CallFunc, {Ctx, FuncIdx, Args, Rets});
  }
  llvm::Value *callMemGrow(llvm::IRBuilder<> &Builder, llvm::Value *Ctx,
                           llvm::Value *NewSize) {
    auto *MemGrowFunc = Builder.CreateLoad(MemGrow);
    return Builder.CreateCall(MemGrowFunc, {Ctx, NewSize});
  }
  llvm::Value *callMemSize(llvm::IRBuilder<> &Builder, llvm::Value *Ctx) {
    auto *MemSizeFunc = Builder.CreateLoad(MemSize);
    return Builder.CreateCall(MemSizeFunc, {Ctx});
  }
};

namespace {

using namespace SSVM;

static bool isVoidReturn(Span<const SSVM::ValType> ValTypes) {
  return ValTypes.empty() ||
         (ValTypes.size() == 1 && ValTypes.front() == ValType::None);
}

static llvm::Type *toLLVMType(llvm::LLVMContext &Context,
                              const ValType &ValType) {
  switch (ValType) {
  case ValType::I32:
    return llvm::Type::getInt32Ty(Context);
  case ValType::I64:
    return llvm::Type::getInt64Ty(Context);
  case ValType::F32:
    return llvm::Type::getFloatTy(Context);
  case ValType::F64:
    return llvm::Type::getDoubleTy(Context);
  default:
    assert(false);
    __builtin_unreachable();
  }
}

static std::vector<llvm::Type *> toLLVMArgsType(llvm::LLVMContext &Context,
                                                Span<const ValType> ValTypes) {
  std::vector<llvm::Type *> Result = {llvm::Type::getInt8PtrTy(Context)};
  Result.reserve(ValTypes.size() + 1);
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(Context, Type));
  }
  return Result;
}

static llvm::Type *toLLVMRetsType(llvm::LLVMContext &Context,
                                  Span<const ValType> ValTypes) {
  if (isVoidReturn(ValTypes)) {
    return llvm::Type::getVoidTy(Context);
  }
  if (ValTypes.size() == 1) {
    return toLLVMType(Context, ValTypes.front());
  }
  std::vector<llvm::Type *> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(Context, Type));
  }
  return llvm::StructType::create(Result);
}

static llvm::FunctionType *toLLVMType(llvm::LLVMContext &Context,
                                      const AST::FunctionType &FuncType) {
  auto ArgsTy = toLLVMArgsType(Context, FuncType.getParamTypes());
  auto RetTy = toLLVMRetsType(Context, FuncType.getReturnTypes());
  return llvm::FunctionType::get(RetTy, ArgsTy, false);
}

static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &Context,
                                          const ValType &ValType) {
  switch (ValType) {
  case ValType::I32:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0);
  case ValType::I64:
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), 0);
  case ValType::F32:
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(Context), 0.0f);
  case ValType::F64:
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(Context), 0.0);
  default:
    assert(false);
    __builtin_unreachable();
  }
}

class FunctionCompiler {
public:
  FunctionCompiler(AOT::Compiler::CompileContext &Context, llvm::Function *F,
                   Span<const ValType> Locals, bool CalculateInstrCount)
      : Context(Context), VMContext(Context.Context), F(F),
        Builder(llvm::BasicBlock::Create(VMContext, "entry", F)) {
    if (F) {
      Builder.setIsFPConstrained(true);
      Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
      Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);
      Ctx = F->arg_begin();
      for (llvm::Argument *Arg = Ctx + 1; Arg != F->arg_end(); ++Arg) {
        llvm::Value *ArgPtr = Builder.CreateAlloca(Arg->getType());
        Builder.CreateStore(Arg, ArgPtr);
        Local.push_back(ArgPtr);
      }

      for (const auto &Type : Locals) {
        llvm::Value *ArgPtr = Builder.CreateAlloca(toLLVMType(VMContext, Type));
        Builder.CreateStore(toLLVMConstantZero(VMContext, Type), ArgPtr);
        Local.push_back(ArgPtr);
      }

      auto *MemPtr = Builder.CreateLoad(Context.Mem);
      LocalMemPtr = Builder.CreateAlloca(MemPtr->getType());
      Builder.CreateStore(MemPtr, LocalMemPtr);

      if (CalculateInstrCount) {
        LocalInstrCount = Builder.CreateAlloca(Builder.getInt64Ty());
        Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
      }
    }
  }

  ~FunctionCompiler() noexcept {
    if (!F) {
      delete Builder.GetInsertBlock();
    }
  }

  Expect<void> compile(const AST::InstrVec &Instrs) {
    for (const auto &Instr : Instrs) {
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
              /// Make the instruction node according to Code.
              if (LocalInstrCount) {
                Builder.CreateStore(
                    Builder.CreateAdd(Builder.CreateLoad(LocalInstrCount),
                                      Builder.getInt64(1)),
                    LocalInstrCount);
              }
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
      updateInstrCount();
      Context.callTrap(Builder, Ctx,
                       Builder.getInt32(uint32_t(ErrCode::Unreachable)));
      Builder.CreateUnreachable();

      Builder.SetInsertPoint(
          llvm::BasicBlock::Create(VMContext, "unreachable.end", F));
      for (auto *Undef : getUndefValue(F->getReturnType())) {
        Stack.push_back(Undef);
      }
      break;
    }
    case OpCode::Nop:
      break;
    case OpCode::Return: {
      updateInstrCount();
      auto *Ty = F->getReturnType();
      if (Ty->isVoidTy()) {
        Builder.CreateRetVoid();
      } else if (Ty->isStructTy()) {
        const auto Count = Ty->getStructNumElements();
        if (Stack.size() < Count) {
          return Unexpect(ErrCode::ValidationFailed);
        }
        Builder.CreateAggregateRet(Stack.data() + Stack.size() - Count, Count);
      } else {
        if (Stack.empty()) {
          return Unexpect(ErrCode::ValidationFailed);
        }
        Builder.CreateRet(Stack.back());
      }
      Builder.SetInsertPoint(llvm::BasicBlock::Create(VMContext, "ret.end", F));
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
      auto *Block = llvm::BasicBlock::Create(VMContext, "block", F);
      auto *EndBlock = llvm::BasicBlock::Create(VMContext, "block.end", F);
      Builder.CreateBr(Block);

      enterBlock(EndBlock, true, Instr.getResultType());
      Builder.SetInsertPoint(Block);
      compile(Instr.getBody());
      buildPHI(std::array{Instr.getResultType()}, leaveBlock(EndBlock));
      break;
    }
    case OpCode::Loop: {
      auto *Loop = llvm::BasicBlock::Create(VMContext, "loop", F);
      auto *EndLoop = llvm::BasicBlock::Create(VMContext, "loop.end", F);
      Builder.CreateBr(Loop);

      enterBlock(Loop, false, Instr.getResultType());
      Builder.SetInsertPoint(Loop);
      compile(Instr.getBody());
      buildPHI(std::array{Instr.getResultType()}, leaveBlock(EndLoop));
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
      llvm::Value *Cond =
          Builder.CreateICmpNE(Stack.back(), Builder.getInt32(0));
      Stack.pop_back();

      auto *Then = llvm::BasicBlock::Create(VMContext, "then", F);
      auto *Else = llvm::BasicBlock::Create(VMContext, "else", F);
      auto *EndIf = llvm::BasicBlock::Create(VMContext, "if.end", F);
      Builder.CreateCondBr(Cond, Then, Else);

      enterBlock(EndIf, true, Instr.getResultType());
      Builder.SetInsertPoint(Then);
      compile(Instr.getIfStatement());
      auto IfResult = leaveBlock(EndIf);

      enterBlock(EndIf, true, Instr.getResultType());
      Builder.SetInsertPoint(Else);
      compile(Instr.getElseStatement());
      auto ElseResult = leaveBlock(EndIf);

      IfResult.reserve(IfResult.size() + ElseResult.size());
      IfResult.insert(IfResult.end(), ElseResult.begin(), ElseResult.end());

      buildPHI(std::array{Instr.getResultType()}, IfResult);

      break;
    }
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::BrControlInstruction &Instr) {
    const unsigned int Label = Instr.getLabelIndex();
    switch (Instr.getOpCode()) {
    case OpCode::Br: {
      if (!setLableJumpPHI(Label)) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      Builder.CreateBr(getLabel(Label));
      Builder.SetInsertPoint(llvm::BasicBlock::Create(VMContext, "br.end", F));
      break;
    }
    case OpCode::Br_if: {
      llvm::Value *Cond =
          Builder.CreateICmpNE(Stack.back(), Builder.getInt32(0));
      Stack.pop_back();
      if (!setLableJumpPHI(Label)) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      llvm::BasicBlock *Next =
          llvm::BasicBlock::Create(VMContext, "br_if.end", F);
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
    auto LabelTable = Instr.getLabelTable();
    switch (Instr.getOpCode()) {
    case OpCode::Br_table: {
      if (!setLableJumpPHI(Instr.getLabelIndex())) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      llvm::SwitchInst *Switch = Builder.CreateSwitch(
          Stack.back(), getLabel(Instr.getLabelIndex()), LabelTable.size());
      for (size_t I = 0; I < LabelTable.size(); ++I) {
        if (!setLableJumpPHI(LabelTable[I])) {
          return Unexpect(ErrCode::ValidationFailed);
        }
        Switch->addCase(Builder.getInt32(I), getLabel(LabelTable[I]));
      }
      llvm::BasicBlock *End =
          llvm::BasicBlock::Create(VMContext, "switch.end", F);
      Builder.SetInsertPoint(End);

      break;
    }
    default:
      __builtin_unreachable();
    }

    return {};
  }
  Expect<void> compile(const AST::CallControlInstruction &Instr) {
    updateInstrCount();
    switch (Instr.getOpCode()) {
    case OpCode::Call:
      return compileCallOp(Instr.getFuncIndex());
    case OpCode::Call_indirect: {
      return compileIndirectCallOp(Instr.getFuncIndex());
    }
    default:
      __builtin_unreachable();
    }
  }
  Expect<void> compile(const AST::ParametricInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::Drop:
      Stack.pop_back();
      break;
    case OpCode::Select: {
      llvm::Value *Cond =
          Builder.CreateICmpNE(Stack.back(), Builder.getInt32(0));
      Stack.pop_back();
      llvm::Value *False = Stack.back();
      Stack.pop_back();
      llvm::Value *True = Stack.back();
      Stack.back() = Builder.CreateSelect(Cond, True, False);
      break;
    }
    default:
      __builtin_unreachable();
    }

    return {};
  }
  Expect<void> compile(const AST::VariableInstruction &Instr) {
    /// Get variable index.
    const unsigned int Index = Instr.getVariableIndex();

    /// Check OpCode and run the specific instruction.
    switch (Instr.getOpCode()) {
    case OpCode::Local__get:
      Stack.push_back(Builder.CreateLoad(Local[Index]));
      break;
    case OpCode::Local__set:
      Builder.CreateStore(Stack.back(), Local[Index]);
      Stack.pop_back();
      break;
    case OpCode::Local__tee:
      Builder.CreateStore(Stack.back(), Local[Index]);
      break;
    case OpCode::Global__get:
      if (Index >= Context.Globals.size()) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      Stack.push_back(Builder.CreateLoad(Context.Globals[Index]));
      break;
    case OpCode::Global__set:
      if (Index >= Context.Globals.size()) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      Builder.CreateStore(Stack.back(), Context.Globals[Index]);
      Stack.pop_back();
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
                           Builder.getInt32Ty());
    case OpCode::I64__load:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt64Ty());
    case OpCode::F32__load:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getFloatTy());
    case OpCode::F64__load:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getDoubleTy());
    case OpCode::I32__load8_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt8Ty(), Builder.getInt32Ty(), true);
    case OpCode::I32__load8_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt8Ty(), Builder.getInt32Ty(), false);
    case OpCode::I32__load16_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt16Ty(), Builder.getInt32Ty(), true);
    case OpCode::I32__load16_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt16Ty(), Builder.getInt32Ty(), false);
    case OpCode::I64__load8_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt8Ty(), Builder.getInt64Ty(), true);
    case OpCode::I64__load8_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt8Ty(), Builder.getInt64Ty(), false);
    case OpCode::I64__load16_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt16Ty(), Builder.getInt64Ty(), true);
    case OpCode::I64__load16_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt16Ty(), Builder.getInt64Ty(), false);
    case OpCode::I64__load32_s:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt32Ty(), Builder.getInt64Ty(), true);
    case OpCode::I64__load32_u:
      return compileLoadOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                           Builder.getInt32Ty(), Builder.getInt64Ty(), false);

    case OpCode::I32__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Builder.getInt32Ty());
    case OpCode::I64__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Builder.getInt64Ty());
    case OpCode::F32__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Builder.getFloatTy());
    case OpCode::F64__store:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Builder.getDoubleTy());
    case OpCode::I32__store8:
    case OpCode::I64__store8:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Builder.getInt8Ty(), true);
    case OpCode::I32__store16:
    case OpCode::I64__store16:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Builder.getInt16Ty(), true);
    case OpCode::I64__store32:
      return compileStoreOp(Instr.getMemoryOffset(), Instr.getMemoryAlign(),
                            Builder.getInt32Ty(), true);
    case OpCode::Memory__size:
      Stack.push_back(Context.callMemSize(Builder, Ctx));
      break;
    case OpCode::Memory__grow:
      Stack.push_back(Context.callMemGrow(Builder, Ctx, Stack.back()));
      Builder.CreateStore(Builder.CreateLoad(Context.Mem), LocalMemPtr);
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::ConstInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I32__const:
      Stack.push_back(
          Builder.getInt32(std::get<uint32_t>(Instr.getConstValue())));
      break;
    case OpCode::I64__const:
      Stack.push_back(
          Builder.getInt64(std::get<uint64_t>(Instr.getConstValue())));
      break;
    case OpCode::F32__const:
      Stack.push_back(llvm::ConstantFP::get(
          Builder.getFloatTy(), std::get<float>(Instr.getConstValue())));
      break;
    case OpCode::F64__const:
      Stack.push_back(llvm::ConstantFP::get(
          Builder.getDoubleTy(), std::get<double>(Instr.getConstValue())));
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::UnaryNumericInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I32__eqz:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpEQ(Stack.back(), Builder.getInt32(0)),
          Builder.getInt32Ty());
      break;
    case OpCode::I64__eqz:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpEQ(Stack.back(), Builder.getInt64(0)),
          Builder.getInt32Ty());
      break;
    case OpCode::I32__clz:
    case OpCode::I64__clz:
      Stack.back() = Builder.CreateBinaryIntrinsic(
          llvm::Intrinsic::ctlz, Stack.back(), Builder.getFalse());
      break;
    case OpCode::I32__ctz:
    case OpCode::I64__ctz:
      Stack.back() = Builder.CreateBinaryIntrinsic(
          llvm::Intrinsic::cttz, Stack.back(), Builder.getFalse());
      break;
    case OpCode::I32__popcnt:
    case OpCode::I64__popcnt:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, Stack.back());
      break;
    case OpCode::F32__abs:
    case OpCode::F64__abs:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::fabs, Stack.back());
      break;
    case OpCode::F32__neg:
    case OpCode::F64__neg:
      Stack.back() = Builder.CreateFNeg(Stack.back());
      break;
    case OpCode::F32__ceil:
    case OpCode::F64__ceil:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ceil, Stack.back());
      break;
    case OpCode::F32__floor:
    case OpCode::F64__floor:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::floor, Stack.back());
      break;
    case OpCode::F32__trunc:
    case OpCode::F64__trunc:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Stack.back());
      break;
    case OpCode::F32__nearest:
    case OpCode::F64__nearest: {
      const bool IsFloat = Instr.getOpCode() == OpCode::F32__nearest;
      const uint32_t VectorSize = IsFloat ? 4 : 2;
      llvm::Value *Value = Stack.back();

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
        Stack.back() = Ret;
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
        Stack.back() = Ret;
        break;
      }
#endif

      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::nearbyint, Value);
      break;
    }
    case OpCode::F32__sqrt:
    case OpCode::F64__sqrt:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, Stack.back());
      break;
    case OpCode::I32__wrap_i64:
      Stack.back() = Builder.CreateTrunc(Stack.back(), Builder.getInt32Ty());
      break;
    case OpCode::I32__trunc_f32_s:
    case OpCode::I32__trunc_f64_s:
      Stack.back() = Builder.CreateFPToSI(Stack.back(), Builder.getInt32Ty());
      break;
    case OpCode::I32__trunc_f32_u:
    case OpCode::I32__trunc_f64_u:
      Stack.back() = Builder.CreateFPToUI(Stack.back(), Builder.getInt32Ty());
      break;
    case OpCode::I64__extend_i32_s:
      Stack.back() = Builder.CreateSExt(Stack.back(), Builder.getInt64Ty());
      break;
    case OpCode::I64__extend_i32_u:
      Stack.back() = Builder.CreateZExt(Stack.back(), Builder.getInt64Ty());
      break;
    case OpCode::I64__trunc_f32_s:
    case OpCode::I64__trunc_f64_s:
      Stack.back() = Builder.CreateFPToSI(Stack.back(), Builder.getInt64Ty());
      break;
    case OpCode::I64__trunc_f32_u:
    case OpCode::I64__trunc_f64_u:
      Stack.back() = Builder.CreateFPToUI(Stack.back(), Builder.getInt64Ty());
      break;
    case OpCode::F32__convert_i32_s:
    case OpCode::F32__convert_i64_s:
      Stack.back() = Builder.CreateSIToFP(Stack.back(), Builder.getFloatTy());
      break;
    case OpCode::F32__convert_i32_u:
    case OpCode::F32__convert_i64_u:
      Stack.back() = Builder.CreateUIToFP(Stack.back(), Builder.getFloatTy());
      break;
    case OpCode::F64__convert_i32_s:
    case OpCode::F64__convert_i64_s:
      Stack.back() = Builder.CreateSIToFP(Stack.back(), Builder.getDoubleTy());
      break;
    case OpCode::F64__convert_i32_u:
    case OpCode::F64__convert_i64_u:
      Stack.back() = Builder.CreateUIToFP(Stack.back(), Builder.getDoubleTy());
      break;
    case OpCode::F32__demote_f64:
      Stack.back() = Builder.CreateFPTrunc(Stack.back(), Builder.getFloatTy());
      break;
    case OpCode::F64__promote_f32:
      Stack.back() = Builder.CreateFPExt(Stack.back(), Builder.getDoubleTy());
      break;
    case OpCode::I32__reinterpret_f32:
      Stack.back() = Builder.CreateBitCast(Stack.back(), Builder.getInt32Ty());
      break;
    case OpCode::I64__reinterpret_f64:
      Stack.back() = Builder.CreateBitCast(Stack.back(), Builder.getInt64Ty());
      break;
    case OpCode::F32__reinterpret_i32:
      Stack.back() = Builder.CreateBitCast(Stack.back(), Builder.getFloatTy());
      break;
    case OpCode::F64__reinterpret_i64:
      Stack.back() = Builder.CreateBitCast(Stack.back(), Builder.getDoubleTy());
      break;
    case OpCode::I32__extend8_s:
      Stack.back() = Builder.CreateSExt(
          Builder.CreateTrunc(Stack.back(), Builder.getInt8Ty()),
          Builder.getInt32Ty());
      break;
    case OpCode::I32__extend16_s:
      Stack.back() = Builder.CreateSExt(
          Builder.CreateTrunc(Stack.back(), Builder.getInt16Ty()),
          Builder.getInt32Ty());
      break;
    case OpCode::I64__extend8_s:
      Stack.back() = Builder.CreateSExt(
          Builder.CreateTrunc(Stack.back(), Builder.getInt8Ty()),
          Builder.getInt64Ty());
      break;
    case OpCode::I64__extend16_s:
      Stack.back() = Builder.CreateSExt(
          Builder.CreateTrunc(Stack.back(), Builder.getInt16Ty()),
          Builder.getInt64Ty());
      break;
    case OpCode::I64__extend32_s:
      Stack.back() = Builder.CreateSExt(
          Builder.CreateTrunc(Stack.back(), Builder.getInt32Ty()),
          Builder.getInt64Ty());
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }
  Expect<void> compile(const AST::BinaryNumericInstruction &Instr) {
    llvm::Value *RHS = Stack.back();
    Stack.pop_back();
    switch (Instr.getOpCode()) {
    case OpCode::I32__eq:
    case OpCode::I64__eq:
      Stack.back() = Builder.CreateZExt(Builder.CreateICmpEQ(Stack.back(), RHS),
                                        Builder.getInt32Ty());
      break;
    case OpCode::I32__ne:
    case OpCode::I64__ne:
      Stack.back() = Builder.CreateZExt(Builder.CreateICmpNE(Stack.back(), RHS),
                                        Builder.getInt32Ty());
      break;
    case OpCode::I32__lt_s:
    case OpCode::I64__lt_s:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpSLT(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::I32__lt_u:
    case OpCode::I64__lt_u:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpULT(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::I32__gt_s:
    case OpCode::I64__gt_s:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpSGT(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::I32__gt_u:
    case OpCode::I64__gt_u:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpUGT(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::I32__le_s:
    case OpCode::I64__le_s:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpSLE(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::I32__le_u:
    case OpCode::I64__le_u:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpULE(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::I32__ge_s:
    case OpCode::I64__ge_s:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpSGE(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::I32__ge_u:
    case OpCode::I64__ge_u:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpUGE(Stack.back(), RHS), Builder.getInt32Ty());
      break;

    case OpCode::F32__eq:
    case OpCode::F64__eq:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateFCmpOEQ(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::F32__ne:
    case OpCode::F64__ne:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateFCmpUNE(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::F32__lt:
    case OpCode::F64__lt:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateFCmpOLT(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::F32__gt:
    case OpCode::F64__gt:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateFCmpOGT(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::F32__le:
    case OpCode::F64__le:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateFCmpOLE(Stack.back(), RHS), Builder.getInt32Ty());
      break;
    case OpCode::F32__ge:
    case OpCode::F64__ge:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateFCmpOGE(Stack.back(), RHS), Builder.getInt32Ty());
      break;

    case OpCode::I32__add:
    case OpCode::I64__add:
      Stack.back() = Builder.CreateAdd(Stack.back(), RHS);
      break;
    case OpCode::I32__sub:
    case OpCode::I64__sub:
      Stack.back() = Builder.CreateSub(Stack.back(), RHS);
      break;
    case OpCode::I32__mul:
    case OpCode::I64__mul:
      Stack.back() = Builder.CreateMul(Stack.back(), RHS);
      break;
    case OpCode::I32__div_s:
    case OpCode::I64__div_s:
      Stack.back() = Builder.CreateSDiv(Stack.back(), RHS);
      break;
    case OpCode::I32__div_u:
    case OpCode::I64__div_u:
      Stack.back() = Builder.CreateUDiv(Stack.back(), RHS);
      break;
    case OpCode::I32__rem_s:
    case OpCode::I64__rem_s: {
      // handle INT32_MIN % -1
      llvm::ConstantInt *IntZero = Instr.getOpCode() == OpCode::I32__rem_s
                                       ? Builder.getInt32(0)
                                       : Builder.getInt64(0);
      llvm::ConstantInt *IntMinusOne = Instr.getOpCode() == OpCode::I32__rem_s
                                           ? Builder.getInt32(int32_t(-1))
                                           : Builder.getInt64(int64_t(-1));
      llvm::ConstantInt *IntMin =
          Instr.getOpCode() == OpCode::I32__rem_s
              ? Builder.getInt32(std::numeric_limits<int32_t>::min())
              : Builder.getInt64(std::numeric_limits<int64_t>::min());

      llvm::BasicBlock *CurrentBB = Builder.GetInsertBlock();
      llvm::BasicBlock *NoOverflowBB =
          llvm::BasicBlock::Create(VMContext, "no.overflow", F);
      llvm::BasicBlock *EndBB =
          llvm::BasicBlock::Create(VMContext, "end.overflow", F);

      llvm::Value *LHS = Stack.back();
      llvm::Value *NotOverflow =
          Builder.CreateOr(Builder.CreateICmpNE(LHS, IntMin),
                           Builder.CreateICmpNE(RHS, IntMinusOne));

      Builder.CreateCondBr(NotOverflow, NoOverflowBB, EndBB, Context.Likely);

      Builder.SetInsertPoint(NoOverflowBB);
      llvm::Value *Ret1 = Builder.CreateSRem(LHS, RHS);
      Builder.CreateBr(EndBB);

      Builder.SetInsertPoint(EndBB);
      llvm::PHINode *Ret = Builder.CreatePHI(Ret1->getType(), 2);
      Ret->addIncoming(Ret1, NoOverflowBB);
      Ret->addIncoming(IntZero, CurrentBB);

      Stack.back() = Ret;
      break;
    }
    case OpCode::I32__rem_u:
    case OpCode::I64__rem_u:
      Stack.back() = Builder.CreateURem(Stack.back(), RHS);
      break;
    case OpCode::I32__and:
    case OpCode::I64__and:
      Stack.back() = Builder.CreateAnd(Stack.back(), RHS);
      break;
    case OpCode::I32__or:
    case OpCode::I64__or:
      Stack.back() = Builder.CreateOr(Stack.back(), RHS);
      break;
    case OpCode::I32__xor:
    case OpCode::I64__xor:
      Stack.back() = Builder.CreateXor(Stack.back(), RHS);
      break;
    case OpCode::I32__shl:
    case OpCode::I64__shl:
      Stack.back() = Builder.CreateShl(Stack.back(), RHS);
      break;
    case OpCode::I32__shr_s:
    case OpCode::I64__shr_s:
      Stack.back() = Builder.CreateAShr(Stack.back(), RHS);
      break;
    case OpCode::I32__shr_u:
    case OpCode::I64__shr_u:
      Stack.back() = Builder.CreateLShr(Stack.back(), RHS);
      break;
    case OpCode::I32__rotl:
      Stack.back() =
          Builder.CreateIntrinsic(llvm::Intrinsic::fshl, {Builder.getInt32Ty()},
                                  {Stack.back(), Stack.back(), RHS});
      break;
    case OpCode::I32__rotr:
      Stack.back() =
          Builder.CreateIntrinsic(llvm::Intrinsic::fshr, {Builder.getInt32Ty()},
                                  {Stack.back(), Stack.back(), RHS});
      break;
    case OpCode::I64__rotl:
      Stack.back() =
          Builder.CreateIntrinsic(llvm::Intrinsic::fshl, {Builder.getInt64Ty()},
                                  {Stack.back(), Stack.back(), RHS});
      break;
    case OpCode::I64__rotr:
      Stack.back() =
          Builder.CreateIntrinsic(llvm::Intrinsic::fshr, {Builder.getInt64Ty()},
                                  {Stack.back(), Stack.back(), RHS});
      break;

    case OpCode::F32__add:
    case OpCode::F64__add:
      Stack.back() = Builder.CreateFAdd(Stack.back(), RHS);
      break;
    case OpCode::F32__sub:
    case OpCode::F64__sub:
      Stack.back() = Builder.CreateFSub(Stack.back(), RHS);
      break;
    case OpCode::F32__mul:
    case OpCode::F64__mul:
      Stack.back() = Builder.CreateFMul(Stack.back(), RHS);
      break;
    case OpCode::F32__div:
    case OpCode::F64__div:
      Stack.back() = Builder.CreateFDiv(Stack.back(), RHS);
      break;
    case OpCode::F32__min:
    case OpCode::F64__min: {
      llvm::Value *LHS = Stack.back();
      llvm::Type *FpTy = Instr.getOpCode() == OpCode::F32__min
                             ? Builder.getFloatTy()
                             : Builder.getDoubleTy();
      llvm::Type *IntTy = Instr.getOpCode() == OpCode::F32__min
                              ? Builder.getInt32Ty()
                              : Builder.getInt64Ty();

      llvm::Value *UEQ = Builder.CreateFCmpUEQ(LHS, RHS);
      llvm::Value *UNO = Builder.CreateFCmpUNO(LHS, RHS);

      llvm::Value *LHSInt = Builder.CreateBitCast(LHS, IntTy);
      llvm::Value *RHSInt = Builder.CreateBitCast(RHS, IntTy);
      llvm::Value *OrInt = Builder.CreateOr(LHSInt, RHSInt);
      llvm::Value *OrFp = Builder.CreateBitCast(OrInt, FpTy);

      llvm::Value *AddFp = Builder.CreateFAdd(LHS, RHS);

      llvm::CallInst *MinFp =
          Builder.CreateBinaryIntrinsic(llvm::Intrinsic::minnum, LHS, RHS);
      MinFp->setHasNoNaNs(true);

      llvm::Value *Ret = Builder.CreateSelect(
          UEQ, Builder.CreateSelect(UNO, AddFp, OrFp), MinFp);
      Stack.back() = Ret;
    } break;
    case OpCode::F32__max:
    case OpCode::F64__max: {
      llvm::Value *LHS = Stack.back();
      llvm::Type *FpTy = Instr.getOpCode() == OpCode::F32__max
                             ? Builder.getFloatTy()
                             : Builder.getDoubleTy();
      llvm::Type *IntTy = Instr.getOpCode() == OpCode::F32__max
                              ? Builder.getInt32Ty()
                              : Builder.getInt64Ty();

      llvm::Value *UEQ = Builder.CreateFCmpUEQ(LHS, RHS);
      llvm::Value *UNO = Builder.CreateFCmpUNO(LHS, RHS);

      llvm::Value *LHSInt = Builder.CreateBitCast(LHS, IntTy);
      llvm::Value *RHSInt = Builder.CreateBitCast(RHS, IntTy);
      llvm::Value *AndInt = Builder.CreateAnd(LHSInt, RHSInt);
      llvm::Value *AndFp = Builder.CreateBitCast(AndInt, FpTy);

      llvm::Value *AddFp = Builder.CreateFAdd(LHS, RHS);

      llvm::CallInst *MaxFp =
          Builder.CreateBinaryIntrinsic(llvm::Intrinsic::maxnum, LHS, RHS);
      MaxFp->setHasNoNaNs(true);

      llvm::Value *Ret = Builder.CreateSelect(
          UEQ, Builder.CreateSelect(UNO, AddFp, AndFp), MaxFp);
      Stack.back() = Ret;
    } break;
    case OpCode::F32__copysign:
    case OpCode::F64__copysign:
      Stack.back() = Builder.CreateBinaryIntrinsic(llvm::Intrinsic::copysign,
                                                   Stack.back(), RHS);
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }
  void compileSignedTruncSat(llvm::ConstantFP *MinFp, llvm::ConstantFP *MaxFp,
                             llvm::ConstantInt *MinInt,
                             llvm::ConstantInt *MaxInt) {
    llvm::BasicBlock *CurrBB = Builder.GetInsertBlock();
    llvm::BasicBlock *NormBB =
        llvm::BasicBlock::Create(VMContext, "sats.norm", F);
    llvm::BasicBlock *NotMinBB =
        llvm::BasicBlock::Create(VMContext, "sats.notmin", F);
    llvm::BasicBlock *NotMaxBB =
        llvm::BasicBlock::Create(VMContext, "sats.notmax", F);
    llvm::BasicBlock *EndBB =
        llvm::BasicBlock::Create(VMContext, "sats.end", F);
    llvm::Value *Value = Stack.back();

    Builder.CreateCondBr(Builder.CreateFCmpORD(Value, Value), NormBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NormBB);
    Builder.CreateCondBr(Builder.CreateFCmpUGT(Value, MinFp), NotMinBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NotMinBB);
    Builder.CreateCondBr(Builder.CreateFCmpULT(Value, MaxFp), NotMaxBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NotMaxBB);
    llvm::Type *Ty = MaxInt->getType();
    llvm::Value *IntValue = Builder.CreateFPToSI(Value, Ty);
    Builder.CreateBr(EndBB);

    Builder.SetInsertPoint(EndBB);
    llvm::PHINode *PHIRet = Builder.CreatePHI(Ty, 4);
    PHIRet->addIncoming(llvm::ConstantInt::get(Ty, 0), CurrBB);
    PHIRet->addIncoming(MinInt, NormBB);
    PHIRet->addIncoming(MaxInt, NotMinBB);
    PHIRet->addIncoming(IntValue, NotMaxBB);

    Stack.back() = PHIRet;
  }
  void compileUnsignedTruncSat(llvm::ConstantFP *MaxFp,
                               llvm::ConstantInt *MaxInt) {
    llvm::BasicBlock *CurrBB = Builder.GetInsertBlock();
    llvm::BasicBlock *NormBB =
        llvm::BasicBlock::Create(VMContext, "sats.norm", F);
    llvm::BasicBlock *NotMaxBB =
        llvm::BasicBlock::Create(VMContext, "sats.notmax", F);
    llvm::BasicBlock *EndBB =
        llvm::BasicBlock::Create(VMContext, "sats.end", F);
    llvm::Value *Value = Stack.back();

    Builder.CreateCondBr(
        Builder.CreateFCmpOGT(Value,
                              llvm::ConstantFP::get(Value->getType(), 0.0)),
        NormBB, EndBB, Context.Likely);

    Builder.SetInsertPoint(NormBB);
    Builder.CreateCondBr(Builder.CreateFCmpOLT(Value, MaxFp), NotMaxBB, EndBB,
                         Context.Likely);

    Builder.SetInsertPoint(NotMaxBB);
    llvm::Type *Ty = MaxInt->getType();
    llvm::Value *IntValue = Builder.CreateFPToSI(Value, Ty);
    Builder.CreateBr(EndBB);

    Builder.SetInsertPoint(EndBB);
    llvm::PHINode *PHIRet = Builder.CreatePHI(Ty, 3);
    PHIRet->addIncoming(llvm::ConstantInt::get(Ty, 0), CurrBB);
    PHIRet->addIncoming(MaxInt, NormBB);
    PHIRet->addIncoming(IntValue, NotMaxBB);

    Stack.back() = PHIRet;
  }
  Expect<void> compile(const AST::TruncSatNumericInstruction &Instr) {
    if (Instr.getOpCode() != OpCode::Trunc_sat) {
      __builtin_unreachable();
    }

    switch (Instr.getSubOp()) {
    case 0x00U: // i32.trunc_sat_f32_s
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+31f)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+31f)),
          Builder.getInt32(INT32_MIN), Builder.getInt32(INT32_MAX));
      break;
    case 0x01U: // i32.trunc_sat_f32_u
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+32f)),
          Builder.getInt32(UINT32_MAX));
      break;
    case 0x02U: // i32.trunc_sat_f64_s
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+31)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+31)),
          Builder.getInt32(INT32_MIN), Builder.getInt32(INT32_MAX));
      break;
    case 0x03U: // i32.trunc_sat_f64_u
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+32)),
          Builder.getInt32(UINT32_MAX));
      break;
    case 0x04U: // i64.trunc_sat_f32_s
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+63f)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+63f)),
          Builder.getInt64(INT64_MIN), Builder.getInt64(INT64_MAX));
      break;
    case 0x05U: // i64.trunc_sat_f32_u
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+64f)),
          Builder.getInt64(UINT64_MAX));
      break;
    case 0x06U: // i64.trunc_sat_f64_s
      compileSignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(-0x1p+63)),
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+63)),
          Builder.getInt64(INT64_MIN), Builder.getInt64(INT64_MAX));
      break;
    case 0x07U: // i64.trunc_sat_f64_u
      compileUnsignedTruncSat(
          llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(0x1p+64)),
          Builder.getInt64(UINT64_MAX));
      break;
    default:
      __builtin_unreachable();
    }
    return {};
  }

  void updateInstrCount() {
    if (LocalInstrCount) {
      Builder.CreateStore(
          Builder.CreateAdd(Builder.CreateLoad(LocalInstrCount),
                            Builder.CreateLoad(Context.InstrCount)),
          Context.InstrCount);
      Builder.CreateStore(Builder.getInt64(0), LocalInstrCount);
    }
  }

  Expect<void> epilog() {
    updateInstrCount();
    auto *Ty = F->getReturnType();
    if (Ty->isVoidTy()) {
      Builder.CreateRetVoid();
    } else if (Ty->isStructTy()) {
      const auto Count = Ty->getStructNumElements();
      if (Stack.size() < Count) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      Builder.CreateAggregateRet(Stack.data() + Stack.size() - Count, Count);
    } else {
      if (Stack.empty()) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      Builder.CreateRet(Stack.back());
    }
    return {};
  }

  static llvm::Constant *evaluate(const AST::InstrVec &Instrs,
                                  AOT::Compiler::CompileContext &Context) {
    FunctionCompiler FC(Context, nullptr, {}, false);
    FC.compile(Instrs);
    return llvm::cast<llvm::Constant>(FC.Stack.back());
  }

private:
  Expect<void> compileCallOp(const unsigned int FuncIndex) {
    const auto &FuncType =
        *Context.FunctionTypes[std::get<0>(Context.Functions[FuncIndex])];
    const auto &Function = std::get<1>(Context.Functions[FuncIndex]);

    if (Stack.size() < FuncType.getParamTypes().size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    auto Begin = Stack.end() - FuncType.getParamTypes().size();
    auto End = Stack.end();
    std::vector<llvm::Value *> Args = {Ctx};
    Args.insert(Args.end(), Begin, End);

    llvm::Value *Ret = Builder.CreateCall(Function, Args);
    Stack.erase(Begin, End);
    llvm::Type *Ty = Function->getReturnType();
    if (Ty->isVoidTy()) {
      // nothing to do
    } else if (Ty->isStructTy()) {
      for (auto *Val : unpackStruct(Builder, Ret)) {
        Stack.push_back(Val);
      }
    } else {
      Stack.push_back(Ret);
    }

    Builder.CreateStore(Builder.CreateLoad(Context.Mem), LocalMemPtr);
    return {};
  }

  Expect<void> compileIndirectCallOp(const unsigned int FuncTypeIndex) {
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    if (Stack.size() < FuncType.getParamTypes().size()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    auto Begin = Stack.end() - FuncType.getParamTypes().size() - 1;
    auto End = Stack.end() - 1;
    std::vector<llvm::Value *> Args = {Ctx};
    Args.insert(Args.end(), Begin, End);

    std::vector<std::pair<size_t, llvm::Function *>> Table;
    for (uint32_t I = 0; I < Context.Elements.size(); ++I) {
      const size_t FuncIdx = Context.Elements[I];
      if (std::get<0>(Context.Functions[FuncIdx]) == FuncTypeIndex) {
        Table.emplace_back(I, std::get<1>(Context.Functions[FuncIdx]));
      }
    }
    llvm::BasicBlock *OK =
        llvm::BasicBlock::Create(VMContext, "call_indirect.end", F);
    llvm::BasicBlock *Error =
        llvm::BasicBlock::Create(VMContext, "call_indirect.error", F);
    llvm::SwitchInst *Switch =
        Builder.CreateSwitch(Stack.back(), Error, Table.size());

    const bool HasReturnValue = !isVoidReturn(FuncType.getReturnTypes());
    std::vector<std::tuple<llvm::Value *, llvm::BasicBlock *>> ReturnValues;

    for (const auto &[Value, Func] : Table) {
      llvm::BasicBlock *Entry = llvm::BasicBlock::Create(
          VMContext, "call_indirect." + std::to_string(Value), F);
      Builder.SetInsertPoint(Entry);
      llvm::Value *Ret = Builder.CreateCall(Func, Args);
      Builder.CreateBr(OK);
      Switch->addCase(Builder.getInt32(Value), Entry);
      if (HasReturnValue) {
        ReturnValues.emplace_back(Ret, Entry);
      }
    }

    Builder.SetInsertPoint(Error);
    updateInstrCount();
    Context.callTrap(Builder, Ctx,
                     Builder.getInt32(uint32_t(ErrCode::Unreachable)));
    Builder.CreateUnreachable();

    Stack.erase(Begin, Stack.end());

    Builder.SetInsertPoint(OK);
    if (HasReturnValue) {
      buildPHI(FuncType.getReturnTypes(), ReturnValues);
    }

    Builder.CreateStore(Builder.CreateLoad(Context.Mem), LocalMemPtr);
    return {};
  }

  Expect<void> compileLoadOp(unsigned int Offset, unsigned Alignment,
                             llvm::Type *LoadTy) {
    if (Stack.empty()) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    llvm::Value *Off = Builder.CreateZExt(Stack.back(), Builder.getInt64Ty());
    if (Offset != 0) {
      Off = Builder.CreateAdd(Off, Builder.getInt64(Offset));
    }
    llvm::Value *VPtr =
        Builder.CreateInBoundsGEP(Builder.CreateLoad(LocalMemPtr), {Off});
    llvm::Value *Ptr =
        Builder.CreateBitCast(VPtr, llvm::PointerType::getUnqual(LoadTy));
    llvm::LoadInst *LoadInst = Builder.CreateLoad(Ptr);
    LoadInst->setAlignment(Align(UINT64_C(1) << Alignment));
    Stack.back() = LoadInst;
    return {};
  }
  Expect<void> compileLoadOp(unsigned int Offset, unsigned Alignment,
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
  Expect<void> compileStoreOp(unsigned int Offset, unsigned Alignment,
                              llvm::Type *LoadTy, bool Trunc = false) {
    if (Stack.size() < 2) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    llvm::Value *V = Stack.back();
    Stack.pop_back();
    if (Trunc) {
      V = Builder.CreateTrunc(V, LoadTy);
    }

    llvm::Value *Off = Builder.CreateZExt(Stack.back(), Builder.getInt64Ty());
    Stack.pop_back();
    if (Offset != 0) {
      Off = Builder.CreateAdd(Off, Builder.getInt64(Offset));
    }

    llvm::Value *VPtr =
        Builder.CreateInBoundsGEP(Builder.CreateLoad(LocalMemPtr), {Off});
    llvm::Value *Ptr =
        Builder.CreateBitCast(VPtr, llvm::PointerType::getUnqual(LoadTy));
    llvm::StoreInst *StoreInst = Builder.CreateStore(V, Ptr);
    StoreInst->setAlignment(Align(UINT64_C(1) << Alignment));
    return {};
  }

  void enterBlock(llvm::BasicBlock *JumpTarget, bool IsForward,
                  ValType ResultType) {
    ControlStack.emplace_back(Stack.size(), JumpTarget, IsForward, ResultType,
                              0);
  }

  std::vector<std::tuple<llvm::Value *, llvm::BasicBlock *>>
  leaveBlock(llvm::BasicBlock *NextTarget) {
    auto &Entry = ControlStack.back();
    if (auto &Type = std::get<kReturnType>(Entry); Type != ValType::None) {
      llvm::Value *V =
          Stack.empty()
              ? llvm::UndefValue::get(toLLVMType(Context.Context, Type))
              : Stack.back();
      std::get<kReturnPHI>(Entry).emplace_back(V, Builder.GetInsertBlock());
    }
    Builder.CreateBr(NextTarget);
    Builder.SetInsertPoint(NextTarget);

    Stack.erase(Stack.begin() + std::get<kStackSize>(ControlStack.back()),
                Stack.end());
    std::vector<std::tuple<llvm::Value *, llvm::BasicBlock *>> Result =
        std::move(std::get<kReturnPHI>(ControlStack.back()));

    ControlStack.pop_back();
    return Result;
  }

  void buildPHI(
      Span<const ValType> RetType,
      Span<const std::tuple<llvm::Value *, llvm::BasicBlock *>> Incomings) {
    if (isVoidReturn(RetType)) {
      return;
    }
    llvm::Type *Ty = toLLVMRetsType(Context.Context, RetType);
    llvm::Value *Node = nullptr;
    if (Incomings.size() == 0) {
      Node = llvm::UndefValue::get(Ty);
    } else if (Incomings.size() == 1) {
      Node = std::get<0>(Incomings.front());
    } else {
      llvm::PHINode *PHIRet = Builder.CreatePHI(Ty, Incomings.size());
      for (auto &[Value, BB] : Incomings) {
        PHIRet->addIncoming(Value, BB);
      }
      Node = PHIRet;
    }
    if (Ty->isStructTy()) {
      for (auto *Val : unpackStruct(Builder, Node)) {
        Stack.push_back(Val);
      }
    } else {
      Stack.push_back(Node);
    }
  }

  bool setLableJumpPHI(unsigned int Index) {
    if (Index >= ControlStack.size()) {
      return false;
    }
    auto &Entry = *(ControlStack.rbegin() + Index);
    if (auto &Type = std::get<kReturnType>(Entry);
        Type != ValType::None && std::get<kIsForward>(Entry)) {
      llvm::Value *V =
          Stack.empty()
              ? llvm::UndefValue::get(toLLVMType(Context.Context, Type))
              : Stack.back();
      std::get<kReturnPHI>(Entry).emplace_back(V, Builder.GetInsertBlock());
    }
    return true;
  }

  llvm::BasicBlock *getLabel(unsigned int Index) const {
    return std::get<kJumpBlock>(*(ControlStack.rbegin() + Index));
  }

  AOT::Compiler::CompileContext &Context;
  llvm::LLVMContext &VMContext;
  std::vector<llvm::Value *> Local;
  std::vector<llvm::Value *> Stack;
  llvm::Argument *Ctx = nullptr;
  llvm::Value *LocalMemPtr = nullptr;
  llvm::Value *LocalInstrCount = nullptr;
  static inline constexpr size_t kStackSize = 0;
  static inline constexpr size_t kJumpBlock = 1;
  static inline constexpr size_t kIsForward = 2;
  static inline constexpr size_t kReturnType = 3;
  static inline constexpr size_t kReturnPHI = 4;
  std::vector<
      std::tuple<size_t, llvm::BasicBlock *, bool, ValType,
                 std::vector<std::tuple<llvm::Value *, llvm::BasicBlock *>>>>
      ControlStack;
  llvm::Function *F;
  llvm::IRBuilder<> Builder;
};

static std::vector<llvm::Value *> getUndefValue(llvm::Type *Ty) {
  std::vector<llvm::Value *> Ret;
  if (Ty->isVoidTy()) {
    // nothing to do
  } else if (Ty->isStructTy()) {
    const unsigned N = Ty->getStructNumElements();
    Ret.reserve(N);
    for (unsigned I = 0; I < N; ++I) {
      Ret.push_back(llvm::UndefValue::get(Ty->getStructElementType(I)));
    }
  } else {
    Ret.reserve(1);
    Ret.push_back(llvm::UndefValue::get(Ty));
  }
  return Ret;
}

static llvm::Value *packStruct(llvm::IRBuilder<> &Builder, llvm::Value *Vals,
                               unsigned N) {
  llvm::Type *Ty = nullptr;
  {
    std::vector<llvm::Type *> Types;
    Types.reserve(N);
    for (unsigned I = 0; I < N; ++I) {
      Types.push_back((Vals + I)->getType());
    }
    Ty = llvm::StructType::create(Types);
  }
  llvm::Value *Ret = llvm::UndefValue::get(Ty);
  for (unsigned I = 0; I < N; ++I) {
    Ret = Builder.CreateInsertValue(Ret, Vals + I, {I});
  }
  return Ret;
}

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

  llvm::LLVMContext VMContext;
  auto LLModule = std::make_unique<llvm::Module>(LLPath.native(), VMContext);
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
        /// compile Functions in module. (FuncionSec, CodeSec)
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
        if (const AST::ExportSection *ExportSec = Module.getExportSection()) {
          ;
        }
        return {};
      })
      .and_then([&]() -> Expect<void> {
        /// create ctor
        llvm::Function *Ctor = llvm::Function::Create(
            llvm::FunctionType::get(
                llvm::Type::getVoidTy(Context->Context),
                {Context->Trap->getType()->getPointerElementType(),
                 Context->Call->getType()->getPointerElementType(),
                 Context->MemGrow->getType()->getPointerElementType(),
                 Context->MemSize->getType()->getPointerElementType()},
                false),
            llvm::GlobalValue::ExternalLinkage, "ctor", LLModule.get());
        Ctor->addFnAttr(llvm::Attribute::StrictFP);

        llvm::IRBuilder<> Builder(
            llvm::BasicBlock::Create(Context->Context, "entry", Ctor));
        Builder.setIsFPConstrained(true);
        Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
        Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);
        Builder.CreateStore(Ctor->arg_begin(), Context->Trap);
        Builder.CreateStore(Ctor->arg_begin() + 1, Context->Call);
        Builder.CreateStore(Ctor->arg_begin() + 2, Context->MemGrow);
        Builder.CreateStore(Ctor->arg_begin() + 3, Context->MemSize);
        for (auto &F : Context->Ctors) {
          Builder.CreateCall(F);
        }
        Builder.CreateRetVoid();

        /// create wasm.code and wasm.size
        {
          llvm::Constant *Content = llvm::ConstantDataArray::getString(
              VMContext,
              llvm::StringRef(reinterpret_cast<const char *>(Data.data()),
                              Data.size()),
              false);
          new llvm::GlobalVariable(Context->Module, Content->getType(), false,
                                   llvm::GlobalValue::ExternalLinkage, Content,
                                   "wasm.code");
          new llvm::GlobalVariable(Context->Module, Builder.getInt32Ty(), false,
                                   llvm::GlobalValue::ExternalLinkage,
                                   Builder.getInt32(Data.size()), "wasm.size");
        }

        {
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

          // Register the AA manager first so that our version is the one used.
          FAM.registerPass([&] { return PB.buildDefaultAAPipeline(); });

          // Register the target library analysis directly and give it a
          // customized preset TLI.
          auto TLII = std::make_unique<llvm::TargetLibraryInfoImpl>(
              llvm::Triple(LLModule->getTargetTriple()));
          FAM.registerPass([&] { return llvm::TargetLibraryAnalysis(*TLII); });
#if LLVM_VERSION_MAJOR <= 9
          MAM.registerPass([&] { return llvm::TargetLibraryAnalysis(*TLII); });
#endif

          // Register all the basic analyses with the managers.
          PB.registerModuleAnalyses(MAM);
          PB.registerCGSCCAnalyses(CGAM);
          PB.registerFunctionAnalyses(FAM);
          PB.registerLoopAnalyses(LAM);
          PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

          llvm::ModulePassManager MPM(false);

          MPM.addPass(PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::O3));
          MPM.addPass(llvm::AlwaysInlinerPass());

          llvm::legacy::PassManager CodeGenPasses;
          CodeGenPasses.add(llvm::createTargetTransformInfoWrapperPass(
              TM->getTargetIRAnalysis()));

          // Add LibraryInfo.
          CodeGenPasses.add(new llvm::TargetLibraryInfoWrapperPass(*TLII));

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

          MPM.run(*LLModule, MAM);
          {
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
  auto &VMContext = Context->Context;
  /// Iterate and compile import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    /// Get data from import description.
    const auto &ExtType = ImpDesc->getExternalType();
    const std::string ModName(ImpDesc->getModuleName());
    const std::string ExtName(ImpDesc->getExternalName());
    const std::string FullName = '$' + ModName + '.' + ExtName;
    const std::string FullCtxName = FullName + ".ctx";

    /// Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: /// Function type index
    {
      const uint32_t FuncIndex = Context->Functions.size();
      /// Get the function type index in module.
      unsigned int *TypeIdx = nullptr;
      if (auto Res = ImpDesc->getExternalContent<uint32_t>()) {
        TypeIdx = *Res;
      } else {
        return Unexpect(ErrCode::ValidationFailed);
      }
      if (*TypeIdx >= Context->FunctionTypes.size()) {
        return Unexpect(ErrCode::ValidationFailed);
      }
      const auto &FuncType = *Context->FunctionTypes[*TypeIdx];

      llvm::FunctionType *FTy = toLLVMType(VMContext, FuncType);
      llvm::Function *F = llvm::Function::Create(
          FTy, llvm::GlobalValue::InternalLinkage, FullName, Context->Module);
      F->addFnAttr(llvm::Attribute::StrictFP);
      llvm::Type *Ty = FTy->getReturnType();

      llvm::Argument *Ctx = F->arg_begin();

      llvm::BasicBlock *Entry = llvm::BasicBlock::Create(VMContext, "entry", F);
      llvm::IRBuilder<> Builder(Entry);
      Builder.setIsFPConstrained(true);
      Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
      Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);

      llvm::Value *Args;
      if (FTy->getNumParams() == 1) {
        Args = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
      } else {
        Args = Builder.CreateAlloca(
            Builder.getInt8Ty(),
            Builder.getInt64((FTy->getNumParams() - 1) * 8));
      }

      llvm::Value *Rets;
      if (Ty->isVoidTy()) {
        Rets = llvm::ConstantPointerNull::get(Builder.getInt8PtrTy());
      } else if (Ty->isStructTy()) {
        Rets = Builder.CreateAlloca(
            Builder.getInt8Ty(),
            Builder.getInt64(Ty->getStructNumElements() * 8));
      } else {
        Rets = Builder.CreateAlloca(Builder.getInt8Ty(), Builder.getInt64(8));
      }

      unsigned I = 0;
      for (llvm::Argument *Arg = Ctx + 1; Arg != F->arg_end(); ++Arg, ++I) {
        llvm::Value *Ptr = Builder.CreateConstInBoundsGEP1_64(Args, I * 8);
        Builder.CreateStore(
            Arg, Builder.CreateBitCast(
                     Ptr, llvm::PointerType::getUnqual(Arg->getType())));
      }

      Context->callCall(Builder, Ctx, Builder.getInt32(FuncIndex), Args, Rets);

      if (Ty->isVoidTy()) {
        Builder.CreateRetVoid();
      } else if (Ty->isStructTy()) {
        const unsigned N = Ty->getStructNumElements();
        std::vector<llvm::Value *> Ret;
        Ret.reserve(N);
        for (unsigned I = 0; I < N; ++I) {
          llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(Rets, I);
          llvm::Value *Ptr = Builder.CreateBitCast(
              VPtr, llvm::PointerType::getUnqual(Ty->getStructElementType(I)));
          Ret.push_back(Builder.CreateLoad(Ptr));
        }
        Builder.CreateAggregateRet(Ret.data(), N);
      } else {
        llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(Rets, 0);
        llvm::Value *Ptr = Builder.CreateBitCast(
            VPtr, llvm::PointerType::getUnqual(F->getReturnType()));
        Builder.CreateRet(Builder.CreateLoad(Ptr));
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
  auto &VMContext = Context->Context;
  for (const auto &ExpDesc : ExportSec.getContent()) {
    switch (ExpDesc->getExternalType()) {
    case ExternalType::Function: {
      llvm::Function *Wrapper = llvm::Function::Create(
          llvm::FunctionType::get(llvm::Type::getVoidTy(VMContext),
                                  {llvm::Type::getInt8PtrTy(VMContext),
                                   llvm::Type::getInt8PtrTy(VMContext),
                                   llvm::Type::getInt8PtrTy(VMContext)},
                                  false),
          llvm::GlobalValue::ExternalLinkage,
          "$" + std::string(ExpDesc->getExternalName()), Context->Module);
      Wrapper->addFnAttr(llvm::Attribute::StrictFP);
      llvm::Argument *Ctx = Wrapper->arg_begin();
      llvm::Argument *RawArgs = Ctx + 1;
      llvm::Argument *RawRets = RawArgs + 1;
      llvm::IRBuilder<> Builder(
          llvm::BasicBlock::Create(Ctx->getContext(), "entry", Wrapper));
      Builder.setIsFPConstrained(true);
      Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
      Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);
      llvm::Function *F =
          std::get<1>(Context->Functions[ExpDesc->getExternalIndex()]);
      llvm::Type *Ty = F->getReturnType();

      std::vector<llvm::Value *> Args = {Ctx};
      unsigned I = 0;
      for (llvm::Argument *Arg = F->arg_begin() + 1; Arg != F->arg_end();
           ++Arg, ++I) {
        llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(RawArgs, I * 8);
        llvm::Value *Ptr = Builder.CreateBitCast(
            VPtr, llvm::PointerType::getUnqual(Arg->getType()));
        Args.push_back(Builder.CreateLoad(Ptr));
      }

      llvm::Value *Ret = Builder.CreateCall(F, Args);
      if (Ty->isVoidTy()) {
        // nothing to do
      } else if (Ty->isStructTy()) {
        const unsigned N = Ty->getStructNumElements();
        for (unsigned I = 0; I < N; ++I) {
          llvm::Value *VPtr =
              Builder.CreateConstInBoundsGEP1_64(RawRets, I * 8);
          llvm::Value *Ptr = Builder.CreateBitCast(
              VPtr, llvm::PointerType::getUnqual(Ty->getStructElementType(I)));
          Builder.CreateStore(Builder.CreateExtractValue(Ret, {I}), Ptr);
        }
      } else {
        llvm::Value *VPtr = Builder.CreateConstInBoundsGEP1_64(RawRets, 0);
        llvm::Value *Ptr =
            Builder.CreateBitCast(VPtr, llvm::PointerType::getUnqual(Ty));
        Builder.CreateStore(Ret, Ptr);
      }
      Builder.CreateRetVoid();

      break;
    }
    case ExternalType::Global: {
      llvm::GlobalVariable *G = Context->Globals[ExpDesc->getExternalIndex()];
      G->setLinkage(llvm::GlobalValue::ExternalLinkage);
      G->setName("$" + std::string(ExpDesc->getExternalName()));
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
    const SSVM::ValType &ValType =
        GlobalSec.getContent()[I]->getGlobalType()->getValueType();
    llvm::GlobalVariable *G = new llvm::GlobalVariable(
        Context->Module, toLLVMType(Context->Context, ValType), false,
        llvm::GlobalValue::InternalLinkage,
        FunctionCompiler::evaluate(GlobalSec.getContent()[I]->getInstrs(),
                                   *Context),
        "g." + std::to_string(I));
    Context->Globals.push_back(G);
  }
  return {};
}

Expect<void> Compiler::compile(const AST::MemorySection &MemorySection,
                               const AST::DataSection &DataSec) {
  if (MemorySection.getContent().size() != 1) {
    return Unexpect(ErrCode::ValidationFailed);
  }

  /*
  auto &VMContext = Context->Context;
  llvm::Type *Int32Ty = llvm::Type::getInt32Ty(VMContext);

  // create variable for recording limit
  {
    const auto &Limit = MemorySection.getContent()[0]->getLimit();
    const auto Min = Limit->getMin();
    const auto Max = Limit->getMax();
    new llvm::GlobalVariable(Context->Module, Int32Ty, true,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(Int32Ty, Min), "memMin");
    new llvm::GlobalVariable(Context->Module, Int32Ty, true,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(Int32Ty, Max), "memMax");
  }

  std::vector<char> ResultData;
  for (const auto &DataSeg : DataSec.getContent()) {
    llvm::Constant *Temp =
        FunctionCompiler::evaluate(DataSeg->getInstrs(), *Context);
    const uint64_t Offset = llvm::cast<llvm::ConstantInt>(Temp)->getZExtValue();
    const auto &Data = DataSeg->getData();

    if (ResultData.size() < Offset + Data.size()) {
      ResultData.resize(Offset + Data.size());
    }
    std::copy(Data.cbegin(), Data.cend(), ResultData.begin() + Offset);
  }
  llvm::Function *Ctor = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getVoidTy(VMContext), false),
      llvm::GlobalValue::InternalLinkage, "mem.ctor", Context->Module);

  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Context->Context, "entry", Ctor));
  Builder.setIsFPConstrained(true);
  Builder.setDefaultConstrainedRounding(RoundingMode::rmToNearest);
  Builder.setDefaultConstrainedExcept(ExceptionBehavior::ebIgnore);
  llvm::Constant *Content = llvm::ConstantDataArray::getString(
      VMContext, llvm::StringRef(ResultData.data(), ResultData.size()), false);
  llvm::GlobalVariable *GV =
      new llvm::GlobalVariable(Context->Module, Content->getType(), true,
                               llvm::GlobalVariable::InternalLinkage, Content);

  Builder.CreateMemCpy(
      Builder.CreateInBoundsGEP(Builder.CreateLoad(Context->Mem),
                                Builder.getInt32(0)),
      8, GV, 8, Builder.getInt32(ResultData.size()));
  Builder.CreateRetVoid();

  Context->Ctors.push_back(Ctor);
  */
  return {};
}

Expect<void> Compiler::compile(const AST::TableSection &TableSection,
                               const AST::ElementSection &ElementSection) {
  auto &Elements = Context->Elements;
  for (const auto &Element : ElementSection.getContent()) {
    llvm::Constant *Temp =
        FunctionCompiler::evaluate(Element->getInstrs(), *Context);
    const uint64_t Offset = llvm::cast<llvm::ConstantInt>(Temp)->getZExtValue();
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
      return Unexpect(ErrCode::ValidationFailed);
    }
    const auto &FuncType = *Context->FunctionTypes[TypeIdx];
    llvm::FunctionType *FTy = toLLVMType(Context->Context, FuncType);
    llvm::Function *F = llvm::Function::Create(
        FTy, llvm::GlobalValue::InternalLinkage,
        "f" + std::to_string(Context->Functions.size()), Context->Module);
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

    FunctionCompiler FC(*Context, F, Locals, false);
    if (auto Status = FC.compile(Code->getInstrs()); !Status) {
      return Status;
    }
    if (auto Status = FC.epilog(); !Status) {
      return Status;
    }
  }

  return {};
}

} // namespace AOT
} // namespace SSVM
