// SPDX-License-Identifier: Apache-2.0
#include "compiler/compiler.h"
#include "common/ast/instruction.h"
#include "common/ast/section.h"
#include "common/types.h"
#include "compiler/hostfunc/ethereum/calldatacopy.h"
#include "compiler/hostfunc/ethereum/callstatic.h"
#include "compiler/hostfunc/ethereum/finish.h"
#include "compiler/hostfunc/ethereum/getcalldatasize.h"
#include "compiler/hostfunc/ethereum/getcaller.h"
#include "compiler/hostfunc/ethereum/returndatacopy.h"
#include "compiler/hostfunc/ethereum/revert.h"
#include "compiler/hostfunc/ethereum/storageload.h"
#include "compiler/hostfunc/ethereum/storagestore.h"
#include "compiler/hostfunc/wasi/args_Get.h"
#include "compiler/hostfunc/wasi/args_SizesGet.h"
#include "compiler/hostfunc/wasi/environ_Get.h"
#include "compiler/hostfunc/wasi/environ_SizesGet.h"
#include "compiler/hostfunc/wasi/fd_Close.h"
#include "compiler/hostfunc/wasi/fd_FdstatGet.h"
#include "compiler/hostfunc/wasi/fd_FdstatSetFlags.h"
#include "compiler/hostfunc/wasi/fd_PrestatDirName.h"
#include "compiler/hostfunc/wasi/fd_PrestatGet.h"
#include "compiler/hostfunc/wasi/fd_Read.h"
#include "compiler/hostfunc/wasi/fd_Seek.h"
#include "compiler/hostfunc/wasi/fd_Write.h"
#include "compiler/hostfunc/wasi/path_Open.h"
#include "compiler/hostfunc/wasi/proc_Exit.h"
#include "compiler/library.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>

namespace SSVM {
namespace Compiler {

static llvm::Function *createCtxCall(llvm::Function *F,
                                     llvm::GlobalVariable *Ctx) {
  std::vector<llvm::Type *> ArgTy;
  ArgTy.push_back(Ctx->getType());
  for (auto &Arg : F->args()) {
    ArgTy.push_back(Arg.getType());
  }
  llvm::FunctionType *FTy =
      llvm::FunctionType::get(F->getReturnType(), ArgTy, false);

  llvm::Function *Result = llvm::Function::Create(
      FTy, llvm::GlobalValue::ExternalLinkage, "", F->getParent());

  llvm::BasicBlock *OK =
      llvm::BasicBlock::Create(Ctx->getContext(), "entry", F);
  llvm::IRBuilder<> Builder(OK);
  std::vector<llvm::Value *> Args;
  Args.push_back(Ctx);
  std::transform(F->arg_begin(), F->arg_end(), std::back_inserter(Args),
                 [](llvm::Argument &Arg) { return &Arg; });
  llvm::Value *Ret = Builder.CreateCall(Result, Args);
  if (!F->getReturnType()->isVoidTy()) {
    Builder.CreateRet(Ret);
  } else {
    Builder.CreateRetVoid();
  }

  return Result;
}

struct Compiler::CompileContext {
  llvm::LLVMContext &Context;
  llvm::Module &Module;
  std::vector<const AST::FunctionType *> FunctionTypes;
  std::vector<unsigned int> Elements;
  std::vector<
      std::tuple<unsigned int, llvm::Function *, SSVM::AST::CodeSegment *>>
      Functions;
  std::vector<llvm::GlobalVariable *> Globals;
  std::vector<llvm::Function *> Ctors;
  llvm::GlobalVariable *LibCtx;
  llvm::Function *Trap;
  llvm::Function *MemorySize;
  llvm::Function *MemoryGrow;
  llvm::GlobalVariable *Memory;
  CompileContext(llvm::Module &M)
      : Context(M.getContext()), Module(M),
        Trap(llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(Context),
                                    {llvm::Type::getInt32Ty(Context)}, false),
            llvm::GlobalValue::InternalLinkage, "$trap.", Module)),
        MemorySize(llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(Context), false),
            llvm::GlobalValue::InternalLinkage, "$memory.size.", Module)),
        MemoryGrow(llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(Context),
                                    {llvm::Type::getInt32Ty(Context)}, false),
            llvm::GlobalValue::InternalLinkage, "$memory.grow.", Module)),
        LibCtx(new llvm::GlobalVariable(
            Module, llvm::Type::getInt8Ty(Context), false,
            llvm::GlobalValue::ExternalLinkage, nullptr, "$lib.ctx")),
        Memory(new llvm::GlobalVariable(
            Module, llvm::Type::getInt8PtrTy(Context), false,
            llvm::GlobalValue::ExternalLinkage, nullptr, "$memory")) {
    Trap->addFnAttr(llvm::Attribute::NoReturn);
    createCtxCall(Trap, LibCtx)->setName("$trap");
    createCtxCall(MemorySize, LibCtx)->setName("$memory.size");
    createCtxCall(MemoryGrow, LibCtx)->setName("$memory.grow");
  }
};
} // namespace Compiler
} // namespace SSVM

namespace {

static llvm::Type *toLLVMType(llvm::LLVMContext &Context,
                              const SSVM::ValType &ValType) {
  switch (ValType) {
  case SSVM::ValType::I32:
    return llvm::Type::getInt32Ty(Context);
  case SSVM::ValType::I64:
    return llvm::Type::getInt64Ty(Context);
  case SSVM::ValType::F32:
    return llvm::Type::getFloatTy(Context);
  case SSVM::ValType::F64:
    return llvm::Type::getDoubleTy(Context);
  default:
    assert(false);
    __builtin_unreachable();
  }
}

static std::vector<llvm::Type *>
toLLVMType(llvm::LLVMContext &Context,
           const std::vector<SSVM::ValType> &ValTypes) {
  std::vector<llvm::Type *> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(Context, Type));
  }
  return Result;
}

static llvm::FunctionType *
toLLVMType(llvm::LLVMContext &Context,
           const SSVM ::AST::FunctionType &FuncType) {
  llvm::Type *RetTy =
      FuncType.getReturnTypes().empty()
          ? llvm::Type::getVoidTy(Context)
          : toLLVMType(Context, FuncType.getReturnTypes().front());
  return llvm::FunctionType::get(
      RetTy, toLLVMType(Context, FuncType.getParamTypes()), false);
}

static llvm::Constant *toLLVMConstantZero(llvm::LLVMContext &Context,
                                          const SSVM::ValType &ValType) {
  switch (ValType) {
  case SSVM::ValType::I32:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0);
  case SSVM::ValType::I64:
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), 0);
  case SSVM::ValType::F32:
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(Context), 0.0f);
  case SSVM::ValType::F64:
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(Context), 0.0);
  default:
    assert(false);
    __builtin_unreachable();
  }
}

class FunctionCompiler {
public:
  using ErrCode = SSVM::Compiler::ErrCode;
  using OpCode = SSVM::AST::Instruction::OpCode;
  FunctionCompiler(SSVM::Compiler::Compiler::CompileContext &Context,
                   llvm::Function *F, const std::vector<SSVM::ValType> &Locals)
      : Context(Context), VMContext(Context.Context), F(F),
        Builder(llvm::BasicBlock::Create(VMContext, "entry", F)) {
    if (F) {
      for (auto &Arg : F->args()) {
        llvm::Value *ArgPtr = Builder.CreateAlloca(Arg.getType());
        Builder.CreateStore(&Arg, ArgPtr);
        Local.push_back(ArgPtr);
      }

      for (const auto &Type : Locals) {
        llvm::Value *ArgPtr = Builder.CreateAlloca(toLLVMType(VMContext, Type));
        Builder.CreateStore(toLLVMConstantZero(VMContext, Type), ArgPtr);
        Local.push_back(ArgPtr);
      }
    }
  }

  ~FunctionCompiler() noexcept {
    if (!F) {
      delete Builder.GetInsertBlock();
    }
  }

  ErrCode compile(const SSVM::AST::InstrVec &Instrs) {
    for (const auto &Instr : Instrs) {
      if (ErrCode Status = SSVM::AST::dispatchInstruction(
              Instr->getOpCode(),
              [this, &Instr](const auto &&Arg) {
                if constexpr (std::is_void_v<
                                  typename std::decay_t<decltype(Arg)>::type>) {
                  /// If the Code not matched, return null pointer.
                  return ErrCode::Failed;
                } else {
                  /// Make the instruction node according to Code.
                  return compile(
                      *static_cast<
                          const typename std::decay_t<decltype(Arg)>::type *>(
                          Instr.get()));
                }
              });
          Status != ErrCode::Success) {
        return Status;
      }
    }
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::ControlInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::Unreachable:
      Builder.CreateCall(Context.Trap,
                         {Builder.getInt32(uint32_t(ErrCode::Unreachable))});
      if (!F->getReturnType()->isVoidTy()) {
        Stack.push_back(llvm::UndefValue::get(F->getReturnType()));
      }
      break;
    case OpCode::Nop:
      break;
    case OpCode::Return:
      if (F->getReturnType()->isVoidTy()) {
        Builder.CreateRetVoid();
      } else {
        Builder.CreateRet(Stack.back());
      }
      Builder.SetInsertPoint(llvm::BasicBlock::Create(VMContext, "ret.end", F));
      break;
    default:
      __builtin_unreachable();
    }
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::BlockControlInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::Block: {
      auto *Block = llvm::BasicBlock::Create(VMContext, "block", F);
      auto *EndBlock = llvm::BasicBlock::Create(VMContext, "block.end", F);
      Builder.CreateBr(Block);

      enterBlock(EndBlock, EndBlock);
      Builder.SetInsertPoint(Block);
      compile(Instr.getBody());
      leaveBlock();
      break;
    }
    case OpCode::Loop: {
      auto *Loop = llvm::BasicBlock::Create(VMContext, "loop", F);
      auto *EndLoop = llvm::BasicBlock::Create(VMContext, "loop.end", F);
      Builder.CreateBr(Loop);

      enterBlock(Loop, EndLoop);
      Builder.SetInsertPoint(Loop);
      compile(Instr.getBody());
      leaveBlock();
      break;
    }
    default:
      __builtin_unreachable();
    }
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::IfElseControlInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::If: {
      llvm::Value *Cond =
          Builder.CreateICmpNE(Stack.back(), Builder.getInt32(0));
      Stack.pop_back();

      auto *Then = llvm::BasicBlock::Create(VMContext, "then", F);
      auto *Else = llvm::BasicBlock::Create(VMContext, "else", F);
      auto *EndIf = llvm::BasicBlock::Create(VMContext, "if.end", F);
      Builder.CreateCondBr(Cond, Then, EndIf);

      enterBlock(EndIf, Else);
      Builder.SetInsertPoint(Then);
      compile(Instr.getIfStatement());
      leaveBlock();

      enterBlock(EndIf, EndIf);
      compile(Instr.getIfStatement());
      leaveBlock();

      break;
    }
    default:
      __builtin_unreachable();
    }
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::BrControlInstruction &Instr) {
    const unsigned int Label = Instr.getLabelIndex();
    if (Label >= ControlStack.size()) {
      return ErrCode::Failed;
    }
    llvm::BasicBlock *Target = getLabel(Label);
    switch (Instr.getOpCode()) {
    case OpCode::Br:
      Builder.CreateBr(Target);
      Builder.SetInsertPoint(llvm::BasicBlock::Create(VMContext, "br.end", F));
      break;
    case OpCode::Br_if: {
      llvm::Value *Cond =
          Builder.CreateICmpNE(Stack.back(), Builder.getInt32(0));
      Stack.pop_back();
      llvm::BasicBlock *Next =
          llvm::BasicBlock::Create(VMContext, "br_if.end", F);
      Builder.CreateCondBr(Cond, Target, Next);
      Builder.SetInsertPoint(Next);
      break;
    }
    default:
      __builtin_unreachable();
    }
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::BrTableControlInstruction &Instr) {
    const std::vector<unsigned int> &LabelTable = *Instr.getLabelTable();
    switch (Instr.getOpCode()) {
    case OpCode::Br_table: {
      llvm::SwitchInst *Switch = Builder.CreateSwitch(
          Stack.back(), getLabel(Instr.getLabelIndex()), LabelTable.size());
      for (size_t I = 0; I < LabelTable.size(); ++I) {
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

    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::CallControlInstruction &Instr) {
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
  ErrCode compile(const SSVM::AST::ParametricInstruction &Instr) {
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

    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::VariableInstruction &Instr) {
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
        return ErrCode::Failed;
      }
      Stack.push_back(Builder.CreateLoad(Context.Globals[Index]));
      break;
    case OpCode::Global__set:
      if (Index >= Context.Globals.size()) {
        return ErrCode::Failed;
      }
      Builder.CreateStore(Stack.back(), Context.Globals[Index]);
      Stack.pop_back();
      break;
    default:
      __builtin_unreachable();
    }

    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::MemoryInstruction &Instr) {
    llvm::ConstantInt *Offset = Builder.getInt32(Instr.getMemoryOffset());
    switch (Instr.getOpCode()) {
    case OpCode::I32__load:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt32Ty());
    case OpCode::I64__load:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt64Ty());
    case OpCode::F32__load:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getFloatTy());
    case OpCode::F64__load:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getDoubleTy());
    case OpCode::I32__load8_s:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt8Ty(),
                           Builder.getInt32Ty(), true);
    case OpCode::I32__load8_u:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt8Ty(),
                           Builder.getInt32Ty(), false);
    case OpCode::I32__load16_s:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt16Ty(),
                           Builder.getInt32Ty(), true);
    case OpCode::I32__load16_u:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt16Ty(),
                           Builder.getInt32Ty(), false);
    case OpCode::I64__load8_s:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt8Ty(),
                           Builder.getInt64Ty(), true);
    case OpCode::I64__load8_u:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt8Ty(),
                           Builder.getInt64Ty(), false);
    case OpCode::I64__load16_s:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt16Ty(),
                           Builder.getInt64Ty(), true);
    case OpCode::I64__load16_u:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt16Ty(),
                           Builder.getInt64Ty(), false);
    case OpCode::I64__load32_s:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt32Ty(),
                           Builder.getInt64Ty(), true);
    case OpCode::I64__load32_u:
      return compileLoadOp(Instr.getMemoryOffset(), Builder.getInt32Ty(),
                           Builder.getInt64Ty(), false);

    case OpCode::I32__store:
      return compileStoreOp(Instr.getMemoryOffset(), Builder.getInt32Ty());
    case OpCode::I64__store:
      return compileStoreOp(Instr.getMemoryOffset(), Builder.getInt64Ty());
    case OpCode::F32__store:
      return compileStoreOp(Instr.getMemoryOffset(), Builder.getFloatTy());
    case OpCode::F64__store:
      return compileStoreOp(Instr.getMemoryOffset(), Builder.getDoubleTy());
    case OpCode::I32__store8:
    case OpCode::I64__store8:
      return compileStoreOp(Instr.getMemoryOffset(), Builder.getInt8Ty(), true);
    case OpCode::I32__store16:
    case OpCode::I64__store16:
      return compileStoreOp(Instr.getMemoryOffset(), Builder.getInt16Ty(),
                            true);
    case OpCode::I64__store32:
      return compileStoreOp(Instr.getMemoryOffset(), Builder.getInt32Ty(),
                            true);
    case OpCode::Memory__size:
      Stack.push_back(Builder.CreateCall(Context.MemorySize));
      break;
    case OpCode::Memory__grow:
      Stack.back() = Builder.CreateCall(Context.MemoryGrow, {Stack.back()});
      break;
    default:
      __builtin_unreachable();
    }
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::ConstInstruction &Instr) {
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
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::UnaryNumericInstruction &Instr) {
    switch (Instr.getOpCode()) {
    case OpCode::I32__eqz:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpEQ(Stack.back(), Builder.getInt32(0)),
          Builder.getInt32Ty());
      break;
    case OpCode::I32__clz:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctlz, Stack.back());
      break;
    case OpCode::I32__ctz:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::cttz, Stack.back());
      break;
    case OpCode::I32__popcnt:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, Stack.back());
      break;
    case OpCode::I64__eqz:
      Stack.back() = Builder.CreateZExt(
          Builder.CreateICmpEQ(Stack.back(), Builder.getInt64(0)),
          Builder.getInt32Ty());
      break;
    case OpCode::I64__clz:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctlz, Stack.back());
      break;
    case OpCode::I64__ctz:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::cttz, Stack.back());
      break;
    case OpCode::I64__popcnt:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, Stack.back());
      break;
    case OpCode::F32__abs:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::fabs, Stack.back());
      break;
    case OpCode::F32__neg:
      Stack.back() = Builder.CreateFNeg(Stack.back());
      break;
    case OpCode::F32__ceil:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ceil, Stack.back());
      break;
    case OpCode::F32__floor:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::floor, Stack.back());
      break;
    case OpCode::F32__trunc:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Stack.back());
      break;
    case OpCode::F32__nearest:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::rint, Stack.back());
      break;
    case OpCode::F32__sqrt:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, Stack.back());
      break;
    case OpCode::F64__abs:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::fabs, Stack.back());
      break;
    case OpCode::F64__neg:
      Stack.back() = Builder.CreateFNeg(Stack.back());
      break;
    case OpCode::F64__ceil:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::ceil, Stack.back());
      break;
    case OpCode::F64__floor:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::floor, Stack.back());
      break;
    case OpCode::F64__trunc:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::trunc, Stack.back());
      break;
    case OpCode::F64__nearest:
      Stack.back() =
          Builder.CreateUnaryIntrinsic(llvm::Intrinsic::rint, Stack.back());
      break;
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
    default:
      __builtin_unreachable();
    }
    return ErrCode::Success;
  }
  ErrCode compile(const SSVM::AST::BinaryNumericInstruction &Instr) {
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
    case OpCode::I64__rem_s:
      Stack.back() = Builder.CreateSRem(Stack.back(), RHS);
      break;
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
      Stack.back() = Builder.CreateBinaryIntrinsic(
          llvm::Intrinsic::experimental_constrained_minnum, Stack.back(), RHS);
      break;
    case OpCode::F32__max:
      Stack.back() = Builder.CreateBinaryIntrinsic(
          llvm::Intrinsic::experimental_constrained_maxnum, Stack.back(), RHS);
      break;
    case OpCode::F32__copysign:
      Stack.back() = Builder.CreateBinaryIntrinsic(llvm::Intrinsic::copysign,
                                                   Stack.back(), RHS);
      break;
    case OpCode::F64__min:
      Stack.back() = Builder.CreateBinaryIntrinsic(
          llvm::Intrinsic::experimental_constrained_minnum, Stack.back(), RHS);
      break;
    case OpCode::F64__max:
      Stack.back() = Builder.CreateBinaryIntrinsic(
          llvm::Intrinsic::experimental_constrained_maxnum, Stack.back(), RHS);
      break;
    case OpCode::F64__copysign:
      Stack.back() = Builder.CreateBinaryIntrinsic(llvm::Intrinsic::copysign,
                                                   Stack.back(), RHS);
      break;
    default:
      __builtin_unreachable();
    }
    return ErrCode::Success;
  }

  void epilog() {
    if (F->getReturnType()->isVoidTy()) {
      Builder.CreateRetVoid();
    } else {
      Builder.CreateRet(Stack.back());
    }
  }

  static llvm::Constant *
  evaluate(const SSVM::AST::InstrVec &Instrs,
           SSVM::Compiler::Compiler::CompileContext &Context) {
    FunctionCompiler FC(Context, nullptr, {});
    FC.compile(Instrs);
    return llvm::cast<llvm::Constant>(FC.Stack.back());
  }

private:
  ErrCode compileCallOp(const unsigned int FuncIndex) {
    const auto &FuncType =
        *Context.FunctionTypes[std::get<0>(Context.Functions[FuncIndex])];
    const auto &Function = std::get<1>(Context.Functions[FuncIndex]);

    if (Stack.size() < FuncType.getParamTypes().size()) {
      return ErrCode::Failed;
    }
    auto Begin = Stack.end() - FuncType.getParamTypes().size();
    auto End = Stack.end();
    llvm::Value *Ret = Builder.CreateCall(
        Function, llvm::ArrayRef<llvm::Value *>(&*Begin, &*End));
    Stack.erase(Begin, End);
    if (!FuncType.getReturnTypes().empty()) {
      Stack.push_back(Ret);
    }

    return ErrCode::Success;
  }

  ErrCode compileIndirectCallOp(const unsigned int FuncTypeIndex) {
    const auto &FuncType = *Context.FunctionTypes[FuncTypeIndex];
    if (Stack.size() < FuncType.getParamTypes().size()) {
      return ErrCode::Failed;
    }
    auto Begin = Stack.end() - FuncType.getParamTypes().size() - 1;
    auto End = Stack.end() - 1;

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

    llvm::PHINode *PHIRet = nullptr;
    if (!FuncType.getReturnTypes().empty()) {
      PHIRet = llvm::PHINode::Create(
          toLLVMType(VMContext, FuncType.getReturnTypes().front()),
          Table.size(), "", OK);
    }

    for (const auto &[Value, Func] : Table) {
      llvm::BasicBlock *Entry = llvm::BasicBlock::Create(
          VMContext, "call_indirect." + std::to_string(Value), F);
      Builder.SetInsertPoint(Entry);
      llvm::Value *Ret = Builder.CreateCall(
          Func, llvm::ArrayRef<llvm::Value *>(&*Begin, &*End));
      Builder.CreateBr(OK);
      Switch->addCase(Builder.getInt32(Value), Entry);
      if (PHIRet) {
        PHIRet->addIncoming(Ret, Entry);
      }
    }

    Builder.SetInsertPoint(Error);
    Builder.CreateCall(Context.Trap,
                       {Builder.getInt32(uint32_t(ErrCode::Unreachable))});
    if (F->getReturnType()->isVoidTy()) {
      Builder.CreateRetVoid();
    } else {
      Builder.CreateRet(llvm::UndefValue::get(F->getReturnType()));
    }

    Stack.erase(Begin, Stack.end());
    if (PHIRet) {
      Stack.push_back(PHIRet);
    }

    Builder.SetInsertPoint(OK);
    return ErrCode::Success;
  }

  ErrCode compileLoadOp(unsigned int Offset, llvm::Type *LoadTy) {
    llvm::Value *O = Stack.back();
    if (Offset != 0) {
      O = Builder.CreateAdd(O, Builder.getInt32(Offset));
    }
    O = Builder.CreateZExt(O, Builder.getInt64Ty());
    llvm::Value *VPtr =
        Builder.CreateInBoundsGEP(Builder.CreateLoad(Context.Memory), {O});
    llvm::Value *Ptr =
        Builder.CreateBitCast(VPtr, llvm::PointerType::getUnqual(LoadTy));
    Stack.back() = Builder.CreateLoad(Ptr);
    return ErrCode::Success;
  }
  ErrCode compileLoadOp(unsigned int Offset, llvm::Type *LoadTy,
                        llvm::Type *ExtendTy, bool Signed) {
    compileLoadOp(Offset, LoadTy);
    if (Signed) {
      Stack.back() = Builder.CreateSExt(Stack.back(), ExtendTy);
    } else {
      Stack.back() = Builder.CreateZExt(Stack.back(), ExtendTy);
    }
    return ErrCode::Success;
  }
  ErrCode compileStoreOp(unsigned int Offset, llvm::Type *LoadTy,
                         bool Trunc = false) {
    llvm::Value *V = Stack.back();
    Stack.pop_back();
    if (Trunc) {
      V = Builder.CreateTrunc(V, LoadTy);
    }

    llvm::Value *O = Stack.back();
    Stack.pop_back();
    if (Offset != 0) {
      O = Builder.CreateAdd(O, Builder.getInt32(Offset));
    }
    O = Builder.CreateZExt(O, Builder.getInt64Ty());

    llvm::Value *VPtr =
        Builder.CreateInBoundsGEP(Builder.CreateLoad(Context.Memory), {O});
    llvm::Value *Ptr =
        Builder.CreateBitCast(VPtr, llvm::PointerType::getUnqual(LoadTy));
    Builder.CreateStore(V, Ptr);
    return ErrCode::Success;
  }

  void enterBlock(llvm::BasicBlock *JumpTarget, llvm::BasicBlock *NextTarget) {
    ControlStack.emplace_back(Stack.size(), JumpTarget, NextTarget);
  }

  void leaveBlock() {
    Stack.resize(std::get<0>(ControlStack.back()));
    Builder.CreateBr(std::get<2>(ControlStack.back()));
    Builder.SetInsertPoint(std::get<2>(ControlStack.back()));
    ControlStack.pop_back();
  }

  llvm::BasicBlock *getLabel(unsigned int Index) const {
    return std::get<1>(*(ControlStack.rbegin() + Index));
  }

  SSVM::Compiler::Compiler::CompileContext &Context;
  llvm::LLVMContext &VMContext;
  std::vector<llvm::Value *> Local;
  std::vector<llvm::Value *> Stack;
  std::vector<std::tuple<size_t, llvm::BasicBlock *, llvm::BasicBlock *>>
      ControlStack;
  llvm::Function *F;
  llvm::IRBuilder<> Builder;
};

} // namespace

namespace SSVM {
namespace Compiler {

ErrCode Compiler::runLoader() {
  Expect<std::unique_ptr<AST::Module>> Res;
  if (WasmPath == "") {
    Res = LoaderEngine.parseModule(WasmCode);
  } else {
    Res = LoaderEngine.parseModule(WasmPath);
  }
  if (!Res) {
    return ErrCode::Failed;
  } else {
    Mod = std::move(*Res);
  }
  return ErrCode::Success;
}

ErrCode Compiler::compile() {
  /// Load code.
  if (ErrCode Status = runLoader(); Status != ErrCode::Success) {
    return Status;
  }

  Lib.reset(new Library);
  auto Module = std::make_unique<llvm::Module>("wasm.ll", Lib->getContext());
  CompileContext NewContext(*Module);
  struct RAIICleanup {
    RAIICleanup(CompileContext *&Context, CompileContext &NewContext)
        : Context(Context) {
      Context = &NewContext;
    }
    ~RAIICleanup() { Context = nullptr; }
    CompileContext *&Context;
  };
  RAIICleanup Cleanup(Context, NewContext);

  /// compile code.
  if (Mod) {
    if (ErrCode Status = compile(*Mod); Status != ErrCode::Success) {
      return Status;
    }
  }

  {
    llvm::Function *Ctor = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getVoidTy(Context->Context), false),
        llvm::GlobalValue::ExternalLinkage, "$ctor", Context->Module);

    llvm::IRBuilder<> Builder(
        llvm::BasicBlock::Create(Context->Context, "entry", Ctor));
    for (auto &F : Context->Ctors) {
      Builder.CreateCall(F);
    }
    Builder.CreateRetVoid();
  }

  llvm::verifyModule(*Module, &llvm::errs());

  // optimize
  {
    llvm::PassBuilder PB;
    llvm::LoopAnalysisManager LAM(false);
    llvm::FunctionAnalysisManager FAM(false);
    llvm::CGSCCAnalysisManager CGAM(false);
    llvm::ModuleAnalysisManager MAM(false);
    FAM.registerPass([&] { return PB.buildDefaultAAPipeline(); });
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::O1).run(*Module, MAM);
  }

  // write module for debug
  {
    int Fd;
    llvm::sys::fs::openFileForWrite("wasm.ll", Fd);
    llvm::raw_fd_ostream OS(Fd, true);
    Module->print(OS, nullptr);
  }

  Lib->setModule(std::move(Module));

  if (Config.hasVMType(SSVM::VM::Configure::VMType::Ewasm)) {
    /// Ewasm case, insert EEI host functions.
    auto *EVMEnv = getEnvironment<SSVM::VM::EVMEnvironment>(
        SSVM::VM::Configure::VMType::Ewasm);

    Lib->setHostFunction<EEICallDataCopy>("ethereum", "callDataCopy", *EVMEnv);
    Lib->setHostFunction<EEICallStatic>("ethereum", "callStatic", *EVMEnv);
    Lib->setHostFunction<EEIFinish>("ethereum", "finish", *EVMEnv);
    Lib->setHostFunction<EEIGetCallDataSize>("ethereum", "getCallDataSize",
                                             *EVMEnv);
    Lib->setHostFunction<EEIGetCaller>("ethereum", "getCaller", *EVMEnv);
    Lib->setHostFunction<EEIReturnDataCopy>("ethereum", "returnDataCopy",
                                            *EVMEnv);
    Lib->setHostFunction<EEIRevert>("ethereum", "revert", *EVMEnv);
    Lib->setHostFunction<EEIStorageLoad>("ethereum", "storageLoad", *EVMEnv);
    Lib->setHostFunction<EEIStorageStore>("ethereum", "storageStore", *EVMEnv);
  }

  if (Config.hasVMType(SSVM::VM::Configure::VMType::Wasi)) {
    /// Wasi case, insert Wasi host functions.
    auto *WasiEnv = getEnvironment<SSVM::VM::WasiEnvironment>(
        SSVM::VM::Configure::VMType::Wasi);

    Lib->setHostFunction<WasiArgsGet>("wasi_unstable", "args_get", *WasiEnv);
    Lib->setHostFunction<WasiArgsSizesGet>("wasi_unstable", "args_sizes_get",
                                           *WasiEnv);
    Lib->setHostFunction<WasiEnvironGet>("wasi_unstable", "environ_get",
                                         *WasiEnv);
    Lib->setHostFunction<WasiEnvironSizesGet>("wasi_unstable",
                                              "environ_sizes_get", *WasiEnv);
    Lib->setHostFunction<WasiFdClose>("wasi_unstable", "fd_close", *WasiEnv);
    Lib->setHostFunction<WasiFdFdstatGet>("wasi_unstable", "fd_fdstat_get",
                                          *WasiEnv);
    Lib->setHostFunction<WasiFdFdstatSetFlags>("wasi_unstable",
                                               "fd_fdstat_set_flags", *WasiEnv);
    Lib->setHostFunction<WasiFdPrestatDirName>("wasi_unstable",
                                               "fd_prestat_dir_name", *WasiEnv);
    Lib->setHostFunction<WasiFdPrestatGet>("wasi_unstable", "fd_prestat_get",
                                           *WasiEnv);
    Lib->setHostFunction<WasiFdRead>("wasi_unstable", "fd_read", *WasiEnv);
    Lib->setHostFunction<WasiFdSeek>("wasi_unstable", "fd_seek", *WasiEnv);
    Lib->setHostFunction<WasiFdWrite>("wasi_unstable", "fd_write", *WasiEnv);
    Lib->setHostFunction<WasiPathOpen>("wasi_unstable", "path_open", *WasiEnv);
    Lib->setHostFunction<WasiProcExit>("wasi_unstable", "proc_exit", *WasiEnv);
  }

  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::Module &Module) {
  /// Compile Function Types
  if (const AST::TypeSection *TypeSec = Module.getTypeSection()) {
    if (ErrCode Status = compile(*TypeSec); Status != ErrCode::Success) {
      return Status;
    }
  }

  /// Compile ImportSection
  if (const AST::ImportSection *ImportSec = Module.getImportSection()) {
    if (ErrCode Status = compile(*ImportSec); Status != ErrCode::Success) {
      return Status;
    }
  }

  /// Compile GlobalSection
  if (const AST::GlobalSection *GlobSec = Module.getGlobalSection()) {
    if (ErrCode Status = compile(*GlobSec); Status != ErrCode::Success) {
      return Status;
    }
  }

  /// Compile MemorySection (MemorySec, DataSec)
  if (const AST::MemorySection *MemSec = Module.getMemorySection()) {
    if (const AST::DataSection *DataSec = Module.getDataSection()) {
      if (ErrCode Status = compile(*MemSec, *DataSec);
          Status != ErrCode::Success) {
        return Status;
      }
    }
  }

  /// Compile TableSection
  if (const AST::TableSection *TabSec = Module.getTableSection()) {
    if (const AST::ElementSection *ElemSec = Module.getElementSection()) {
      if (ErrCode Status = compile(*TabSec, *ElemSec);
          Status != ErrCode::Success) {
        return Status;
      }
    }
  }

  /// compile Functions in module. (FuncionSec, CodeSec)
  if (const AST::FunctionSection *FuncSec = Module.getFunctionSection()) {
    if (const AST::CodeSection *CodeSec = Module.getCodeSection()) {
      if (ErrCode Status = compile(*FuncSec, *CodeSec);
          Status != ErrCode::Success) {
        return Status;
      }
    }
  }

  /// Compile ExportSection
  if (const AST::ExportSection *ExportSec = Module.getExportSection()) {
    if (ErrCode Status = compile(*ExportSec); Status != ErrCode::Success) {
      return Status;
    }
  }

  /// Compile StartSection (StartSec)
  if (const AST::StartSection *StartSec = Module.getStartSection()) {
  }

  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::TypeSection &TypeSection) {
  /// Iterate and compile types.
  for (const auto &FuncType : TypeSection.getContent()) {
    /// Copy param and return lists to module instance.
    Context->FunctionTypes.push_back(FuncType.get());
  }

  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::ImportSection &ImportSec) {
  /// Iterate and compile import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    /// Get data from import description.
    const auto &ExtType = ImpDesc->getExternalType();
    const std::string &ModName = ImpDesc->getModuleName();
    const std::string &ExtName = ImpDesc->getExternalName();
    const std::string &FullName = ModName + '.' + ExtName;
    const std::string &FullCtxName = FullName + ".ctx";

    /// Add the imports into module istance.
    switch (ExtType) {
    case AST::Desc::ExternalType::Function: /// Function type index
    {
      /// Get the function type index in module.
      unsigned int *TypeIdx = nullptr;
      if (auto Res = ImpDesc->getExternalContent<uint32_t>()) {
        TypeIdx = *Res;
      } else {
        return ErrCode::Failed;
      }
      if (*TypeIdx >= Context->FunctionTypes.size()) {
        return ErrCode::Failed;
      }
      const auto &FuncType = *Context->FunctionTypes[*TypeIdx];
      llvm::FunctionType *FTy = toLLVMType(Context->Context, FuncType);
      llvm::Function *F =
          llvm::Function::Create(FTy, llvm::GlobalValue::InternalLinkage,
                                 FullName + ".wrap", Context->Module);

      llvm::GlobalVariable *Ctx = new llvm::GlobalVariable(
          Context->Module, llvm::Type::getInt8Ty(Context->Context), false,
          llvm::GlobalValue::ExternalLinkage, nullptr, FullCtxName);

      llvm::Function *FE = createCtxCall(F, Ctx);
      FE->setName(FullName);

      Context->Functions.emplace_back(*TypeIdx, F, nullptr);
    }
    case AST::Desc::ExternalType::Table: /// Table type
    {
      break;
    }
    case AST::Desc::ExternalType::Memory: /// Memory type
    {
      break;
    }
    case AST::Desc::ExternalType::Global: /// Global type
    {
      break;
    }
    default:
      break;
    }
  }
  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::ExportSection &ExportSec) {
  for (const auto &ExpDesc : ExportSec.getContent()) {
    switch (ExpDesc->getExternalType()) {
    case AST::Desc::ExternalType::Function: {
      llvm::Function *F =
          std::get<1>(Context->Functions[ExpDesc->getExternalIndex()]);
      F->setLinkage(llvm::GlobalValue::ExternalLinkage);
      F->setName(ExpDesc->getExternalName());
      break;
    }
    default:
      break;
    }
  }
  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::GlobalSection &GlobalSec) {
  for (size_t I = 0; I < GlobalSec.getContent().size(); ++I) {
    const SSVM::ValType &ValType =
        GlobalSec.getContent()[I]->getGlobalType()->getValueType();
    llvm::GlobalVariable *G = new llvm::GlobalVariable(
        Context->Module, toLLVMType(Context->Context, ValType), false,
        llvm::GlobalValue::InternalLinkage,
        FunctionCompiler::evaluate(GlobalSec.getContent()[I]->getInstrs(),
                                   *Context),
        "$g." + std::to_string(I));
    Context->Globals.push_back(G);
  }
  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::MemorySection &MemorySection,
                          const AST::DataSection &DataSec) {
  auto &VMContext = Context->Context;
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
      llvm::GlobalValue::InternalLinkage, "$memory.ctor", Context->Module);

  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Context->Context, "entry", Ctor));
  llvm::Constant *Content = llvm::ConstantDataArray::getString(
      VMContext, llvm::StringRef(ResultData.data(), ResultData.size()), false);
  llvm::GlobalVariable *GV =
      new llvm::GlobalVariable(Context->Module, Content->getType(), true,
                               llvm::GlobalVariable::PrivateLinkage, Content);
  Builder.CreateMemCpy(
      Builder.CreateInBoundsGEP(Builder.CreateLoad(Context->Memory),
                                Builder.getInt32(0)),
      8, GV, 8, Builder.getInt32(ResultData.size()));
  Builder.CreateRetVoid();

  Context->Ctors.push_back(Ctor);
  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::TableSection &TableSection,
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
    std::copy(FuncIdxes.cbegin(), FuncIdxes.cend(), Elements.begin() + Offset);
  }
  return ErrCode::Success;
}

ErrCode Compiler::compile(const AST::FunctionSection &FuncSec,
                          const AST::CodeSection &CodeSec) {
  const auto &TypeIdxs = FuncSec.getContent();
  const auto &CodeSegs = CodeSec.getContent();

  for (size_t I = 0; I < TypeIdxs.size() && I < CodeSegs.size(); ++I) {
    const auto &TypeIdx = TypeIdxs[I];
    const auto &Code = CodeSegs[I];
    if (TypeIdx >= Context->FunctionTypes.size()) {
      return ErrCode::Failed;
    }
    const auto &FuncType = *Context->FunctionTypes[TypeIdx];
    llvm::FunctionType *FTy = toLLVMType(Context->Context, FuncType);
    llvm::Function *F = llvm::Function::Create(
        FTy, llvm::GlobalValue::InternalLinkage,
        "$f" + std::to_string(Context->Functions.size()), Context->Module);

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

    FunctionCompiler FC(*Context, F, Locals);
    if (ErrCode Status = FC.compile(Code->getInstrs());
        Status != ErrCode::Success) {
      return Status;
    }
    FC.epilog();
  }

  return ErrCode::Success;
}

Library &Compiler::getLibrary() { return *Lib; }

} // namespace Compiler
} // namespace SSVM
