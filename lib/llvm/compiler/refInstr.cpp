// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/function_compiler.h"

#include <algorithm>
#include <array>

namespace WasmEdge {

Expect<void>
FunctionCompiler::compileRefOp(const AST::Instruction &Instr) noexcept {
  switch (Instr.getOpCode()) {
  case OpCode::Ref__null: {
    std::array<uint8_t, 16> Buf = {0};
    // For null references, dynamic type downscaling is needed.
    ValType VType;
    if (Instr.getValType().isAbsHeapType()) {
      switch (Instr.getValType().getHeapTypeCode()) {
      case TypeCode::NullFuncRef:
      case TypeCode::FuncRef:
        VType = TypeCode::NullFuncRef;
        break;
      case TypeCode::NullExternRef:
      case TypeCode::ExternRef:
        VType = TypeCode::NullExternRef;
        break;
      case TypeCode::NullExnRef:
      case TypeCode::ExnRef:
        VType = TypeCode::NullExnRef;
        break;
      case TypeCode::NullRef:
      case TypeCode::AnyRef:
      case TypeCode::EqRef:
      case TypeCode::I31Ref:
      case TypeCode::StructRef:
      case TypeCode::ArrayRef:
        VType = TypeCode::NullRef;
        break;
      default:
        assumingUnreachable();
      }
    } else {
      assuming(Instr.getValType().getTypeIndex() <
               Context.CompositeTypes.size());
      const auto *CompType =
          Context.CompositeTypes[Instr.getValType().getTypeIndex()];
      assuming(CompType != nullptr);
      if (CompType->isFunc()) {
        VType = TypeCode::NullFuncRef;
      } else {
        VType = TypeCode::NullRef;
      }
    }
    std::copy_n(VType.getRawData().cbegin(), 8, Buf.begin());
    stackPush(Builder.createBitCast(
        LLVM::Value::getConstVector8(LLContext, Buf), Context.Int64x2Ty));
    break;
  }
  case OpCode::Ref__is_null:
    stackPush(Builder.createZExt(
        Builder.createICmpEQ(
            Builder.createExtractElement(
                Builder.createBitCast(stackPop(), Context.Int64x2Ty),
                LLContext.getInt64(1)),
            LLContext.getInt64(0)),
        Context.Int32Ty));
    break;
  case OpCode::Ref__func:
    stackPush(Builder.createCall(
        Context.getIntrinsic(Builder, Executable::Intrinsics::kRefFunc,
                             LLVM::Type::getFunctionType(
                                 Context.Int64x2Ty, {Context.Int32Ty}, false)),
        {LLContext.getInt32(Instr.getTargetIndex())}));
    break;
  case OpCode::Ref__eq: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createZExt(
        Builder.createICmpEQ(
            Builder.createExtractElement(LHS, LLContext.getInt64(1)),
            Builder.createExtractElement(RHS, LLContext.getInt64(1))),
        Context.Int32Ty));
    break;
  }
  case OpCode::Ref__as_non_null: {
    auto Next = LLVM::BasicBlock::create(LLContext, F.Fn, "ref_as_non_null.ok");
    Stack.back() = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
    auto IsNotNull = Builder.createLikely(Builder.createICmpNE(
        Builder.createExtractElement(Stack.back(), LLContext.getInt64(1)),
        LLContext.getInt64(0)));
    Builder.createCondBr(IsNotNull, Next,
                         getTrapBB(ErrCode::Value::CastNullToNonNull));
    Builder.positionAtEnd(Next);
    break;
  }

  // Reference Instructions (GC proposal)
  case OpCode::Struct__new:
  case OpCode::Struct__new_default: {
    LLVM::Value Args = LLVM::Value::getConstPointerNull(Context.Int8PtrTy);
    assuming(Instr.getTargetIndex() < Context.CompositeTypes.size());
    const auto *CompType = Context.CompositeTypes[Instr.getTargetIndex()];
    assuming(CompType != nullptr && !CompType->isFunc());
    auto ArgSize = CompType->getFieldTypes().size();
    if (Instr.getOpCode() == OpCode::Struct__new) {
      std::vector<LLVM::Value> ArgsVec(ArgSize, nullptr);
      for (size_t I = 0; I < ArgSize; ++I) {
        ArgsVec[ArgSize - I - 1] = stackPop();
      }
      Args = Builder.createArray(ArgSize, LLVM::kValSize);
      Builder.createArrayPtrStore(ArgsVec, Args, Context.Int8Ty,
                                  LLVM::kValSize);
    } else {
      ArgSize = 0;
    }
    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kStructNew,
            LLVM::Type::getFunctionType(
                Context.Int64x2Ty,
                {Context.Int32Ty, Context.Int8PtrTy, Context.Int32Ty}, false)),
        {LLContext.getInt32(Instr.getTargetIndex()), Args,
         LLContext.getInt32(static_cast<uint32_t>(ArgSize))}));
    break;
  }
  case OpCode::Struct__get:
  case OpCode::Struct__get_u:
  case OpCode::Struct__get_s: {
    assuming(static_cast<size_t>(Instr.getTargetIndex()) <
             Context.CompositeTypes.size());
    const auto *CompType = Context.CompositeTypes[Instr.getTargetIndex()];
    assuming(CompType != nullptr && !CompType->isFunc());
    assuming(static_cast<size_t>(Instr.getSourceIndex()) <
             CompType->getFieldTypes().size());
    const auto &StorageType =
        CompType->getFieldTypes()[Instr.getSourceIndex()].getStorageType();
    auto Ref = stackPop();
    auto IsSigned = (Instr.getOpCode() == OpCode::Struct__get_s)
                        ? LLContext.getInt8(1)
                        : LLContext.getInt8(0);
    LLVM::Value Ret = Builder.createAlloca(Context.Int64x2Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kStructGet,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int64x2Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int8Ty,
                                         Context.Int8PtrTy},
                                        false)),
        {Ref, LLContext.getInt32(Instr.getTargetIndex()),
         LLContext.getInt32(Instr.getSourceIndex()), IsSigned, Ret});

    switch (StorageType.getCode()) {
    case TypeCode::I8:
    case TypeCode::I16:
    case TypeCode::I32: {
      stackPush(
          Builder.createValuePtrLoad(Context.Int32Ty, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::I64: {
      stackPush(
          Builder.createValuePtrLoad(Context.Int64Ty, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::F32: {
      stackPush(
          Builder.createValuePtrLoad(Context.FloatTy, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::F64: {
      stackPush(
          Builder.createValuePtrLoad(Context.DoubleTy, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::V128:
    case TypeCode::Ref:
    case TypeCode::RefNull: {
      stackPush(Builder.createValuePtrLoad(Context.Int64x2Ty, Ret,
                                           Context.Int64x2Ty));
      break;
    }
    default:
      assumingUnreachable();
    }
    break;
  }
  case OpCode::Struct__set: {
    auto Val = stackPop();
    auto Ref = stackPop();
    LLVM::Value Arg = Builder.createAlloca(Context.Int64x2Ty);
    Builder.createValuePtrStore(Val, Arg, Context.Int64x2Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kStructSet,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int64x2Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int8PtrTy},
                                        false)),
        {Ref, LLContext.getInt32(Instr.getTargetIndex()),
         LLContext.getInt32(Instr.getSourceIndex()), Arg});
    break;
  }
  case OpCode::Array__new: {
    auto Length = stackPop();
    auto Val = stackPop();
    LLVM::Value Arg = Builder.createAlloca(Context.Int64x2Ty);
    Builder.createValuePtrStore(Val, Arg, Context.Int64x2Ty);
    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kArrayNew,
            LLVM::Type::getFunctionType(Context.Int64x2Ty,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int8PtrTy, Context.Int32Ty},
                                        false)),
        {LLContext.getInt32(Instr.getTargetIndex()), Length, Arg,
         LLContext.getInt32(1)}));
    break;
  }
  case OpCode::Array__new_default: {
    auto Length = stackPop();
    LLVM::Value Arg = LLVM::Value::getConstPointerNull(Context.Int8PtrTy);
    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kArrayNew,
            LLVM::Type::getFunctionType(Context.Int64x2Ty,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int8PtrTy, Context.Int32Ty},
                                        false)),
        {LLContext.getInt32(Instr.getTargetIndex()), Length, Arg,
         LLContext.getInt32(0)}));
    break;
  }
  case OpCode::Array__new_fixed: {
    const auto ArgSize = Instr.getSourceIndex();
    std::vector<LLVM::Value> ArgsVec(ArgSize, nullptr);
    for (size_t I = 0; I < ArgSize; ++I) {
      ArgsVec[ArgSize - I - 1] = stackPop();
    }
    LLVM::Value Args = Builder.createArray(ArgSize, LLVM::kValSize);
    Builder.createArrayPtrStore(ArgsVec, Args, Context.Int8Ty, LLVM::kValSize);
    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kArrayNew,
            LLVM::Type::getFunctionType(Context.Int64x2Ty,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int8PtrTy, Context.Int32Ty},
                                        false)),
        {LLContext.getInt32(Instr.getTargetIndex()),
         LLContext.getInt32(ArgSize), Args, LLContext.getInt32(ArgSize)}));
    break;
  }
  case OpCode::Array__new_data:
  case OpCode::Array__new_elem: {
    auto Length = stackPop();
    auto Start = stackPop();
    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder,
            ((Instr.getOpCode() == OpCode::Array__new_data)
                 ? Executable::Intrinsics::kArrayNewData
                 : Executable::Intrinsics::kArrayNewElem),
            LLVM::Type::getFunctionType(Context.Int64x2Ty,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty},
                                        false)),
        {LLContext.getInt32(Instr.getTargetIndex()),
         LLContext.getInt32(Instr.getSourceIndex()), Start, Length}));
    break;
  }
  case OpCode::Array__get:
  case OpCode::Array__get_u:
  case OpCode::Array__get_s: {
    assuming(static_cast<size_t>(Instr.getTargetIndex()) <
             Context.CompositeTypes.size());
    const auto *CompType = Context.CompositeTypes[Instr.getTargetIndex()];
    assuming(CompType != nullptr && !CompType->isFunc());
    assuming(static_cast<size_t>(1) == CompType->getFieldTypes().size());
    const auto &StorageType = CompType->getFieldTypes()[0].getStorageType();
    auto Idx = stackPop();
    auto Ref = stackPop();
    auto IsSigned = (Instr.getOpCode() == OpCode::Array__get_s)
                        ? LLContext.getInt8(1)
                        : LLContext.getInt8(0);
    LLVM::Value Ret = Builder.createAlloca(Context.Int64x2Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kArrayGet,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int64x2Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int8Ty,
                                         Context.Int8PtrTy},
                                        false)),
        {Ref, LLContext.getInt32(Instr.getTargetIndex()), Idx, IsSigned, Ret});

    switch (StorageType.getCode()) {
    case TypeCode::I8:
    case TypeCode::I16:
    case TypeCode::I32: {
      stackPush(
          Builder.createValuePtrLoad(Context.Int32Ty, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::I64: {
      stackPush(
          Builder.createValuePtrLoad(Context.Int64Ty, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::F32: {
      stackPush(
          Builder.createValuePtrLoad(Context.FloatTy, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::F64: {
      stackPush(
          Builder.createValuePtrLoad(Context.DoubleTy, Ret, Context.Int64x2Ty));
      break;
    }
    case TypeCode::V128:
    case TypeCode::Ref:
    case TypeCode::RefNull: {
      stackPush(Builder.createValuePtrLoad(Context.Int64x2Ty, Ret,
                                           Context.Int64x2Ty));
      break;
    }
    default:
      assumingUnreachable();
    }
    break;
  }
  case OpCode::Array__set: {
    auto Val = stackPop();
    auto Idx = stackPop();
    auto Ref = stackPop();
    LLVM::Value Arg = Builder.createAlloca(Context.Int64x2Ty);
    Builder.createValuePtrStore(Val, Arg, Context.Int64x2Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kArraySet,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int64x2Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int8PtrTy},
                                        false)),
        {Ref, LLContext.getInt32(Instr.getTargetIndex()), Idx, Arg});
    break;
  }
  case OpCode::Array__len: {
    auto Ref = stackPop();
    stackPush(Builder.createCall(
        Context.getIntrinsic(Builder, Executable::Intrinsics::kArrayLen,
                             LLVM::Type::getFunctionType(
                                 Context.Int32Ty, {Context.Int64x2Ty}, false)),
        {Ref}));
    break;
  }
  case OpCode::Array__fill: {
    auto Cnt = stackPop();
    auto Val = stackPop();
    auto Off = stackPop();
    auto Ref = stackPop();
    LLVM::Value Arg = Builder.createAlloca(Context.Int64x2Ty);
    Builder.createValuePtrStore(Val, Arg, Context.Int64x2Ty);
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kArrayFill,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int64x2Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty,
                                         Context.Int8PtrTy},
                                        false)),
        {Ref, LLContext.getInt32(Instr.getTargetIndex()), Off, Cnt, Arg});
    break;
  }
  case OpCode::Array__copy: {
    auto Cnt = stackPop();
    auto SrcOff = stackPop();
    auto SrcRef = stackPop();
    auto DstOff = stackPop();
    auto DstRef = stackPop();
    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kArrayCopy,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int64x2Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int64x2Ty,
                                         Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty},
                                        false)),
        {DstRef, LLContext.getInt32(Instr.getTargetIndex()), DstOff, SrcRef,
         LLContext.getInt32(Instr.getSourceIndex()), SrcOff, Cnt});
    break;
  }
  case OpCode::Array__init_data:
  case OpCode::Array__init_elem: {
    auto Cnt = stackPop();
    auto SrcOff = stackPop();
    auto DstOff = stackPop();
    auto Ref = stackPop();
    Builder.createCall(
        Context.getIntrinsic(
            Builder,
            ((Instr.getOpCode() == OpCode::Array__init_data)
                 ? Executable::Intrinsics::kArrayInitData
                 : Executable::Intrinsics::kArrayInitElem),
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int64x2Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int32Ty},
                                        false)),
        {Ref, LLContext.getInt32(Instr.getTargetIndex()),
         LLContext.getInt32(Instr.getSourceIndex()), DstOff, SrcOff, Cnt});
    break;
  }
  case OpCode::Ref__test:
  case OpCode::Ref__test_null: {
    auto Ref = stackPop();
    std::array<uint8_t, 16> Buf = {0};
    std::copy_n(Instr.getValType().getRawData().cbegin(), 8, Buf.begin());
    auto VType = Builder.createExtractElement(
        Builder.createBitCast(LLVM::Value::getConstVector8(LLContext, Buf),
                              Context.Int64x2Ty),
        LLContext.getInt64(0));
    stackPush(Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kRefTest,
            LLVM::Type::getFunctionType(
                Context.Int32Ty, {Context.Int64x2Ty, Context.Int64Ty}, false)),
        {Ref, VType}));
    break;
  }
  case OpCode::Ref__cast:
  case OpCode::Ref__cast_null: {
    auto Ref = stackPop();
    std::array<uint8_t, 16> Buf = {0};
    std::copy_n(Instr.getValType().getRawData().cbegin(), 8, Buf.begin());
    auto VType = Builder.createExtractElement(
        Builder.createBitCast(LLVM::Value::getConstVector8(LLContext, Buf),
                              Context.Int64x2Ty),
        LLContext.getInt64(0));
    stackPush(Builder.createCall(
        Context.getIntrinsic(Builder, Executable::Intrinsics::kRefCast,
                             LLVM::Type::getFunctionType(
                                 Context.Int64x2Ty,
                                 {Context.Int64x2Ty, Context.Int64Ty}, false)),
        {Ref, VType}));
    break;
  }
  case OpCode::Any__convert_extern: {
    std::array<uint8_t, 16> RawRef = {0};
    auto Ref = stackPop();
    auto PtrVal = Builder.createExtractElement(Ref, LLContext.getInt64(1));
    auto IsNullBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "any_conv_extern.null");
    auto NotNullBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "any_conv_extern.not_null");
    auto IsExtrefBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "any_conv_extern.is_extref");
    auto EndBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "any_conv_extern.end");
    auto CondIsNull = Builder.createICmpEQ(PtrVal, LLContext.getInt64(0));
    Builder.createCondBr(CondIsNull, IsNullBB, NotNullBB);

    Builder.positionAtEnd(IsNullBB);
    auto VT = ValType(TypeCode::RefNull, TypeCode::NullRef);
    std::copy_n(VT.getRawData().cbegin(), 8, RawRef.begin());
    auto Ret1 = Builder.createBitCast(
        LLVM::Value::getConstVector8(LLContext, RawRef), Context.Int64x2Ty);
    Builder.createBr(EndBB);

    Builder.positionAtEnd(NotNullBB);
    auto Ret2 =
        Builder.createBitCast(Builder.createInsertElement(
                                  Builder.createBitCast(Ref, Context.Int8x16Ty),
                                  LLContext.getInt8(0), LLContext.getInt64(1)),
                              Context.Int64x2Ty);
    auto HType = Builder.createExtractElement(
        Builder.createBitCast(Ret2, Context.Int8x16Ty), LLContext.getInt64(3));
    auto CondIsExtref = Builder.createOr(
        Builder.createICmpEQ(HType, LLContext.getInt8(static_cast<uint8_t>(
                                        TypeCode::ExternRef))),
        Builder.createICmpEQ(HType, LLContext.getInt8(static_cast<uint8_t>(
                                        TypeCode::NullExternRef))));
    Builder.createCondBr(CondIsExtref, IsExtrefBB, EndBB);

    Builder.positionAtEnd(IsExtrefBB);
    VT = ValType(TypeCode::Ref, TypeCode::AnyRef);
    std::copy_n(VT.getRawData().cbegin(), 8, RawRef.begin());
    auto Ret3 = Builder.createInsertElement(
        Builder.createBitCast(LLVM::Value::getConstVector8(LLContext, RawRef),
                              Context.Int64x2Ty),
        PtrVal, LLContext.getInt64(1));
    Builder.createBr(EndBB);

    Builder.positionAtEnd(EndBB);
    auto Ret = Builder.createPHI(Context.Int64x2Ty);
    Ret.addIncoming(Ret1, IsNullBB);
    Ret.addIncoming(Ret2, NotNullBB);
    Ret.addIncoming(Ret3, IsExtrefBB);
    stackPush(Ret);
    break;
  }
  case OpCode::Extern__convert_any: {
    std::array<uint8_t, 16> RawRef = {0};
    auto Ref = stackPop();
    auto IsNullBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "extern_conv_any.null");
    auto NotNullBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "extern_conv_any.not_null");
    auto EndBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "extern_conv_any.end");
    auto CondIsNull = Builder.createICmpEQ(
        Builder.createExtractElement(Ref, LLContext.getInt64(1)),
        LLContext.getInt64(0));
    Builder.createCondBr(CondIsNull, IsNullBB, NotNullBB);

    Builder.positionAtEnd(IsNullBB);
    auto VT = ValType(TypeCode::RefNull, TypeCode::NullExternRef);
    std::copy_n(VT.getRawData().cbegin(), 8, RawRef.begin());
    auto Ret1 = Builder.createBitCast(
        LLVM::Value::getConstVector8(LLContext, RawRef), Context.Int64x2Ty);
    Builder.createBr(EndBB);

    Builder.positionAtEnd(NotNullBB);
    auto Ret2 =
        Builder.createBitCast(Builder.createInsertElement(
                                  Builder.createBitCast(Ref, Context.Int8x16Ty),
                                  LLContext.getInt8(1), LLContext.getInt64(1)),
                              Context.Int64x2Ty);
    Builder.createBr(EndBB);

    Builder.positionAtEnd(EndBB);
    auto Ret = Builder.createPHI(Context.Int64x2Ty);
    Ret.addIncoming(Ret1, IsNullBB);
    Ret.addIncoming(Ret2, NotNullBB);
    stackPush(Ret);
    break;
  }
  case OpCode::Ref__i31: {
    std::array<uint8_t, 16> RawRef = {0};
    auto VT = ValType(TypeCode::Ref, TypeCode::I31Ref);
    std::copy_n(VT.getRawData().cbegin(), 8, RawRef.begin());
    auto Ref = Builder.createBitCast(
        LLVM::Value::getConstVector8(LLContext, RawRef), Context.Int64x2Ty);
    auto Val = Builder.createZExt(
        Builder.createOr(
            Builder.createAnd(stackPop(), LLContext.getInt32(0x7FFFFFFFU)),
            LLContext.getInt32(0x80000000U)),
        Context.Int64Ty);
    stackPush(Builder.createInsertElement(Ref, Val, LLContext.getInt64(1)));
    break;
  }
  case OpCode::I31__get_s: {
    auto Next = LLVM::BasicBlock::create(LLContext, F.Fn, "i31.get.ok");
    auto Ref = Builder.createBitCast(stackPop(), Context.Int64x2Ty);
    auto Val = Builder.createTrunc(
        Builder.createExtractElement(Ref, LLContext.getInt64(1)),
        Context.Int32Ty);
    auto IsNotNull = Builder.createLikely(Builder.createICmpNE(
        Builder.createAnd(Val, LLContext.getInt32(0x80000000U)),
        LLContext.getInt32(0)));
    Builder.createCondBr(IsNotNull, Next,
                         getTrapBB(ErrCode::Value::AccessNullI31));
    Builder.positionAtEnd(Next);
    Val = Builder.createAnd(Val, LLContext.getInt32(0x7FFFFFFFU));
    stackPush(Builder.createOr(
        Val, Builder.createShl(
                 Builder.createAnd(Val, LLContext.getInt32(0x40000000U)),
                 LLContext.getInt32(1))));
    break;
  }
  case OpCode::I31__get_u: {
    auto Next = LLVM::BasicBlock::create(LLContext, F.Fn, "i31.get.ok");
    auto Ref = Builder.createBitCast(stackPop(), Context.Int64x2Ty);
    auto Val = Builder.createTrunc(
        Builder.createExtractElement(Ref, LLContext.getInt64(1)),
        Context.Int32Ty);
    auto IsNotNull = Builder.createLikely(Builder.createICmpNE(
        Builder.createAnd(Val, LLContext.getInt32(0x80000000U)),
        LLContext.getInt32(0)));
    Builder.createCondBr(IsNotNull, Next,
                         getTrapBB(ErrCode::Value::AccessNullI31));
    Builder.positionAtEnd(Next);
    stackPush(Builder.createAnd(Val, LLContext.getInt32(0x7FFFFFFFU)));
    break;
  }

  // Parametric Instructions
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

    // Variable Instructions
  default:
    assumingUnreachable();
  }
  return {};
}

} // namespace WasmEdge
