// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/function_compiler.h"

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
      auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
      stackPush(Builder.createCall(
          Context.getIntrinsic(Builder, Executable::Intrinsics::kTableGet,
                               LLVM::Type::getFunctionType(
                                   Context.Int64x2Ty,
                                   {Context.Int32Ty, Context.Int64Ty}, false)),
          {LLContext.getInt32(Instr.getTargetIndex()), Off}));
      break;
    }
    case OpCode::Table__set: {
      auto Ref = stackPop();
      auto Off = Builder.createZExt(stackPop(), Context.Int64Ty);
      Builder.createCall(
          Context.getIntrinsic(
              Builder, Executable::Intrinsics::kTableSet,
              LLVM::Type::getFunctionType(
                  Context.Int64Ty,
                  {Context.Int32Ty, Context.Int64Ty, Context.Int64x2Ty},
                  false)),
          {LLContext.getInt32(Instr.getTargetIndex()), Off, Ref});
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
          Builder.createCall(
              Context.getIntrinsic(
                  Builder, Executable::Intrinsics::kTableSize,
                  LLVM::Type::getFunctionType(Context.Int64Ty,
                                              {Context.Int32Ty}, false)),
              {LLContext.getInt32(Instr.getTargetIndex())}),
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
    case OpCode::V128__load8x8_s:
    case OpCode::V128__load8x8_u:
    case OpCode::V128__load16x4_s:
    case OpCode::V128__load16x4_u:
    case OpCode::V128__load32x2_s:
    case OpCode::V128__load32x2_u:
    case OpCode::V128__load8_splat:
    case OpCode::V128__load16_splat:
    case OpCode::V128__load32_splat:
    case OpCode::V128__load64_splat:
    case OpCode::V128__load32_zero:
    case OpCode::V128__load64_zero:
    case OpCode::V128__store:
    case OpCode::V128__load8_lane:
    case OpCode::V128__load16_lane:
    case OpCode::V128__load32_lane:
    case OpCode::V128__load64_lane:
    case OpCode::V128__store8_lane:
    case OpCode::V128__store16_lane:
    case OpCode::V128__store32_lane:
    case OpCode::V128__store64_lane:
    case OpCode::V128__const:
    case OpCode::I8x16__shuffle:
    case OpCode::I8x16__extract_lane_s:
    case OpCode::I8x16__extract_lane_u:
    case OpCode::I8x16__replace_lane:
    case OpCode::I16x8__extract_lane_s:
    case OpCode::I16x8__extract_lane_u:
    case OpCode::I16x8__replace_lane:
    case OpCode::I32x4__extract_lane:
    case OpCode::I32x4__replace_lane:
    case OpCode::I64x2__extract_lane:
    case OpCode::I64x2__replace_lane:
    case OpCode::F32x4__extract_lane:
    case OpCode::F32x4__replace_lane:
    case OpCode::F64x2__extract_lane:
    case OpCode::F64x2__replace_lane:
    case OpCode::I8x16__swizzle:
    case OpCode::I8x16__splat:
    case OpCode::I16x8__splat:
    case OpCode::I32x4__splat:
    case OpCode::I64x2__splat:
    case OpCode::F32x4__splat:
    case OpCode::F64x2__splat:
    case OpCode::I8x16__eq:
    case OpCode::I8x16__ne:
    case OpCode::I8x16__lt_s:
    case OpCode::I8x16__lt_u:
    case OpCode::I8x16__gt_s:
    case OpCode::I8x16__gt_u:
    case OpCode::I8x16__le_s:
    case OpCode::I8x16__le_u:
    case OpCode::I8x16__ge_s:
    case OpCode::I8x16__ge_u:
    case OpCode::I16x8__eq:
    case OpCode::I16x8__ne:
    case OpCode::I16x8__lt_s:
    case OpCode::I16x8__lt_u:
    case OpCode::I16x8__gt_s:
    case OpCode::I16x8__gt_u:
    case OpCode::I16x8__le_s:
    case OpCode::I16x8__le_u:
    case OpCode::I16x8__ge_s:
    case OpCode::I16x8__ge_u:
    case OpCode::I32x4__eq:
    case OpCode::I32x4__ne:
    case OpCode::I32x4__lt_s:
    case OpCode::I32x4__lt_u:
    case OpCode::I32x4__gt_s:
    case OpCode::I32x4__gt_u:
    case OpCode::I32x4__le_s:
    case OpCode::I32x4__le_u:
    case OpCode::I32x4__ge_s:
    case OpCode::I32x4__ge_u:
    case OpCode::I64x2__eq:
    case OpCode::I64x2__ne:
    case OpCode::I64x2__lt_s:
    case OpCode::I64x2__gt_s:
    case OpCode::I64x2__le_s:
    case OpCode::I64x2__ge_s:
    case OpCode::F32x4__eq:
    case OpCode::F32x4__ne:
    case OpCode::F32x4__lt:
    case OpCode::F32x4__gt:
    case OpCode::F32x4__le:
    case OpCode::F32x4__ge:
    case OpCode::F64x2__eq:
    case OpCode::F64x2__ne:
    case OpCode::F64x2__lt:
    case OpCode::F64x2__gt:
    case OpCode::F64x2__le:
    case OpCode::F64x2__ge:
    case OpCode::V128__not:
    case OpCode::V128__and:
    case OpCode::V128__andnot:
    case OpCode::V128__or:
    case OpCode::V128__xor:
    case OpCode::V128__bitselect:
    case OpCode::V128__any_true:
    case OpCode::I8x16__abs:
    case OpCode::I8x16__neg:
    case OpCode::I8x16__popcnt:
    case OpCode::I8x16__all_true:
    case OpCode::I8x16__bitmask:
    case OpCode::I8x16__narrow_i16x8_s:
    case OpCode::I8x16__narrow_i16x8_u:
    case OpCode::I8x16__shl:
    case OpCode::I8x16__shr_s:
    case OpCode::I8x16__shr_u:
    case OpCode::I8x16__add:
    case OpCode::I8x16__add_sat_s:
    case OpCode::I8x16__add_sat_u:
    case OpCode::I8x16__sub:
    case OpCode::I8x16__sub_sat_s:
    case OpCode::I8x16__sub_sat_u:
    case OpCode::I8x16__min_s:
    case OpCode::I8x16__min_u:
    case OpCode::I8x16__max_s:
    case OpCode::I8x16__max_u:
    case OpCode::I8x16__avgr_u:
    case OpCode::I16x8__abs:
    case OpCode::I16x8__neg:
    case OpCode::I16x8__all_true:
    case OpCode::I16x8__bitmask:
    case OpCode::I16x8__narrow_i32x4_s:
    case OpCode::I16x8__narrow_i32x4_u:
    case OpCode::I16x8__extend_low_i8x16_s:
    case OpCode::I16x8__extend_high_i8x16_s:
    case OpCode::I16x8__extend_low_i8x16_u:
    case OpCode::I16x8__extend_high_i8x16_u:
    case OpCode::I16x8__shl:
    case OpCode::I16x8__shr_s:
    case OpCode::I16x8__shr_u:
    case OpCode::I16x8__add:
    case OpCode::I16x8__add_sat_s:
    case OpCode::I16x8__add_sat_u:
    case OpCode::I16x8__sub:
    case OpCode::I16x8__sub_sat_s:
    case OpCode::I16x8__sub_sat_u:
    case OpCode::I16x8__mul:
    case OpCode::I16x8__min_s:
    case OpCode::I16x8__min_u:
    case OpCode::I16x8__max_s:
    case OpCode::I16x8__max_u:
    case OpCode::I16x8__avgr_u:
    case OpCode::I16x8__extmul_low_i8x16_s:
    case OpCode::I16x8__extmul_high_i8x16_s:
    case OpCode::I16x8__extmul_low_i8x16_u:
    case OpCode::I16x8__extmul_high_i8x16_u:
    case OpCode::I16x8__q15mulr_sat_s:
    case OpCode::I16x8__extadd_pairwise_i8x16_s:
    case OpCode::I16x8__extadd_pairwise_i8x16_u:
    case OpCode::I32x4__abs:
    case OpCode::I32x4__neg:
    case OpCode::I32x4__all_true:
    case OpCode::I32x4__bitmask:
    case OpCode::I32x4__extend_low_i16x8_s:
    case OpCode::I32x4__extend_high_i16x8_s:
    case OpCode::I32x4__extend_low_i16x8_u:
    case OpCode::I32x4__extend_high_i16x8_u:
    case OpCode::I32x4__shl:
    case OpCode::I32x4__shr_s:
    case OpCode::I32x4__shr_u:
    case OpCode::I32x4__add:
    case OpCode::I32x4__sub:
    case OpCode::I32x4__mul:
    case OpCode::I32x4__min_s:
    case OpCode::I32x4__min_u:
    case OpCode::I32x4__max_s:
    case OpCode::I32x4__max_u:
    case OpCode::I32x4__extmul_low_i16x8_s:
    case OpCode::I32x4__extmul_high_i16x8_s:
    case OpCode::I32x4__extmul_low_i16x8_u:
    case OpCode::I32x4__extmul_high_i16x8_u:
    case OpCode::I32x4__extadd_pairwise_i16x8_s:
    case OpCode::I32x4__extadd_pairwise_i16x8_u:
    case OpCode::I32x4__dot_i16x8_s:
    case OpCode::I64x2__abs:
    case OpCode::I64x2__neg:
    case OpCode::I64x2__all_true:
    case OpCode::I64x2__bitmask:
    case OpCode::I64x2__extend_low_i32x4_s:
    case OpCode::I64x2__extend_high_i32x4_s:
    case OpCode::I64x2__extend_low_i32x4_u:
    case OpCode::I64x2__extend_high_i32x4_u:
    case OpCode::I64x2__shl:
    case OpCode::I64x2__shr_s:
    case OpCode::I64x2__shr_u:
    case OpCode::I64x2__add:
    case OpCode::I64x2__sub:
    case OpCode::I64x2__mul:
    case OpCode::I64x2__extmul_low_i32x4_s:
    case OpCode::I64x2__extmul_high_i32x4_s:
    case OpCode::I64x2__extmul_low_i32x4_u:
    case OpCode::I64x2__extmul_high_i32x4_u:
    case OpCode::F32x4__abs:
    case OpCode::F32x4__neg:
    case OpCode::F32x4__sqrt:
    case OpCode::F32x4__add:
    case OpCode::F32x4__sub:
    case OpCode::F32x4__mul:
    case OpCode::F32x4__div:
    case OpCode::F32x4__min:
    case OpCode::F32x4__max:
    case OpCode::F32x4__pmin:
    case OpCode::F32x4__pmax:
    case OpCode::F32x4__ceil:
    case OpCode::F32x4__floor:
    case OpCode::F32x4__trunc:
    case OpCode::F32x4__nearest:
    case OpCode::F64x2__abs:
    case OpCode::F64x2__neg:
    case OpCode::F64x2__sqrt:
    case OpCode::F64x2__add:
    case OpCode::F64x2__sub:
    case OpCode::F64x2__mul:
    case OpCode::F64x2__div:
    case OpCode::F64x2__min:
    case OpCode::F64x2__max:
    case OpCode::F64x2__pmin:
    case OpCode::F64x2__pmax:
    case OpCode::F64x2__ceil:
    case OpCode::F64x2__floor:
    case OpCode::F64x2__trunc:
    case OpCode::F64x2__nearest:
    case OpCode::I32x4__trunc_sat_f32x4_s:
    case OpCode::I32x4__trunc_sat_f32x4_u:
    case OpCode::F32x4__convert_i32x4_s:
    case OpCode::F32x4__convert_i32x4_u:
    case OpCode::I32x4__trunc_sat_f64x2_s_zero:
    case OpCode::I32x4__trunc_sat_f64x2_u_zero:
    case OpCode::F64x2__convert_low_i32x4_s:
    case OpCode::F64x2__convert_low_i32x4_u:
    case OpCode::F32x4__demote_f64x2_zero:
    case OpCode::F64x2__promote_low_f32x4:
    case OpCode::I8x16__relaxed_swizzle:
    case OpCode::I32x4__relaxed_trunc_f32x4_s:
    case OpCode::I32x4__relaxed_trunc_f32x4_u:
    case OpCode::I32x4__relaxed_trunc_f64x2_s_zero:
    case OpCode::I32x4__relaxed_trunc_f64x2_u_zero:
    case OpCode::F32x4__relaxed_madd:
    case OpCode::F32x4__relaxed_nmadd:
    case OpCode::F64x2__relaxed_madd:
    case OpCode::F64x2__relaxed_nmadd:
    case OpCode::I8x16__relaxed_laneselect:
    case OpCode::I16x8__relaxed_laneselect:
    case OpCode::I32x4__relaxed_laneselect:
    case OpCode::I64x2__relaxed_laneselect:
    case OpCode::F32x4__relaxed_min:
    case OpCode::F32x4__relaxed_max:
    case OpCode::F64x2__relaxed_min:
    case OpCode::F64x2__relaxed_max:
    case OpCode::I16x8__relaxed_q15mulr_s:
    case OpCode::I16x8__relaxed_dot_i8x16_i7x16_s:
    case OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s:
      return compileVectorOp(Instr);
    case OpCode::Atomic__fence:
    case OpCode::Memory__atomic__notify:
    case OpCode::Memory__atomic__wait32:
    case OpCode::Memory__atomic__wait64:
    case OpCode::I32__atomic__load:
    case OpCode::I64__atomic__load:
    case OpCode::I32__atomic__load8_u:
    case OpCode::I32__atomic__load16_u:
    case OpCode::I64__atomic__load8_u:
    case OpCode::I64__atomic__load16_u:
    case OpCode::I64__atomic__load32_u:
    case OpCode::I32__atomic__store:
    case OpCode::I64__atomic__store:
    case OpCode::I32__atomic__store8:
    case OpCode::I32__atomic__store16:
    case OpCode::I64__atomic__store8:
    case OpCode::I64__atomic__store16:
    case OpCode::I64__atomic__store32:
    case OpCode::I32__atomic__rmw__add:
    case OpCode::I64__atomic__rmw__add:
    case OpCode::I32__atomic__rmw8__add_u:
    case OpCode::I32__atomic__rmw16__add_u:
    case OpCode::I64__atomic__rmw8__add_u:
    case OpCode::I64__atomic__rmw16__add_u:
    case OpCode::I64__atomic__rmw32__add_u:
    case OpCode::I32__atomic__rmw__sub:
    case OpCode::I64__atomic__rmw__sub:
    case OpCode::I32__atomic__rmw8__sub_u:
    case OpCode::I32__atomic__rmw16__sub_u:
    case OpCode::I64__atomic__rmw8__sub_u:
    case OpCode::I64__atomic__rmw16__sub_u:
    case OpCode::I64__atomic__rmw32__sub_u:
    case OpCode::I32__atomic__rmw__and:
    case OpCode::I64__atomic__rmw__and:
    case OpCode::I32__atomic__rmw8__and_u:
    case OpCode::I32__atomic__rmw16__and_u:
    case OpCode::I64__atomic__rmw8__and_u:
    case OpCode::I64__atomic__rmw16__and_u:
    case OpCode::I64__atomic__rmw32__and_u:
    case OpCode::I32__atomic__rmw__or:
    case OpCode::I64__atomic__rmw__or:
    case OpCode::I32__atomic__rmw8__or_u:
    case OpCode::I32__atomic__rmw16__or_u:
    case OpCode::I64__atomic__rmw8__or_u:
    case OpCode::I64__atomic__rmw16__or_u:
    case OpCode::I64__atomic__rmw32__or_u:
    case OpCode::I32__atomic__rmw__xor:
    case OpCode::I64__atomic__rmw__xor:
    case OpCode::I32__atomic__rmw8__xor_u:
    case OpCode::I32__atomic__rmw16__xor_u:
    case OpCode::I64__atomic__rmw8__xor_u:
    case OpCode::I64__atomic__rmw16__xor_u:
    case OpCode::I64__atomic__rmw32__xor_u:
    case OpCode::I32__atomic__rmw__xchg:
    case OpCode::I64__atomic__rmw__xchg:
    case OpCode::I32__atomic__rmw8__xchg_u:
    case OpCode::I32__atomic__rmw16__xchg_u:
    case OpCode::I64__atomic__rmw8__xchg_u:
    case OpCode::I64__atomic__rmw16__xchg_u:
    case OpCode::I64__atomic__rmw32__xchg_u:
    case OpCode::I32__atomic__rmw__cmpxchg:
    case OpCode::I64__atomic__rmw__cmpxchg:
    case OpCode::I32__atomic__rmw8__cmpxchg_u:
    case OpCode::I32__atomic__rmw16__cmpxchg_u:
    case OpCode::I64__atomic__rmw8__cmpxchg_u:
    case OpCode::I64__atomic__rmw16__cmpxchg_u:
    case OpCode::I64__atomic__rmw32__cmpxchg_u:
      return compileAtomicOp(Instr);
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
  auto NotNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.not_null");
  auto IsNullBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.is_null");
  auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "c_i.end");

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

  std::vector<LLVM::Value> FPtrRetsVec;
  FPtrRetsVec.reserve(RetSize);
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
            Builder, Executable::Intrinsics::kCallIndirect,
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
