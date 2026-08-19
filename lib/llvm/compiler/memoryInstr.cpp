// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/function_compiler.h"

#include <limits>

namespace WasmEdge {

Expect<void>
FunctionCompiler::compileMemoryOp(const AST::Instruction &Instr) noexcept {
  switch (Instr.getOpCode()) {
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
    stackPush(Builder.createTrunc(
        Context.getMemorySize(Builder, ModCtx, Instr.getTargetIndex()),
        Context.MemoryAddrTypes[Instr.getTargetIndex()]));
    break;
  case OpCode::Memory__grow: {
    auto NewPageSize = Builder.createZExt(stackPop(), Context.Int64Ty);
    stackPush(Builder.createTrunc(
        Builder.createCall(
            Context.getIntrinsic(
                Builder, Executable::Intrinsics::kMemGrow,
                LLVM::Type::getFunctionType(
                    Context.Int64Ty,
                    {Context.Int8PtrTy, Context.Int32Ty, Context.Int64Ty},
                    false)),
            {Context.getModuleInst(Builder, ModCtx),
             LLContext.getInt32(Instr.getTargetIndex()), NewPageSize}),
        Context.MemoryAddrTypes[Instr.getTargetIndex()]));
    break;
  }
  case OpCode::Memory__init: {
    auto Len = stackPop();
    auto Src = stackPop();
    auto Dst = Builder.createZExt(stackPop(), Context.Int64Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kMemInit,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int8PtrTy, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int64Ty,
                                         Context.Int32Ty, Context.Int32Ty},
                                        false)),
        {Context.getModuleInst(Builder, ModCtx),
         LLContext.getInt32(Instr.getTargetIndex()),
         LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
    break;
  }
  case OpCode::Data__drop: {
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kDataDrop,
            LLVM::Type::getFunctionType(
                Context.VoidTy, {Context.Int8PtrTy, Context.Int32Ty}, false)),
        {Context.getModuleInst(Builder, ModCtx),
         LLContext.getInt32(Instr.getTargetIndex())});
    break;
  }
  case OpCode::Memory__copy: {
    auto Len = Builder.createZExt(stackPop(), Context.Int64Ty);
    auto Src = Builder.createZExt(stackPop(), Context.Int64Ty);
    auto Dst = Builder.createZExt(stackPop(), Context.Int64Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kMemCopy,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int8PtrTy, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int64Ty,
                                         Context.Int64Ty, Context.Int64Ty},
                                        false)),
        {Context.getModuleInst(Builder, ModCtx),
         LLContext.getInt32(Instr.getTargetIndex()),
         LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
    break;
  }
  case OpCode::Memory__fill: {
    auto Len = Builder.createZExt(stackPop(), Context.Int64Ty);
    auto Val = Builder.createTrunc(stackPop(), Context.Int8Ty);
    auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kMemFill,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int8PtrTy, Context.Int32Ty,
                                         Context.Int64Ty, Context.Int8Ty,
                                         Context.Int64Ty},
                                        false)),
        {Context.getModuleInst(Builder, ModCtx),
         LLContext.getInt32(Instr.getTargetIndex()), Off, Val, Len});
    break;
  }

  // Const Numeric Instructions
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

  default:
    assumingUnreachable();
  }
  return {};
}

// Memory32 cannot escape the reservation's guard region, so it keeps the
// guard-page fast path. Memory64 addresses are guest-controlled 64-bit values.
void FunctionCompiler::boundsCheckMemory64(unsigned MemoryIndex,
                                           LLVM::Value Addr, uint64_t Offset,
                                           uint64_t AccessSize) noexcept {
  if (Context.MemoryAddrTypes[MemoryIndex].getIntegerBitWidth() != 64) {
    return;
  }
  if (Offset > std::numeric_limits<uint64_t>::max() - AccessSize) {
    // No address can satisfy the access.
    Builder.createBr(getTrapBB(ErrCode::Value::MemoryOutOfBounds));
    Builder.positionAtEnd(
        LLVM::BasicBlock::create(LLContext, F.Fn, "mem64.ok"));
    return;
  }
  // Addr < usub_sat(SizeBytes, Offset + AccessSize - 1) is exactly
  // Addr + Offset + AccessSize <= SizeBytes with every term kept in 64 bits.
  const uint64_t OffsetAndSize = Offset + AccessSize;
  auto SizeBytes =
      Builder.createShl(Context.getMemorySize(Builder, ModCtx, MemoryIndex),
                        LLContext.getInt64(16));
  auto Limit = Builder.createIntrinsic(
      LLVM::Core::USubSat, {Context.Int64Ty},
      {SizeBytes, LLContext.getInt64(OffsetAndSize - 1)});
  auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "mem64.ok");
  Builder.createCondBr(Builder.createLikely(Builder.createICmpULT(Addr, Limit)),
                       OkBB, getTrapBB(ErrCode::Value::MemoryOutOfBounds));
  Builder.positionAtEnd(OkBB);
}

void FunctionCompiler::compileLoadOp(unsigned MemoryIndex, uint64_t Offset,
                                     unsigned Alignment,
                                     LLVM::Type LoadTy) noexcept {
  if constexpr (LLVM::kForceUnalignment) {
    Alignment = 0;
  }
  auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
  boundsCheckMemory64(MemoryIndex, Off, Offset,
                      LoadTy.getPrimitiveSizeInBits() / 8);
  if (Offset != 0) {
    Off = Builder.createAdd(Off, LLContext.getInt64(Offset));
  }

  auto VPtr = Builder.createInBoundsGEP1(
      Context.Int8Ty, Context.getMemory(Builder, ModCtx, MemoryIndex), Off);
  auto Ptr = Builder.createBitCast(VPtr, LoadTy.getPointerTo());
  auto LoadInst = Builder.createLoad(LoadTy, Ptr, true);
  LoadInst.setAlignment(1 << Alignment);
  stackPush(switchEndian(LoadInst));
}

void FunctionCompiler::compileLoadOp(unsigned MemoryIndex, uint64_t Offset,
                                     unsigned Alignment, LLVM::Type LoadTy,
                                     LLVM::Type ExtendTy,
                                     bool Signed) noexcept {
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
  if (Signed) {
    Stack.back() = Builder.createSExt(Stack.back(), ExtendTy);
  } else {
    Stack.back() = Builder.createZExt(Stack.back(), ExtendTy);
  }
}

void FunctionCompiler::compileStoreOp(uint32_t MemoryIndex, uint64_t Offset,
                                      uint32_t Alignment, LLVM::Type LoadTy,
                                      bool Trunc, bool BitCast) noexcept {
  if constexpr (LLVM::kForceUnalignment) {
    Alignment = 0;
  }
  auto V = stackPop();
  auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
  boundsCheckMemory64(MemoryIndex, Off, Offset,
                      LoadTy.getPrimitiveSizeInBits() / 8);
  if (Offset != 0) {
    Off = Builder.createAdd(Off, LLContext.getInt64(Offset));
  }

  if (Trunc) {
    V = Builder.createTrunc(V, LoadTy);
  }
  if (BitCast) {
    V = Builder.createBitCast(V, LoadTy);
  }
  V = switchEndian(V);
  auto VPtr = Builder.createInBoundsGEP1(
      Context.Int8Ty, Context.getMemory(Builder, ModCtx, MemoryIndex), Off);
  auto Ptr = Builder.createBitCast(VPtr, LoadTy.getPointerTo());
  auto StoreInst = Builder.createStore(V, Ptr, true);
  StoreInst.setAlignment(1 << Alignment);
}

} // namespace WasmEdge
