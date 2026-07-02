// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/function_compiler.h"

#include "runtime/instance/function.h"

using namespace std::literals;

namespace WasmEdge {

FunctionCompiler::FunctionCompiler(LLVM::Compiler::CompileContext &Context,
                                   LLVM::FunctionCallee F,
                                   Span<const ValType> Locals,
                                   bool Interruptible, bool InstructionCounting,
                                   bool GasMeasuring, bool IsLazyJIT) noexcept
    : Context(Context), LLContext(Context.LLContext),
      Interruptible(Interruptible), IsLazyJIT(IsLazyJIT), F(F),
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
      Builder.createStore(
          toLLVMConstantZero(LLContext, Type, Context.CompositeTypes), ArgPtr);
      Local.emplace_back(Ty, ArgPtr);
    }
  }
}

LLVM::BasicBlock FunctionCompiler::getTrapBB(ErrCode::Value Error) noexcept {
  if (auto Iter = TrapBB.find(Error); Iter != TrapBB.end()) {
    return Iter->second;
  }
  auto BB = LLVM::BasicBlock::create(LLContext, F.Fn, "trap");
  TrapBB.emplace(Error, BB);
  return BB;
}

Expect<void> FunctionCompiler::compile(
    const AST::CodeSegment &Code,
    std::pair<std::vector<ValType>, std::vector<ValType>> Type) noexcept {
  auto RetBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ret");
  Type.first.clear();
  enterBlock(RetBB, {}, {}, {}, std::move(Type));
  EXPECTED_TRY(compile(Code.getExpr().getInstrs()));
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
  return {};
}

Expect<void> FunctionCompiler::compile(AST::InstrView Instrs) noexcept {
  auto Dispatch = [this](const AST::Instruction &Instr) -> Expect<void> {
    switch (Instr.getOpCode()) {
    // Control instructions (for blocks)
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
      return {};
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
      return {};
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
      return {};
    }
    case OpCode::Try_table:
      // TODO: EXCEPTION - implement the AOT.
      return Unexpect(ErrCode::Value::AOTNotImpl);
    case OpCode::End: {
      auto Entry = leaveBlock();
      if (Entry.ElseBlock) {
        auto Block = Builder.getInsertBlock();
        Builder.positionAtEnd(Entry.ElseBlock);
        enterBlock(Block, {}, {}, std::move(Entry.Args), std::move(Entry.Type),
                   std::move(Entry.ReturnPHI));
        Entry = leaveBlock();
      }
      buildPHI(Entry.Type.second, Entry.ReturnPHI);
      return {};
    }
    case OpCode::Else: {
      auto Entry = leaveBlock();
      Builder.positionAtEnd(Entry.ElseBlock);
      enterBlock(Entry.JumpBlock, {}, {}, std::move(Entry.Args),
                 std::move(Entry.Type), std::move(Entry.ReturnPHI));
      return {};
    }
    default:
      break;
    }

    if (isUnreachable()) {
      return {};
    }

    switch (Instr.getOpCode()) {
    // Control instructions
    case OpCode::Unreachable:
      Builder.createBr(getTrapBB(ErrCode::Value::Unreachable));
      setUnreachable();
      Builder.positionAtEnd(
          LLVM::BasicBlock::create(LLContext, F.Fn, "unreachable.end"));
      break;
    case OpCode::Nop:
      break;
    case OpCode::Throw:
    case OpCode::Throw_ref:
      // TODO: EXCEPTION - implement the AOT.
      return Unexpect(ErrCode::Value::AOTNotImpl);
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
      const auto LabelTableSize = static_cast<uint32_t>(LabelTable.size() - 1);
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
    case OpCode::Br_on_null: {
      const auto Label = Instr.getJump().TargetIndex;
      auto Value = Builder.createBitCast(stackPop(), Context.Int64x2Ty);
      auto Cond = Builder.createICmpEQ(
          Builder.createExtractElement(Value, LLContext.getInt64(1)),
          LLContext.getInt64(0));
      setLableJumpPHI(Label);
      auto Next = LLVM::BasicBlock::create(LLContext, F.Fn, "br_on_null.end");
      Builder.createCondBr(Cond, getLabel(Label), Next);
      Builder.positionAtEnd(Next);
      stackPush(Value);
      break;
    }
    case OpCode::Br_on_non_null: {
      const auto Label = Instr.getJump().TargetIndex;
      auto Cond = Builder.createICmpNE(
          Builder.createExtractElement(
              Builder.createBitCast(Stack.back(), Context.Int64x2Ty),
              LLContext.getInt64(1)),
          LLContext.getInt64(0));
      setLableJumpPHI(Label);
      auto Next =
          LLVM::BasicBlock::create(LLContext, F.Fn, "br_on_non_null.end");
      Builder.createCondBr(Cond, getLabel(Label), Next);
      Builder.positionAtEnd(Next);
      stackPop();
      break;
    }
    case OpCode::Br_on_cast:
    case OpCode::Br_on_cast_fail: {
      auto Ref = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
      const auto Label = Instr.getBrCast().Jump.TargetIndex;
      std::array<uint8_t, 16> Buf = {0};
      std::copy_n(Instr.getBrCast().RType2.getRawData().cbegin(), 8,
                  Buf.begin());
      auto VType = Builder.createExtractElement(
          Builder.createBitCast(LLVM::Value::getConstVector8(LLContext, Buf),
                                Context.Int64x2Ty),
          LLContext.getInt64(0));
      auto IsRefTest = Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kRefTest,
              LLVM::Type::getFunctionType(Context.Int32Ty,
                                          {Context.Int64x2Ty, Context.Int64Ty},
                                          false)),
          {Ref, VType});
      auto Cond = (Instr.getOpCode() == OpCode::Br_on_cast)
                      ? Builder.createICmpNE(IsRefTest, LLContext.getInt32(0))
                      : Builder.createICmpEQ(IsRefTest, LLContext.getInt32(0));
      setLableJumpPHI(Label);
      auto Next = LLVM::BasicBlock::create(LLContext, F.Fn, "br_on_cast.end");
      Builder.createCondBr(Cond, getLabel(Label), Next);
      Builder.positionAtEnd(Next);
      break;
    }
    case OpCode::Return:
      compileReturn();
      setUnreachable();
      Builder.positionAtEnd(
          LLVM::BasicBlock::create(LLContext, F.Fn, "ret.end"));
      break;
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
    case OpCode::Call_ref:
      updateInstrCount();
      updateGas();
      compileCallRefOp(Instr.getTargetIndex());
      break;
    case OpCode::Return_call_ref:
      updateInstrCount();
      updateGas();
      compileReturnCallRefOp(Instr.getTargetIndex());
      setUnreachable();
      Builder.positionAtEnd(
          LLVM::BasicBlock::create(LLContext, F.Fn, "ret_call_ref.end"));
      break;
    case OpCode::Try_table:
      // TODO: EXCEPTION - implement the AOT.
      return Unexpect(ErrCode::Value::AOTNotImpl);

    // Reference Instructions
    case OpCode::Ref__null:
    case OpCode::Ref__is_null:
    case OpCode::Ref__func:
    case OpCode::Ref__eq:
    case OpCode::Ref__as_non_null:
    case OpCode::Struct__new:
    case OpCode::Struct__new_default:
    case OpCode::Struct__get:
    case OpCode::Struct__get_u:
    case OpCode::Struct__get_s:
    case OpCode::Struct__set:
    case OpCode::Array__new:
    case OpCode::Array__new_default:
    case OpCode::Array__new_fixed:
    case OpCode::Array__new_data:
    case OpCode::Array__new_elem:
    case OpCode::Array__get:
    case OpCode::Array__get_u:
    case OpCode::Array__get_s:
    case OpCode::Array__set:
    case OpCode::Array__len:
    case OpCode::Array__fill:
    case OpCode::Array__copy:
    case OpCode::Array__init_data:
    case OpCode::Array__init_elem:
    case OpCode::Ref__test:
    case OpCode::Ref__test_null:
    case OpCode::Ref__cast:
    case OpCode::Ref__cast_null:
    case OpCode::Any__convert_extern:
    case OpCode::Extern__convert_any:
    case OpCode::Ref__i31:
    case OpCode::I31__get_s:
    case OpCode::I31__get_u:
    case OpCode::Drop:
    case OpCode::Select:
    case OpCode::Select_t:
      return compileRefOp(Instr);
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

    // Table Instructions
    case OpCode::Table__get: {
      const auto TableIndex = Instr.getTargetIndex();
      auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "t_get.ok");
      Builder.createCondBr(
          Builder.createLikely(Builder.createICmpULT(
              Off, Context.getTableSize(Builder, ExecCtx, TableIndex))),
          OkBB, getTrapBB(ErrCode::Value::TableOutOfBounds));
      Builder.positionAtEnd(OkBB);
      stackPush(Builder.createLoad(
          Context.Int64x2Ty,
          Builder.createInBoundsGEP1(
              Context.Int64x2Ty, Context.getTable(Builder, ExecCtx, TableIndex),
              Off)));
      break;
    }
    case OpCode::Table__set: {
      const auto TableIndex = Instr.getTargetIndex();
      auto Ref = Builder.createBitCast(stackPop(), Context.Int64x2Ty);
      auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "t_set.ok");
      Builder.createCondBr(
          Builder.createLikely(Builder.createICmpULT(
              Off, Context.getTableSize(Builder, ExecCtx, TableIndex))),
          OkBB, getTrapBB(ErrCode::Value::TableOutOfBounds));
      Builder.positionAtEnd(OkBB);
      Builder.createStore(
          Ref, Builder.createInBoundsGEP1(
                   Context.Int64x2Ty,
                   Context.getTable(Builder, ExecCtx, TableIndex), Off));
      break;
    }
    case OpCode::Table__init: {
      auto Len = stackPop();
      auto Src = stackPop();
      auto Dst = Builder.createZExt(stackPop(), Context.Int64Ty);
      Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kTableInit,
              LLVM::Type::getFunctionType(Context.VoidTy,
                                          {Context.Int32Ty, Context.Int32Ty,
                                           Context.Int64Ty, Context.Int32Ty,
                                           Context.Int32Ty},
                                          false)),
          {LLContext.getInt32(Instr.getTargetIndex()),
           LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
      break;
    }
    case OpCode::Elem__drop: {
      Builder.createCall(
          Context.getIntrinsic(Builder, Executable::Intrinsics::kElemDrop,
                               LLVM::Type::getFunctionType(
                                   Context.VoidTy, {Context.Int32Ty}, false)),
          {LLContext.getInt32(Instr.getTargetIndex())});
      break;
    }
    case OpCode::Table__copy: {
      auto Len = Builder.createZExt(stackPop(), Context.Int64Ty);
      auto Src = Builder.createZExt(stackPop(), Context.Int64Ty);
      auto Dst = Builder.createZExt(stackPop(), Context.Int64Ty);
      Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kTableCopy,
              LLVM::Type::getFunctionType(Context.VoidTy,
                                          {Context.Int32Ty, Context.Int32Ty,
                                           Context.Int64Ty, Context.Int64Ty,
                                           Context.Int64Ty},
                                          false)),
          {LLContext.getInt32(Instr.getTargetIndex()),
           LLContext.getInt32(Instr.getSourceIndex()), Dst, Src, Len});
      break;
    }
    case OpCode::Table__grow: {
      auto NewSize = Builder.createZExt(stackPop(), Context.Int64Ty);
      auto Val = stackPop();
      stackPush(Builder.createTrunc(
          Builder.createCall(
              Context.getIntrinsic(
                  Builder, Executable::Intrinsics::kTableGrow,
                  LLVM::Type::getFunctionType(
                      Context.Int64Ty,
                      {Context.Int32Ty, Context.Int64x2Ty, Context.Int64Ty},
                      false)),
              {LLContext.getInt32(Instr.getTargetIndex()), Val, NewSize}),
          Context.TableAddrTypes[Instr.getTargetIndex()]));
      break;
    }
    case OpCode::Table__size: {
      stackPush(Builder.createTrunc(
          Context.getTableSize(Builder, ExecCtx, Instr.getTargetIndex()),
          Context.TableAddrTypes[Instr.getTargetIndex()]));
      break;
    }
    case OpCode::Table__fill: {
      auto Len = Builder.createZExt(stackPop(), Context.Int64Ty);
      auto Val = stackPop();
      auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
      Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kTableFill,
              LLVM::Type::getFunctionType(Context.Int32Ty,
                                          {Context.Int32Ty, Context.Int64Ty,
                                           Context.Int64x2Ty, Context.Int64Ty},
                                          false)),
          {LLContext.getInt32(Instr.getTargetIndex()), Off, Val, Len});
      break;
    }

    // Memory Instructions
    case OpCode::I32__load:
    case OpCode::I64__load:
    case OpCode::F32__load:
    case OpCode::F64__load:
    case OpCode::I32__load8_s:
    case OpCode::I32__load8_u:
    case OpCode::I32__load16_s:
    case OpCode::I32__load16_u:
    case OpCode::I64__load8_s:
    case OpCode::I64__load8_u:
    case OpCode::I64__load16_s:
    case OpCode::I64__load16_u:
    case OpCode::I64__load32_s:
    case OpCode::I64__load32_u:
    case OpCode::I32__store:
    case OpCode::I64__store:
    case OpCode::F32__store:
    case OpCode::F64__store:
    case OpCode::I32__store8:
    case OpCode::I64__store8:
    case OpCode::I32__store16:
    case OpCode::I64__store16:
    case OpCode::I64__store32:
    case OpCode::Memory__size:
    case OpCode::Memory__grow:
    case OpCode::Memory__init:
    case OpCode::Data__drop:
    case OpCode::Memory__copy:
    case OpCode::Memory__fill:
    case OpCode::I32__const:
    case OpCode::I64__const:
    case OpCode::F32__const:
    case OpCode::F64__const:
      return compileMemoryOp(Instr);
    // Unary Numeric Instructions
    case OpCode::I32__eqz:
    case OpCode::I64__eqz:
    case OpCode::I32__clz:
    case OpCode::I64__clz:
    case OpCode::I32__ctz:
    case OpCode::I64__ctz:
    case OpCode::I32__popcnt:
    case OpCode::I64__popcnt:
    case OpCode::F32__abs:
    case OpCode::F64__abs:
    case OpCode::F32__neg:
    case OpCode::F64__neg:
    case OpCode::F32__ceil:
    case OpCode::F64__ceil:
    case OpCode::F32__floor:
    case OpCode::F64__floor:
    case OpCode::F32__trunc:
    case OpCode::F64__trunc:
    case OpCode::F32__nearest:
    case OpCode::F64__nearest:
    case OpCode::F32__sqrt:
    case OpCode::F64__sqrt:
    case OpCode::I32__wrap_i64:
    case OpCode::I32__trunc_f32_s:
    case OpCode::I32__trunc_f64_s:
    case OpCode::I32__trunc_f32_u:
    case OpCode::I32__trunc_f64_u:
    case OpCode::I64__extend_i32_s:
    case OpCode::I64__extend_i32_u:
    case OpCode::I64__trunc_f32_s:
    case OpCode::I64__trunc_f64_s:
    case OpCode::I64__trunc_f32_u:
    case OpCode::I64__trunc_f64_u:
    case OpCode::F32__convert_i32_s:
    case OpCode::F32__convert_i64_s:
    case OpCode::F32__convert_i32_u:
    case OpCode::F32__convert_i64_u:
    case OpCode::F64__convert_i32_s:
    case OpCode::F64__convert_i64_s:
    case OpCode::F64__convert_i32_u:
    case OpCode::F64__convert_i64_u:
    case OpCode::F32__demote_f64:
    case OpCode::F64__promote_f32:
    case OpCode::I32__reinterpret_f32:
    case OpCode::I64__reinterpret_f64:
    case OpCode::F32__reinterpret_i32:
    case OpCode::F64__reinterpret_i64:
    case OpCode::I32__extend8_s:
    case OpCode::I32__extend16_s:
    case OpCode::I64__extend8_s:
    case OpCode::I64__extend16_s:
    case OpCode::I64__extend32_s:
    case OpCode::I32__eq:
    case OpCode::I64__eq:
    case OpCode::I32__ne:
    case OpCode::I64__ne:
    case OpCode::I32__lt_s:
    case OpCode::I64__lt_s:
    case OpCode::I32__lt_u:
    case OpCode::I64__lt_u:
    case OpCode::I32__gt_s:
    case OpCode::I64__gt_s:
    case OpCode::I32__gt_u:
    case OpCode::I64__gt_u:
    case OpCode::I32__le_s:
    case OpCode::I64__le_s:
    case OpCode::I32__le_u:
    case OpCode::I64__le_u:
    case OpCode::I32__ge_s:
    case OpCode::I64__ge_s:
    case OpCode::I32__ge_u:
    case OpCode::I64__ge_u:
    case OpCode::F32__eq:
    case OpCode::F64__eq:
    case OpCode::F32__ne:
    case OpCode::F64__ne:
    case OpCode::F32__lt:
    case OpCode::F64__lt:
    case OpCode::F32__gt:
    case OpCode::F64__gt:
    case OpCode::F32__le:
    case OpCode::F64__le:
    case OpCode::F32__ge:
    case OpCode::F64__ge:
    case OpCode::I32__add:
    case OpCode::I64__add:
    case OpCode::I32__sub:
    case OpCode::I64__sub:
    case OpCode::I32__mul:
    case OpCode::I64__mul:
    case OpCode::I32__div_s:
    case OpCode::I64__div_s:
    case OpCode::I32__div_u:
    case OpCode::I64__div_u:
    case OpCode::I32__rem_s:
    case OpCode::I64__rem_s:
    case OpCode::I32__rem_u:
    case OpCode::I64__rem_u:
    case OpCode::I32__and:
    case OpCode::I64__and:
    case OpCode::I32__or:
    case OpCode::I64__or:
    case OpCode::I32__xor:
    case OpCode::I64__xor:
    case OpCode::I32__shl:
    case OpCode::I64__shl:
    case OpCode::I32__shr_s:
    case OpCode::I64__shr_s:
    case OpCode::I32__shr_u:
    case OpCode::I64__shr_u:
    case OpCode::I32__rotl:
    case OpCode::I32__rotr:
    case OpCode::I64__rotl:
    case OpCode::I64__rotr:
    case OpCode::F32__add:
    case OpCode::F64__add:
    case OpCode::F32__sub:
    case OpCode::F64__sub:
    case OpCode::F32__mul:
    case OpCode::F64__mul:
    case OpCode::F32__div:
    case OpCode::F64__div:
    case OpCode::F32__min:
    case OpCode::F64__min:
    case OpCode::F32__max:
    case OpCode::F64__max:
    case OpCode::F32__copysign:
    case OpCode::F64__copysign:
    case OpCode::I32__trunc_sat_f32_s:
    case OpCode::I32__trunc_sat_f32_u:
    case OpCode::I32__trunc_sat_f64_s:
    case OpCode::I32__trunc_sat_f64_u:
    case OpCode::I64__trunc_sat_f32_s:
    case OpCode::I64__trunc_sat_f32_u:
    case OpCode::I64__trunc_sat_f64_s:
    case OpCode::I64__trunc_sat_f64_u:
      return compileNumericOp(Instr);
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

    // SIMD Const Instructions
    case OpCode::V128__const: {
      const auto Value = Instr.getNum().get<uint64x2_t>();
      auto Vector =
          LLVM::Value::getConstVector64(LLContext, {Value[0], Value[1]});
      stackPush(Builder.createBitCast(Vector, Context.Int64x2Ty));
      break;
    }

    // SIMD Shuffle Instructions
    case OpCode::I8x16__shuffle: {
      auto V2 = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
      auto V1 = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
      const auto V3 = Instr.getNum().get<uint128_t>();
      std::array<uint8_t, 16> Mask;
      for (size_t I = 0; I < 16; ++I) {
        auto Num = static_cast<uint8_t>(V3 >> (I * 8));
        if constexpr (Endian::native == Endian::little) {
          Mask[I] = Num;
        } else {
          Mask[15 - I] = Num < 16 ? 15 - Num : 47 - Num;
        }
      }
      stackPush(Builder.createBitCast(
          Builder.createShuffleVector(
              V1, V2, LLVM::Value::getConstVector8(LLContext, Mask)),
          Context.Int64x2Ty));
      break;
    }

    // SIMD Lane Instructions
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

    // SIMD Numeric Instructions
    case OpCode::I8x16__swizzle:
      compileVectorSwizzle();
      break;
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
      compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOEQ, Context.Int32x4Ty);
      break;
    case OpCode::F32x4__ne:
      compileVectorCompareOp(Context.Floatx4Ty, LLVMRealUNE, Context.Int32x4Ty);
      break;
    case OpCode::F32x4__lt:
      compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOLT, Context.Int32x4Ty);
      break;
    case OpCode::F32x4__gt:
      compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOGT, Context.Int32x4Ty);
      break;
    case OpCode::F32x4__le:
      compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOLE, Context.Int32x4Ty);
      break;
    case OpCode::F32x4__ge:
      compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOGE, Context.Int32x4Ty);
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
          M, Undef, LLVM::Value::getConstVector32(LLContext, {0U, 2U, 4U, 6U}));
      auto R = Builder.createShuffleVector(
          M, Undef, LLVM::Value::getConstVector32(LLContext, {1U, 3U, 5U, 7U}));
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

    // Relaxed SIMD Instructions
    case OpCode::I8x16__relaxed_swizzle:
      compileVectorSwizzle();
      break;
    case OpCode::I32x4__relaxed_trunc_f32x4_s:
      compileVectorTruncSatS32(Context.Floatx4Ty, false);
      break;
    case OpCode::I32x4__relaxed_trunc_f32x4_u:
      compileVectorTruncSatU32(Context.Floatx4Ty, false);
      break;
    case OpCode::I32x4__relaxed_trunc_f64x2_s_zero:
      compileVectorTruncSatS32(Context.Doublex2Ty, true);
      break;
    case OpCode::I32x4__relaxed_trunc_f64x2_u_zero:
      compileVectorTruncSatU32(Context.Doublex2Ty, true);
      break;
    case OpCode::F32x4__relaxed_madd:
      compileVectorVectorMAdd(Context.Floatx4Ty);
      break;
    case OpCode::F32x4__relaxed_nmadd:
      compileVectorVectorNMAdd(Context.Floatx4Ty);
      break;
    case OpCode::F64x2__relaxed_madd:
      compileVectorVectorMAdd(Context.Doublex2Ty);
      break;
    case OpCode::F64x2__relaxed_nmadd:
      compileVectorVectorNMAdd(Context.Doublex2Ty);
      break;
    case OpCode::I8x16__relaxed_laneselect:
    case OpCode::I16x8__relaxed_laneselect:
    case OpCode::I32x4__relaxed_laneselect:
    case OpCode::I64x2__relaxed_laneselect: {
      auto C = stackPop();
      auto V2 = stackPop();
      auto V1 = stackPop();
      stackPush(Builder.createXor(
          Builder.createAnd(Builder.createXor(V1, V2), C), V2));
      break;
    }
    case OpCode::F32x4__relaxed_min:
      compileVectorVectorFMin(Context.Floatx4Ty);
      break;
    case OpCode::F32x4__relaxed_max:
      compileVectorVectorFMax(Context.Floatx4Ty);
      break;
    case OpCode::F64x2__relaxed_min:
      compileVectorVectorFMin(Context.Doublex2Ty);
      break;
    case OpCode::F64x2__relaxed_max:
      compileVectorVectorFMax(Context.Doublex2Ty);
      break;
    case OpCode::I16x8__relaxed_q15mulr_s:
      compileVectorVectorQ15MulSat();
      break;
    case OpCode::I16x8__relaxed_dot_i8x16_i7x16_s:
      compileVectorRelaxedIntegerDotProduct();
      break;
    case OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s:
      compileVectorRelaxedIntegerDotProductAdd();
      break;

    // Atomic Instructions
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
                        Instr.getMemoryAlign(), Context.Int32Ty,
                        Context.Int32Ty, true);
      break;
    case OpCode::I64__atomic__load:
      compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int64Ty,
                        Context.Int64Ty, true);
      break;
    case OpCode::I32__atomic__load8_u:
      compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int32Ty,
                        Context.Int8Ty);
      break;
    case OpCode::I32__atomic__load16_u:
      compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int32Ty,
                        Context.Int16Ty);
      break;
    case OpCode::I64__atomic__load8_u:
      compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int64Ty,
                        Context.Int8Ty);
      break;
    case OpCode::I64__atomic__load16_u:
      compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int64Ty,
                        Context.Int16Ty);
      break;
    case OpCode::I64__atomic__load32_u:
      compileAtomicLoad(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int64Ty,
                        Context.Int32Ty);
      break;
    case OpCode::I32__atomic__store:
      compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                         Instr.getMemoryAlign(), Context.Int32Ty,
                         Context.Int32Ty, true);
      break;
    case OpCode::I64__atomic__store:
      compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                         Instr.getMemoryAlign(), Context.Int64Ty,
                         Context.Int64Ty, true);
      break;
    case OpCode::I32__atomic__store8:
      compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                         Instr.getMemoryAlign(), Context.Int32Ty,
                         Context.Int8Ty, true);
      break;
    case OpCode::I32__atomic__store16:
      compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                         Instr.getMemoryAlign(), Context.Int32Ty,
                         Context.Int16Ty, true);
      break;
    case OpCode::I64__atomic__store8:
      compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                         Instr.getMemoryAlign(), Context.Int64Ty,
                         Context.Int8Ty, true);
      break;
    case OpCode::I64__atomic__store16:
      compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                         Instr.getMemoryAlign(), Context.Int64Ty,
                         Context.Int16Ty, true);
      break;
    case OpCode::I64__atomic__store32:
      compileAtomicStore(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                         Instr.getMemoryAlign(), Context.Int64Ty,
                         Context.Int32Ty, true);
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
          Instr.getTargetIndex(), Instr.getMemoryOffset(),
          Instr.getMemoryAlign(), Context.Int32Ty, Context.Int32Ty, true);
      break;
    case OpCode::I64__atomic__rmw__cmpxchg:
      compileAtomicCompareExchange(
          Instr.getTargetIndex(), Instr.getMemoryOffset(),
          Instr.getMemoryAlign(), Context.Int64Ty, Context.Int64Ty, true);
      break;
    case OpCode::I32__atomic__rmw8__cmpxchg_u:
      compileAtomicCompareExchange(
          Instr.getTargetIndex(), Instr.getMemoryOffset(),
          Instr.getMemoryAlign(), Context.Int32Ty, Context.Int8Ty);
      break;
    case OpCode::I32__atomic__rmw16__cmpxchg_u:
      compileAtomicCompareExchange(
          Instr.getTargetIndex(), Instr.getMemoryOffset(),
          Instr.getMemoryAlign(), Context.Int32Ty, Context.Int16Ty);
      break;
    case OpCode::I64__atomic__rmw8__cmpxchg_u:
      compileAtomicCompareExchange(
          Instr.getTargetIndex(), Instr.getMemoryOffset(),
          Instr.getMemoryAlign(), Context.Int64Ty, Context.Int8Ty);
      break;
    case OpCode::I64__atomic__rmw16__cmpxchg_u:
      compileAtomicCompareExchange(
          Instr.getTargetIndex(), Instr.getMemoryOffset(),
          Instr.getMemoryAlign(), Context.Int64Ty, Context.Int16Ty);
      break;
    case OpCode::I64__atomic__rmw32__cmpxchg_u:
      compileAtomicCompareExchange(
          Instr.getTargetIndex(), Instr.getMemoryOffset(),
          Instr.getMemoryAlign(), Context.Int64Ty, Context.Int32Ty);
      break;

    default:
      assumingUnreachable();
    }
    return {};
  };

  for (const auto &Instr : Instrs) {
    // Update instruction count
    if (LocalInstrCount) {
      Builder.createStore(Builder.createAdd(Builder.createLoad(Context.Int64Ty,
                                                               LocalInstrCount),
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
    EXPECTED_TRY(Dispatch(Instr));
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
  stackPush(Builder.createTrunc(
      Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kMemAtomicNotify,
              LLVM::Type::getFunctionType(
                  Context.Int64Ty,
                  {Context.Int32Ty, Context.Int64Ty, Context.Int64Ty}, false)),
          {LLContext.getInt32(MemoryIndex), Offset, Count}),
      Context.MemoryAddrTypes[MemoryIndex]));
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
      Context.MemoryAddrTypes[MemoryIndex]));
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

void FunctionCompiler::compileReturn() noexcept {
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

void FunctionCompiler::updateInstrCount() noexcept {
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

void FunctionCompiler::updateGas() noexcept {
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

void FunctionCompiler::updateGasAtTrap() noexcept {
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

void FunctionCompiler::compileCallOp(const unsigned int FuncIndex) noexcept {
  const auto &FuncType =
      Context.CompositeTypes[std::get<0>(Context.Functions[FuncIndex])]
          ->getFuncType();
  const auto &Function = std::get<1>(Context.Functions[FuncIndex]);
  const auto &ParamTypes = FuncType.getParamTypes();

  std::vector<LLVM::Value> Args(ParamTypes.size() + 1);
  Args[0] = F.Fn.getFirstParam();
  for (size_t I = 0; I < ParamTypes.size(); ++I) {
    const size_t J = ParamTypes.size() - 1 - I;
    Args[J + 1] = stackPop();
  }

  LLVM::Value Ret;
  if (IsLazyJIT) {
    bool IsImport = std::get<2>(Context.Functions[FuncIndex]) == nullptr;
    if (IsImport) {
      Ret = Builder.createCall(Function, Args);
    } else {
      auto FTy = toLLVMType(LLContext, Context.ExecCtxPtrTy, FuncType);

      if (Context.LazyJITCacheVars.size() <= FuncIndex) {
        Context.LazyJITCacheVars.resize(Context.Functions.size());
      }
      auto &CacheVar = Context.LazyJITCacheVars[FuncIndex];
      if (!CacheVar) {
        CacheVar = Context.LLModule.get().addGlobal(
            FTy.getPointerTo(), false, LLVMPrivateLinkage,
            LLVM::Value::getConstNull(FTy.getPointerTo()), "");
      }

      auto CheckBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ic.check");
      auto ResolveBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ic.resolve");
      auto CallBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ic.call");

      Builder.createBr(CheckBB);
      Builder.positionAtEnd(CheckBB);

      auto CachedPtr = Builder.createLoad(FTy.getPointerTo(), CacheVar, false);
      CachedPtr.setAlignment(8);
      CachedPtr.setOrdering(LLVMAtomicOrderingAcquire);
      auto IsNull = Builder.createIsNull(CachedPtr);
      auto IsNotNull = Builder.createLikely(Builder.createNot(IsNull));
      Builder.createCondBr(IsNotNull, CallBB, ResolveBB);

      Builder.positionAtEnd(ResolveBB);
      auto FPtr = Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kFuncGetFuncSymbol,
              LLVM::Type::getFunctionType(FTy.getPointerTo(), {Context.Int32Ty},
                                          false)),
          {LLContext.getInt32(FuncIndex)});
      auto Store = Builder.createStore(FPtr, CacheVar);
      Store.setAlignment(8);
      Store.setOrdering(LLVMAtomicOrderingRelease);
      Builder.createBr(CallBB);

      Builder.positionAtEnd(CallBB);
      auto FinalPtr = Builder.createPHI(FTy.getPointerTo());
      FinalPtr.addIncoming(CachedPtr, CheckBB);
      FinalPtr.addIncoming(FPtr, ResolveBB);

      Ret = Builder.createCall(LLVM::FunctionCallee(FTy, FinalPtr), Args);
    }
  } else {
    Ret = Builder.createCall(Function, Args);
  }

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

void FunctionCompiler::compileIndirectCallOp(
    const uint32_t TableIndex, const uint32_t FuncTypeIndex) noexcept {
  auto TryFastBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.tryfast");
  auto NonNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.nonnull");
  auto FastBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.fast");
  auto SlowBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.slow");
  auto NotNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.not_null");
  auto IsNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.is_null");
  auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.end");

  LLVM::Value FuncIndex = stackPop();
  const auto &FuncType = Context.CompositeTypes[FuncTypeIndex]->getFuncType();
  auto FTy = toLLVMType(Context.LLContext, Context.ExecCtxPtrTy, FuncType);
  auto RTy = FTy.getReturnType();
  auto FPtrTy = FTy.getPointerTo();
  auto TableIdx = LLContext.getInt32(TableIndex);
  auto TypeIdx = LLContext.getInt32(FuncTypeIndex);

  const size_t ArgSize = FuncType.getParamTypes().size();
  const size_t RetSize = RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();
  std::vector<LLVM::Value> ArgsVec(ArgSize + 1, nullptr);
  ArgsVec[0] = F.Fn.getFirstParam();
  for (size_t I = 0; I < ArgSize; ++I) {
    const size_t J = ArgSize - I;
    ArgsVec[J] = stackPop();
  }
  auto UnpackRets = [&](LLVM::Value Ret) -> std::vector<LLVM::Value> {
    if (RetSize == 0) {
      return {};
    } else if (RetSize == 1) {
      return {Ret};
    } else {
      return unpackStruct(Builder, Ret);
    }
  };

  auto Idx64 = Builder.createZExt(FuncIndex, Context.Int64Ty);

  // Fast path: an in-bounds funcref defined in the running module with the
  // call site's type index and compiled code is called directly.
  std::vector<LLVM::Value> FastRetsVec;
  {
    Builder.createCondBr(
        Builder.createLikely(Builder.createICmpULT(
            Idx64, Context.getTableSize(Builder, ExecCtx, TableIndex))),
        TryFastBB, SlowBB);
    Builder.positionAtEnd(TryFastBB);

    auto FuncRef = Builder.createLoad(
        Context.Int64x2Ty,
        Builder.createInBoundsGEP1(
            Context.Int64x2Ty, Context.getTable(Builder, ExecCtx, TableIndex),
            Idx64));
    auto FuncInstInt =
        Builder.createExtractElement(FuncRef, LLContext.getInt64(1));
    Builder.createCondBr(Builder.createLikely(Builder.createICmpNE(
                             FuncInstInt, LLContext.getInt64(0))),
                         NonNullBB, SlowBB);
    Builder.positionAtEnd(NonNullBB);

    auto FuncInstPtr = Builder.createIntToPtr(FuncInstInt, Context.Int8PtrTy);
    auto LoadField = [&](uint64_t Off, LLVM::Type Ty) {
      return Builder.createLoad(
          Ty, Builder.createBitCast(
                  Builder.createInBoundsGEP1(Context.Int8Ty, FuncInstPtr,
                                             LLContext.getInt64(Off)),
                  Ty.getPointerTo()));
    };
    using Runtime::Instance::FunctionInstance;
    auto DefModule =
        LoadField(FunctionInstance::getModuleOffset(), Context.Int8PtrTy);
    auto CalleeTypeIdx =
        LoadField(FunctionInstance::getTypeIndexOffset(), Context.Int32Ty);
    auto Code =
        LoadField(FunctionInstance::getCompiledCodeOffset(), Context.Int8PtrTy);
    auto Hit = Builder.createAnd(
        Builder.createAnd(
            Builder.createICmpEQ(DefModule,
                                 Context.getModuleInst(Builder, ExecCtx)),
            Builder.createICmpEQ(CalleeTypeIdx, TypeIdx)),
        Builder.createNot(Builder.createIsNull(Code)));
    Builder.createCondBr(Builder.createLikely(Hit), FastBB, SlowBB);

    Builder.positionAtEnd(FastBB);
    auto FastRet = Builder.createCall(
        LLVM::FunctionCallee{FTy, Builder.createBitCast(Code, FPtrTy)},
        ArgsVec);
    FastRetsVec = UnpackRets(FastRet);
    Builder.createBr(EndBB);
  }

  // Slow path: resolve through the runtime, which handles cross-module, host,
  // subtype, uninitialized, and not-yet-compiled cases.
  Builder.positionAtEnd(SlowBB);
  std::vector<LLVM::Value> FPtrRetsVec;
  {
    auto FPtr = Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kTableGetFuncSymbol,
            LLVM::Type::getFunctionType(
                FPtrTy, {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty},
                false)),
        {TableIdx, TypeIdx, FuncIndex});
    Builder.createCondBr(
        Builder.createLikely(Builder.createNot(Builder.createIsNull(FPtr))),
        NotNullBB, IsNullBB);
    Builder.positionAtEnd(NotNullBB);

    auto FPtrRet = Builder.createCall(LLVM::FunctionCallee{FTy, FPtr}, ArgsVec);
    FPtrRetsVec = UnpackRets(FPtrRet);
  }

  Builder.createBr(EndBB);
  Builder.positionAtEnd(IsNullBB);

  std::vector<LLVM::Value> RetsVec;
  {
    LLVM::Value Args = Builder.createArray(ArgSize, LLVM::kValSize);
    LLVM::Value Rets = Builder.createArray(RetSize, LLVM::kValSize);
    Builder.createArrayPtrStore(Span<LLVM::Value>(ArgsVec.begin() + 1, ArgSize),
                                Args, Context.Int8Ty, LLVM::kValSize);

    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kCallIndirect,
            LLVM::Type::getFunctionType(Context.VoidTy,
                                        {Context.Int32Ty, Context.Int32Ty,
                                         Context.Int32Ty, Context.Int8PtrTy,
                                         Context.Int8PtrTy},
                                        false)),
        {TableIdx, TypeIdx, FuncIndex, Args, Rets});

    if (RetSize == 0) {
      // nothing to do
    } else if (RetSize == 1) {
      RetsVec.push_back(Builder.createValuePtrLoad(RTy, Rets, Context.Int8Ty));
    } else {
      RetsVec = Builder.createArrayPtrLoad(RetSize, RTy, Rets, Context.Int8Ty,
                                           LLVM::kValSize);
    }
    Builder.createBr(EndBB);
    Builder.positionAtEnd(EndBB);
  }

  for (unsigned I = 0; I < RetSize; ++I) {
    auto PHIRet = Builder.createPHI(FPtrRetsVec[I].getType());
    PHIRet.addIncoming(FastRetsVec[I], FastBB);
    PHIRet.addIncoming(FPtrRetsVec[I], NotNullBB);
    PHIRet.addIncoming(RetsVec[I], IsNullBB);
    stackPush(PHIRet);
  }
}

void FunctionCompiler::compileReturnCallOp(
    const unsigned int FuncIndex) noexcept {
  const auto &FuncType =
      Context.CompositeTypes[std::get<0>(Context.Functions[FuncIndex])]
          ->getFuncType();
  const auto &Function = std::get<1>(Context.Functions[FuncIndex]);
  const auto &ParamTypes = FuncType.getParamTypes();

  std::vector<LLVM::Value> Args(ParamTypes.size() + 1);
  Args[0] = F.Fn.getFirstParam();
  for (size_t I = 0; I < ParamTypes.size(); ++I) {
    const size_t J = ParamTypes.size() - 1 - I;
    Args[J + 1] = stackPop();
  }

  LLVM::Value Ret;
  if (IsLazyJIT) {
    bool IsImport = std::get<2>(Context.Functions[FuncIndex]) == nullptr;
    if (IsImport) {
      Ret = Builder.createCall(Function, Args);
    } else {
      auto FTy = toLLVMType(LLContext, Context.ExecCtxPtrTy, FuncType);

      if (Context.LazyJITCacheVars.size() <= FuncIndex) {
        Context.LazyJITCacheVars.resize(Context.Functions.size());
      }
      auto &CacheVar = Context.LazyJITCacheVars[FuncIndex];
      if (!CacheVar) {
        CacheVar = Context.LLModule.get().addGlobal(
            FTy.getPointerTo(), false, LLVMPrivateLinkage,
            LLVM::Value::getConstNull(FTy.getPointerTo()), "");
      }

      auto CheckBB = LLVM::BasicBlock::create(LLContext, F.Fn, "rc.check");
      auto ResolveBB = LLVM::BasicBlock::create(LLContext, F.Fn, "rc.resolve");
      auto CallBB = LLVM::BasicBlock::create(LLContext, F.Fn, "rc.call");

      Builder.createBr(CheckBB);
      Builder.positionAtEnd(CheckBB);

      auto CachedPtr = Builder.createLoad(FTy.getPointerTo(), CacheVar, false);
      CachedPtr.setAlignment(8);
      CachedPtr.setOrdering(LLVMAtomicOrderingAcquire);
      auto IsNull = Builder.createIsNull(CachedPtr);
      auto IsNotNull = Builder.createLikely(Builder.createNot(IsNull));
      Builder.createCondBr(IsNotNull, CallBB, ResolveBB);

      Builder.positionAtEnd(ResolveBB);
      auto FPtr = Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kFuncGetFuncSymbol,
              LLVM::Type::getFunctionType(FTy.getPointerTo(), {Context.Int32Ty},
                                          false)),
          {LLContext.getInt32(FuncIndex)});
      auto Store = Builder.createStore(FPtr, CacheVar);
      Store.setAlignment(8);
      Store.setOrdering(LLVMAtomicOrderingRelease);
      Builder.createBr(CallBB);

      Builder.positionAtEnd(CallBB);
      auto FinalPtr = Builder.createPHI(FTy.getPointerTo());
      FinalPtr.addIncoming(CachedPtr, CheckBB);
      FinalPtr.addIncoming(FPtr, ResolveBB);

      Ret = Builder.createCall(LLVM::FunctionCallee(FTy, FinalPtr), Args);
    }
  } else {
    Ret = Builder.createCall(Function, Args);
  }

  Ret.setMustTailCall();
  auto Ty = Ret.getType();
  if (Ty.isVoidTy()) {
    Builder.createRetVoid();
  } else {
    Builder.createRet(Ret);
  }
}

void FunctionCompiler::compileReturnIndirectCallOp(
    const uint32_t TableIndex, const uint32_t FuncTypeIndex) noexcept {
  auto NotNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.not_null");
  auto IsNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.is_null");

  LLVM::Value FuncIndex = stackPop();
  const auto &FuncType = Context.CompositeTypes[FuncTypeIndex]->getFuncType();
  auto FTy = toLLVMType(Context.LLContext, Context.ExecCtxPtrTy, FuncType);
  auto RTy = FTy.getReturnType();

  const size_t ArgSize = FuncType.getParamTypes().size();
  const size_t RetSize = RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();
  std::vector<LLVM::Value> ArgsVec(ArgSize + 1, nullptr);
  ArgsVec[0] = F.Fn.getFirstParam();
  for (size_t I = 0; I < ArgSize; ++I) {
    const size_t J = ArgSize - I;
    ArgsVec[J] = stackPop();
  }

  {
    auto FPtr = Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kTableGetFuncSymbol,
            LLVM::Type::getFunctionType(
                FTy.getPointerTo(),
                {Context.Int32Ty, Context.Int32Ty, Context.Int32Ty}, false)),
        {LLContext.getInt32(TableIndex), LLContext.getInt32(FuncTypeIndex),
         FuncIndex});
    Builder.createCondBr(
        Builder.createLikely(Builder.createNot(Builder.createIsNull(FPtr))),
        NotNullBB, IsNullBB);
    Builder.positionAtEnd(NotNullBB);

    auto FPtrRet = Builder.createCall(LLVM::FunctionCallee(FTy, FPtr), ArgsVec);
    FPtrRet.setMustTailCall();
    if (RetSize == 0) {
      Builder.createRetVoid();
    } else {
      Builder.createRet(FPtrRet);
    }
  }

  Builder.positionAtEnd(IsNullBB);

  {
    LLVM::Value Args = Builder.createArray(ArgSize, LLVM::kValSize);
    LLVM::Value Rets = Builder.createArray(RetSize, LLVM::kValSize);
    Builder.createArrayPtrStore(Span<LLVM::Value>(ArgsVec.begin() + 1, ArgSize),
                                Args, Context.Int8Ty, LLVM::kValSize);

    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kCallIndirect,
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
      Builder.createRet(Builder.createValuePtrLoad(RTy, Rets, Context.Int8Ty));
    } else {
      Builder.createAggregateRet(Builder.createArrayPtrLoad(
          RetSize, RTy, Rets, Context.Int8Ty, LLVM::kValSize));
    }
  }
}

void FunctionCompiler::compileCallRefOp(const unsigned int TypeIndex) noexcept {
  auto NotNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_r.not_null");
  auto IsNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_r.is_null");
  auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.end");

  auto Ref = Builder.createBitCast(stackPop(), Context.Int64x2Ty);
  auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_r.ref_not_null");
  auto IsRefNotNull = Builder.createLikely(Builder.createICmpNE(
      Builder.createExtractElement(Ref, LLContext.getInt64(1)),
      LLContext.getInt64(0)));
  Builder.createCondBr(IsRefNotNull, OkBB,
                       getTrapBB(ErrCode::Value::AccessNullFunc));
  Builder.positionAtEnd(OkBB);

  const auto &FuncType = Context.CompositeTypes[TypeIndex]->getFuncType();
  auto FTy = toLLVMType(Context.LLContext, Context.ExecCtxPtrTy, FuncType);
  auto RTy = FTy.getReturnType();

  const size_t ArgSize = FuncType.getParamTypes().size();
  const size_t RetSize = RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();
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
        Context.getIntrinsic(Builder, Executable::Intrinsics::kRefGetFuncSymbol,
                             LLVM::Type::getFunctionType(FTy.getPointerTo(),
                                                         {Context.Int64x2Ty},
                                                         false)),
        {Ref});
    Builder.createCondBr(
        Builder.createLikely(Builder.createNot(Builder.createIsNull(FPtr))),
        NotNullBB, IsNullBB);
    Builder.positionAtEnd(NotNullBB);

    auto FPtrRet = Builder.createCall(LLVM::FunctionCallee{FTy, FPtr}, ArgsVec);
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

  std::vector<LLVM::Value> RetsVec;
  {
    LLVM::Value Args = Builder.createArray(ArgSize, LLVM::kValSize);
    LLVM::Value Rets = Builder.createArray(RetSize, LLVM::kValSize);
    Builder.createArrayPtrStore(Span<LLVM::Value>(ArgsVec.begin() + 1, ArgSize),
                                Args, Context.Int8Ty, LLVM::kValSize);

    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kCallRef,
            LLVM::Type::getFunctionType(
                Context.VoidTy,
                {Context.Int64x2Ty, Context.Int8PtrTy, Context.Int8PtrTy},
                false)),
        {Ref, Args, Rets});

    if (RetSize == 0) {
      // nothing to do
    } else if (RetSize == 1) {
      RetsVec.push_back(Builder.createValuePtrLoad(RTy, Rets, Context.Int8Ty));
    } else {
      RetsVec = Builder.createArrayPtrLoad(RetSize, RTy, Rets, Context.Int8Ty,
                                           LLVM::kValSize);
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

void FunctionCompiler::compileReturnCallRefOp(
    const unsigned int TypeIndex) noexcept {
  auto NotNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_r.not_null");
  auto IsNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_r.is_null");

  auto Ref = Builder.createBitCast(stackPop(), Context.Int64x2Ty);
  auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_r.ref_not_null");
  auto IsRefNotNull = Builder.createLikely(Builder.createICmpNE(
      Builder.createExtractElement(Ref, LLContext.getInt64(1)),
      LLContext.getInt64(0)));
  Builder.createCondBr(IsRefNotNull, OkBB,
                       getTrapBB(ErrCode::Value::AccessNullFunc));
  Builder.positionAtEnd(OkBB);

  const auto &FuncType = Context.CompositeTypes[TypeIndex]->getFuncType();
  auto FTy = toLLVMType(Context.LLContext, Context.ExecCtxPtrTy, FuncType);
  auto RTy = FTy.getReturnType();

  const size_t ArgSize = FuncType.getParamTypes().size();
  const size_t RetSize = RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();
  std::vector<LLVM::Value> ArgsVec(ArgSize + 1, nullptr);
  ArgsVec[0] = F.Fn.getFirstParam();
  for (size_t I = 0; I < ArgSize; ++I) {
    const size_t J = ArgSize - I;
    ArgsVec[J] = stackPop();
  }

  {
    auto FPtr = Builder.createCall(
        Context.getIntrinsic(Builder, Executable::Intrinsics::kRefGetFuncSymbol,
                             LLVM::Type::getFunctionType(FTy.getPointerTo(),
                                                         {Context.Int64x2Ty},
                                                         false)),
        {Ref});
    Builder.createCondBr(
        Builder.createLikely(Builder.createNot(Builder.createIsNull(FPtr))),
        NotNullBB, IsNullBB);
    Builder.positionAtEnd(NotNullBB);

    auto FPtrRet = Builder.createCall(LLVM::FunctionCallee(FTy, FPtr), ArgsVec);
    FPtrRet.setMustTailCall();
    if (RetSize == 0) {
      Builder.createRetVoid();
    } else {
      Builder.createRet(FPtrRet);
    }
  }

  Builder.positionAtEnd(IsNullBB);

  {
    LLVM::Value Args = Builder.createArray(ArgSize, LLVM::kValSize);
    LLVM::Value Rets = Builder.createArray(RetSize, LLVM::kValSize);
    Builder.createArrayPtrStore(Span<LLVM::Value>(ArgsVec.begin() + 1, ArgSize),
                                Args, Context.Int8Ty, LLVM::kValSize);

    Builder.createCall(
        Context.getIntrinsic(
            Builder, Executable::Intrinsics::kCallRef,
            LLVM::Type::getFunctionType(
                Context.VoidTy,
                {Context.Int64x2Ty, Context.Int8PtrTy, Context.Int8PtrTy},
                false)),
        {Ref, Args, Rets});

    if (RetSize == 0) {
      Builder.createRetVoid();
    } else if (RetSize == 1) {
      Builder.createRet(Builder.createValuePtrLoad(RTy, Rets, Context.Int8Ty));
    } else {
      Builder.createAggregateRet(Builder.createArrayPtrLoad(
          RetSize, RTy, Rets, Context.Int8Ty, LLVM::kValSize));
    }
  }
}

void FunctionCompiler::compileVectorLoadOp(unsigned MemoryIndex,
                                           uint64_t Offset, unsigned Alignment,
                                           LLVM::Type LoadTy) noexcept {
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
  Stack.back() = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
}

void FunctionCompiler::compileVectorLoadOp(unsigned MemoryIndex,
                                           uint64_t Offset, unsigned Alignment,
                                           LLVM::Type LoadTy,
                                           LLVM::Type ExtendTy,
                                           bool Signed) noexcept {
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy, ExtendTy, Signed);
  Stack.back() = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
}

void FunctionCompiler::compileSplatLoadOp(unsigned MemoryIndex, uint64_t Offset,
                                          unsigned Alignment, LLVM::Type LoadTy,
                                          LLVM::Type VectorTy) noexcept {
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
  compileSplatOp(VectorTy);
}

void FunctionCompiler::compileLoadLaneOp(unsigned MemoryIndex, uint64_t Offset,
                                         unsigned Alignment, unsigned Index,
                                         LLVM::Type LoadTy,
                                         LLVM::Type VectorTy) noexcept {
  auto Vector = stackPop();
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
  if constexpr (Endian::native == Endian::big) {
    Index = VectorTy.getVectorSize() - 1 - Index;
  }
  auto Value = Stack.back();
  Stack.back() = Builder.createBitCast(
      Builder.createInsertElement(Builder.createBitCast(Vector, VectorTy),
                                  Value, LLContext.getInt64(Index)),
      Context.Int64x2Ty);
}

void FunctionCompiler::compileStoreLaneOp(uint32_t MemoryIndex, uint64_t Offset,
                                          uint32_t Alignment, uint8_t Index,
                                          LLVM::Type LoadTy,
                                          LLVM::Type VectorTy) noexcept {
  auto Vector = Stack.back();
  if constexpr (Endian::native == Endian::big) {
    Index = static_cast<uint8_t>(VectorTy.getVectorSize() - Index - 1);
  }
  Stack.back() = Builder.createExtractElement(
      Builder.createBitCast(Vector, VectorTy), LLContext.getInt64(Index));
  compileStoreOp(MemoryIndex, Offset, Alignment, LoadTy);
}

void FunctionCompiler::compileSplatOp(LLVM::Type VectorTy) noexcept {
  auto Undef = LLVM::Value::getUndef(VectorTy);
  auto Zeros = LLVM::Value::getConstNull(
      LLVM::Type::getVectorType(Context.Int32Ty, VectorTy.getVectorSize()));
  auto Value = Builder.createTrunc(Stack.back(), VectorTy.getElementType());
  auto Vector =
      Builder.createInsertElement(Undef, Value, LLContext.getInt64(0));
  Vector = Builder.createShuffleVector(Vector, Undef, Zeros);

  Stack.back() = Builder.createBitCast(Vector, Context.Int64x2Ty);
}

void FunctionCompiler::compileExtractLaneOp(LLVM::Type VectorTy,
                                            unsigned Index) noexcept {
  auto Vector = Builder.createBitCast(Stack.back(), VectorTy);
  if constexpr (Endian::native == Endian::big) {
    Index = VectorTy.getVectorSize() - Index - 1;
  }
  Stack.back() =
      Builder.createExtractElement(Vector, LLContext.getInt64(Index));
}

void FunctionCompiler::compileExtractLaneOp(LLVM::Type VectorTy, unsigned Index,
                                            LLVM::Type ExtendTy,
                                            bool Signed) noexcept {
  compileExtractLaneOp(VectorTy, Index);
  if (Signed) {
    Stack.back() = Builder.createSExt(Stack.back(), ExtendTy);
  } else {
    Stack.back() = Builder.createZExt(Stack.back(), ExtendTy);
  }
}

void FunctionCompiler::compileReplaceLaneOp(LLVM::Type VectorTy,
                                            unsigned Index) noexcept {
  auto Value = Builder.createTrunc(stackPop(), VectorTy.getElementType());
  auto Vector = Stack.back();
  if constexpr (Endian::native == Endian::big) {
    Index = VectorTy.getVectorSize() - Index - 1;
  }
  Stack.back() = Builder.createBitCast(
      Builder.createInsertElement(Builder.createBitCast(Vector, VectorTy),
                                  Value, LLContext.getInt64(Index)),
      Context.Int64x2Ty);
}

void FunctionCompiler::compileVectorCompareOp(
    LLVM::Type VectorTy, LLVMIntPredicate Predicate) noexcept {
  auto RHS = stackPop();
  auto LHS = stackPop();
  auto Result = Builder.createSExt(
      Builder.createICmp(Predicate, Builder.createBitCast(LHS, VectorTy),
                         Builder.createBitCast(RHS, VectorTy)),
      VectorTy);
  stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorCompareOp(LLVM::Type VectorTy,
                                              LLVMRealPredicate Predicate,
                                              LLVM::Type ResultTy) noexcept {
  auto RHS = stackPop();
  auto LHS = stackPop();
  auto Result = Builder.createSExt(
      Builder.createFCmp(Predicate, Builder.createBitCast(LHS, VectorTy),
                         Builder.createBitCast(RHS, VectorTy)),
      ResultTy);
  stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorAbs(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    return Builder.createIntrinsic(LLVM::Core::Abs, {V.getType()},
                                   {V, LLContext.getFalse()});
  });
}

void FunctionCompiler::compileVectorNeg(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy,
                  [this](auto V) noexcept { return Builder.createNeg(V); });
}

void FunctionCompiler::compileVectorPopcnt() noexcept {
  compileVectorOp(Context.Int8x16Ty, [this](auto V) noexcept {
    assuming(LLVM::Core::Ctpop != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Ctpop, V);
  });
}

void FunctionCompiler::compileVectorAnyTrue() noexcept {
  compileVectorReduceIOp(Context.Int128x1Ty, [this](auto V) noexcept {
    auto Zero = LLVM::Value::getConstNull(Context.Int128x1Ty);
    return Builder.createBitCast(Builder.createICmpNE(V, Zero),
                                 LLContext.getInt1Ty());
  });
}

void FunctionCompiler::compileVectorAllTrue(LLVM::Type VectorTy) noexcept {
  compileVectorReduceIOp(VectorTy, [this, VectorTy](auto V) noexcept {
    const auto Size = VectorTy.getVectorSize();
    auto IntType = LLContext.getIntNTy(Size);
    auto Zero = LLVM::Value::getConstNull(VectorTy);
    auto Cmp = Builder.createBitCast(Builder.createICmpEQ(V, Zero), IntType);
    auto CmpZero = LLVM::Value::getConstInt(IntType, 0);
    return Builder.createICmpEQ(Cmp, CmpZero);
  });
}

void FunctionCompiler::compileVectorBitMask(LLVM::Type VectorTy) noexcept {
  compileVectorReduceIOp(VectorTy, [this, VectorTy](auto V) noexcept {
    const auto Size = VectorTy.getVectorSize();
    auto IntType = LLContext.getIntNTy(Size);
    auto Zero = LLVM::Value::getConstNull(VectorTy);
    return Builder.createBitCast(Builder.createICmpSLT(V, Zero), IntType);
  });
}

void FunctionCompiler::compileVectorShl(LLVM::Type VectorTy) noexcept {
  compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createShl(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorLShr(LLVM::Type VectorTy) noexcept {
  compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createLShr(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorAShr(LLVM::Type VectorTy) noexcept {
  compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createAShr(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorAdd(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createAdd(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorAddSat(LLVM::Type VectorTy,
                                                 bool Signed) noexcept {
  auto ID = Signed ? LLVM::Core::SAddSat : LLVM::Core::UAddSat;
  assuming(ID != LLVM::Core::NotIntrinsic);
  compileVectorVectorOp(
      VectorTy, [this, VectorTy, ID](auto LHS, auto RHS) noexcept {
        return Builder.createIntrinsic(ID, {VectorTy}, {LHS, RHS});
      });
}

void FunctionCompiler::compileVectorVectorSub(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createSub(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorSubSat(LLVM::Type VectorTy,
                                                 bool Signed) noexcept {
  auto ID = Signed ? LLVM::Core::SSubSat : LLVM::Core::USubSat;
  assuming(ID != LLVM::Core::NotIntrinsic);
  compileVectorVectorOp(
      VectorTy, [this, VectorTy, ID](auto LHS, auto RHS) noexcept {
        return Builder.createIntrinsic(ID, {VectorTy}, {LHS, RHS});
      });
}

void FunctionCompiler::compileVectorVectorMul(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createMul(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorSwizzle() noexcept {
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
    return;
  }
#endif

#if defined(__aarch64__)
  if (Context.SupportNEON) {
    assuming(LLVM::Core::AArch64NeonTbl1 != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createBitCast(
        Builder.createIntrinsic(LLVM::Core::AArch64NeonTbl1,
                                {Context.Int8x16Ty}, {Vector, Index}),
        Context.Int64x2Ty));
    return;
  }
#endif

  auto Mask = Builder.createVectorSplat(16, LLContext.getInt8(15));
  auto Zero = Builder.createVectorSplat(16, LLContext.getInt8(0));

#if defined(__s390x__)
  assuming(LLVM::Core::S390VPerm != LLVM::Core::NotIntrinsic);
  auto Exceed = Builder.createICmpULE(Index, Mask);
  Index = Builder.createSub(Mask, Index);
  auto Result =
      Builder.createIntrinsic(LLVM::Core::S390VPerm, {}, {Vector, Zero, Index});
  Result = Builder.createSelect(Exceed, Result, Zero);
  stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
  return;
#endif

  // Fallback case.
  // If the SSSE3 is not supported on the x86_64 platform or
  // the NEON is not supported on the aarch64 platform,
  // then fallback to this.
  auto IsOver = Builder.createICmpUGT(Index, Mask);
  auto InboundIndex = Builder.createAnd(Index, Mask);
  auto Array = Builder.createArray(16, 1);
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
        Context.Int8Ty, Builder.createInBoundsGEP1(Context.Int8Ty, Array, Idx));
    Ret = Builder.createInsertElement(Ret, Value, LLContext.getInt64(I));
  }
  Ret = Builder.createSelect(IsOver, Zero, Ret);
  stackPush(Builder.createBitCast(Ret, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorVectorQ15MulSat() noexcept {
  compileVectorVectorOp(
      Context.Int16x8Ty, [this](auto LHS, auto RHS) noexcept -> LLVM::Value {
#if defined(__x86_64__)
        if (Context.SupportSSSE3) {
          assuming(LLVM::Core::X86SSSE3PMulHrSw128 != LLVM::Core::NotIntrinsic);
          auto Result = Builder.createIntrinsic(LLVM::Core::X86SSSE3PMulHrSw128,
                                                {}, {LHS, RHS});
          auto IntMaxV = Builder.createVectorSplat(
              8, LLContext.getInt16(UINT16_C(0x8000)));
          auto NotOver = Builder.createSExt(
              Builder.createICmpEQ(Result, IntMaxV), Context.Int16x8Ty);
          return Builder.createXor(Result, NotOver);
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON) {
          assuming(LLVM::Core::AArch64NeonSQRDMulH != LLVM::Core::NotIntrinsic);
          return Builder.createBinaryIntrinsic(LLVM::Core::AArch64NeonSQRDMulH,
                                               LHS, RHS);
        }
#endif

        // Fallback case.
        // If the SSSE3 is not supported on the x86_64 platform or
        // the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        auto ExtTy = Context.Int16x8Ty.getExtendedElementVectorType();
        auto Offset =
            Builder.createVectorSplat(8, LLContext.getInt32(UINT32_C(0x4000)));
        auto Shift =
            Builder.createVectorSplat(8, LLContext.getInt32(UINT32_C(15)));
        auto ExtLHS = Builder.createSExt(LHS, ExtTy);
        auto ExtRHS = Builder.createSExt(RHS, ExtTy);
        auto Result = Builder.createTrunc(
            Builder.createAShr(
                Builder.createAdd(Builder.createMul(ExtLHS, ExtRHS), Offset),
                Shift),
            Context.Int16x8Ty);
        auto IntMaxV =
            Builder.createVectorSplat(8, LLContext.getInt16(UINT16_C(0x8000)));
        auto NotOver = Builder.createSExt(Builder.createICmpEQ(Result, IntMaxV),
                                          Context.Int16x8Ty);
        return Builder.createXor(Result, NotOver);
      });
}

void FunctionCompiler::compileVectorVectorSMin(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::SMin, {LHS.getType()},
                                   {LHS, RHS});
  });
}

void FunctionCompiler::compileVectorVectorUMin(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::UMin, {LHS.getType()},
                                   {LHS, RHS});
  });
}

void FunctionCompiler::compileVectorVectorSMax(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::SMax, {LHS.getType()},
                                   {LHS, RHS});
  });
}

void FunctionCompiler::compileVectorVectorUMax(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::UMax, {LHS.getType()},
                                   {LHS, RHS});
  });
}

void FunctionCompiler::compileVectorVectorUAvgr(LLVM::Type VectorTy) noexcept {
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

void FunctionCompiler::compileVectorNarrow(LLVM::Type FromTy,
                                           bool Signed) noexcept {
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
  auto V = Endian::native == Endian::little
               ? Builder.createShuffleVector(
                     F1, F2, LLVM::Value::getConstVector32(LLContext, Mask))
               : Builder.createShuffleVector(
                     F2, F1, LLVM::Value::getConstVector32(LLContext, Mask));
  stackPush(Builder.createBitCast(V, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorExtend(LLVM::Type FromTy, bool Signed,
                                           bool Low) noexcept {
  auto ExtTy = FromTy.getExtendedElementVectorType();
  const auto Count = FromTy.getVectorSize();
  std::vector<uint32_t> Mask(Count / 2);
  if constexpr (Endian::native == Endian::big) {
    Low = !Low;
  }
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

void FunctionCompiler::compileVectorExtMul(LLVM::Type FromTy, bool Signed,
                                           bool Low) noexcept {
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

void FunctionCompiler::compileVectorExtAddPairwise(LLVM::Type VectorTy,
                                                   bool Signed) noexcept {
  compileVectorOp(
      VectorTy, [this, VectorTy, Signed](auto V) noexcept -> LLVM::Value {
        auto ExtTy =
            VectorTy.getExtendedElementVectorType().getHalfElementsVectorType();
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
        auto Width = LLVM::Value::getConstInt(
            ExtTy.getElementType(),
            VectorTy.getElementType().getIntegerBitWidth());
        Width = Builder.createVectorSplat(ExtTy.getVectorSize(), Width);
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

void FunctionCompiler::compileVectorFAbs(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Fabs != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Fabs, V);
  });
}

void FunctionCompiler::compileVectorFNeg(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy,
                  [this](auto V) noexcept { return Builder.createFNeg(V); });
}

void FunctionCompiler::compileVectorFSqrt(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Sqrt != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Sqrt, V);
  });
}

void FunctionCompiler::compileVectorFCeil(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Ceil != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Ceil, V);
  });
}

void FunctionCompiler::compileVectorFFloor(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Floor != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Floor, V);
  });
}

void FunctionCompiler::compileVectorFTrunc(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Trunc, V);
  });
}

void FunctionCompiler::compileVectorFNearest(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [&](auto V) noexcept {
#if LLVM_VERSION_MAJOR >= 12 && !defined(__s390x__)
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

void FunctionCompiler::compileVectorVectorFAdd(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFAdd(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorFSub(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFSub(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorFMul(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFMul(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorFDiv(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFDiv(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorFMin(LLVM::Type VectorTy) noexcept {
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
    Ret = Builder.createSelect(LNaN, LHS, Ret);
    Ret = Builder.createSelect(RNaN, RHS, Ret);
    return Ret;
  });
}

void FunctionCompiler::compileVectorVectorFMax(LLVM::Type VectorTy) noexcept {
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
    Ret = Builder.createSelect(LNaN, LHS, Ret);
    Ret = Builder.createSelect(RNaN, RHS, Ret);
    return Ret;
  });
}

void FunctionCompiler::compileVectorVectorFPMin(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    auto Cmp = Builder.createFCmpOLT(RHS, LHS);
    return Builder.createSelect(Cmp, RHS, LHS);
  });
}

void FunctionCompiler::compileVectorVectorFPMax(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    auto Cmp = Builder.createFCmpOGT(RHS, LHS);
    return Builder.createSelect(Cmp, RHS, LHS);
  });
}

void FunctionCompiler::compileVectorTruncSatS32(LLVM::Type VectorTy,
                                                bool PadZero) noexcept {
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
      if constexpr (Endian::native == Endian::little) {
        V = Builder.createShuffleVector(
            V, IntZeroV, LLVM::Value::getConstVector32(LLContext, Mask));
      } else {
        V = Builder.createShuffleVector(
            IntZeroV, V, LLVM::Value::getConstVector32(LLContext, Mask));
      }
    }
    return V;
  });
}

void FunctionCompiler::compileVectorTruncSatU32(LLVM::Type VectorTy,
                                                bool PadZero) noexcept {
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
      if constexpr (Endian::native == Endian::little) {
        V = Builder.createShuffleVector(
            V, IntZeroV, LLVM::Value::getConstVector32(LLContext, Mask));
      } else {
        V = Builder.createShuffleVector(
            IntZeroV, V, LLVM::Value::getConstVector32(LLContext, Mask));
      }
    }
    return V;
  });
}

void FunctionCompiler::compileVectorConvertS(LLVM::Type VectorTy,
                                             LLVM::Type FPVectorTy,
                                             bool Low) noexcept {
  compileVectorOp(VectorTy, [this, VectorTy, FPVectorTy, Low](auto V) noexcept {
    if (Low) {
      const auto Size = VectorTy.getVectorSize() / 2;
      std::vector<uint32_t> Mask(Size);
      if constexpr (Endian::native == Endian::little) {
        std::iota(Mask.begin(), Mask.end(), 0);
      } else {
        std::iota(Mask.begin(), Mask.end(), Size);
      }
      V = Builder.createShuffleVector(
          V, LLVM::Value::getUndef(VectorTy),
          LLVM::Value::getConstVector32(LLContext, Mask));
    }
    return Builder.createSIToFP(V, FPVectorTy);
  });
}

void FunctionCompiler::compileVectorConvertU(LLVM::Type VectorTy,
                                             LLVM::Type FPVectorTy,
                                             bool Low) noexcept {
  compileVectorOp(VectorTy, [this, VectorTy, FPVectorTy, Low](auto V) noexcept {
    if (Low) {
      const auto Size = VectorTy.getVectorSize() / 2;
      std::vector<uint32_t> Mask(Size);
      if constexpr (Endian::native == Endian::little) {
        std::iota(Mask.begin(), Mask.end(), 0);
      } else {
        std::iota(Mask.begin(), Mask.end(), Size);
      }
      V = Builder.createShuffleVector(
          V, LLVM::Value::getUndef(VectorTy),
          LLVM::Value::getConstVector32(LLContext, Mask));
    }
    return Builder.createUIToFP(V, FPVectorTy);
  });
}

void FunctionCompiler::compileVectorDemote() noexcept {
  compileVectorOp(Context.Doublex2Ty, [this](auto V) noexcept {
    auto Demoted =
        Builder.createFPTrunc(V, LLVM::Type::getVectorType(Context.FloatTy, 2));
    auto ZeroV = LLVM::Value::getConstNull(Demoted.getType());
    if constexpr (Endian::native == Endian::little) {
      return Builder.createShuffleVector(
          Demoted, ZeroV,
          LLVM::Value::getConstVector32(LLContext, {0u, 1u, 2u, 3u}));
    } else {
      return Builder.createShuffleVector(
          Demoted, ZeroV,
          LLVM::Value::getConstVector32(LLContext, {3u, 2u, 1u, 0u}));
    }
  });
}

void FunctionCompiler::compileVectorPromote() noexcept {
  compileVectorOp(Context.Floatx4Ty, [this](auto V) noexcept {
    auto UndefV = LLVM::Value::getUndef(V.getType());
    auto Low = Builder.createShuffleVector(
        V, UndefV, LLVM::Value::getConstVector32(LLContext, {0u, 1u}));
    return Builder.createFPExt(Low,
                               LLVM::Type::getVectorType(Context.DoubleTy, 2));
  });
}

void FunctionCompiler::compileVectorVectorMAdd(LLVM::Type VectorTy) noexcept {
  auto C = Builder.createBitCast(stackPop(), VectorTy);
  auto RHS = Builder.createBitCast(stackPop(), VectorTy);
  auto LHS = Builder.createBitCast(stackPop(), VectorTy);
  stackPush(Builder.createBitCast(
      Builder.createFAdd(Builder.createFMul(LHS, RHS), C), Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorVectorNMAdd(LLVM::Type VectorTy) noexcept {
  auto C = Builder.createBitCast(stackPop(), VectorTy);
  auto RHS = Builder.createBitCast(stackPop(), VectorTy);
  auto LHS = Builder.createBitCast(stackPop(), VectorTy);
  stackPush(Builder.createBitCast(
      Builder.createFAdd(Builder.createFMul(Builder.createFNeg(LHS), RHS), C),
      Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorRelaxedIntegerDotProduct() noexcept {
  auto OriTy = Context.Int8x16Ty;
  auto ExtTy = Context.Int16x8Ty;
  auto RHS = Builder.createBitCast(stackPop(), OriTy);
  auto LHS = Builder.createBitCast(stackPop(), OriTy);
#if defined(__x86_64__)
  if (Context.SupportSSSE3) {
    assuming(LLVM::Core::X86SSSE3PMAddUbSw128 != LLVM::Core::NotIntrinsic);
    // WebAssembly Relaxed SIMD spec: signed(LHS) * unsigned/signed(RHS)
    // But PMAddUbSw128 is unsigned(LHS) * signed(RHS). Therefore swap both
    // side to match the WebAssembly spec
    return stackPush(Builder.createBitCast(
        Builder.createIntrinsic(LLVM::Core::X86SSSE3PMAddUbSw128, {},
                                {RHS, LHS}),
        Context.Int64x2Ty));
  }
#endif
  auto Width = LLVM::Value::getConstInt(
      ExtTy.getElementType(), OriTy.getElementType().getIntegerBitWidth());
  Width = Builder.createVectorSplat(ExtTy.getVectorSize(), Width);
  auto EA = Builder.createBitCast(LHS, ExtTy);
  auto EB = Builder.createBitCast(RHS, ExtTy);

  LLVM::Value AL, AR, BL, BR;
  AL = Builder.createAShr(EA, Width);
  AR = Builder.createAShr(Builder.createShl(EA, Width), Width);
  BL = Builder.createAShr(EB, Width);
  BR = Builder.createAShr(Builder.createShl(EB, Width), Width);

  return stackPush(Builder.createBitCast(
      Builder.createAdd(Builder.createMul(AL, BL), Builder.createMul(AR, BR)),
      Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorRelaxedIntegerDotProductAdd() noexcept {
  auto OriTy = Context.Int8x16Ty;
  auto ExtTy = Context.Int16x8Ty;
  auto FinTy = Context.Int32x4Ty;
  auto VC = Builder.createBitCast(stackPop(), FinTy);
  auto RHS = Builder.createBitCast(stackPop(), OriTy);
  auto LHS = Builder.createBitCast(stackPop(), OriTy);
  LLVM::Value IM;
#if defined(__x86_64__)
  if (Context.SupportSSSE3) {
    assuming(LLVM::Core::X86SSSE3PMAddUbSw128 != LLVM::Core::NotIntrinsic);
    // WebAssembly Relaxed SIMD spec: signed(LHS) * unsigned/signed(RHS)
    // But PMAddUbSw128 is unsigned(LHS) * signed(RHS). Therefore swap both
    // side to match the WebAssembly spec
    IM = Builder.createIntrinsic(LLVM::Core::X86SSSE3PMAddUbSw128, {},
                                 {RHS, LHS});
  } else
#endif
  {
    auto Width = LLVM::Value::getConstInt(
        ExtTy.getElementType(), OriTy.getElementType().getIntegerBitWidth());
    Width = Builder.createVectorSplat(ExtTy.getVectorSize(), Width);
    auto EA = Builder.createBitCast(LHS, ExtTy);
    auto EB = Builder.createBitCast(RHS, ExtTy);

    LLVM::Value AL, AR, BL, BR;
    AL = Builder.createAShr(EA, Width);
    AR = Builder.createAShr(Builder.createShl(EA, Width), Width);
    BL = Builder.createAShr(EB, Width);
    BR = Builder.createAShr(Builder.createShl(EB, Width), Width);
    IM =
        Builder.createAdd(Builder.createMul(AL, BL), Builder.createMul(AR, BR));
  }

  auto Width = LLVM::Value::getConstInt(
      FinTy.getElementType(), ExtTy.getElementType().getIntegerBitWidth());
  Width = Builder.createVectorSplat(FinTy.getVectorSize(), Width);
  auto IME = Builder.createBitCast(IM, FinTy);
  auto L = Builder.createAShr(IME, Width);
  auto R = Builder.createAShr(Builder.createShl(IME, Width), Width);

  return stackPush(Builder.createBitCast(
      Builder.createAdd(Builder.createAdd(L, R), VC), Context.Int64x2Ty));
}

void FunctionCompiler::enterBlock(
    LLVM::BasicBlock JumpBlock, LLVM::BasicBlock NextBlock,
    LLVM::BasicBlock ElseBlock, std::vector<LLVM::Value> Args,
    std::pair<std::vector<ValType>, std::vector<ValType>> Type,
    std::vector<std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
        ReturnPHI) noexcept {
  assuming(Type.first.size() == Args.size());
  for (auto &Value : Args) {
    stackPush(Value);
  }
  const auto Unreachable = isUnreachable();
  ControlStack.emplace_back(Stack.size() - Args.size(), Unreachable, JumpBlock,
                            NextBlock, ElseBlock, std::move(Args),
                            std::move(Type), std::move(ReturnPHI));
}

FunctionCompiler::Control FunctionCompiler::leaveBlock() noexcept {
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

void FunctionCompiler::checkStop() noexcept {
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

void FunctionCompiler::setUnreachable() noexcept {
  if (ControlStack.empty()) {
    IsUnreachable = true;
  } else {
    ControlStack.back().Unreachable = true;
  }
}

bool FunctionCompiler::isUnreachable() const noexcept {
  if (ControlStack.empty()) {
    return IsUnreachable;
  } else {
    return ControlStack.back().Unreachable;
  }
}

void FunctionCompiler::buildPHI(
    Span<const ValType> RetType,
    Span<const std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
        Incomings) noexcept {
  if (LLVM::isVoidReturn(RetType)) {
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

void FunctionCompiler::setLableJumpPHI(unsigned int Index) noexcept {
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

LLVM::BasicBlock FunctionCompiler::getLabel(unsigned int Index) const noexcept {
  return (ControlStack.rbegin() + Index)->JumpBlock;
}

LLVM::Value FunctionCompiler::stackPop() noexcept {
  assuming(!ControlStack.empty() || !Stack.empty());
  assuming(ControlStack.empty() ||
           Stack.size() > ControlStack.back().StackSize);
  auto Value = Stack.back();
  Stack.pop_back();
  return Value;
}

LLVM::Value FunctionCompiler::switchEndian(LLVM::Value Value) {
  if constexpr (Endian::native == Endian::big) {
    auto Type = Value.getType();
    if ((Type.isIntegerTy() && Type.getIntegerBitWidth() > 8) ||
        (Type.isVectorTy() && Type.getVectorSize() == 1)) {
      return Builder.createUnaryIntrinsic(LLVM::Core::Bswap, Value);
    }
    if (Type.isVectorTy()) {
      LLVM::Type VecType = Type.getElementType().getIntegerBitWidth() == 128
                               ? Context.Int128Ty
                               : Context.Int64Ty;
      Value = Builder.createBitCast(Value, VecType);
      Value = Builder.createUnaryIntrinsic(LLVM::Core::Bswap, Value);
      return Builder.createBitCast(Value, Type);
    }
    if (Type.isFloatTy() || Type.isDoubleTy()) {
      LLVM::Type IntType = Type.isFloatTy() ? Context.Int32Ty : Context.Int64Ty;
      Value = Builder.createBitCast(Value, IntType);
      Value = Builder.createUnaryIntrinsic(LLVM::Core::Bswap, Value);
      return Builder.createBitCast(Value, Type);
    }
  }
  return Value;
}

} // namespace WasmEdge
