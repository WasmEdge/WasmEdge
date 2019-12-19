#include "validator/validator.h"

#include "ast/module.h"
#include "vm/common.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

namespace SSVM {
namespace Validator {

void ValidateMachine::reset(bool CleanGlobal) {
  ValStack.clear();
  CtrlStack.clear();
  Local.clear();
  ReturnVals.clear();

  if (CleanGlobal) {
    Global.clear();
    Funcs.clear();
    Types.clear();
  }
}

static ValType Ast2ValType(AST::ValType V) {
  ValType Res;
  switch (V) {
  case AST::ValType::I32:
    Res = ValType::I32;
    break;
  case AST::ValType::I64:
    Res = ValType::I64;
    break;
  case AST::ValType::F32:
    Res = ValType::F32;
    break;
  case AST::ValType::F64:
    Res = ValType::F64;
    break;
  default:
    throw "unlisted Ast2ValType";
  }
  return Res;
}

void ValidateMachine::addLocal(unsigned int Idx, AST::ValType V) {
  Local[Idx] = Ast2ValType(V);
}

ValType ValidateMachine::getLocal(unsigned int Idx) {
  if (Local.count(Idx) == 0)
    throw "Local value id is not exist.(getlocal)";
  return Local[Idx];
}

void ValidateMachine::setLocal(unsigned int Idx, ValType V) {
  if (Local.count(Idx) == 0)
    throw "Local value id is not exist.(setlocal)";
  Local[Idx] = V;
}

void ValidateMachine::addGlobal(AST::GlobalType V) {
  // Check type
  (void)Ast2ValType(V.getValueType());
  Global.emplace_back(V);
}

ValType ValidateMachine::getGlobal(unsigned int Idx) {
  if (Global.size() <= Idx)
    throw "Global value id is not exist. (getglobal)";
  return Ast2ValType(Global[Idx].getValueType());
}

void ValidateMachine::setGlobal(unsigned int Idx, ValType V) {
  if (Global.size() <= Idx)
    throw "Global value id is not exist. (setglobal)";
  if (Global[Idx].getValueMutation() != AST::ValMut::Var)
    throw "Global value can not be change.";
  if (Ast2ValType(Global[Idx].getValueType()) != V)
    throw "Global Value type is not matched.";
}

void ValidateMachine::addFunc(AST::FunctionType *Func) {
  std::vector<ValType> Param, Ret;

  for (auto Val : Func->getParamTypes())
    Param.emplace_back(Ast2ValType(Val));
  for (auto Val : Func->getReturnTypes())
    Ret.emplace_back(Ast2ValType(Val));
  Funcs.emplace_back(Param, Ret);
}

void ValidateMachine::addType(AST::FunctionType *Func) {
  std::vector<ValType> Param, Ret;

  for (auto Val : Func->getParamTypes())
    Param.emplace_back(Ast2ValType(Val));
  for (auto Val : Func->getReturnTypes())
    Ret.emplace_back(Ast2ValType(Val));
  Types.emplace_back(Param, Ret);
}

void ValidateMachine::push_opd(ValType V) { ValStack.emplace_front(V); }

ValType ValidateMachine::pop_opd() {
  if (ValStack.size() == CtrlStack[0].Height && CtrlStack[0].IsUnreachable)
    return ValType::Unknown;

  if (ValStack.size() == CtrlStack[0].Height) {
    throw "ValidatMachine Stack underflow";
  }

  auto Res = ValStack.front();
  ValStack.pop_front();
  return Res;
}

ValType ValidateMachine::pop_opd(ValType Expect) {
  auto Res = pop_opd();
  if (Res == ValType::Unknown)
    return Expect;
  if (Expect == ValType::Unknown)
    return Res;
  if (Res != Expect) {
    throw "Expect value on ValidatMachine stack is not matched.";
  }
  return Res;
}

void ValidateMachine::pop_opds(const std::vector<ValType> &Input) {
  auto Vals = Input;
  reverse(Vals.begin(), Vals.end());

  for (auto Val : Vals)
    pop_opd(Val);
}

void ValidateMachine::push_opds(const std::vector<ValType> &Input) {
  for (auto Val : Input)
    push_opd(Val);
}

void ValidateMachine::push_ctrl(const std::vector<ValType> &Label,
                                const std::vector<ValType> &Out) {
  CtrlFrame Frame;

  Frame.LabelTypes = Label;
  Frame.EndTypes = Out;
  Frame.Height = ValStack.size();
  Frame.IsUnreachable = false;

  CtrlStack.emplace_front(std::move(Frame));
}

std::vector<ValType> ValidateMachine::pop_ctrl() {
  if (CtrlStack.empty())
    throw "ValidatMachine::pop_ctr CtrlStack.empty";
  auto Head = CtrlStack.front();

  pop_opds(Head.EndTypes);

  if (ValStack.size() != Head.Height)
    throw "ValStack.size() != head.height";

  CtrlStack.pop_front();
  return Head.EndTypes;
}

void ValidateMachine::unreachable() {
  while (ValStack.size() > CtrlStack[0].Height)
    pop_opd();
  CtrlStack[0].IsUnreachable = true;
}

ErrCode ValidateMachine::validateWarp(const AST::InstrVec &Instrs) {
  for (auto &Instr : Instrs)
    runOp(Instr.get());
  return ErrCode::Success;
}

ErrCode ValidateMachine::validate(const AST::InstrVec &Instrs,
                                  const std::vector<AST::ValType> &RetVals) {
  try {
    for (AST::ValType Val : RetVals)
      ReturnVals.push_back(Ast2ValType(Val));
    push_ctrl({}, ReturnVals);
    validateWarp(Instrs);
  } catch (const char *Str) {
    std::cout << "Error:" << Str << std::endl;
    return ErrCode::Invalid;
  } catch (...) {
    return ErrCode::Invalid;
  }
  return ErrCode::Success;
}

void ValidateMachine::runOp(AST::Instruction *Instr) {
  auto StackTrans = [&](const std::vector<ValType> &Take,
                        const std::vector<ValType> &Put) -> void {
    pop_opds(Take);
    push_opds(Put);
  };

  auto Code = Instr->getOpCode();
  switch (Code) {
  /// 0x00
  case OpCode::Unreachable:
    unreachable();
    break;
  case OpCode::Nop:
    break;
  case OpCode::Block: {
    AST::BlockControlInstruction *BlockInstr =
        dynamic_cast<AST::BlockControlInstruction *>(Instr);

    auto Res = BlockInstr->getResultType();
    std::vector<ValType> ResVec;
    if (Res != SSVM::AST::ValType::None)
      ResVec.emplace_back(Ast2ValType(Res));
    push_ctrl(ResVec, ResVec);
    validateWarp(BlockInstr->getBody());
    push_opds(pop_ctrl());
    break;
  }
  case OpCode::Loop: {
    AST::BlockControlInstruction *BlockInstr =
        dynamic_cast<AST::BlockControlInstruction *>(Instr);

    auto Res = BlockInstr->getResultType();
    std::vector<ValType> ResVec;
    if (Res != SSVM::AST::ValType::None)
      ResVec.emplace_back(Ast2ValType(Res));
    push_ctrl({}, ResVec);
    validateWarp(BlockInstr->getBody());
    push_opds(pop_ctrl());
    break;
  }
  case OpCode::If: {
    // case OpCode::Else: Unused OpCode in AST
    AST::IfElseControlInstruction *IfInstr =
        dynamic_cast<AST::IfElseControlInstruction *>(Instr);
    pop_opd(ValType::I32);

    auto Res = IfInstr->getResultType();
    std::vector<ValType> ResVec;
    if (Res != SSVM::AST::ValType::None)
      ResVec.emplace_back(Ast2ValType(Res));
    push_ctrl(ResVec, ResVec);
    validateWarp(IfInstr->getIfStatement());
    if (IfInstr->getElseStatement().size() != 0) {
      auto Result = pop_ctrl();
      push_ctrl(Result, Result);
      validateWarp(IfInstr->getElseStatement());
    }
    push_opds(pop_ctrl());
    break;
  }

  // case OpCode::End: Unused Code in AST
  case OpCode::Br: {
    AST::BrControlInstruction *BrInstr =
        dynamic_cast<AST::BrControlInstruction *>(Instr);
    auto N = BrInstr->getLabelIndex();
    if (CtrlStack.size() <= N)
      throw "Br CtrlStack.size() <= N";
    pop_opds(CtrlStack[N].LabelTypes);
    unreachable();
    break;
  }
  case OpCode::Br_if: {
    AST::BrControlInstruction *BrInstr =
        dynamic_cast<AST::BrControlInstruction *>(Instr);
    auto N = BrInstr->getLabelIndex();
    if (CtrlStack.size() <= N)
      throw "Br_if CtrlStack.size() <= N";
    pop_opd(ValType::I32);
    pop_opds(CtrlStack[N].LabelTypes);
    push_opds(CtrlStack[N].LabelTypes);
    break;
  }
  case OpCode::Br_table: {
    AST::BrTableControlInstruction *BrTInstr =
        dynamic_cast<AST::BrTableControlInstruction *>(Instr);
    auto M = BrTInstr->getLabelIndex();
    if (CtrlStack.size() <= M)
      throw "Br_table CtrlStack.size() <= M";
    for (auto N : *BrTInstr->getLabelTable()) {
      // Error_if(ctrls.size() < n || ctrls[n].label_types =/=
      // ctrls[m].label_types)
      if (CtrlStack.size() <= N)
        throw "Br_table CtrlStack.size() <= N";
      if (CtrlStack[N].LabelTypes != CtrlStack[M].LabelTypes)
        throw "CtrlStack[N].label_types != CtrlStack[M].label_types";
    }
    pop_opd(ValType::I32);
    pop_opds(CtrlStack[M].LabelTypes);
    unreachable();
    break;
  }
  case OpCode::Return:
    pop_opds(ReturnVals);
    unreachable();
    break;

  /// 0x10
  case OpCode::Call: {
    AST::CallControlInstruction *CallInstr =
        dynamic_cast<AST::CallControlInstruction *>(Instr);
    auto N = CallInstr->getFuncIndex();
    if (Funcs.size() <= N)
      throw "Call funcs.size() <= N";
    StackTrans({Funcs[N].first}, {Funcs[N].second});
    break;
  }
  case OpCode::Call_indirect: {
    AST::CallControlInstruction *CallInstr =
        dynamic_cast<AST::CallControlInstruction *>(Instr);
    auto N = CallInstr->getFuncIndex();
    if (Types.size() <= N)
      throw "Call funcs.size() <= N";
    pop_opd(ValType::I32);
    StackTrans({Types[N].first}, {Types[N].second});
    break;
  }
  case OpCode::Drop:
    StackTrans({ValType::Unknown}, {});
    break;
  case OpCode::Select: {
    pop_opd(ValType::I32);
    auto T1 = pop_opd();
    auto T2 = pop_opd(T1);
    push_opd(T2);
    break;
  }

  /// 0x20
  case OpCode::Local__get: {
    AST::VariableInstruction *VarInstr =
        dynamic_cast<AST::VariableInstruction *>(Instr);
    StackTrans({}, {getLocal(VarInstr->getVariableIndex())});
    break;
  }
  case OpCode::Local__set: {
    AST::VariableInstruction *VarInstr =
        dynamic_cast<AST::VariableInstruction *>(Instr);
    auto T = pop_opd();
    setLocal(VarInstr->getVariableIndex(), T);
    break;
  }
  case OpCode::Local__tee: {
    AST::VariableInstruction *VarInstr =
        dynamic_cast<AST::VariableInstruction *>(Instr);
    auto T = pop_opd();
    setLocal(VarInstr->getVariableIndex(), T);
    push_opd(T);
    break;
  }
  case OpCode::Global__get: {
    AST::VariableInstruction *VarInstr =
        dynamic_cast<AST::VariableInstruction *>(Instr);
    StackTrans({}, {getGlobal(VarInstr->getVariableIndex())});
    break;
  }
  case OpCode::Global__set: {
    AST::VariableInstruction *VarInstr =
        dynamic_cast<AST::VariableInstruction *>(Instr);
    VarInstr = dynamic_cast<AST::VariableInstruction *>(Instr);
    auto T = pop_opd();
    setGlobal(VarInstr->getVariableIndex(), T);
    break;
  }

  /// TODO: Check memory arg
  case OpCode::I32__load:
    StackTrans({ValType::I32}, {ValType::I32});
    break;
  case OpCode::I64__load:
    StackTrans({ValType::I32}, {ValType::I64});
    break;
  case OpCode::F32__load:
    StackTrans({ValType::I32}, {ValType::F32});
    break;
  case OpCode::F64__load:
    StackTrans({ValType::I32}, {ValType::F64});
    break;
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
    StackTrans({ValType::I32}, {ValType::I32});
    break;

  /// 0x30
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
    StackTrans({ValType::I32}, {ValType::I64});
    break;
  /// TODO: check store:
  /// https://webassembly.github.io/spec/core/appendix/properties.html
  case OpCode::I32__store:
    StackTrans({ValType::I32, ValType::I32}, {});
    break;
  case OpCode::I64__store:
    StackTrans({ValType::I32, ValType::I64}, {});
    break;
  case OpCode::F32__store:
    StackTrans({ValType::I32, ValType::F32}, {});
    break;
  case OpCode::F64__store:
    StackTrans({ValType::I32, ValType::F64}, {});
    break;
  case OpCode::I32__store8:
  case OpCode::I32__store16:
    StackTrans({ValType::I32, ValType::I32}, {});
    break;
  case OpCode::I64__store8:
  case OpCode::I64__store16:
  case OpCode::I64__store32:
    StackTrans({ValType::I32, ValType::I64}, {});
    break;
  case OpCode::Memory__size:
    /// TODO: check memory[0]
    StackTrans({}, {ValType::I32});
    break;

    /// 0x40
  case OpCode::Memory__grow:
    /// TODO: check memory[0]
    StackTrans({ValType::I32}, {ValType::I32});
    break;
  case OpCode::I32__const:
    StackTrans({}, {ValType::I32});
    break;
  case OpCode::I64__const:
    StackTrans({}, {ValType::I64});
    break;
  case OpCode::F32__const:
    StackTrans({}, {ValType::F32});
    break;
  case OpCode::F64__const:
    StackTrans({}, {ValType::F64});
    break;
  case OpCode::I32__eqz:
    StackTrans({ValType::I32}, {ValType::I32});
    break;
  case OpCode::I32__eq:
  case OpCode::I32__ne:
  case OpCode::I32__lt_s:
  case OpCode::I32__lt_u:
  case OpCode::I32__gt_s:
  case OpCode::I32__gt_u:
  case OpCode::I32__le_s:
  case OpCode::I32__le_u:
  case OpCode::I32__ge_s:
  case OpCode::I32__ge_u:
    StackTrans({ValType::I32, ValType::I32}, {ValType::I32});
    break;

  /// 0x50
  case OpCode::I64__eqz:
    StackTrans({ValType::I64}, {ValType::I32});
    break;
  case OpCode::I64__eq:
  case OpCode::I64__ne:
  case OpCode::I64__lt_s:
  case OpCode::I64__lt_u:
  case OpCode::I64__gt_s:
  case OpCode::I64__gt_u:
  case OpCode::I64__le_s:
  case OpCode::I64__le_u:
  case OpCode::I64__ge_s:
  case OpCode::I64__ge_u:
    StackTrans({ValType::I64, ValType::I64}, {ValType::I32});
    break;
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
    StackTrans({ValType::F32, ValType::F32}, {ValType::I32});
    break;

  /// 0x60
  case OpCode::F32__ge:
    StackTrans({ValType::F32, ValType::F32}, {ValType::I32});
    break;
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:
    StackTrans({ValType::F64, ValType::F64}, {ValType::I32});
    break;
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
    StackTrans({ValType::I32}, {ValType::I32});
    break;
  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I32__mul:
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
    StackTrans({ValType::I32, ValType::I32}, {ValType::I32});
    break;

  /// 0x70
  case OpCode::I32__rem_u:
  case OpCode::I32__and:
  case OpCode::I32__or:
  case OpCode::I32__xor:
  case OpCode::I32__shl:
  case OpCode::I32__shr_s:
  case OpCode::I32__shr_u:
  case OpCode::I32__rotl:
  case OpCode::I32__rotr:
    StackTrans({ValType::I32, ValType::I32}, {ValType::I32});
    break;
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
    StackTrans({ValType::I64}, {ValType::I64});
    break;
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_s:
    StackTrans({ValType::I64, ValType::I64}, {ValType::I64});
    break;

  /// 0x80
  case OpCode::I64__div_u:
  case OpCode::I64__rem_s:
  case OpCode::I64__rem_u:
  case OpCode::I64__and:
  case OpCode::I64__or:
  case OpCode::I64__xor:
  case OpCode::I64__shl:
  case OpCode::I64__shr_s:
  case OpCode::I64__shr_u:
  case OpCode::I64__rotl:
  case OpCode::I64__rotr:
    StackTrans({ValType::I64, ValType::I64}, {ValType::I64});
    break;
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
    StackTrans({ValType::F32}, {ValType::F32});
    break;

  /// 0x90
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
    StackTrans({ValType::F32}, {ValType::F32});
    break;
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
    StackTrans({ValType::F32, ValType::F32}, {ValType::F32});
    break;
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
    StackTrans({ValType::F64}, {ValType::F64});
    break;

  /// 0xA0
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    StackTrans({ValType::F64, ValType::F64}, {ValType::F64});
    break;
  case OpCode::I32__wrap_i64:
    StackTrans({ValType::I64}, {ValType::I32});
    break;
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
    StackTrans({ValType::F32}, {ValType::I32});
    break;
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
    StackTrans({ValType::F64}, {ValType::I32});
    break;
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
    StackTrans({ValType::I32}, {ValType::I64});
    break;
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
    StackTrans({ValType::F32}, {ValType::I64});
    break;

  /// 0XB0
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
    StackTrans({ValType::F64}, {ValType::I64});
    break;
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
    StackTrans({ValType::I32}, {ValType::F32});
    break;
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
    StackTrans({ValType::I64}, {ValType::F32});
    break;
  case OpCode::F32__demote_f64:
    StackTrans({ValType::F64}, {ValType::F32});
    break;
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
    StackTrans({ValType::I32}, {ValType::F64});
    break;
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
    StackTrans({ValType::I64}, {ValType::F64});
    break;
  case OpCode::F64__promote_f32:
    StackTrans({ValType::F32}, {ValType::F64});
    break;
  case OpCode::I32__reinterpret_f32:
    StackTrans({ValType::F32}, {ValType::I32});
    break;
  case OpCode::I64__reinterpret_f64:
    StackTrans({ValType::F64}, {ValType::I64});
    break;
  case OpCode::F32__reinterpret_i32:
    StackTrans({ValType::I32}, {ValType::F32});
    break;
  case OpCode::F64__reinterpret_i64:
    StackTrans({ValType::I64}, {ValType::F64});
    break;

  default:
    std::cout << "unimp OpCode: 0x" << std::hex << (uint)Code << std::dec
              << std::endl;
    throw;
  }
}

} // namespace Validator
} // namespace SSVM