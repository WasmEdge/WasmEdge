// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <array>
#include <cstdint>
#include <cstring>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runExpression(Runtime::StoreManager &StoreMgr,
                                     Runtime::StackManager &StackMgr,
                                     AST::InstrView Instrs) {
  StackMgr.pushLabel(0, 0, Instrs.end() - 1);
  return execute(StoreMgr, StackMgr, Instrs.begin(), Instrs.end());
}

Expect<void>
Executor::runFunction(Runtime::StoreManager &StoreMgr,
                      Runtime::StackManager &StackMgr,
                      const Runtime::Instance::FunctionInstance &Func,
                      Span<const ValVariant> Params) {
  // Set start time.
  if (Stat && Conf.getStatisticsConfigure().isTimeMeasuring()) {
    Stat->startRecordWasm();
  }

  // Reset and push a dummy frame into stack.
  StackMgr.reset();
  StackMgr.pushDummyFrame();

  // Push arguments.
  assuming(Params.size() == Func.getFuncType().getParamTypes().size());
  for (uint32_t I = 0; I < Params.size(); ++I) {
    StackMgr.push(Func.getFuncType().getParamTypes()[I], Params[I]);
  }

  // Enter and execute function.
  AST::InstrView::iterator StartIt;
  if (auto Res =
          enterFunction(StoreMgr, StackMgr, Func, Func.getInstrs().end())) {
    StartIt = *Res;
  } else {
    return Unexpect(Res);
  }
  auto Res = execute(StoreMgr, StackMgr, StartIt, Func.getInstrs().end());

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
                               Runtime::StackManager &StackMgr,
                               const AST::InstrView::iterator Start,
                               const AST::InstrView::iterator End) {
  AST::InstrView::iterator PC = Start;
  AST::InstrView::iterator PCEnd = End;

  auto Dispatch = [this, &PC, &StoreMgr, &StackMgr]() -> Expect<void> {
    const AST::Instruction &Instr = *PC;
    switch (Instr.getOpCode()) {
    // Control instructions.
    case OpCode::Unreachable:
      spdlog::error(ErrCode::Unreachable);
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Unreachable);
    case OpCode::Nop:
      return {};
    case OpCode::Block:
      return runBlockOp(StoreMgr, StackMgr, Instr, PC);
    case OpCode::Loop:
      return runLoopOp(StoreMgr, StackMgr, Instr, PC);
    case OpCode::If:
      return runIfElseOp(StoreMgr, StackMgr, Instr, PC);
    case OpCode::Else:
      if (Stat && Conf.getStatisticsConfigure().isCostMeasuring()) {
        // Reach here means end of if-statement.
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
      return runBrOp(StoreMgr, StackMgr, Instr, PC);
    case OpCode::Br_if:
      return runBrIfOp(StoreMgr, StackMgr, Instr, PC);
    case OpCode::Br_table:
      return runBrTableOp(StoreMgr, StackMgr, Instr, PC);
    case OpCode::Return:
      return runReturnOp(StackMgr, PC);
    case OpCode::Call:
      return runCallOp(StoreMgr, StackMgr, Instr, PC);
    case OpCode::Call_indirect:
      return runCallIndirectOp(StoreMgr, StackMgr, Instr, PC);

    // Reference Instructions
    case OpCode::Ref__null:
      StackMgr.push(UnknownRef());
      return {};
    case OpCode::Ref__is_null: {
      RefVariant Val = StackMgr.pop<UnknownRef>();
      if (isNullRef(Val)) {
        StackMgr.push<uint32_t>(UINT32_C(1));
      } else {
        StackMgr.push<uint32_t>(UINT32_C(0));
      }
      return {};
    }
    case OpCode::Ref__func: {
      const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
      const uint32_t FuncAddr = *ModInst->getFuncAddr(Instr.getTargetIndex());
      StackMgr.push(FuncRef(FuncAddr));
      return {};
    }

    // Parametric Instructions
    case OpCode::Drop:
      StackMgr.popUnknown();
      return {};
    case OpCode::Select:
    case OpCode::Select_t: {
      // Pop the i32 value and select values from stack.
      uint32_t CondVal = StackMgr.pop<uint32_t>();
      ValVariant Val2 = StackMgr.popUnknown();
      ValVariant Val1 = StackMgr.popUnknown();

      // Select the value.
      if (CondVal == 0) {
        StackMgr.pushUnknown(Val2);
      } else {
        StackMgr.pushUnknown(Val1);
      }
      return {};
    }

    // Variable Instructions
    case OpCode::Local__get:
      return runLocalGetOp(StackMgr, Instr.getTargetIndex());
    case OpCode::Local__set:
      return runLocalSetOp(StackMgr, Instr.getTargetIndex());
    case OpCode::Local__tee:
      return runLocalTeeOp(StackMgr, Instr.getTargetIndex());
    case OpCode::Global__get:
      return runGlobalGetOp(StoreMgr, StackMgr, Instr.getTargetIndex());
    case OpCode::Global__set:
      return runGlobalSetOp(StoreMgr, StackMgr, Instr.getTargetIndex());

    // Table Instructions
    case OpCode::Table__get:
      return runTableGetOp(
          StackMgr,
          *getTabInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::Table__set:
      return runTableSetOp(
          StackMgr,
          *getTabInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::Table__init:
      return runTableInitOp(
          StackMgr,
          *getTabInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()),
          *getElemInstByIdx(StoreMgr, StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Elem__drop:
      return runElemDropOp(
          *getElemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()));
    case OpCode::Table__copy:
      return runTableCopyOp(
          StackMgr,
          *getTabInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()),
          *getTabInstByIdx(StoreMgr, StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Table__grow:
      return runTableGrowOp(StackMgr, *getTabInstByIdx(StoreMgr, StackMgr,
                                                       Instr.getTargetIndex()));
    case OpCode::Table__size:
      return runTableSizeOp(StackMgr, *getTabInstByIdx(StoreMgr, StackMgr,
                                                       Instr.getTargetIndex()));
    case OpCode::Table__fill:
      return runTableFillOp(
          StackMgr,
          *getTabInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);

    // Memory Instructions
    case OpCode::I32__load:
      return runLoadOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load:
      return runLoadOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F32__load:
      return runLoadOp<float>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F64__load:
      return runLoadOp<double>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__load8_s:
      return runLoadOp<int32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          8);
    case OpCode::I32__load8_u:
      return runLoadOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          8);
    case OpCode::I32__load16_s:
      return runLoadOp<int32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          16);
    case OpCode::I32__load16_u:
      return runLoadOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          16);
    case OpCode::I64__load8_s:
      return runLoadOp<int64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          8);
    case OpCode::I64__load8_u:
      return runLoadOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          8);
    case OpCode::I64__load16_s:
      return runLoadOp<int64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          16);
    case OpCode::I64__load16_u:
      return runLoadOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          16);
    case OpCode::I64__load32_s:
      return runLoadOp<int64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          32);
    case OpCode::I64__load32_u:
      return runLoadOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          32);
    case OpCode::I32__store:
      return runStoreOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__store:
      return runStoreOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F32__store:
      return runStoreOp<float>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F64__store:
      return runStoreOp<double>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__store8:
      return runStoreOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          8);
    case OpCode::I32__store16:
      return runStoreOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          16);
    case OpCode::I64__store8:
      return runStoreOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          8);
    case OpCode::I64__store16:
      return runStoreOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          16);
    case OpCode::I64__store32:
      return runStoreOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          32);
    case OpCode::Memory__grow:
      return runMemoryGrowOp(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()));
    case OpCode::Memory__size:
      return runMemorySizeOp(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()));
    case OpCode::Memory__init:
      return runMemoryInitOp(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()),
          *getDataInstByIdx(StoreMgr, StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Data__drop:
      return runDataDropOp(
          *getDataInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()));
    case OpCode::Memory__copy:
      return runMemoryCopyOp(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()),
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Memory__fill:
      return runMemoryFillOp(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);

    // Const numeric instructions
    case OpCode::I32__const:
      StackMgr.push<uint32_t>(Instr.getNum().get<uint32_t>());
      return {};
    case OpCode::I64__const:
      StackMgr.push<uint64_t>(Instr.getNum().get<uint64_t>());
      return {};
    case OpCode::F32__const:
      StackMgr.push<float>(Instr.getNum().get<float>());
      return {};
    case OpCode::F64__const:
      StackMgr.push<double>(Instr.getNum().get<double>());
      return {};

    // Unary numeric instructions
    case OpCode::I32__eqz:
      return runEqzOp<uint32_t>(StackMgr);
    case OpCode::I64__eqz:
      return runEqzOp<uint64_t>(StackMgr);
    case OpCode::I32__clz:
      return runClzOp<uint32_t>(StackMgr);
    case OpCode::I32__ctz:
      return runCtzOp<uint32_t>(StackMgr);
    case OpCode::I32__popcnt:
      return runPopcntOp<uint32_t>(StackMgr);
    case OpCode::I64__clz:
      return runClzOp<uint64_t>(StackMgr);
    case OpCode::I64__ctz:
      return runCtzOp<uint64_t>(StackMgr);
    case OpCode::I64__popcnt:
      return runPopcntOp<uint64_t>(StackMgr);
    case OpCode::F32__abs:
      return runAbsOp<float>(StackMgr);
    case OpCode::F32__neg:
      return runNegOp<float>(StackMgr);
    case OpCode::F32__ceil:
      return runCeilOp<float>(StackMgr);
    case OpCode::F32__floor:
      return runFloorOp<float>(StackMgr);
    case OpCode::F32__trunc:
      return runTruncOp<float>(StackMgr);
    case OpCode::F32__nearest:
      return runNearestOp<float>(StackMgr);
    case OpCode::F32__sqrt:
      return runSqrtOp<float>(StackMgr);
    case OpCode::F64__abs:
      return runAbsOp<double>(StackMgr);
    case OpCode::F64__neg:
      return runNegOp<double>(StackMgr);
    case OpCode::F64__ceil:
      return runCeilOp<double>(StackMgr);
    case OpCode::F64__floor:
      return runFloorOp<double>(StackMgr);
    case OpCode::F64__trunc:
      return runTruncOp<double>(StackMgr);
    case OpCode::F64__nearest:
      return runNearestOp<double>(StackMgr);
    case OpCode::F64__sqrt:
      return runSqrtOp<double>(StackMgr);
    case OpCode::I32__wrap_i64:
      return runWrapOp<uint64_t, uint32_t>(StackMgr);
    case OpCode::I32__trunc_f32_s:
      return runTruncateOp<float, int32_t>(StackMgr, Instr);
    case OpCode::I32__trunc_f32_u:
      return runTruncateOp<float, uint32_t>(StackMgr, Instr);
    case OpCode::I32__trunc_f64_s:
      return runTruncateOp<double, int32_t>(StackMgr, Instr);
    case OpCode::I32__trunc_f64_u:
      return runTruncateOp<double, uint32_t>(StackMgr, Instr);
    case OpCode::I64__extend_i32_s:
      return runExtendOp<int32_t, uint64_t>(StackMgr);
    case OpCode::I64__extend_i32_u:
      return runExtendOp<uint32_t, uint64_t>(StackMgr);
    case OpCode::I64__trunc_f32_s:
      return runTruncateOp<float, int64_t>(StackMgr, Instr);
    case OpCode::I64__trunc_f32_u:
      return runTruncateOp<float, uint64_t>(StackMgr, Instr);
    case OpCode::I64__trunc_f64_s:
      return runTruncateOp<double, int64_t>(StackMgr, Instr);
    case OpCode::I64__trunc_f64_u:
      return runTruncateOp<double, uint64_t>(StackMgr, Instr);
    case OpCode::F32__convert_i32_s:
      return runConvertOp<int32_t, float>(StackMgr);
    case OpCode::F32__convert_i32_u:
      return runConvertOp<uint32_t, float>(StackMgr);
    case OpCode::F32__convert_i64_s:
      return runConvertOp<int64_t, float>(StackMgr);
    case OpCode::F32__convert_i64_u:
      return runConvertOp<uint64_t, float>(StackMgr);
    case OpCode::F32__demote_f64:
      return runDemoteOp<double, float>(StackMgr);
    case OpCode::F64__convert_i32_s:
      return runConvertOp<int32_t, double>(StackMgr);
    case OpCode::F64__convert_i32_u:
      return runConvertOp<uint32_t, double>(StackMgr);
    case OpCode::F64__convert_i64_s:
      return runConvertOp<int64_t, double>(StackMgr);
    case OpCode::F64__convert_i64_u:
      return runConvertOp<uint64_t, double>(StackMgr);
    case OpCode::F64__promote_f32:
      return runPromoteOp<float, double>(StackMgr);
    case OpCode::I32__reinterpret_f32:
      return runReinterpretOp<float, uint32_t>(StackMgr);
    case OpCode::I64__reinterpret_f64:
      return runReinterpretOp<double, uint64_t>(StackMgr);
    case OpCode::F32__reinterpret_i32:
      return runReinterpretOp<uint32_t, float>(StackMgr);
    case OpCode::F64__reinterpret_i64:
      return runReinterpretOp<uint64_t, double>(StackMgr);
    case OpCode::I32__extend8_s:
      return runExtendOp<int32_t, uint32_t, 8>(StackMgr);
    case OpCode::I32__extend16_s:
      return runExtendOp<int32_t, uint32_t, 16>(StackMgr);
    case OpCode::I64__extend8_s:
      return runExtendOp<int64_t, uint64_t, 8>(StackMgr);
    case OpCode::I64__extend16_s:
      return runExtendOp<int64_t, uint64_t, 16>(StackMgr);
    case OpCode::I64__extend32_s:
      return runExtendOp<int64_t, uint64_t, 32>(StackMgr);
    case OpCode::I32__trunc_sat_f32_s:
      return runTruncateSatOp<float, int32_t>(StackMgr);
    case OpCode::I32__trunc_sat_f32_u:
      return runTruncateSatOp<float, uint32_t>(StackMgr);
    case OpCode::I32__trunc_sat_f64_s:
      return runTruncateSatOp<double, int32_t>(StackMgr);
    case OpCode::I32__trunc_sat_f64_u:
      return runTruncateSatOp<double, uint32_t>(StackMgr);
    case OpCode::I64__trunc_sat_f32_s:
      return runTruncateSatOp<float, int64_t>(StackMgr);
    case OpCode::I64__trunc_sat_f32_u:
      return runTruncateSatOp<float, uint64_t>(StackMgr);
    case OpCode::I64__trunc_sat_f64_s:
      return runTruncateSatOp<double, int64_t>(StackMgr);
    case OpCode::I64__trunc_sat_f64_u:
      return runTruncateSatOp<double, uint64_t>(StackMgr);

      // Binary numeric instructions
    case OpCode::I32__eq:
      return runEqOp<uint32_t>(StackMgr);
    case OpCode::I32__ne:
      return runNeOp<uint32_t>(StackMgr);
    case OpCode::I32__lt_s:
      return runLtOp<int32_t>(StackMgr);
    case OpCode::I32__lt_u:
      return runLtOp<uint32_t>(StackMgr);
    case OpCode::I32__gt_s:
      return runGtOp<int32_t>(StackMgr);
    case OpCode::I32__gt_u:
      return runGtOp<uint32_t>(StackMgr);
    case OpCode::I32__le_s:
      return runLeOp<int32_t>(StackMgr);
    case OpCode::I32__le_u:
      return runLeOp<uint32_t>(StackMgr);
    case OpCode::I32__ge_s:
      return runGeOp<int32_t>(StackMgr);
    case OpCode::I32__ge_u:
      return runGeOp<uint32_t>(StackMgr);
    case OpCode::I64__eq:
      return runEqOp<uint64_t>(StackMgr);
    case OpCode::I64__ne:
      return runNeOp<uint64_t>(StackMgr);
    case OpCode::I64__lt_s:
      return runLtOp<int64_t>(StackMgr);
    case OpCode::I64__lt_u:
      return runLtOp<uint64_t>(StackMgr);
    case OpCode::I64__gt_s:
      return runGtOp<int64_t>(StackMgr);
    case OpCode::I64__gt_u:
      return runGtOp<uint64_t>(StackMgr);
    case OpCode::I64__le_s:
      return runLeOp<int64_t>(StackMgr);
    case OpCode::I64__le_u:
      return runLeOp<uint64_t>(StackMgr);
    case OpCode::I64__ge_s:
      return runGeOp<int64_t>(StackMgr);
    case OpCode::I64__ge_u:
      return runGeOp<uint64_t>(StackMgr);
    case OpCode::F32__eq:
      return runEqOp<float>(StackMgr);
    case OpCode::F32__ne:
      return runNeOp<float>(StackMgr);
    case OpCode::F32__lt:
      return runLtOp<float>(StackMgr);
    case OpCode::F32__gt:
      return runGtOp<float>(StackMgr);
    case OpCode::F32__le:
      return runLeOp<float>(StackMgr);
    case OpCode::F32__ge:
      return runGeOp<float>(StackMgr);
    case OpCode::F64__eq:
      return runEqOp<double>(StackMgr);
    case OpCode::F64__ne:
      return runNeOp<double>(StackMgr);
    case OpCode::F64__lt:
      return runLtOp<double>(StackMgr);
    case OpCode::F64__gt:
      return runGtOp<double>(StackMgr);
    case OpCode::F64__le:
      return runLeOp<double>(StackMgr);
    case OpCode::F64__ge:
      return runGeOp<double>(StackMgr);
    case OpCode::I32__add:
      return runAddOp<uint32_t>(StackMgr);
    case OpCode::I32__sub:
      return runSubOp<uint32_t>(StackMgr);
    case OpCode::I32__mul:
      return runMulOp<uint32_t>(StackMgr);
    case OpCode::I32__div_s:
      return runDivOp<int32_t>(StackMgr, Instr);
    case OpCode::I32__div_u:
      return runDivOp<uint32_t>(StackMgr, Instr);
    case OpCode::I32__rem_s:
      return runRemOp<int32_t>(StackMgr, Instr);
    case OpCode::I32__rem_u:
      return runRemOp<uint32_t>(StackMgr, Instr);
    case OpCode::I32__and:
      return runAndOp<uint32_t>(StackMgr);
    case OpCode::I32__or:
      return runOrOp<uint32_t>(StackMgr);
    case OpCode::I32__xor:
      return runXorOp<uint32_t>(StackMgr);
    case OpCode::I32__shl:
      return runShlOp<uint32_t>(StackMgr);
    case OpCode::I32__shr_s:
      return runShrOp<int32_t>(StackMgr);
    case OpCode::I32__shr_u:
      return runShrOp<uint32_t>(StackMgr);
    case OpCode::I32__rotl:
      return runRotlOp<uint32_t>(StackMgr);
    case OpCode::I32__rotr:
      return runRotrOp<uint32_t>(StackMgr);
    case OpCode::I64__add:
      return runAddOp<uint64_t>(StackMgr);
    case OpCode::I64__sub:
      return runSubOp<uint64_t>(StackMgr);
    case OpCode::I64__mul:
      return runMulOp<uint64_t>(StackMgr);
    case OpCode::I64__div_s:
      return runDivOp<int64_t>(StackMgr, Instr);
    case OpCode::I64__div_u:
      return runDivOp<uint64_t>(StackMgr, Instr);
    case OpCode::I64__rem_s:
      return runRemOp<int64_t>(StackMgr, Instr);
    case OpCode::I64__rem_u:
      return runRemOp<uint64_t>(StackMgr, Instr);
    case OpCode::I64__and:
      return runAndOp<uint64_t>(StackMgr);
    case OpCode::I64__or:
      return runOrOp<uint64_t>(StackMgr);
    case OpCode::I64__xor:
      return runXorOp<uint64_t>(StackMgr);
    case OpCode::I64__shl:
      return runShlOp<uint64_t>(StackMgr);
    case OpCode::I64__shr_s:
      return runShrOp<int64_t>(StackMgr);
    case OpCode::I64__shr_u:
      return runShrOp<uint64_t>(StackMgr);
    case OpCode::I64__rotl:
      return runRotlOp<uint64_t>(StackMgr);
    case OpCode::I64__rotr:
      return runRotrOp<uint64_t>(StackMgr);
    case OpCode::F32__add:
      return runAddOp<float>(StackMgr);
    case OpCode::F32__sub:
      return runSubOp<float>(StackMgr);
    case OpCode::F32__mul:
      return runMulOp<float>(StackMgr);
    case OpCode::F32__div:
      return runDivOp<float>(StackMgr, Instr);
    case OpCode::F32__min:
      return runMinOp<float>(StackMgr);
    case OpCode::F32__max:
      return runMaxOp<float>(StackMgr);
    case OpCode::F32__copysign:
      return runCopysignOp<float>(StackMgr);
    case OpCode::F64__add:
      return runAddOp<double>(StackMgr);
    case OpCode::F64__sub:
      return runSubOp<double>(StackMgr);
    case OpCode::F64__mul:
      return runMulOp<double>(StackMgr);
    case OpCode::F64__div:
      return runDivOp<double>(StackMgr, Instr);
    case OpCode::F64__min:
      return runMinOp<double>(StackMgr);
    case OpCode::F64__max:
      return runMaxOp<double>(StackMgr);
    case OpCode::F64__copysign:
      return runCopysignOp<double>(StackMgr);

    // SIMD Memory Instructions
    case OpCode::V128__load:
      return runLoadOp<uint128_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8x8_s:
      return runLoadExpandOp<int8_t, int16_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8x8_u:
      return runLoadExpandOp<uint8_t, uint16_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16x4_s:
      return runLoadExpandOp<int16_t, int32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16x4_u:
      return runLoadExpandOp<uint16_t, uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32x2_s:
      return runLoadExpandOp<int32_t, int64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32x2_u:
      return runLoadExpandOp<uint32_t, uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8_splat:
      return runLoadSplatOp<uint8_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16_splat:
      return runLoadSplatOp<uint16_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32_splat:
      return runLoadSplatOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load64_splat:
      return runLoadSplatOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32_zero:
      return runLoadOp<uint128_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          32);
    case OpCode::V128__load64_zero:
      return runLoadOp<uint128_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr,
          64);
    case OpCode::V128__store:
      return runStoreOp<uint128_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8_lane:
      return runLoadLaneOp<uint8_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16_lane:
      return runLoadLaneOp<uint16_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32_lane:
      return runLoadLaneOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load64_lane:
      return runLoadLaneOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store8_lane:
      return runStoreLaneOp<uint8_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store16_lane:
      return runStoreLaneOp<uint16_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store32_lane:
      return runStoreLaneOp<uint32_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store64_lane:
      return runStoreLaneOp<uint64_t>(
          StackMgr,
          *getMemInstByIdx(StoreMgr, StackMgr, Instr.getTargetIndex()), Instr);

    // SIMD Const Instructions
    case OpCode::V128__const:
      StackMgr.push<uint128_t>(Instr.getNum().get<uint128_t>());
      return {};

    // SIMD Shuffle Instructions
    case OpCode::I8x16__shuffle: {
      const uint128_t V2 = StackMgr.pop<uint128_t>();
      const uint128_t V1 = StackMgr.pop<uint128_t>();
      const uint128_t V3 = Instr.getNum().get<uint128_t>();
      std::array<uint8_t, 32> Data;
      std::array<uint8_t, 16> Result;
      std::memcpy(&Data[0], &V1, 16);
      std::memcpy(&Data[16], &V2, 16);
      for (size_t I = 0; I < 16; ++I) {
        const uint8_t Index = static_cast<uint8_t>(V3 >> (I * 8)) % 32;
        Result[I] = Data[Index];
      }
      uint128_t R;
      std::memcpy(&R, &Result[0], 16);
      StackMgr.push<uint128_t>(R);
      return {};
    }

    // SIMD Lane Instructions
    case OpCode::I8x16__extract_lane_s:
      return runExtractLaneOp<int8_t, int32_t>(StackMgr, Instr.getMemoryLane());
    case OpCode::I8x16__extract_lane_u:
      return runExtractLaneOp<uint8_t, uint32_t>(StackMgr,
                                                 Instr.getMemoryLane());
    case OpCode::I16x8__extract_lane_s:
      return runExtractLaneOp<int16_t, int32_t>(StackMgr,
                                                Instr.getMemoryLane());
    case OpCode::I16x8__extract_lane_u:
      return runExtractLaneOp<uint16_t, uint32_t>(StackMgr,
                                                  Instr.getMemoryLane());
    case OpCode::I32x4__extract_lane:
      return runExtractLaneOp<uint32_t>(StackMgr, Instr.getMemoryLane());
    case OpCode::I64x2__extract_lane:
      return runExtractLaneOp<uint64_t>(StackMgr, Instr.getMemoryLane());
    case OpCode::F32x4__extract_lane:
      return runExtractLaneOp<float>(StackMgr, Instr.getMemoryLane());
    case OpCode::F64x2__extract_lane:
      return runExtractLaneOp<double>(StackMgr, Instr.getMemoryLane());
    case OpCode::I8x16__replace_lane:
      return runReplaceLaneOp<uint32_t, uint8_t>(StackMgr,
                                                 Instr.getMemoryLane());
    case OpCode::I16x8__replace_lane:
      return runReplaceLaneOp<uint32_t, uint16_t>(StackMgr,
                                                  Instr.getMemoryLane());
    case OpCode::I32x4__replace_lane:
      return runReplaceLaneOp<uint32_t>(StackMgr, Instr.getMemoryLane());
    case OpCode::I64x2__replace_lane:
      return runReplaceLaneOp<uint64_t>(StackMgr, Instr.getMemoryLane());
    case OpCode::F32x4__replace_lane:
      return runReplaceLaneOp<float>(StackMgr, Instr.getMemoryLane());
    case OpCode::F64x2__replace_lane:
      return runReplaceLaneOp<double>(StackMgr, Instr.getMemoryLane());

      // SIMD Numeric Instructions
    case OpCode::I8x16__swizzle: {
      const uint8x16_t Index = StackMgr.pop<uint8x16_t>();
      const uint8x16_t Vector = StackMgr.pop<uint8x16_t>();
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
      StackMgr.push<uint8x16_t>(detail::vectorSelect(Exceed, Zero, Result));
      return {};
    }
    case OpCode::I8x16__splat:
      return runSplatOp<uint32_t, uint8_t>(StackMgr);
    case OpCode::I16x8__splat:
      return runSplatOp<uint32_t, uint16_t>(StackMgr);
    case OpCode::I32x4__splat:
      return runSplatOp<uint32_t>(StackMgr);
    case OpCode::I64x2__splat:
      return runSplatOp<uint64_t>(StackMgr);
    case OpCode::F32x4__splat:
      return runSplatOp<float>(StackMgr);
    case OpCode::F64x2__splat:
      return runSplatOp<double>(StackMgr);
    case OpCode::I8x16__eq:
      return runVectorEqOp<uint8_t>(StackMgr);
    case OpCode::I8x16__ne:
      return runVectorNeOp<uint8_t>(StackMgr);
    case OpCode::I8x16__lt_s:
      return runVectorLtOp<int8_t>(StackMgr);
    case OpCode::I8x16__lt_u:
      return runVectorLtOp<uint8_t>(StackMgr);
    case OpCode::I8x16__gt_s:
      return runVectorGtOp<int8_t>(StackMgr);
    case OpCode::I8x16__gt_u:
      return runVectorGtOp<uint8_t>(StackMgr);
    case OpCode::I8x16__le_s:
      return runVectorLeOp<int8_t>(StackMgr);
    case OpCode::I8x16__le_u:
      return runVectorLeOp<uint8_t>(StackMgr);
    case OpCode::I8x16__ge_s:
      return runVectorGeOp<int8_t>(StackMgr);
    case OpCode::I8x16__ge_u:
      return runVectorGeOp<uint8_t>(StackMgr);
    case OpCode::I16x8__eq:
      return runVectorEqOp<uint16_t>(StackMgr);
    case OpCode::I16x8__ne:
      return runVectorNeOp<uint16_t>(StackMgr);
    case OpCode::I16x8__lt_s:
      return runVectorLtOp<int16_t>(StackMgr);
    case OpCode::I16x8__lt_u:
      return runVectorLtOp<uint16_t>(StackMgr);
    case OpCode::I16x8__gt_s:
      return runVectorGtOp<int16_t>(StackMgr);
    case OpCode::I16x8__gt_u:
      return runVectorGtOp<uint16_t>(StackMgr);
    case OpCode::I16x8__le_s:
      return runVectorLeOp<int16_t>(StackMgr);
    case OpCode::I16x8__le_u:
      return runVectorLeOp<uint16_t>(StackMgr);
    case OpCode::I16x8__ge_s:
      return runVectorGeOp<int16_t>(StackMgr);
    case OpCode::I16x8__ge_u:
      return runVectorGeOp<uint16_t>(StackMgr);
    case OpCode::I32x4__eq:
      return runVectorEqOp<uint32_t>(StackMgr);
    case OpCode::I32x4__ne:
      return runVectorNeOp<uint32_t>(StackMgr);
    case OpCode::I32x4__lt_s:
      return runVectorLtOp<int32_t>(StackMgr);
    case OpCode::I32x4__lt_u:
      return runVectorLtOp<uint32_t>(StackMgr);
    case OpCode::I32x4__gt_s:
      return runVectorGtOp<int32_t>(StackMgr);
    case OpCode::I32x4__gt_u:
      return runVectorGtOp<uint32_t>(StackMgr);
    case OpCode::I32x4__le_s:
      return runVectorLeOp<int32_t>(StackMgr);
    case OpCode::I32x4__le_u:
      return runVectorLeOp<uint32_t>(StackMgr);
    case OpCode::I32x4__ge_s:
      return runVectorGeOp<int32_t>(StackMgr);
    case OpCode::I32x4__ge_u:
      return runVectorGeOp<uint32_t>(StackMgr);
    case OpCode::I64x2__eq:
      return runVectorEqOp<uint64_t>(StackMgr);
    case OpCode::I64x2__ne:
      return runVectorNeOp<uint64_t>(StackMgr);
    case OpCode::I64x2__lt_s:
      return runVectorLtOp<int64_t>(StackMgr);
    case OpCode::I64x2__gt_s:
      return runVectorGtOp<int64_t>(StackMgr);
    case OpCode::I64x2__le_s:
      return runVectorLeOp<int64_t>(StackMgr);
    case OpCode::I64x2__ge_s:
      return runVectorGeOp<int64_t>(StackMgr);
    case OpCode::F32x4__eq:
      return runVectorEqOp<float>(StackMgr);
    case OpCode::F32x4__ne:
      return runVectorNeOp<float>(StackMgr);
    case OpCode::F32x4__lt:
      return runVectorLtOp<float>(StackMgr);
    case OpCode::F32x4__gt:
      return runVectorGtOp<float>(StackMgr);
    case OpCode::F32x4__le:
      return runVectorLeOp<float>(StackMgr);
    case OpCode::F32x4__ge:
      return runVectorGeOp<float>(StackMgr);
    case OpCode::F64x2__eq:
      return runVectorEqOp<double>(StackMgr);
    case OpCode::F64x2__ne:
      return runVectorNeOp<double>(StackMgr);
    case OpCode::F64x2__lt:
      return runVectorLtOp<double>(StackMgr);
    case OpCode::F64x2__gt:
      return runVectorGtOp<double>(StackMgr);
    case OpCode::F64x2__le:
      return runVectorLeOp<double>(StackMgr);
    case OpCode::F64x2__ge:
      return runVectorGeOp<double>(StackMgr);

    case OpCode::V128__not: {
      const uint64x2_t V = StackMgr.pop<uint64x2_t>();
      StackMgr.push<uint64x2_t>(~V);
      return {};
    }
    case OpCode::V128__and: {
      const uint64x2_t V2 = StackMgr.pop<uint64x2_t>();
      const uint64x2_t V1 = StackMgr.pop<uint64x2_t>();
      StackMgr.push<uint64x2_t>(V1 & V2);
      return {};
    }
    case OpCode::V128__andnot: {
      const uint64x2_t V2 = StackMgr.pop<uint64x2_t>();
      const uint64x2_t V1 = StackMgr.pop<uint64x2_t>();
      StackMgr.push<uint64x2_t>(V1 & ~V2);
      return {};
    }
    case OpCode::V128__or: {
      const uint64x2_t V2 = StackMgr.pop<uint64x2_t>();
      const uint64x2_t V1 = StackMgr.pop<uint64x2_t>();
      StackMgr.push<uint64x2_t>(V1 | V2);
      return {};
    }
    case OpCode::V128__xor: {
      const uint64x2_t V2 = StackMgr.pop<uint64x2_t>();
      const uint64x2_t V1 = StackMgr.pop<uint64x2_t>();
      StackMgr.push<uint64x2_t>(V1 ^ V2);
      return {};
    }
    case OpCode::V128__bitselect: {
      const uint64x2_t C = StackMgr.pop<uint64x2_t>();
      const uint64x2_t V2 = StackMgr.pop<uint64x2_t>();
      const uint64x2_t V1 = StackMgr.pop<uint64x2_t>();
      StackMgr.push<uint64x2_t>((V1 & C) | (V2 & ~C));
      return {};
    }
    case OpCode::V128__any_true:
      return runVectorAnyTrueOp(StackMgr);

    case OpCode::I8x16__abs:
      return runVectorAbsOp<int8_t>(StackMgr);
    case OpCode::I8x16__neg:
      return runVectorNegOp<int8_t>(StackMgr);
    case OpCode::I8x16__popcnt:
      return runVectorPopcntOp(StackMgr);
    case OpCode::I8x16__all_true:
      return runVectorAllTrueOp<uint8_t>(StackMgr);
    case OpCode::I8x16__bitmask:
      return runVectorBitMaskOp<uint8_t>(StackMgr);
    case OpCode::I8x16__narrow_i16x8_s:
      return runVectorNarrowOp<int16_t, int8_t>(StackMgr);
    case OpCode::I8x16__narrow_i16x8_u:
      return runVectorNarrowOp<int16_t, uint8_t>(StackMgr);
    case OpCode::I8x16__shl:
      return runVectorShlOp<uint8_t>(StackMgr);
    case OpCode::I8x16__shr_s:
      return runVectorShrOp<int8_t>(StackMgr);
    case OpCode::I8x16__shr_u:
      return runVectorShrOp<uint8_t>(StackMgr);
    case OpCode::I8x16__add:
      return runVectorAddOp<uint8_t>(StackMgr);
    case OpCode::I8x16__add_sat_s:
      return runVectorAddSatOp<int8_t>(StackMgr);
    case OpCode::I8x16__add_sat_u:
      return runVectorAddSatOp<uint8_t>(StackMgr);
    case OpCode::I8x16__sub:
      return runVectorSubOp<uint8_t>(StackMgr);
    case OpCode::I8x16__sub_sat_s:
      return runVectorSubSatOp<int8_t>(StackMgr);
    case OpCode::I8x16__sub_sat_u:
      return runVectorSubSatOp<uint8_t>(StackMgr);
    case OpCode::I8x16__min_s:
      return runVectorMinOp<int8_t>(StackMgr);
    case OpCode::I8x16__min_u:
      return runVectorMinOp<uint8_t>(StackMgr);
    case OpCode::I8x16__max_s:
      return runVectorMaxOp<int8_t>(StackMgr);
    case OpCode::I8x16__max_u:
      return runVectorMaxOp<uint8_t>(StackMgr);
    case OpCode::I8x16__avgr_u:
      return runVectorAvgrOp<uint8_t, uint16_t>(StackMgr);

    case OpCode::I16x8__abs:
      return runVectorAbsOp<int16_t>(StackMgr);
    case OpCode::I16x8__neg:
      return runVectorNegOp<int16_t>(StackMgr);
    case OpCode::I16x8__all_true:
      return runVectorAllTrueOp<uint16_t>(StackMgr);
    case OpCode::I16x8__bitmask:
      return runVectorBitMaskOp<uint16_t>(StackMgr);
    case OpCode::I16x8__narrow_i32x4_s:
      return runVectorNarrowOp<int32_t, int16_t>(StackMgr);
    case OpCode::I16x8__narrow_i32x4_u:
      return runVectorNarrowOp<int32_t, uint16_t>(StackMgr);
    case OpCode::I16x8__extend_low_i8x16_s:
      return runVectorExtendLowOp<int8_t, int16_t>(StackMgr);
    case OpCode::I16x8__extend_high_i8x16_s:
      return runVectorExtendHighOp<int8_t, int16_t>(StackMgr);
    case OpCode::I16x8__extend_low_i8x16_u:
      return runVectorExtendLowOp<uint8_t, uint16_t>(StackMgr);
    case OpCode::I16x8__extend_high_i8x16_u:
      return runVectorExtendHighOp<uint8_t, uint16_t>(StackMgr);
    case OpCode::I16x8__shl:
      return runVectorShlOp<uint16_t>(StackMgr);
    case OpCode::I16x8__shr_s:
      return runVectorShrOp<int16_t>(StackMgr);
    case OpCode::I16x8__shr_u:
      return runVectorShrOp<uint16_t>(StackMgr);
    case OpCode::I16x8__add:
      return runVectorAddOp<uint16_t>(StackMgr);
    case OpCode::I16x8__add_sat_s:
      return runVectorAddSatOp<int16_t>(StackMgr);
    case OpCode::I16x8__add_sat_u:
      return runVectorAddSatOp<uint16_t>(StackMgr);
    case OpCode::I16x8__sub:
      return runVectorSubOp<uint16_t>(StackMgr);
    case OpCode::I16x8__sub_sat_s:
      return runVectorSubSatOp<int16_t>(StackMgr);
    case OpCode::I16x8__sub_sat_u:
      return runVectorSubSatOp<uint16_t>(StackMgr);
    case OpCode::I16x8__mul:
      return runVectorMulOp<uint16_t>(StackMgr);
    case OpCode::I16x8__min_s:
      return runVectorMinOp<int16_t>(StackMgr);
    case OpCode::I16x8__min_u:
      return runVectorMinOp<uint16_t>(StackMgr);
    case OpCode::I16x8__max_s:
      return runVectorMaxOp<int16_t>(StackMgr);
    case OpCode::I16x8__max_u:
      return runVectorMaxOp<uint16_t>(StackMgr);
    case OpCode::I16x8__avgr_u:
      return runVectorAvgrOp<uint16_t, uint32_t>(StackMgr);
    case OpCode::I16x8__extmul_low_i8x16_s:
      return runVectorExtMulLowOp<int8_t, int16_t>(StackMgr);
    case OpCode::I16x8__extmul_high_i8x16_s:
      return runVectorExtMulHighOp<int8_t, int16_t>(StackMgr);
    case OpCode::I16x8__extmul_low_i8x16_u:
      return runVectorExtMulLowOp<uint8_t, uint16_t>(StackMgr);
    case OpCode::I16x8__extmul_high_i8x16_u:
      return runVectorExtMulHighOp<uint8_t, uint16_t>(StackMgr);
    case OpCode::I16x8__q15mulr_sat_s:
      return runVectorQ15MulSatOp(StackMgr);
    case OpCode::I16x8__extadd_pairwise_i8x16_s:
      return runVectorExtAddPairwiseOp<int8_t, int16_t>(StackMgr);
    case OpCode::I16x8__extadd_pairwise_i8x16_u:
      return runVectorExtAddPairwiseOp<uint8_t, uint16_t>(StackMgr);

    case OpCode::I32x4__abs:
      return runVectorAbsOp<int32_t>(StackMgr);
    case OpCode::I32x4__neg:
      return runVectorNegOp<int32_t>(StackMgr);
    case OpCode::I32x4__all_true:
      return runVectorAllTrueOp<uint32_t>(StackMgr);
    case OpCode::I32x4__bitmask:
      return runVectorBitMaskOp<uint32_t>(StackMgr);
    case OpCode::I32x4__extend_low_i16x8_s:
      return runVectorExtendLowOp<int16_t, int32_t>(StackMgr);
    case OpCode::I32x4__extend_high_i16x8_s:
      return runVectorExtendHighOp<int16_t, int32_t>(StackMgr);
    case OpCode::I32x4__extend_low_i16x8_u:
      return runVectorExtendLowOp<uint16_t, uint32_t>(StackMgr);
    case OpCode::I32x4__extend_high_i16x8_u:
      return runVectorExtendHighOp<uint16_t, uint32_t>(StackMgr);
    case OpCode::I32x4__shl:
      return runVectorShlOp<uint32_t>(StackMgr);
    case OpCode::I32x4__shr_s:
      return runVectorShrOp<int32_t>(StackMgr);
    case OpCode::I32x4__shr_u:
      return runVectorShrOp<uint32_t>(StackMgr);
    case OpCode::I32x4__add:
      return runVectorAddOp<uint32_t>(StackMgr);
    case OpCode::I32x4__sub:
      return runVectorSubOp<uint32_t>(StackMgr);
    case OpCode::I32x4__mul:
      return runVectorMulOp<uint32_t>(StackMgr);
    case OpCode::I32x4__min_s:
      return runVectorMinOp<int32_t>(StackMgr);
    case OpCode::I32x4__min_u:
      return runVectorMinOp<uint32_t>(StackMgr);
    case OpCode::I32x4__max_s:
      return runVectorMaxOp<int32_t>(StackMgr);
    case OpCode::I32x4__max_u:
      return runVectorMaxOp<uint32_t>(StackMgr);
    case OpCode::I32x4__extmul_low_i16x8_s:
      return runVectorExtMulLowOp<int16_t, int32_t>(StackMgr);
    case OpCode::I32x4__extmul_high_i16x8_s:
      return runVectorExtMulHighOp<int16_t, int32_t>(StackMgr);
    case OpCode::I32x4__extmul_low_i16x8_u:
      return runVectorExtMulLowOp<uint16_t, uint32_t>(StackMgr);
    case OpCode::I32x4__extmul_high_i16x8_u:
      return runVectorExtMulHighOp<uint16_t, uint32_t>(StackMgr);
    case OpCode::I32x4__extadd_pairwise_i16x8_s:
      return runVectorExtAddPairwiseOp<int16_t, int32_t>(StackMgr);
    case OpCode::I32x4__extadd_pairwise_i16x8_u:
      return runVectorExtAddPairwiseOp<uint16_t, uint32_t>(StackMgr);

    case OpCode::I64x2__abs:
      return runVectorAbsOp<int64_t>(StackMgr);
    case OpCode::I64x2__neg:
      return runVectorNegOp<int64_t>(StackMgr);
    case OpCode::I64x2__all_true:
      return runVectorAllTrueOp<uint64_t>(StackMgr);
    case OpCode::I64x2__bitmask:
      return runVectorBitMaskOp<uint64_t>(StackMgr);
    case OpCode::I64x2__extend_low_i32x4_s:
      return runVectorExtendLowOp<int32_t, int64_t>(StackMgr);
    case OpCode::I64x2__extend_high_i32x4_s:
      return runVectorExtendHighOp<int32_t, int64_t>(StackMgr);
    case OpCode::I64x2__extend_low_i32x4_u:
      return runVectorExtendLowOp<uint32_t, uint64_t>(StackMgr);
    case OpCode::I64x2__extend_high_i32x4_u:
      return runVectorExtendHighOp<uint32_t, uint64_t>(StackMgr);
    case OpCode::I64x2__shl:
      return runVectorShlOp<uint64_t>(StackMgr);
    case OpCode::I64x2__shr_s:
      return runVectorShrOp<int64_t>(StackMgr);
    case OpCode::I64x2__shr_u:
      return runVectorShrOp<uint64_t>(StackMgr);
    case OpCode::I64x2__add:
      return runVectorAddOp<uint64_t>(StackMgr);
    case OpCode::I64x2__sub:
      return runVectorSubOp<uint64_t>(StackMgr);
    case OpCode::I64x2__mul:
      return runVectorMulOp<uint64_t>(StackMgr);
    case OpCode::I64x2__extmul_low_i32x4_s:
      return runVectorExtMulLowOp<int32_t, int64_t>(StackMgr);
    case OpCode::I64x2__extmul_high_i32x4_s:
      return runVectorExtMulHighOp<int32_t, int64_t>(StackMgr);
    case OpCode::I64x2__extmul_low_i32x4_u:
      return runVectorExtMulLowOp<uint32_t, uint64_t>(StackMgr);
    case OpCode::I64x2__extmul_high_i32x4_u:
      return runVectorExtMulHighOp<uint32_t, uint64_t>(StackMgr);

    case OpCode::F32x4__abs:
      return runVectorAbsOp<float>(StackMgr);
    case OpCode::F32x4__neg:
      return runVectorNegOp<float>(StackMgr);
    case OpCode::F32x4__sqrt:
      return runVectorSqrtOp<float>(StackMgr);
    case OpCode::F32x4__add:
      return runVectorAddOp<float>(StackMgr);
    case OpCode::F32x4__sub:
      return runVectorSubOp<float>(StackMgr);
    case OpCode::F32x4__mul:
      return runVectorMulOp<float>(StackMgr);
    case OpCode::F32x4__div:
      return runVectorDivOp<float>(StackMgr);
    case OpCode::F32x4__min:
      return runVectorFMinOp<float>(StackMgr);
    case OpCode::F32x4__max:
      return runVectorFMaxOp<float>(StackMgr);
    case OpCode::F32x4__pmin:
      return runVectorMinOp<float>(StackMgr);
    case OpCode::F32x4__pmax:
      return runVectorMaxOp<float>(StackMgr);

    case OpCode::F64x2__abs:
      return runVectorAbsOp<double>(StackMgr);
    case OpCode::F64x2__neg:
      return runVectorNegOp<double>(StackMgr);
    case OpCode::F64x2__sqrt:
      return runVectorSqrtOp<double>(StackMgr);
    case OpCode::F64x2__add:
      return runVectorAddOp<double>(StackMgr);
    case OpCode::F64x2__sub:
      return runVectorSubOp<double>(StackMgr);
    case OpCode::F64x2__mul:
      return runVectorMulOp<double>(StackMgr);
    case OpCode::F64x2__div:
      return runVectorDivOp<double>(StackMgr);
    case OpCode::F64x2__min:
      return runVectorFMinOp<double>(StackMgr);
    case OpCode::F64x2__max:
      return runVectorFMaxOp<double>(StackMgr);
    case OpCode::F64x2__pmin:
      return runVectorMinOp<double>(StackMgr);
    case OpCode::F64x2__pmax:
      return runVectorMaxOp<double>(StackMgr);

    case OpCode::I32x4__trunc_sat_f32x4_s:
      return runVectorTruncSatOp<float, int32_t>(StackMgr);
    case OpCode::I32x4__trunc_sat_f32x4_u:
      return runVectorTruncSatOp<float, uint32_t>(StackMgr);
    case OpCode::F32x4__convert_i32x4_s:
      return runVectorConvertOp<int32_t, float>(StackMgr);
    case OpCode::F32x4__convert_i32x4_u:
      return runVectorConvertOp<uint32_t, float>(StackMgr);
    case OpCode::I32x4__trunc_sat_f64x2_s_zero:
      return runVectorTruncSatOp<double, int32_t>(StackMgr);
    case OpCode::I32x4__trunc_sat_f64x2_u_zero:
      return runVectorTruncSatOp<double, uint32_t>(StackMgr);
    case OpCode::F64x2__convert_low_i32x4_s:
      return runVectorConvertOp<int32_t, double>(StackMgr);
    case OpCode::F64x2__convert_low_i32x4_u:
      return runVectorConvertOp<uint32_t, double>(StackMgr);
    case OpCode::F32x4__demote_f64x2_zero:
      return runVectorDemoteOp(StackMgr);
    case OpCode::F64x2__promote_low_f32x4:
      return runVectorPromoteOp(StackMgr);

    case OpCode::I32x4__dot_i16x8_s: {
      using int32x8_t [[gnu::vector_size(32)]] = int32_t;
      const auto V2 = StackMgr.pop<int16x8_t>();
      const auto V1 = StackMgr.pop<int16x8_t>();
      const auto M = __builtin_convertvector(V1, int32x8_t) *
                     __builtin_convertvector(V2, int32x8_t);
      const int32x4_t L = {M[0], M[2], M[4], M[6]};
      const int32x4_t R = {M[1], M[3], M[5], M[7]};
      StackMgr.push<int32x4_t>(L + R);
      return {};
    }
    case OpCode::F32x4__ceil:
      return runVectorCeilOp<float>(StackMgr);
    case OpCode::F32x4__floor:
      return runVectorFloorOp<float>(StackMgr);
    case OpCode::F32x4__trunc:
      return runVectorTruncOp<float>(StackMgr);
    case OpCode::F32x4__nearest:
      return runVectorNearestOp<float>(StackMgr);
    case OpCode::F64x2__ceil:
      return runVectorCeilOp<double>(StackMgr);
    case OpCode::F64x2__floor:
      return runVectorFloorOp<double>(StackMgr);
    case OpCode::F64x2__trunc:
      return runVectorTruncOp<double>(StackMgr);
    case OpCode::F64x2__nearest:
      return runVectorNearestOp<double>(StackMgr);

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
      // Add cost. Note: if-else case should be processed additionally.
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
