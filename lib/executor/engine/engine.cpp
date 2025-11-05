// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/endian.h"
#include "executor/coredump.h"
#include "executor/executor.h"
#include "system/stacktrace.h"

#include <array>
#include <cstdint>
#include <cstring>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runExpression(Runtime::StackManager &StackMgr,
                                     AST::InstrView Instrs) {
  return execute(StackMgr, Instrs.begin(), Instrs.end());
}

Expect<void>
Executor::runFunction(Runtime::StackManager &StackMgr,
                      const Runtime::Instance::FunctionInstance &Func,
                      Span<const ValVariant> Params) {
  // Set start time.
  if (Stat && Conf.getStatisticsConfigure().isTimeMeasuring()) {
    Stat->startRecordWasm();
  }

  // Reset and push a dummy frame into stack.
  StackMgr.pushFrame(nullptr, AST::InstrView::iterator());

  // Push arguments.
  const auto &PTypes = Func.getFuncType().getParamTypes();
  for (uint32_t I = 0; I < Params.size(); I++) {
    // For the references, transform to non-null reference type if the value not
    // null.
    if (PTypes[I].isRefType() && Params[I].get<RefVariant>().getPtr<void>() &&
        Params[I].get<RefVariant>().getType().isNullableRefType()) {
      auto Val = Params[I];
      Val.get<RefVariant>().getType().toNonNullableRef();
      StackMgr.push(Val);
    } else {
      StackMgr.push(Params[I]);
    }
  }

  // Enter and execute function.
  Expect<void> Res =
      enterFunction(StackMgr, Func, Func.getInstrs().end())
          .and_then([&](AST::InstrView::iterator StartIt) {
            // If not terminated, execute the instructions in interpreter mode.
            // For the entering AOT or host functions, the `StartIt` is equal to
            // the end of instruction list, therefore the execution will return
            // immediately.
            return execute(StackMgr, StartIt, Func.getInstrs().end());
          });

  if (Res) {
    spdlog::debug(" Execution succeeded."sv);
  } else if (likely(Res.error() == ErrCode::Value::Terminated)) {
    spdlog::debug(" Terminated."sv);
  }

  if (Stat && Conf.getStatisticsConfigure().isTimeMeasuring()) {
    Stat->stopRecordWasm();
  }

  // If Statistics is enabled, then dump it here.
  if (Stat) {
    Stat->dumpToLog(Conf);
  }

  if (!Res && likely(Res.error() == ErrCode::Value::Terminated)) {
    StackMgr.reset();
  }

  return Res;
}

Expect<void> Executor::execute(Runtime::StackManager &StackMgr,
                               const AST::InstrView::iterator Start,
                               const AST::InstrView::iterator End) {
  AST::InstrView::iterator PC = Start;
  AST::InstrView::iterator PCEnd = End;

  auto Dispatch = [this, &PC, &StackMgr]() -> Expect<void> {
    const AST::Instruction &Instr = *PC;
    switch (Instr.getOpCode()) {
    // Control instructions
    case OpCode::Unreachable:
      spdlog::error(ErrCode::Value::Unreachable);
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Value::Unreachable);
    case OpCode::Nop:
      return {};
    case OpCode::Block:
      return {};
    case OpCode::Loop:
      return {};
    case OpCode::If:
      return runIfElseOp(StackMgr, Instr, PC);
    case OpCode::Else:
      if (Stat && Conf.getStatisticsConfigure().isCostMeasuring()) {
        // Reach here means end of if-statement.
        if (unlikely(!Stat->subInstrCost(Instr.getOpCode()))) {
          spdlog::error(ErrCode::Value::CostLimitExceeded);
          spdlog::error(
              ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
          return Unexpect(ErrCode::Value::CostLimitExceeded);
        }
        if (unlikely(!Stat->addInstrCost(OpCode::End))) {
          spdlog::error(ErrCode::Value::CostLimitExceeded);
          spdlog::error(
              ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
          return Unexpect(ErrCode::Value::CostLimitExceeded);
        }
      }
      PC += PC->getJumpEnd() - 1;
      return {};
    case OpCode::End:
      PC = StackMgr.maybePopFrameOrHandler(PC);
      return {};
    // LEGACY-EH: remove the `Try` cases after deprecating legacy EH.
    case OpCode::Try:
      return runTryTableOp(StackMgr, Instr, PC);
    case OpCode::Throw:
      return runThrowOp(StackMgr, Instr, PC);
    case OpCode::Throw_ref:
      return runThrowRefOp(StackMgr, Instr, PC);
    case OpCode::Br:
      return runBrOp(StackMgr, Instr, PC);
    case OpCode::Br_if:
      return runBrIfOp(StackMgr, Instr, PC);
    case OpCode::Br_table:
      return runBrTableOp(StackMgr, Instr, PC);
    case OpCode::Br_on_null:
      return runBrOnNullOp(StackMgr, Instr, PC);
    case OpCode::Br_on_non_null:
      return runBrOnNonNullOp(StackMgr, Instr, PC);
    case OpCode::Br_on_cast:
      return runBrOnCastOp(StackMgr, Instr, PC);
    case OpCode::Br_on_cast_fail:
      return runBrOnCastOp(StackMgr, Instr, PC, true);
    case OpCode::Return:
      return runReturnOp(StackMgr, PC);
    case OpCode::Call:
      return runCallOp(StackMgr, Instr, PC);
    case OpCode::Call_indirect:
      return runCallIndirectOp(StackMgr, Instr, PC);
    case OpCode::Return_call:
      return runCallOp(StackMgr, Instr, PC, true);
    case OpCode::Return_call_indirect:
      return runCallIndirectOp(StackMgr, Instr, PC, true);
    case OpCode::Call_ref:
      return runCallRefOp(StackMgr, Instr, PC);
    case OpCode::Return_call_ref:
      return runCallRefOp(StackMgr, Instr, PC, true);
    // LEGACY-EH: remove the `Catch` cases after deprecating legacy EH.
    case OpCode::Catch:
    case OpCode::Catch_all:
      PC -= Instr.getCatchLegacy().CatchPCOffset;
      PC += PC->getTryCatch().JumpEnd;
      return {};
    case OpCode::Try_table:
      return runTryTableOp(StackMgr, Instr, PC);

    // Reference Instructions
    case OpCode::Ref__null:
      return runRefNullOp(StackMgr, Instr.getValType());
    case OpCode::Ref__is_null:
      return runRefIsNullOp(StackMgr);
    case OpCode::Ref__func:
      return runRefFuncOp(StackMgr, Instr.getTargetIndex());
    case OpCode::Ref__eq:
      return runRefEqOp(StackMgr);
    case OpCode::Ref__as_non_null:
      return runRefAsNonNullOp(StackMgr, Instr);

    // Reference Instructions (GC proposal)
    case OpCode::Struct__new:
      return runStructNewOp(StackMgr, Instr.getTargetIndex());
    case OpCode::Struct__new_default:
      return runStructNewOp(StackMgr, Instr.getTargetIndex(), true);
    case OpCode::Struct__get:
    case OpCode::Struct__get_u:
      return runStructGetOp(StackMgr, Instr.getTargetIndex(),
                            Instr.getSourceIndex(), Instr);
    case OpCode::Struct__get_s:
      return runStructGetOp(StackMgr, Instr.getTargetIndex(),
                            Instr.getSourceIndex(), Instr, true);
    case OpCode::Struct__set:
      return runStructSetOp(StackMgr, Instr.getTargetIndex(),
                            Instr.getSourceIndex(), Instr);
    case OpCode::Array__new:
      return runArrayNewOp(StackMgr, Instr.getTargetIndex(), 1,
                           StackMgr.pop<uint32_t>());
    case OpCode::Array__new_default:
      return runArrayNewOp(StackMgr, Instr.getTargetIndex(), 0,
                           StackMgr.pop<uint32_t>());
    case OpCode::Array__new_fixed:
      return runArrayNewOp(StackMgr, Instr.getTargetIndex(),
                           Instr.getSourceIndex(), Instr.getSourceIndex());
    case OpCode::Array__new_data:
      return runArrayNewDataOp(StackMgr, Instr.getTargetIndex(),
                               Instr.getSourceIndex(), Instr);
    case OpCode::Array__new_elem:
      return runArrayNewElemOp(StackMgr, Instr.getTargetIndex(),
                               Instr.getSourceIndex(), Instr);
    case OpCode::Array__get:
    case OpCode::Array__get_u:
      return runArrayGetOp(StackMgr, Instr.getTargetIndex(), Instr);
    case OpCode::Array__get_s:
      return runArrayGetOp(StackMgr, Instr.getTargetIndex(), Instr, true);
    case OpCode::Array__set:
      return runArraySetOp(StackMgr, Instr.getTargetIndex(), Instr);
    case OpCode::Array__len:
      return runArrayLenOp(StackMgr, Instr);
    case OpCode::Array__fill: {
      return runArrayFillOp(StackMgr, Instr.getTargetIndex(), Instr);
    }
    case OpCode::Array__copy:
      return runArrayCopyOp(StackMgr, Instr.getTargetIndex(),
                            Instr.getSourceIndex(), Instr);
    case OpCode::Array__init_data:
      return runArrayInitDataOp(StackMgr, Instr.getTargetIndex(),
                                Instr.getSourceIndex(), Instr);
    case OpCode::Array__init_elem:
      return runArrayInitElemOp(StackMgr, Instr.getTargetIndex(),
                                Instr.getSourceIndex(), Instr);
    case OpCode::Ref__test:
    case OpCode::Ref__test_null:
      return runRefTestOp(StackMgr, Instr);
    case OpCode::Ref__cast:
    case OpCode::Ref__cast_null:
      return runRefTestOp(StackMgr, Instr, true);
    case OpCode::Any__convert_extern:
      return runRefConvOp(StackMgr, TypeCode::AnyRef);
    case OpCode::Extern__convert_any:
      return runRefConvOp(StackMgr, TypeCode::ExternRef);
    case OpCode::Ref__i31:
      return runRefI31Op(StackMgr);
    case OpCode::I31__get_s:
      return runI31GetOp(StackMgr, Instr, true);
    case OpCode::I31__get_u:
      return runI31GetOp(StackMgr, Instr);

    // Parametric Instructions
    case OpCode::Drop:
      StackMgr.pop<ValVariant>();
      return {};
    case OpCode::Select:
    case OpCode::Select_t: {
      // Pop the i32 value and select values from stack.
      auto [CondVal, Val2, Val1] =
          StackMgr.pops<uint32_t, ValVariant, ValVariant>();

      // Select the value.
      if (CondVal == 0) {
        StackMgr.push(std::move(Val2));
      } else {
        StackMgr.push(std::move(Val1));
      }
      return {};
    }

    // Variable Instructions
    case OpCode::Local__get:
      return runLocalGetOp(StackMgr, Instr.getStackOffset());
    case OpCode::Local__set:
      return runLocalSetOp(StackMgr, Instr.getStackOffset());
    case OpCode::Local__tee:
      return runLocalTeeOp(StackMgr, Instr.getStackOffset());
    case OpCode::Global__get:
      return runGlobalGetOp(StackMgr, Instr.getTargetIndex());
    case OpCode::Global__set:
      return runGlobalSetOp(StackMgr, Instr.getTargetIndex());

    // Table Instructions
    case OpCode::Table__get:
      return runTableGetOp(
          StackMgr, *getTabInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::Table__set:
      return runTableSetOp(
          StackMgr, *getTabInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::Table__init:
      return runTableInitOp(
          StackMgr, *getTabInstByIdx(StackMgr, Instr.getTargetIndex()),
          *getElemInstByIdx(StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Elem__drop:
      return runElemDropOp(*getElemInstByIdx(StackMgr, Instr.getTargetIndex()));
    case OpCode::Table__copy:
      return runTableCopyOp(
          StackMgr, *getTabInstByIdx(StackMgr, Instr.getTargetIndex()),
          *getTabInstByIdx(StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Table__grow:
      return runTableGrowOp(StackMgr,
                            *getTabInstByIdx(StackMgr, Instr.getTargetIndex()));
    case OpCode::Table__size:
      return runTableSizeOp(StackMgr,
                            *getTabInstByIdx(StackMgr, Instr.getTargetIndex()));
    case OpCode::Table__fill:
      return runTableFillOp(
          StackMgr, *getTabInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);

    // Memory Instructions
    case OpCode::I32__load:
      return runLoadOp<uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load:
      return runLoadOp<uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F32__load:
      return runLoadOp<float>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F64__load:
      return runLoadOp<double>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__load8_s:
      return runLoadOp<int32_t, 8>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__load8_u:
      return runLoadOp<uint32_t, 8>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__load16_s:
      return runLoadOp<int32_t, 16>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__load16_u:
      return runLoadOp<uint32_t, 16>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load8_s:
      return runLoadOp<int64_t, 8>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load8_u:
      return runLoadOp<uint64_t, 8>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load16_s:
      return runLoadOp<int64_t, 16>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load16_u:
      return runLoadOp<uint64_t, 16>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load32_s:
      return runLoadOp<int64_t, 32>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__load32_u:
      return runLoadOp<uint64_t, 32>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__store:
      return runStoreOp<uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__store:
      return runStoreOp<uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F32__store:
      return runStoreOp<float>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::F64__store:
      return runStoreOp<double>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__store8:
      return runStoreOp<uint32_t, 8>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__store16:
      return runStoreOp<uint32_t, 16>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__store8:
      return runStoreOp<uint64_t, 8>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__store16:
      return runStoreOp<uint64_t, 16>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__store32:
      return runStoreOp<uint64_t, 32>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::Memory__grow:
      return runMemoryGrowOp(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()));
    case OpCode::Memory__size:
      return runMemorySizeOp(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()));
    case OpCode::Memory__init:
      return runMemoryInitOp(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()),
          *getDataInstByIdx(StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Data__drop:
      return runDataDropOp(*getDataInstByIdx(StackMgr, Instr.getTargetIndex()));
    case OpCode::Memory__copy:
      return runMemoryCopyOp(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()),
          *getMemInstByIdx(StackMgr, Instr.getSourceIndex()), Instr);
    case OpCode::Memory__fill:
      return runMemoryFillOp(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);

    // Const Numeric Instructions
    case OpCode::I32__const:
    case OpCode::I64__const:
    case OpCode::F32__const:
    case OpCode::F64__const:
      StackMgr.push(Instr.getNum());
      return {};

    // Unary Numeric Instructions
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

    // Binary Numeric Instructions
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

    // Saturating Truncation Numeric Instructions
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

    // SIMD Memory Instructions
    case OpCode::V128__load:
      return runLoadOp<uint128_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8x8_s:
      return runLoadExpandOp<int8_t, int16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8x8_u:
      return runLoadExpandOp<uint8_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16x4_s:
      return runLoadExpandOp<int16_t, int32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16x4_u:
      return runLoadExpandOp<uint16_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32x2_s:
      return runLoadExpandOp<int32_t, int64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32x2_u:
      return runLoadExpandOp<uint32_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8_splat:
      return runLoadSplatOp<uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16_splat:
      return runLoadSplatOp<uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32_splat:
      return runLoadSplatOp<uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load64_splat:
      return runLoadSplatOp<uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32_zero:
      return runLoadOp<uint128_t, 32>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load64_zero:
      return runLoadOp<uint128_t, 64>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store:
      return runStoreOp<uint128_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load8_lane:
      return runLoadLaneOp<uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load16_lane:
      return runLoadLaneOp<uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load32_lane:
      return runLoadLaneOp<uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__load64_lane:
      return runLoadLaneOp<uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store8_lane:
      return runStoreLaneOp<uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store16_lane:
      return runStoreLaneOp<uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store32_lane:
      return runStoreLaneOp<uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::V128__store64_lane:
      return runStoreLaneOp<uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);

    // SIMD Const Instructions
    case OpCode::V128__const:
      StackMgr.push(Instr.getNum());
      return {};

    // SIMD Shuffle Instructions
    case OpCode::I8x16__shuffle: {
      auto [Val2, Val1] = StackMgr.popsPeekTop<uint8x16_t, uint8x16_t>();
      std::array<uint8_t, 32> Data;
      std::array<uint8_t, 16> Result;
      std::memcpy(&Data[0], &Val1, 16);
      std::memcpy(&Data[16], &Val2, 16);
      const auto V3 = Instr.getNum().get<uint128_t>();
      for (size_t I = 0; I < 16; ++I) {
        const uint8_t Index = static_cast<uint8_t>(V3 >> (I * 8));
        if constexpr (Endian::native == Endian::little) {
          Result[I] = Data[Index];
        } else {
          Result[15 - I] = Index < 16 ? Data[15 - Index] : Data[47 - Index];
        }
      }
      std::memcpy(&Val1, &Result[0], 16);
      StackMgr.emplaceTop(Val1);
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
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    case OpCode::I8x16__swizzle: {
      const auto [Index, Vector] =
          StackMgr.popsPeekTop<uint8x16_t, uint8x16_t>();
      uint8x16_t Result;
      for (size_t I = 0; I < 16; ++I) {
        const uint8_t SwizzleIndex = Index[I];
        if (SwizzleIndex < 16) {
          Result[I] = Vector[SwizzleIndex];
        } else {
          Result[I] = 0;
        }
      }
      StackMgr.emplaceTop(std::move(Result));
      return {};
    }
#else
    case OpCode::I8x16__swizzle: {
      const auto [Index, Vector] =
          StackMgr.popsPeekTop<uint8x16_t, uint8x16_t>();

      auto Lane = Index;
      if constexpr (Endian::native == Endian::big) {
#if defined(_MSC_VER) && !defined(__clang__)
        std::for_each(Lane.begin(), Lane.end(), [](auto &I) { I = 15 - I; });
#else
        Lane = 15 - Lane;
#endif
      }
      const uint8x16_t Limit = uint8x16_t{} + 16;
      const uint8x16_t Zero = uint8x16_t{};
      const uint8x16_t Exceed = (Lane >= Limit);
#ifdef __clang__
      uint8x16_t Result = __builtin_shufflevector(Vector, Lane);
#else
      uint8x16_t Result = __builtin_shuffle(Vector, Lane);
#endif
      StackMgr.emplaceTop<uint8x16_t>(
          detail::vectorSelect(Exceed, Zero, Result));
      return {};
    }
#endif // MSVC
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
    case OpCode::V128__not:
      StackMgr.emplaceTop(~StackMgr.peekTop<uint128_t>());
      return {};
    case OpCode::V128__and: {
      const auto [Val2, Val1] = StackMgr.popsPeekTop<uint64x2_t, uint64x2_t>();
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      uint64x2_t Result;
      Result[0] = Val1[0] & Val2[0];
      Result[1] = Val1[1] & Val2[1];
      StackMgr.emplaceTop(Result);
#else
      StackMgr.emplaceTop(Val1 & Val2);
#endif // MSVC
      return {};
    }
    case OpCode::V128__andnot: {
      const auto [Val2, Val1] = StackMgr.popsPeekTop<uint64x2_t, uint64x2_t>();
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      uint64x2_t Result;
      Result[0] = Val1[0] & ~Val2[0];
      Result[1] = Val1[1] & ~Val2[1];
      StackMgr.emplaceTop(Result);
#else
      StackMgr.emplaceTop(Val1 & ~Val2);
#endif // MSVC
      return {};
    }
    case OpCode::V128__or: {
      const auto [Val2, Val1] = StackMgr.popsPeekTop<uint64x2_t, uint64x2_t>();
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      uint64x2_t Result;
      Result[0] = Val1[0] | Val2[0];
      Result[1] = Val1[1] | Val2[1];
      StackMgr.emplaceTop(Result);
#else
      StackMgr.emplaceTop(Val1 | Val2);
#endif // MSVC
      return {};
    }
    case OpCode::V128__xor: {
      const auto [Val2, Val1] = StackMgr.popsPeekTop<uint64x2_t, uint64x2_t>();
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      uint64x2_t Result;
      Result[0] = Val1[0] ^ Val2[0];
      Result[1] = Val1[1] ^ Val2[1];
      StackMgr.emplaceTop(Result);
#else
      StackMgr.emplaceTop(Val1 ^ Val2);
#endif // MSVC
      return {};
    }
    case OpCode::V128__bitselect: {
      const auto [C, Val2, Val1] =
          StackMgr.popsPeekTop<uint64x2_t, uint64x2_t, uint64x2_t>();
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      uint64x2_t Result;
      Result[0] = (Val1[0] & C[0]) | (Val2[0] & ~C[0]);
      Result[1] = (Val1[1] & C[1]) | (Val2[1] & ~C[1]);
      StackMgr.emplaceTop(Result);
#else
      StackMgr.emplaceTop((Val1 & C) | (Val2 & ~C));
#endif // MSVC
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
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      using int32x8_t = SIMDArray<int32_t, 32>;
      const auto [V2, V1] = StackMgr.popsPeekTop<int16x8_t, int16x8_t>();
      int32x8_t M;

      for (size_t I = 0; I < 8; ++I) {
        M[I] = V1[I] * V2[I];
      }

      int32x4_t Result;
      for (size_t I = 0; I < 4; ++I) {
        Result[I] = M[I * 2] + M[I * 2 + 1];
      }
      StackMgr.emplaceTop<int32x4_t>(std::move(Result));
      return {};
#else
      using int32x8_t [[gnu::vector_size(32)]] = int32_t;
      const auto [V2, V1] = StackMgr.popsPeekTop<int16x8_t, int16x8_t>();
      const auto M = __builtin_convertvector(V1, int32x8_t) *
                     __builtin_convertvector(V2, int32x8_t);
      const int32x4_t L = {M[0], M[2], M[4], M[6]};
      const int32x4_t R = {M[1], M[3], M[5], M[7]};
      StackMgr.emplaceTop<int32x4_t>(L + R);

      return {};
#endif // MSVC
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

    // Relaxed SIMD Instructions
    case OpCode::I8x16__relaxed_swizzle: {
      auto [Index, Vector] = StackMgr.popsPeekTop<uint8x16_t, uint8x16_t>();
      auto Lane = Index;
      if constexpr (Endian::native == Endian::big) {
#if defined(_MSC_VER) && !defined(__clang__)
        std::for_each(Lane.begin(), Lane.end(), [](auto &I) { I = 15 - I; });
#else
        Lane = 15 - Lane;
#endif
      }
      uint8x16_t Result{};
      for (size_t I = 0; I < 16; ++I) {
        const uint8_t SwizzleIndex = Lane[I];
        if (SwizzleIndex < 16) {
          Result[I] = Vector[SwizzleIndex];
        } else {
          Result[I] = 0;
        }
      }
      StackMgr.emplaceTop(Result);
      return {};
    }
    case OpCode::I32x4__relaxed_trunc_f32x4_s:
      return runVectorTruncSatOp<float, int32_t>(StackMgr);
    case OpCode::I32x4__relaxed_trunc_f32x4_u:
      return runVectorTruncSatOp<float, uint32_t>(StackMgr);
    case OpCode::I32x4__relaxed_trunc_f64x2_s_zero:
      return runVectorTruncSatOp<double, int32_t>(StackMgr);
    case OpCode::I32x4__relaxed_trunc_f64x2_u_zero:
      return runVectorTruncSatOp<double, uint32_t>(StackMgr);
    case OpCode::F32x4__relaxed_madd:
      return runVectorMAddOp<float>(StackMgr);
    case OpCode::F32x4__relaxed_nmadd:
      return runVectorNMAddOp<float>(StackMgr);
    case OpCode::F64x2__relaxed_madd:
      return runVectorMAddOp<double>(StackMgr);
    case OpCode::F64x2__relaxed_nmadd:
      return runVectorNMAddOp<double>(StackMgr);
    case OpCode::I8x16__relaxed_laneselect:
      return runVectorRelaxedLaneselectOp<uint8_t>(StackMgr);
    case OpCode::I16x8__relaxed_laneselect:
      return runVectorRelaxedLaneselectOp<uint16_t>(StackMgr);
    case OpCode::I32x4__relaxed_laneselect:
      return runVectorRelaxedLaneselectOp<uint32_t>(StackMgr);
    case OpCode::I64x2__relaxed_laneselect:
      return runVectorRelaxedLaneselectOp<uint64_t>(StackMgr);
    case OpCode::F32x4__relaxed_min:
      return runVectorFMinOp<float>(StackMgr);
    case OpCode::F32x4__relaxed_max:
      return runVectorFMaxOp<float>(StackMgr);
    case OpCode::F64x2__relaxed_min:
      return runVectorFMinOp<double>(StackMgr);
    case OpCode::F64x2__relaxed_max:
      return runVectorFMaxOp<double>(StackMgr);
    case OpCode::I16x8__relaxed_q15mulr_s:
      return runVectorQ15MulSatOp(StackMgr);
    case OpCode::I16x8__relaxed_dot_i8x16_i7x16_s:
      return runVectorRelaxedIntegerDotProductOp(StackMgr);
    case OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s:
      return runVectorRelaxedIntegerDotProductOpAdd(StackMgr);

    // Atomic Instructions
    case OpCode::Atomic__fence:
      return runMemoryFenceOp();
    case OpCode::Memory__atomic__notify:
      return runAtomicNotifyOp(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::Memory__atomic__wait32:
      return runAtomicWaitOp<int32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::Memory__atomic__wait64:
      return runAtomicWaitOp<int64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__load:
      return runAtomicLoadOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__load:
      return runAtomicLoadOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__load8_u:
      return runAtomicLoadOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__load16_u:
      return runAtomicLoadOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__load8_u:
      return runAtomicLoadOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__load16_u:
      return runAtomicLoadOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__load32_u:
      return runAtomicLoadOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__store:
      return runAtomicStoreOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__store:
      return runAtomicStoreOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__store8:
      return runAtomicStoreOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__store16:
      return runAtomicStoreOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__store8:
      return runAtomicStoreOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__store16:
      return runAtomicStoreOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__store32:
      return runAtomicStoreOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw__add:
      return runAtomicAddOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw__add:
      return runAtomicAddOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw8__add_u:
      return runAtomicAddOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw16__add_u:
      return runAtomicAddOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw8__add_u:
      return runAtomicAddOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw16__add_u:
      return runAtomicAddOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw32__add_u:
      return runAtomicAddOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw__sub:
      return runAtomicSubOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw__sub:
      return runAtomicSubOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw8__sub_u:
      return runAtomicSubOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw16__sub_u:
      return runAtomicSubOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw8__sub_u:
      return runAtomicSubOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw16__sub_u:
      return runAtomicSubOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw32__sub_u:
      return runAtomicSubOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw__and:
      return runAtomicAndOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw__and:
      return runAtomicAndOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw8__and_u:
      return runAtomicAndOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw16__and_u:
      return runAtomicAndOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw8__and_u:
      return runAtomicAndOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw16__and_u:
      return runAtomicAndOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw32__and_u:
      return runAtomicAndOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw__or:
      return runAtomicOrOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw__or:
      return runAtomicOrOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw8__or_u:
      return runAtomicOrOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw16__or_u:
      return runAtomicOrOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw8__or_u:
      return runAtomicOrOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw16__or_u:
      return runAtomicOrOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw32__or_u:
      return runAtomicOrOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw__xor:
      return runAtomicXorOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw__xor:
      return runAtomicXorOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw8__xor_u:
      return runAtomicXorOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw16__xor_u:
      return runAtomicXorOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw8__xor_u:
      return runAtomicXorOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw16__xor_u:
      return runAtomicXorOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw32__xor_u:
      return runAtomicXorOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw__xchg:
      return runAtomicExchangeOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw__xchg:
      return runAtomicExchangeOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw8__xchg_u:
      return runAtomicExchangeOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw16__xchg_u:
      return runAtomicExchangeOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw8__xchg_u:
      return runAtomicExchangeOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw16__xchg_u:
      return runAtomicExchangeOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw32__xchg_u:
      return runAtomicExchangeOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw__cmpxchg:
      return runAtomicCompareExchangeOp<int32_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw__cmpxchg:
      return runAtomicCompareExchangeOp<int64_t, uint64_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw8__cmpxchg_u:
      return runAtomicCompareExchangeOp<uint32_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I32__atomic__rmw16__cmpxchg_u:
      return runAtomicCompareExchangeOp<uint32_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw8__cmpxchg_u:
      return runAtomicCompareExchangeOp<uint64_t, uint8_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw16__cmpxchg_u:
      return runAtomicCompareExchangeOp<uint64_t, uint16_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);
    case OpCode::I64__atomic__rmw32__cmpxchg_u:
      return runAtomicCompareExchangeOp<uint64_t, uint32_t>(
          StackMgr, *getMemInstByIdx(StackMgr, Instr.getTargetIndex()), Instr);

    default:
      return {};
    }
  };

  while (PC != PCEnd) {
    if (Stat) {
      OpCode Code = PC->getOpCode();
      if (Conf.getStatisticsConfigure().isInstructionCounting()) {
        Stat->incInstrCount();
      }
      // Add cost. Note: if-else case should be processed additionally.
      if (Conf.getStatisticsConfigure().isCostMeasuring()) {
        if (unlikely(!Stat->addInstrCost(Code))) {
          const AST::Instruction &Instr = *PC;
          spdlog::error(
              ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
          return Unexpect(ErrCode::Value::CostLimitExceeded);
        }
      }
    }
    EXPECTED_TRY(Dispatch().map_error([this, &StackMgr](auto E) {
      StackTraceSize = interpreterStackTrace(StackMgr, StackTrace).size();
      if (Conf.getRuntimeConfigure().isEnableCoredump() &&
          E.getErrCodePhase() == WasmPhase::Execution) {
        Coredump::generateCoredump(
            StackMgr, Conf.getRuntimeConfigure().isCoredumpWasmgdb());
      }
      return E;
    }));
    PC++;
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
