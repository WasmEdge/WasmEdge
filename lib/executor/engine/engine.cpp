// SPDX-License-Identifier: Apache-2.0

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runExpression(Runtime::StoreManager &StoreMgr,
                                     AST::InstrView Instrs) {
  StackMgr.pushLabel(0, 0, Instrs.end() - 1);
  return execute(StoreMgr, Instrs.begin(), Instrs.end());
}

Expect<void>
Executor::runFunction(Runtime::StoreManager &StoreMgr,
                      const Runtime::Instance::FunctionInstance &Func,
                      Span<const ValVariant> Params) {
  /// Set start time.
  if (Stat && Conf.getStatisticsConfigure().isTimeMeasuring()) {
    Stat->startRecordWasm();
  }

  /// Reset and push a dummy frame into stack.
  StackMgr.reset();
  StackMgr.pushDummyFrame();

  /// Push arguments.
  for (auto &Val : Params) {
    StackMgr.push(Val);
  }

  /// Enter and execute function.
  AST::InstrView::iterator StartIt;
  if (auto Res = enterFunction(StoreMgr, Func, Func.getInstrs().end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  auto Res = execute(StoreMgr, StartIt, Func.getInstrs().end());

  if (Res) {
    spdlog::debug(" Execution succeeded.");
  } else if (Res.error() == ErrCode::Terminated) {
    spdlog::debug(" Terminated.");
  }

  if (Stat && Conf.getStatisticsConfigure().isTimeMeasuring()) {
    Stat->stopRecordWasm();
  }

  // If Statistics is enabled, then dump it here.
  if (Stat) {
    Stat->dumpToLog(Conf);
  }

  if (Res || Res.error() == ErrCode::Terminated) {
    return {};
  }
  return Unexpect(Res);
}

Expect<void> Executor::execute(Runtime::StoreManager &StoreMgr,
                               const AST::InstrView::iterator Start,
                               const AST::InstrView::iterator End) {
  AST::InstrView::iterator PC = Start;
  AST::InstrView::iterator PCEnd = End;

  auto Dispatch = [this, &PC, &StoreMgr]() -> Expect<void> {
    const AST::Instruction &Instr = *PC;
    switch (Instr.getOpCode()) {
    /// Control instructions.
    case OpCode::Unreachable:
      spdlog::error(ErrCode::Unreachable);
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Unreachable);
    case OpCode::Nop:
      return {};
    case OpCode::Block:
      return runBlockOp(StoreMgr, Instr, PC);
    case OpCode::Loop:
      return runLoopOp(StoreMgr, Instr, PC);
    case OpCode::If:
      return runIfElseOp(StoreMgr, Instr, PC);
    case OpCode::Else:
      if (Stat && Conf.getStatisticsConfigure().isCostMeasuring()) {
        /// Reach here means end of if-statement.
        if (unlikely(!Stat->subInstrCost(Instr.getOpCode()))) {
          spdlog::error(
              ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
          return Unexpect(ErrCode::CostLimitExceeded);
        }
        if (unlikely(!Stat->addInstrCost(OpCode::End))) {
          spdlog::error(
              ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
          return Unexpect(ErrCode::CostLimitExceeded);
        }
      }
      [[fallthrough]];
    case OpCode::End:
      PC = StackMgr.leaveLabel();
      return {};
    case OpCode::Br:
      return runBrOp(StoreMgr, Instr, PC);
    case OpCode::Br_if:
      return runBrIfOp(StoreMgr, Instr, PC);
    case OpCode::Br_table:
      return runBrTableOp(StoreMgr, Instr, PC);
    case OpCode::Return:
      return runReturnOp(PC);
    case OpCode::Call:
      return runCallOp(StoreMgr, Instr, PC);
    case OpCode::Call_indirect:
      return runCallIndirectOp(StoreMgr, Instr, PC);

    /// Reference Instructions
    case OpCode::Ref__null:
      StackMgr.push(UnknownRef());
      return {};
    case OpCode::Ref__is_null: {
      ValVariant &Val = StackMgr.getTop();
      if (isNullRef(Val)) {
        Val.emplace<uint32_t>(UINT32_C(1));
      } else {
        Val.emplace<uint32_t>(UINT32_C(0));
      }
      return {};
    }
    case OpCode::Ref__func: {
      const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
      const uint32_t FuncAddr = *ModInst->getFuncAddr(Instr.getTargetIndex());
      StackMgr.push(FuncRef(FuncAddr));
      return {};
    }

    /// Parametric Instructions
    case OpCode::Drop:
      StackMgr.pop();
      return {};
    case OpCode::Select:
    case OpCode::Select_t: {
      /// Pop the i32 value and select values from stack.
      ValVariant CondVal = StackMgr.pop();
      ValVariant Val2 = StackMgr.pop();
      ValVariant Val1 = StackMgr.pop();

      /// Select the value.
      if (CondVal.get<uint32_t>() == 0) {
        StackMgr.push(Val2);
      } else {
        StackMgr.push(Val1);
      }
      return {};
    }

    /// Variable Instructions
    case OpCode::Local__get:
      return runLocalGetOp(Instr.getTargetIndex());
    case OpCode::Local__set:
      return runLocalSetOp(Instr.getTargetIndex());
    case OpCode::Local__tee:
      return runLocalTeeOp(Instr.getTargetIndex());
    case OpCode::Global__get:
      return runGlobalGetOp(StoreMgr, Instr.getTargetIndex());
    case OpCode::Global__set:
      return runGlobalSetOp(StoreMgr, Instr.getTargetIndex());

    /// Table Instructions
    case OpCode::Table__get:
      return runTableGetOp(*getTabInstByIdx(StoreMgr, Instr.getTargetIndex()),
                           Instr);
    case OpCode::Table__set:
      return runTableSetOp(*getTabInstByIdx(StoreMgr, Instr.getTargetIndex()),
                           Instr);
    case OpCode::Table__init:
      return runTableInitOp(*getTabInstByIdx(StoreMgr, Instr.getTargetIndex()),
                            *getElemInstByIdx(StoreMgr, Instr.getSourceIndex()),
                            Instr);
    case OpCode::Elem__drop:
      return runElemDropOp(*getElemInstByIdx(StoreMgr, Instr.getTargetIndex()));
    case OpCode::Table__copy:
      return runTableCopyOp(*getTabInstByIdx(StoreMgr, Instr.getTargetIndex()),
                            *getTabInstByIdx(StoreMgr, Instr.getSourceIndex()),
                            Instr);
    case OpCode::Table__grow:
      return runTableGrowOp(*getTabInstByIdx(StoreMgr, Instr.getTargetIndex()));
    case OpCode::Table__size:
      return runTableSizeOp(*getTabInstByIdx(StoreMgr, Instr.getTargetIndex()));
    case OpCode::Table__fill:
      return runTableFillOp(*getTabInstByIdx(StoreMgr, Instr.getTargetIndex()),
                            Instr);

    /// Memory Instructions
    case OpCode::I32__load:
      return runLoadOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::I64__load:
      return runLoadOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::F32__load:
      return runLoadOp<float>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::F64__load:
      return runLoadOp<double>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::I32__load8_s:
      return runLoadOp<int32_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 8);
    case OpCode::I32__load8_u:
      return runLoadOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 8);
    case OpCode::I32__load16_s:
      return runLoadOp<int32_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 16);
    case OpCode::I32__load16_u:
      return runLoadOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 16);
    case OpCode::I64__load8_s:
      return runLoadOp<int64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 8);
    case OpCode::I64__load8_u:
      return runLoadOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 8);
    case OpCode::I64__load16_s:
      return runLoadOp<int64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 16);
    case OpCode::I64__load16_u:
      return runLoadOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 16);
    case OpCode::I64__load32_s:
      return runLoadOp<int64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 32);
    case OpCode::I64__load32_u:
      return runLoadOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 32);
    case OpCode::I32__store:
      return runStoreOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::I64__store:
      return runStoreOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::F32__store:
      return runStoreOp<float>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::F64__store:
      return runStoreOp<double>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::I32__store8:
      return runStoreOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 8);
    case OpCode::I32__store16:
      return runStoreOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 16);
    case OpCode::I64__store8:
      return runStoreOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 8);
    case OpCode::I64__store16:
      return runStoreOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 16);
    case OpCode::I64__store32:
      return runStoreOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 32);
    case OpCode::Memory__grow:
      return runMemoryGrowOp(*getMemInstByIdx(StoreMgr, 0));
    case OpCode::Memory__size:
      return runMemorySizeOp(*getMemInstByIdx(StoreMgr, 0));
    case OpCode::Memory__init:
      return runMemoryInitOp(
          *getMemInstByIdx(StoreMgr, 0),
          *getDataInstByIdx(StoreMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Data__drop:
      return runDataDropOp(*getDataInstByIdx(StoreMgr, Instr.getTargetIndex()));
    case OpCode::Memory__copy:
      return runMemoryCopyOp(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::Memory__fill:
      return runMemoryFillOp(*getMemInstByIdx(StoreMgr, 0), Instr);

    /// Const numeric instructions
    case OpCode::I32__const:
    case OpCode::I64__const:
    case OpCode::F32__const:
    case OpCode::F64__const:
      StackMgr.push(Instr.getNum());
      return {};

    /// Unary numeric instructions
    case OpCode::I32__eqz:
      return runEqzOp<uint32_t>(StackMgr.getTop());
    case OpCode::I64__eqz:
      return runEqzOp<uint64_t>(StackMgr.getTop());
    case OpCode::I32__clz:
      return runClzOp<uint32_t>(StackMgr.getTop());
    case OpCode::I32__ctz:
      return runCtzOp<uint32_t>(StackMgr.getTop());
    case OpCode::I32__popcnt:
      return runPopcntOp<uint32_t>(StackMgr.getTop());
    case OpCode::I64__clz:
      return runClzOp<uint64_t>(StackMgr.getTop());
    case OpCode::I64__ctz:
      return runCtzOp<uint64_t>(StackMgr.getTop());
    case OpCode::I64__popcnt:
      return runPopcntOp<uint64_t>(StackMgr.getTop());
    case OpCode::F32__abs:
      return runAbsOp<float>(StackMgr.getTop());
    case OpCode::F32__neg:
      return runNegOp<float>(StackMgr.getTop());
    case OpCode::F32__ceil:
      return runCeilOp<float>(StackMgr.getTop());
    case OpCode::F32__floor:
      return runFloorOp<float>(StackMgr.getTop());
    case OpCode::F32__trunc:
      return runTruncOp<float>(StackMgr.getTop());
    case OpCode::F32__nearest:
      return runNearestOp<float>(StackMgr.getTop());
    case OpCode::F32__sqrt:
      return runSqrtOp<float>(StackMgr.getTop());
    case OpCode::F64__abs:
      return runAbsOp<double>(StackMgr.getTop());
    case OpCode::F64__neg:
      return runNegOp<double>(StackMgr.getTop());
    case OpCode::F64__ceil:
      return runCeilOp<double>(StackMgr.getTop());
    case OpCode::F64__floor:
      return runFloorOp<double>(StackMgr.getTop());
    case OpCode::F64__trunc:
      return runTruncOp<double>(StackMgr.getTop());
    case OpCode::F64__nearest:
      return runNearestOp<double>(StackMgr.getTop());
    case OpCode::F64__sqrt:
      return runSqrtOp<double>(StackMgr.getTop());
    case OpCode::I32__wrap_i64:
      return runWrapOp<uint64_t, uint32_t>(StackMgr.getTop());
    case OpCode::I32__trunc_f32_s:
      return runTruncateOp<float, int32_t>(Instr, StackMgr.getTop());
    case OpCode::I32__trunc_f32_u:
      return runTruncateOp<float, uint32_t>(Instr, StackMgr.getTop());
    case OpCode::I32__trunc_f64_s:
      return runTruncateOp<double, int32_t>(Instr, StackMgr.getTop());
    case OpCode::I32__trunc_f64_u:
      return runTruncateOp<double, uint32_t>(Instr, StackMgr.getTop());
    case OpCode::I64__extend_i32_s:
      return runExtendOp<int32_t, uint64_t>(StackMgr.getTop());
    case OpCode::I64__extend_i32_u:
      return runExtendOp<uint32_t, uint64_t>(StackMgr.getTop());
    case OpCode::I64__trunc_f32_s:
      return runTruncateOp<float, int64_t>(Instr, StackMgr.getTop());
    case OpCode::I64__trunc_f32_u:
      return runTruncateOp<float, uint64_t>(Instr, StackMgr.getTop());
    case OpCode::I64__trunc_f64_s:
      return runTruncateOp<double, int64_t>(Instr, StackMgr.getTop());
    case OpCode::I64__trunc_f64_u:
      return runTruncateOp<double, uint64_t>(Instr, StackMgr.getTop());
    case OpCode::F32__convert_i32_s:
      return runConvertOp<int32_t, float>(StackMgr.getTop());
    case OpCode::F32__convert_i32_u:
      return runConvertOp<uint32_t, float>(StackMgr.getTop());
    case OpCode::F32__convert_i64_s:
      return runConvertOp<int64_t, float>(StackMgr.getTop());
    case OpCode::F32__convert_i64_u:
      return runConvertOp<uint64_t, float>(StackMgr.getTop());
    case OpCode::F32__demote_f64:
      return runDemoteOp<double, float>(StackMgr.getTop());
    case OpCode::F64__convert_i32_s:
      return runConvertOp<int32_t, double>(StackMgr.getTop());
    case OpCode::F64__convert_i32_u:
      return runConvertOp<uint32_t, double>(StackMgr.getTop());
    case OpCode::F64__convert_i64_s:
      return runConvertOp<int64_t, double>(StackMgr.getTop());
    case OpCode::F64__convert_i64_u:
      return runConvertOp<uint64_t, double>(StackMgr.getTop());
    case OpCode::F64__promote_f32:
      return runPromoteOp<float, double>(StackMgr.getTop());
    case OpCode::I32__reinterpret_f32:
      return runReinterpretOp<float, uint32_t>(StackMgr.getTop());
    case OpCode::I64__reinterpret_f64:
      return runReinterpretOp<double, uint64_t>(StackMgr.getTop());
    case OpCode::F32__reinterpret_i32:
      return runReinterpretOp<uint32_t, float>(StackMgr.getTop());
    case OpCode::F64__reinterpret_i64:
      return runReinterpretOp<uint64_t, double>(StackMgr.getTop());
    case OpCode::I32__extend8_s:
      return runExtendOp<int32_t, uint32_t, 8>(StackMgr.getTop());
    case OpCode::I32__extend16_s:
      return runExtendOp<int32_t, uint32_t, 16>(StackMgr.getTop());
    case OpCode::I64__extend8_s:
      return runExtendOp<int64_t, uint64_t, 8>(StackMgr.getTop());
    case OpCode::I64__extend16_s:
      return runExtendOp<int64_t, uint64_t, 16>(StackMgr.getTop());
    case OpCode::I64__extend32_s:
      return runExtendOp<int64_t, uint64_t, 32>(StackMgr.getTop());
    case OpCode::I32__trunc_sat_f32_s:
      return runTruncateSatOp<float, int32_t>(StackMgr.getTop());
    case OpCode::I32__trunc_sat_f32_u:
      return runTruncateSatOp<float, uint32_t>(StackMgr.getTop());
    case OpCode::I32__trunc_sat_f64_s:
      return runTruncateSatOp<double, int32_t>(StackMgr.getTop());
    case OpCode::I32__trunc_sat_f64_u:
      return runTruncateSatOp<double, uint32_t>(StackMgr.getTop());
    case OpCode::I64__trunc_sat_f32_s:
      return runTruncateSatOp<float, int64_t>(StackMgr.getTop());
    case OpCode::I64__trunc_sat_f32_u:
      return runTruncateSatOp<float, uint64_t>(StackMgr.getTop());
    case OpCode::I64__trunc_sat_f64_s:
      return runTruncateSatOp<double, int64_t>(StackMgr.getTop());
    case OpCode::I64__trunc_sat_f64_u:
      return runTruncateSatOp<double, uint64_t>(StackMgr.getTop());

      /// Binary numeric instructions
    case OpCode::I32__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runEqOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runNeOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__lt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runLtOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__lt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runLtOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__gt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runGtOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__gt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runGtOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__le_s: {
      ValVariant Rhs = StackMgr.pop();
      return runLeOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__le_u: {
      ValVariant Rhs = StackMgr.pop();
      return runLeOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__ge_s: {
      ValVariant Rhs = StackMgr.pop();
      return runGeOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__ge_u: {
      ValVariant Rhs = StackMgr.pop();
      return runGeOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runEqOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runNeOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__lt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runLtOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__lt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runLtOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__gt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runGtOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__gt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runGtOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__le_s: {
      ValVariant Rhs = StackMgr.pop();
      return runLeOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__le_u: {
      ValVariant Rhs = StackMgr.pop();
      return runLeOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__ge_s: {
      ValVariant Rhs = StackMgr.pop();
      return runGeOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__ge_u: {
      ValVariant Rhs = StackMgr.pop();
      return runGeOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runEqOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runNeOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__lt: {
      ValVariant Rhs = StackMgr.pop();
      return runLtOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__gt: {
      ValVariant Rhs = StackMgr.pop();
      return runGtOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__le: {
      ValVariant Rhs = StackMgr.pop();
      return runLeOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__ge: {
      ValVariant Rhs = StackMgr.pop();
      return runGeOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runEqOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runNeOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__lt: {
      ValVariant Rhs = StackMgr.pop();
      return runLtOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__gt: {
      ValVariant Rhs = StackMgr.pop();
      return runGtOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__le: {
      ValVariant Rhs = StackMgr.pop();
      return runLeOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__ge: {
      ValVariant Rhs = StackMgr.pop();
      return runGeOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__add: {
      ValVariant Rhs = StackMgr.pop();
      return runAddOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runSubOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runMulOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__div_s: {
      ValVariant Rhs = StackMgr.pop();
      return runDivOp<int32_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__div_u: {
      ValVariant Rhs = StackMgr.pop();
      return runDivOp<uint32_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__rem_s: {
      ValVariant Rhs = StackMgr.pop();
      return runRemOp<int32_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__rem_u: {
      ValVariant Rhs = StackMgr.pop();
      return runRemOp<uint32_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__and: {
      ValVariant Rhs = StackMgr.pop();
      return runAndOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__or: {
      ValVariant Rhs = StackMgr.pop();
      return runOrOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__xor: {
      ValVariant Rhs = StackMgr.pop();
      return runXorOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__shl: {
      ValVariant Rhs = StackMgr.pop();
      return runShlOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__shr_s: {
      ValVariant Rhs = StackMgr.pop();
      return runShrOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__shr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runShrOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__rotl: {
      ValVariant Rhs = StackMgr.pop();
      return runRotlOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32__rotr: {
      ValVariant Rhs = StackMgr.pop();
      return runRotrOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__add: {
      ValVariant Rhs = StackMgr.pop();
      return runAddOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runSubOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runMulOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__div_s: {
      ValVariant Rhs = StackMgr.pop();
      return runDivOp<int64_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__div_u: {
      ValVariant Rhs = StackMgr.pop();
      return runDivOp<uint64_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__rem_s: {
      ValVariant Rhs = StackMgr.pop();
      return runRemOp<int64_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__rem_u: {
      ValVariant Rhs = StackMgr.pop();
      return runRemOp<uint64_t>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__and: {
      ValVariant Rhs = StackMgr.pop();
      return runAndOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__or: {
      ValVariant Rhs = StackMgr.pop();
      return runOrOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__xor: {
      ValVariant Rhs = StackMgr.pop();
      return runXorOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__shl: {
      ValVariant Rhs = StackMgr.pop();
      return runShlOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__shr_s: {
      ValVariant Rhs = StackMgr.pop();
      return runShrOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__shr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runShrOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__rotl: {
      ValVariant Rhs = StackMgr.pop();
      return runRotlOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64__rotr: {
      ValVariant Rhs = StackMgr.pop();
      return runRotrOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__add: {
      ValVariant Rhs = StackMgr.pop();
      return runAddOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runSubOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runMulOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__div: {
      ValVariant Rhs = StackMgr.pop();
      return runDivOp<float>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__min: {
      ValVariant Rhs = StackMgr.pop();
      return runMinOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__max: {
      ValVariant Rhs = StackMgr.pop();
      return runMaxOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32__copysign: {
      ValVariant Rhs = StackMgr.pop();
      return runCopysignOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__add: {
      ValVariant Rhs = StackMgr.pop();
      return runAddOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runSubOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runMulOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__div: {
      ValVariant Rhs = StackMgr.pop();
      return runDivOp<double>(Instr, StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__min: {
      ValVariant Rhs = StackMgr.pop();
      return runMinOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__max: {
      ValVariant Rhs = StackMgr.pop();
      return runMaxOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64__copysign: {
      ValVariant Rhs = StackMgr.pop();
      return runCopysignOp<double>(StackMgr.getTop(), Rhs);
    }

    /// SIMD Memory Instructions
    case OpCode::V128__load:
      return runLoadOp<uint128_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load8x8_s:
      return runLoadExpandOp<int8_t, int16_t>(*getMemInstByIdx(StoreMgr, 0),
                                              Instr);
    case OpCode::V128__load8x8_u:
      return runLoadExpandOp<uint8_t, uint16_t>(*getMemInstByIdx(StoreMgr, 0),
                                                Instr);
    case OpCode::V128__load16x4_s:
      return runLoadExpandOp<int16_t, int32_t>(*getMemInstByIdx(StoreMgr, 0),
                                               Instr);
    case OpCode::V128__load16x4_u:
      return runLoadExpandOp<uint16_t, uint32_t>(*getMemInstByIdx(StoreMgr, 0),
                                                 Instr);
    case OpCode::V128__load32x2_s:
      return runLoadExpandOp<int32_t, int64_t>(*getMemInstByIdx(StoreMgr, 0),
                                               Instr);
    case OpCode::V128__load32x2_u:
      return runLoadExpandOp<uint32_t, uint64_t>(*getMemInstByIdx(StoreMgr, 0),
                                                 Instr);
    case OpCode::V128__load8_splat:
      return runLoadSplatOp<uint8_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load16_splat:
      return runLoadSplatOp<uint16_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load32_splat:
      return runLoadSplatOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load64_splat:
      return runLoadSplatOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load32_zero:
      return runLoadOp<uint128_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 32);
    case OpCode::V128__load64_zero:
      return runLoadOp<uint128_t>(*getMemInstByIdx(StoreMgr, 0), Instr, 64);
    case OpCode::V128__store:
      return runStoreOp<uint128_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load8_lane:
      return runLoadLaneOp<uint8_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load16_lane:
      return runLoadLaneOp<uint16_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load32_lane:
      return runLoadLaneOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__load64_lane:
      return runLoadLaneOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__store8_lane:
      return runStoreLaneOp<uint8_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__store16_lane:
      return runStoreLaneOp<uint16_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__store32_lane:
      return runStoreLaneOp<uint32_t>(*getMemInstByIdx(StoreMgr, 0), Instr);
    case OpCode::V128__store64_lane:
      return runStoreLaneOp<uint64_t>(*getMemInstByIdx(StoreMgr, 0), Instr);

    /// SIMD Const Instructions
    case OpCode::V128__const:
      StackMgr.push(Instr.getNum());
      return {};

    /// SIMD Shuffle Instructions
    case OpCode::I8x16__shuffle: {
      ValVariant Val2 = StackMgr.pop();
      ValVariant &Val1 = StackMgr.getTop();
      std::array<uint8_t, 32> Data;
      std::array<uint8_t, 16> Result;
      std::memcpy(&Data[0], &Val1, 16);
      std::memcpy(&Data[16], &Val2, 16);
      const auto V3 = Instr.getNum().get<uint128_t>();
      for (size_t I = 0; I < 16; ++I) {
        const uint8_t Index = static_cast<uint8_t>(V3 >> (I * 8));
        Result[I] = Data[Index];
      }
      std::memcpy(&Val1, &Result[0], 16);
      return {};
    }

    /// SIMD Lane Instructions
    case OpCode::I8x16__extract_lane_s:
      return runExtractLaneOp<int8_t, int32_t>(StackMgr.getTop(),
                                               Instr.getTargetIndex());
    case OpCode::I8x16__extract_lane_u:
      return runExtractLaneOp<uint8_t, uint32_t>(StackMgr.getTop(),
                                                 Instr.getTargetIndex());
    case OpCode::I16x8__extract_lane_s:
      return runExtractLaneOp<int16_t, int32_t>(StackMgr.getTop(),
                                                Instr.getTargetIndex());
    case OpCode::I16x8__extract_lane_u:
      return runExtractLaneOp<uint16_t, uint32_t>(StackMgr.getTop(),
                                                  Instr.getTargetIndex());
    case OpCode::I32x4__extract_lane:
      return runExtractLaneOp<uint32_t>(StackMgr.getTop(),
                                        Instr.getTargetIndex());
    case OpCode::I64x2__extract_lane:
      return runExtractLaneOp<uint64_t>(StackMgr.getTop(),
                                        Instr.getTargetIndex());
    case OpCode::F32x4__extract_lane:
      return runExtractLaneOp<float>(StackMgr.getTop(), Instr.getTargetIndex());
    case OpCode::F64x2__extract_lane:
      return runExtractLaneOp<double>(StackMgr.getTop(),
                                      Instr.getTargetIndex());
    case OpCode::I8x16__replace_lane: {
      ValVariant Rhs = StackMgr.pop();
      return runReplaceLaneOp<uint32_t, uint8_t>(StackMgr.getTop(), Rhs,
                                                 Instr.getTargetIndex());
    }
    case OpCode::I16x8__replace_lane: {
      ValVariant Rhs = StackMgr.pop();
      return runReplaceLaneOp<uint32_t, uint16_t>(StackMgr.getTop(), Rhs,
                                                  Instr.getTargetIndex());
    }
    case OpCode::I32x4__replace_lane: {
      ValVariant Rhs = StackMgr.pop();
      return runReplaceLaneOp<uint32_t>(StackMgr.getTop(), Rhs,
                                        Instr.getTargetIndex());
    }
    case OpCode::I64x2__replace_lane: {
      ValVariant Rhs = StackMgr.pop();
      return runReplaceLaneOp<uint64_t>(StackMgr.getTop(), Rhs,
                                        Instr.getTargetIndex());
    }
    case OpCode::F32x4__replace_lane: {
      ValVariant Rhs = StackMgr.pop();
      return runReplaceLaneOp<float>(StackMgr.getTop(), Rhs,
                                     Instr.getTargetIndex());
    }
    case OpCode::F64x2__replace_lane: {
      ValVariant Rhs = StackMgr.pop();
      return runReplaceLaneOp<double>(StackMgr.getTop(), Rhs,
                                      Instr.getTargetIndex());
    }

      /// SIMD Numeric Instructions
    case OpCode::I8x16__swizzle: {
      const ValVariant Val2 = StackMgr.pop();
      ValVariant &Val1 = StackMgr.getTop();
      const uint8x16_t &Index = Val2.get<uint8x16_t>();
      uint8x16_t &Vector = Val1.get<uint8x16_t>();
      const uint8x16_t Limit = uint8x16_t{} + 16;
      const uint8x16_t Zero = uint8x16_t{};
      const uint8x16_t Exceed = (Index >= Limit);
#ifdef __clang__
      uint8x16_t Result = {Vector[Index[0] & 0xF],  Vector[Index[1] & 0xF],
                           Vector[Index[2] & 0xF],  Vector[Index[3] & 0xF],
                           Vector[Index[4] & 0xF],  Vector[Index[5] & 0xF],
                           Vector[Index[6] & 0xF],  Vector[Index[7] & 0xF],
                           Vector[Index[8] & 0xF],  Vector[Index[9] & 0xF],
                           Vector[Index[10] & 0xF], Vector[Index[11] & 0xF],
                           Vector[Index[12] & 0xF], Vector[Index[13] & 0xF],
                           Vector[Index[14] & 0xF], Vector[Index[15] & 0xF]};
#else
      uint8x16_t Result = __builtin_shuffle(Vector, Index);
#endif
      Vector = detail::vectorSelect(Exceed, Zero, Result);
      return {};
    }
    case OpCode::I8x16__splat:
      return runSplatOp<uint32_t, uint8_t>(StackMgr.getTop());
    case OpCode::I16x8__splat:
      return runSplatOp<uint32_t, uint16_t>(StackMgr.getTop());
    case OpCode::I32x4__splat:
      return runSplatOp<uint32_t>(StackMgr.getTop());
    case OpCode::I64x2__splat:
      return runSplatOp<uint64_t>(StackMgr.getTop());
    case OpCode::F32x4__splat:
      return runSplatOp<float>(StackMgr.getTop());
    case OpCode::F64x2__splat:
      return runSplatOp<double>(StackMgr.getTop());
    case OpCode::I8x16__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorEqOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNeOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__lt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__lt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__gt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__gt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__le_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__le_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__ge_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__ge_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorEqOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNeOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__lt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__lt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__gt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__gt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__le_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__le_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__ge_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__ge_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorEqOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNeOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__lt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__lt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__gt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__gt_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__le_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__le_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__ge_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__ge_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorEqOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNeOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__lt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__gt_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__le_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__ge_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorEqOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNeOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__lt: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__gt: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__le: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__ge: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__eq: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorEqOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__ne: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNeOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__lt: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLtOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__gt: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGtOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__le: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorLeOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__ge: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorGeOp<double>(StackMgr.getTop(), Rhs);
    }

    case OpCode::V128__not: {
      auto &Val = StackMgr.getTop().get<uint64x2_t>();
      Val = ~Val;
      return {};
    }
    case OpCode::V128__and: {
      const ValVariant Val2 = StackMgr.pop();
      ValVariant &Val1 = StackMgr.getTop();
      Val1.get<uint64x2_t>() &= Val2.get<uint64x2_t>();
      return {};
    }
    case OpCode::V128__andnot: {
      const ValVariant Val2 = StackMgr.pop();
      ValVariant &Val1 = StackMgr.getTop();
      Val1.get<uint64x2_t>() &= ~Val2.get<uint64x2_t>();
      return {};
    }
    case OpCode::V128__or: {
      const ValVariant Val2 = StackMgr.pop();
      ValVariant &Val1 = StackMgr.getTop();
      Val1.get<uint64x2_t>() |= Val2.get<uint64x2_t>();
      return {};
    }
    case OpCode::V128__xor: {
      const ValVariant Val2 = StackMgr.pop();
      ValVariant &Val1 = StackMgr.getTop();
      Val1.get<uint64x2_t>() ^= Val2.get<uint64x2_t>();
      return {};
    }
    case OpCode::V128__bitselect: {
      const uint64x2_t C = StackMgr.pop().get<uint64x2_t>();
      const uint64x2_t Val2 = StackMgr.pop().get<uint64x2_t>();
      uint64x2_t &Val1 = StackMgr.getTop().get<uint64x2_t>();
      Val1 = (Val1 & C) | (Val2 & ~C);
      return {};
    }
    case OpCode::V128__any_true:
      return runVectorAnyTrueOp(StackMgr.getTop());

    case OpCode::I8x16__abs:
      return runVectorAbsOp<int8_t>(StackMgr.getTop());
    case OpCode::I8x16__neg:
      return runVectorNegOp<int8_t>(StackMgr.getTop());
    case OpCode::I8x16__popcnt:
      return runVectorPopcntOp(StackMgr.getTop());
    case OpCode::I8x16__all_true:
      return runVectorAllTrueOp<uint8_t>(StackMgr.getTop());
    case OpCode::I8x16__bitmask:
      return runVectorBitMaskOp<uint8_t>(StackMgr.getTop());
    case OpCode::I8x16__narrow_i16x8_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNarrowOp<int16_t, int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__narrow_i16x8_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNarrowOp<int16_t, uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__shl: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShlOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__shr_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__shr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__add: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__add_sat_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddSatOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__add_sat_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddSatOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__sub_sat_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubSatOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__sub_sat_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubSatOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__min_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__min_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__max_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<int8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__max_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<uint8_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I8x16__avgr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAvgrOp<uint8_t, uint16_t>(StackMgr.getTop(), Rhs);
    }

    case OpCode::I16x8__abs:
      return runVectorAbsOp<int16_t>(StackMgr.getTop());
    case OpCode::I16x8__neg:
      return runVectorNegOp<int16_t>(StackMgr.getTop());
    case OpCode::I16x8__all_true:
      return runVectorAllTrueOp<uint16_t>(StackMgr.getTop());
    case OpCode::I16x8__bitmask:
      return runVectorBitMaskOp<uint16_t>(StackMgr.getTop());
    case OpCode::I16x8__narrow_i32x4_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNarrowOp<int32_t, int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__narrow_i32x4_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorNarrowOp<int32_t, uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__extend_low_i8x16_s:
      return runVectorExtendLowOp<int8_t, int16_t>(StackMgr.getTop());
    case OpCode::I16x8__extend_high_i8x16_s:
      return runVectorExtendHighOp<int8_t, int16_t>(StackMgr.getTop());
    case OpCode::I16x8__extend_low_i8x16_u:
      return runVectorExtendLowOp<uint8_t, uint16_t>(StackMgr.getTop());
    case OpCode::I16x8__extend_high_i8x16_u:
      return runVectorExtendHighOp<uint8_t, uint16_t>(StackMgr.getTop());
    case OpCode::I16x8__shl: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShlOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__shr_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__shr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__add: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__add_sat_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddSatOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__add_sat_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddSatOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__sub_sat_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubSatOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__sub_sat_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubSatOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMulOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__min_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__min_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__max_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__max_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__avgr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAvgrOp<uint16_t, uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__extmul_low_i8x16_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulLowOp<int8_t, int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__extmul_high_i8x16_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulHighOp<int8_t, int16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__extmul_low_i8x16_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulLowOp<uint8_t, uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__extmul_high_i8x16_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulHighOp<uint8_t, uint16_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__q15mulr_sat_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorQ15MulSatOp(StackMgr.getTop(), Rhs);
    }
    case OpCode::I16x8__extadd_pairwise_i8x16_s:
      return runVectorExtAddPairwiseOp<int8_t, int16_t>(StackMgr.getTop());
    case OpCode::I16x8__extadd_pairwise_i8x16_u:
      return runVectorExtAddPairwiseOp<uint8_t, uint16_t>(StackMgr.getTop());

    case OpCode::I32x4__abs:
      return runVectorAbsOp<int32_t>(StackMgr.getTop());
    case OpCode::I32x4__neg:
      return runVectorNegOp<int32_t>(StackMgr.getTop());
    case OpCode::I32x4__all_true:
      return runVectorAllTrueOp<uint32_t>(StackMgr.getTop());
    case OpCode::I32x4__bitmask:
      return runVectorBitMaskOp<uint32_t>(StackMgr.getTop());
    case OpCode::I32x4__extend_low_i16x8_s:
      return runVectorExtendLowOp<int16_t, int32_t>(StackMgr.getTop());
    case OpCode::I32x4__extend_high_i16x8_s:
      return runVectorExtendHighOp<int16_t, int32_t>(StackMgr.getTop());
    case OpCode::I32x4__extend_low_i16x8_u:
      return runVectorExtendLowOp<uint16_t, uint32_t>(StackMgr.getTop());
    case OpCode::I32x4__extend_high_i16x8_u:
      return runVectorExtendHighOp<uint16_t, uint32_t>(StackMgr.getTop());
    case OpCode::I32x4__shl: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShlOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__shr_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__shr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__add: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMulOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__min_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__min_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__max_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__max_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__extmul_low_i16x8_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulLowOp<int16_t, int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__extmul_high_i16x8_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulHighOp<int16_t, int32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__extmul_low_i16x8_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulLowOp<uint16_t, uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__extmul_high_i16x8_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulHighOp<uint16_t, uint32_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I32x4__extadd_pairwise_i16x8_s:
      return runVectorExtAddPairwiseOp<int16_t, int32_t>(StackMgr.getTop());
    case OpCode::I32x4__extadd_pairwise_i16x8_u:
      return runVectorExtAddPairwiseOp<uint16_t, uint32_t>(StackMgr.getTop());

    case OpCode::I64x2__abs:
      return runVectorAbsOp<int64_t>(StackMgr.getTop());
    case OpCode::I64x2__neg:
      return runVectorNegOp<int64_t>(StackMgr.getTop());
    case OpCode::I64x2__all_true:
      return runVectorAllTrueOp<uint64_t>(StackMgr.getTop());
    case OpCode::I64x2__bitmask:
      return runVectorBitMaskOp<uint64_t>(StackMgr.getTop());
    case OpCode::I64x2__extend_low_i32x4_s:
      return runVectorExtendLowOp<int32_t, int64_t>(StackMgr.getTop());
    case OpCode::I64x2__extend_high_i32x4_s:
      return runVectorExtendHighOp<int32_t, int64_t>(StackMgr.getTop());
    case OpCode::I64x2__extend_low_i32x4_u:
      return runVectorExtendLowOp<uint32_t, uint64_t>(StackMgr.getTop());
    case OpCode::I64x2__extend_high_i32x4_u:
      return runVectorExtendHighOp<uint32_t, uint64_t>(StackMgr.getTop());
    case OpCode::I64x2__shl: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShlOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__shr_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__shr_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorShrOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__add: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMulOp<uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__extmul_low_i32x4_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulLowOp<int32_t, int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__extmul_high_i32x4_s: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulHighOp<int32_t, int64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__extmul_low_i32x4_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulLowOp<uint32_t, uint64_t>(StackMgr.getTop(), Rhs);
    }
    case OpCode::I64x2__extmul_high_i32x4_u: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorExtMulHighOp<uint32_t, uint64_t>(StackMgr.getTop(), Rhs);
    }

    case OpCode::F32x4__abs:
      return runVectorAbsOp<float>(StackMgr.getTop());
    case OpCode::F32x4__neg:
      return runVectorNegOp<float>(StackMgr.getTop());
    case OpCode::F32x4__sqrt:
      return runVectorSqrtOp<float>(StackMgr.getTop());
    case OpCode::F32x4__add: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMulOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__div: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorDivOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__min: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorFMinOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__max: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorFMaxOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__pmin: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<float>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F32x4__pmax: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<float>(StackMgr.getTop(), Rhs);
    }

    case OpCode::F64x2__abs:
      return runVectorAbsOp<double>(StackMgr.getTop());
    case OpCode::F64x2__neg:
      return runVectorNegOp<double>(StackMgr.getTop());
    case OpCode::F64x2__sqrt:
      return runVectorSqrtOp<double>(StackMgr.getTop());
    case OpCode::F64x2__add: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorAddOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__sub: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorSubOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__mul: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMulOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__div: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorDivOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__min: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorFMinOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__max: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorFMaxOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__pmin: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMinOp<double>(StackMgr.getTop(), Rhs);
    }
    case OpCode::F64x2__pmax: {
      ValVariant Rhs = StackMgr.pop();
      return runVectorMaxOp<double>(StackMgr.getTop(), Rhs);
    }

    case OpCode::I32x4__trunc_sat_f32x4_s:
      return runVectorTruncSatOp<float, int32_t>(StackMgr.getTop());
    case OpCode::I32x4__trunc_sat_f32x4_u:
      return runVectorTruncSatOp<float, uint32_t>(StackMgr.getTop());
    case OpCode::F32x4__convert_i32x4_s:
      return runVectorConvertOp<int32_t, float>(StackMgr.getTop());
    case OpCode::F32x4__convert_i32x4_u:
      return runVectorConvertOp<uint32_t, float>(StackMgr.getTop());
    case OpCode::I32x4__trunc_sat_f64x2_s_zero:
      return runVectorTruncSatOp<double, int32_t>(StackMgr.getTop());
    case OpCode::I32x4__trunc_sat_f64x2_u_zero:
      return runVectorTruncSatOp<double, uint32_t>(StackMgr.getTop());
    case OpCode::F64x2__convert_low_i32x4_s:
      return runVectorConvertOp<int32_t, double>(StackMgr.getTop());
    case OpCode::F64x2__convert_low_i32x4_u:
      return runVectorConvertOp<uint32_t, double>(StackMgr.getTop());
    case OpCode::F32x4__demote_f64x2_zero:
      return runVectorDemoteOp(StackMgr.getTop());
    case OpCode::F64x2__promote_low_f32x4:
      return runVectorPromoteOp(StackMgr.getTop());

    case OpCode::I32x4__dot_i16x8_s: {
      using int32x8_t [[gnu::vector_size(32)]] = int32_t;
      const ValVariant Val2 = StackMgr.pop();
      ValVariant &Val1 = StackMgr.getTop();

      auto &V2 = Val2.get<int16x8_t>();
      auto &V1 = Val1.get<int16x8_t>();
      const auto M = __builtin_convertvector(V1, int32x8_t) *
                     __builtin_convertvector(V2, int32x8_t);
      const int32x4_t L = {M[0], M[2], M[4], M[6]};
      const int32x4_t R = {M[1], M[3], M[5], M[7]};
      Val1.emplace<int32x4_t>(L + R);

      return {};
    }
    case OpCode::F32x4__ceil:
      return runVectorCeilOp<float>(StackMgr.getTop());
    case OpCode::F32x4__floor:
      return runVectorFloorOp<float>(StackMgr.getTop());
    case OpCode::F32x4__trunc:
      return runVectorTruncOp<float>(StackMgr.getTop());
    case OpCode::F32x4__nearest:
      return runVectorNearestOp<float>(StackMgr.getTop());
    case OpCode::F64x2__ceil:
      return runVectorCeilOp<double>(StackMgr.getTop());
    case OpCode::F64x2__floor:
      return runVectorFloorOp<double>(StackMgr.getTop());
    case OpCode::F64x2__trunc:
      return runVectorTruncOp<double>(StackMgr.getTop());
    case OpCode::F64x2__nearest:
      return runVectorNearestOp<double>(StackMgr.getTop());

    default:
      return {};
    }
  };

  while (PC != PCEnd) {
    OpCode Code = PC->getOpCode();
    if (Stat) {
      if (Conf.getStatisticsConfigure().isInstructionCounting()) {
        Stat->incInstrCount();
      }
      /// Add cost. Note: if-else case should be processed additionally.
      if (Conf.getStatisticsConfigure().isCostMeasuring()) {
        if (unlikely(!Stat->addInstrCost(Code))) {
          const AST::Instruction &Instr = *PC;
          spdlog::error(
              ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
          return Unexpect(ErrCode::CostLimitExceeded);
        }
      }
    }
    if (auto Res = Dispatch(); !Res) {
      return Unexpect(Res);
    }
    PC++;
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
