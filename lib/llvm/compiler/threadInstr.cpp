// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/function_compiler.h"

namespace WasmEdge {

Expect<void>
FunctionCompiler::compileAtomicOp(const AST::Instruction &Instr) noexcept {
  switch (Instr.getOpCode()) {
  case OpCode::Atomic__fence:
    compileMemoryFence();
    break;
  case OpCode::Memory__atomic__notify:
    compileAtomicNotify(Instr.getTargetIndex(), Instr.getMemoryOffset());
    break;
  case OpCode::Memory__atomic__wait32:
    compileAtomicWait(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Context.Int32Ty, 32);
    break;
  case OpCode::Memory__atomic__wait64:
    compileAtomicWait(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Context.Int64Ty, 64);
    break;
  case OpCode::I32__atomic__load:
    compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int32Ty, Context.Int32Ty,
                      true);
    break;
  case OpCode::I64__atomic__load:
    compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int64Ty, Context.Int64Ty,
                      true);
    break;
  case OpCode::I32__atomic__load8_u:
    compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__load16_u:
    compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__load8_u:
    compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__load16_u:
    compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__load32_u:
    compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Context.Int64Ty, Context.Int32Ty);
    break;
  case OpCode::I32__atomic__store:
    compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int32Ty, Context.Int32Ty,
                       true);
    break;
  case OpCode::I64__atomic__store:
    compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int64Ty, Context.Int64Ty,
                       true);
    break;
  case OpCode::I32__atomic__store8:
    compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int32Ty, Context.Int8Ty,
                       true);
    break;
  case OpCode::I32__atomic__store16:
    compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int32Ty, Context.Int16Ty,
                       true);
    break;
  case OpCode::I64__atomic__store8:
    compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int64Ty, Context.Int8Ty,
                       true);
    break;
  case OpCode::I64__atomic__store16:
    compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int64Ty, Context.Int16Ty,
                       true);
    break;
  case OpCode::I64__atomic__store32:
    compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int64Ty, Context.Int32Ty,
                       true);
    break;
  case OpCode::I32__atomic__rmw__add:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                       Context.Int32Ty, Context.Int32Ty, true);
    break;
  case OpCode::I64__atomic__rmw__add:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                       Context.Int64Ty, Context.Int64Ty, true);
    break;
  case OpCode::I32__atomic__rmw8__add_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                       Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__rmw16__add_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                       Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw8__add_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                       Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__rmw16__add_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                       Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw32__add_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAdd,
                       Context.Int64Ty, Context.Int32Ty);
    break;
  case OpCode::I32__atomic__rmw__sub:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                       Context.Int32Ty, Context.Int32Ty, true);
    break;
  case OpCode::I64__atomic__rmw__sub:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                       Context.Int64Ty, Context.Int64Ty, true);
    break;
  case OpCode::I32__atomic__rmw8__sub_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                       Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__rmw16__sub_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                       Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw8__sub_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                       Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__rmw16__sub_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                       Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw32__sub_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpSub,
                       Context.Int64Ty, Context.Int32Ty);
    break;
  case OpCode::I32__atomic__rmw__and:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                       Context.Int32Ty, Context.Int32Ty, true);
    break;
  case OpCode::I64__atomic__rmw__and:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                       Context.Int64Ty, Context.Int64Ty, true);
    break;
  case OpCode::I32__atomic__rmw8__and_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                       Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__rmw16__and_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                       Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw8__and_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                       Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__rmw16__and_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                       Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw32__and_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpAnd,
                       Context.Int64Ty, Context.Int32Ty);
    break;
  case OpCode::I32__atomic__rmw__or:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                       Context.Int32Ty, Context.Int32Ty, true);
    break;
  case OpCode::I64__atomic__rmw__or:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                       Context.Int64Ty, Context.Int64Ty, true);
    break;
  case OpCode::I32__atomic__rmw8__or_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                       Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__rmw16__or_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                       Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw8__or_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                       Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__rmw16__or_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                       Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw32__or_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpOr,
                       Context.Int64Ty, Context.Int32Ty);
    break;
  case OpCode::I32__atomic__rmw__xor:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                       Context.Int32Ty, Context.Int32Ty, true);
    break;
  case OpCode::I64__atomic__rmw__xor:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                       Context.Int64Ty, Context.Int64Ty, true);
    break;
  case OpCode::I32__atomic__rmw8__xor_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                       Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__rmw16__xor_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                       Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw8__xor_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                       Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__rmw16__xor_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                       Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw32__xor_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXor,
                       Context.Int64Ty, Context.Int32Ty);
    break;
  case OpCode::I32__atomic__rmw__xchg:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg,
                       Context.Int32Ty, Context.Int32Ty, true);
    break;
  case OpCode::I64__atomic__rmw__xchg:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg,
                       Context.Int64Ty, Context.Int64Ty, true);
    break;
  case OpCode::I32__atomic__rmw8__xchg_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg,
                       Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__rmw16__xchg_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg,
                       Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw8__xchg_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg,
                       Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__rmw16__xchg_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg,
                       Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw32__xchg_u:
    compileAtomicRMWOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), LLVMAtomicRMWBinOpXchg,
                       Context.Int64Ty, Context.Int32Ty);
    break;
  case OpCode::I32__atomic__rmw__cmpxchg:
    compileAtomicCompareExchange(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        Context.Int32Ty, Context.Int32Ty, true);
    break;
  case OpCode::I64__atomic__rmw__cmpxchg:
    compileAtomicCompareExchange(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        Context.Int64Ty, Context.Int64Ty, true);
    break;
  case OpCode::I32__atomic__rmw8__cmpxchg_u:
    compileAtomicCompareExchange(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        Context.Int32Ty, Context.Int8Ty);
    break;
  case OpCode::I32__atomic__rmw16__cmpxchg_u:
    compileAtomicCompareExchange(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        Context.Int32Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw8__cmpxchg_u:
    compileAtomicCompareExchange(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        Context.Int64Ty, Context.Int8Ty);
    break;
  case OpCode::I64__atomic__rmw16__cmpxchg_u:
    compileAtomicCompareExchange(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        Context.Int64Ty, Context.Int16Ty);
    break;
  case OpCode::I64__atomic__rmw32__cmpxchg_u:
    compileAtomicCompareExchange(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        Context.Int64Ty, Context.Int32Ty);
    break;

  default:
    assumingUnreachable();
  }
  return {};
}

void FunctionCompiler::compileAtomicCheckOffsetAlignment(
    LLVM::Value Offset, LLVM::Type IntType) noexcept {
  const auto BitWidth = IntType.getIntegerBitWidth();
  auto BWMask = LLContext.getInt64((BitWidth >> 3) - 1);
  auto Value = Builder.createAnd(Offset, BWMask);
  auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "address_align_ok");
  auto IsAddressAligned =
      Builder.createLikely(Builder.createICmpEQ(Value, LLContext.getInt64(0)));
  Builder.createCondBr(IsAddressAligned, OkBB,
                       getTrapBB(ErrCode::Value::UnalignedAtomicAccess));

  Builder.positionAtEnd(OkBB);
}

void FunctionCompiler::compileMemoryFence() noexcept {
  Builder.createFence(LLVMAtomicOrderingSequentiallyConsistent);
}

void FunctionCompiler::compileAtomicNotify(unsigned MemoryIndex,
                                           uint64_t MemoryOffset) noexcept {
  auto Count = Builder.createZExt(stackPop(), Context.Int64Ty);
  auto Offset = Builder.createZExt(stackPop(), Context.Int64Ty);
  if (MemoryOffset != 0) {
    Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
  }
  compileAtomicCheckOffsetAlignment(Offset, Context.Int32Ty);
  // The woken-count result is always i32, even on memory64; truncating to the
  // memory address type would mis-type the operand stack with an i64.
  stackPush(Builder.createTrunc(
      Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kMemAtomicNotify,
              LLVM::Type::getFunctionType(
                  Context.Int64Ty,
                  {Context.Int32Ty, Context.Int64Ty, Context.Int64Ty}, false)),
          {LLContext.getInt32(MemoryIndex), Offset, Count}),
      Context.Int32Ty));
}

void FunctionCompiler::compileAtomicWait(unsigned MemoryIndex,
                                         uint64_t MemoryOffset,
                                         LLVM::Type TargetType,
                                         uint32_t BitWidth) noexcept {
  auto Timeout = stackPop();
  auto ExpectedValue = Builder.createZExtOrTrunc(stackPop(), Context.Int64Ty);
  auto Offset = Builder.createZExt(stackPop(), Context.Int64Ty);
  if (MemoryOffset != 0) {
    Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
  }
  compileAtomicCheckOffsetAlignment(Offset, TargetType);
  stackPush(Builder.createTrunc(
      Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kMemAtomicWait,
              LLVM::Type::getFunctionType(Context.Int64Ty,
                                          {Context.Int32Ty, Context.Int64Ty,
                                           Context.Int64Ty, Context.Int64Ty,
                                           Context.Int32Ty},
                                          false)),
          {LLContext.getInt32(MemoryIndex), Offset, ExpectedValue, Timeout,
           LLContext.getInt32(BitWidth)}),
      // atomic.wait32/64 returns i32 (0/1/2), even on memory64; truncating to
      // the memory address type would mis-type the operand stack with an i64.
      Context.Int32Ty));
}

void FunctionCompiler::compileAtomicLoad(unsigned MemoryIndex,
                                         uint64_t MemoryOffset,
                                         unsigned Alignment, LLVM::Type IntType,
                                         LLVM::Type TargetType,
                                         bool Signed) noexcept {

  auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
  if (MemoryOffset != 0) {
    Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
  }
  compileAtomicCheckOffsetAlignment(Offset, TargetType);
  auto VPtr = Builder.createInBoundsGEP1(
      Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Offset);

  auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());
  auto Load = switchEndian(Builder.createLoad(TargetType, Ptr, true));
  Load.setAlignment(1 << Alignment);
  Load.setOrdering(LLVMAtomicOrderingSequentiallyConsistent);

  if (Signed) {
    Stack.back() = Builder.createSExt(Load, IntType);
  } else {
    Stack.back() = Builder.createZExt(Load, IntType);
  }
}

void FunctionCompiler::compileAtomicStore(unsigned MemoryIndex,
                                          uint64_t MemoryOffset,
                                          unsigned Alignment, LLVM::Type,
                                          LLVM::Type TargetType,
                                          bool Signed) noexcept {
  auto V = stackPop();

  if (Signed) {
    V = Builder.createSExtOrTrunc(V, TargetType);
  } else {
    V = Builder.createZExtOrTrunc(V, TargetType);
  }
  V = switchEndian(V);
  auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
  if (MemoryOffset != 0) {
    Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
  }
  compileAtomicCheckOffsetAlignment(Offset, TargetType);
  auto VPtr = Builder.createInBoundsGEP1(
      Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Offset);
  auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());
  auto Store = Builder.createStore(V, Ptr, true);
  Store.setAlignment(1 << Alignment);
  Store.setOrdering(LLVMAtomicOrderingSequentiallyConsistent);
}

void FunctionCompiler::compileAtomicRMWOp(
    unsigned MemoryIndex, uint64_t MemoryOffset,
    [[maybe_unused]] unsigned Alignment, LLVMAtomicRMWBinOp BinOp,
    LLVM::Type IntType, LLVM::Type TargetType, bool Signed) noexcept {
  auto Value = Builder.createSExtOrTrunc(stackPop(), TargetType);
  auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
  if (MemoryOffset != 0) {
    Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
  }
  compileAtomicCheckOffsetAlignment(Offset, TargetType);
  auto VPtr = Builder.createInBoundsGEP1(
      Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Offset);
  auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());

  LLVM::Value Ret;
  if constexpr (Endian::native == Endian::big) {
    if (BinOp == LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpAdd ||
        BinOp == LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpSub) {
      auto AtomicBB = LLVM::BasicBlock::create(LLContext, F.Fn, "atomic.rmw");
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "atomic.rmw.ok");
      Builder.createBr(AtomicBB);
      Builder.positionAtEnd(AtomicBB);

      auto Load = Builder.createLoad(TargetType, Ptr, true);
      Load.setOrdering(LLVMAtomicOrderingMonotonic);
      Load.setAlignment(1 << Alignment);

      LLVM::Value New;
      if (BinOp == LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpAdd)
        New = Builder.createAdd(switchEndian(Load), Value);
      else if (BinOp == LLVMAtomicRMWBinOp::LLVMAtomicRMWBinOpSub) {
        New = Builder.createSub(switchEndian(Load), Value);
      } else {
        assumingUnreachable();
      }
      New = switchEndian(New);

      auto Exchange = Builder.createAtomicCmpXchg(
          Ptr, Load, New, LLVMAtomicOrderingSequentiallyConsistent,
          LLVMAtomicOrderingSequentiallyConsistent);

      Ret = Builder.createExtractValue(Exchange, 0);
      auto Success = Builder.createExtractValue(Exchange, 1);
      Builder.createCondBr(Success, OkBB, AtomicBB);
      Builder.positionAtEnd(OkBB);
    } else {
      Ret = Builder.createAtomicRMW(BinOp, Ptr, switchEndian(Value),
                                    LLVMAtomicOrderingSequentiallyConsistent);
    }
  } else {
    Ret = Builder.createAtomicRMW(BinOp, Ptr, switchEndian(Value),
                                  LLVMAtomicOrderingSequentiallyConsistent);
  }
  Ret = switchEndian(Ret);
#if LLVM_VERSION_MAJOR >= 13
  Ret.setAlignment(1 << Alignment);
#endif
  if (Signed) {
    Stack.back() = Builder.createSExt(Ret, IntType);
  } else {
    Stack.back() = Builder.createZExt(Ret, IntType);
  }
}

void FunctionCompiler::compileAtomicCompareExchange(
    unsigned MemoryIndex, uint64_t MemoryOffset,
    [[maybe_unused]] unsigned Alignment, LLVM::Type IntType,
    LLVM::Type TargetType, bool Signed) noexcept {

  auto Replacement = Builder.createSExtOrTrunc(stackPop(), TargetType);
  auto Expected = Builder.createSExtOrTrunc(stackPop(), TargetType);
  auto Offset = Builder.createZExt(Stack.back(), Context.Int64Ty);
  if (MemoryOffset != 0) {
    Offset = Builder.createAdd(Offset, LLContext.getInt64(MemoryOffset));
  }
  compileAtomicCheckOffsetAlignment(Offset, TargetType);
  auto VPtr = Builder.createInBoundsGEP1(
      Context.Int8Ty, Context.getMemory(Builder, ExecCtx, MemoryIndex), Offset);
  auto Ptr = Builder.createBitCast(VPtr, TargetType.getPointerTo());

  auto Ret = Builder.createAtomicCmpXchg(
      Ptr, switchEndian(Expected), switchEndian(Replacement),
      LLVMAtomicOrderingSequentiallyConsistent,
      LLVMAtomicOrderingSequentiallyConsistent);
#if LLVM_VERSION_MAJOR >= 13
  Ret.setAlignment(1 << Alignment);
#endif
  auto OldVal = Builder.createExtractValue(Ret, 0);
  OldVal = switchEndian(OldVal);
  if (Signed) {
    Stack.back() = Builder.createSExt(OldVal, IntType);
  } else {
    Stack.back() = Builder.createZExt(OldVal, IntType);
  }
}

} // namespace WasmEdge
