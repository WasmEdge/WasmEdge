#include "validator/validator.h"

#include "ast/module.h"
#include "vm/common.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

using std::cout;
using std::endl;
using std::printf;
using std::puts;

namespace SSVM {
namespace Validator {

void ValidatMachine::reset(bool CleanGlobal) {
  ValStack.clear();
  CtrlStack.clear();
  local.clear();
  ReturnVals.clear();

  if (CleanGlobal) {
    global.clear();
    funcs.clear();
    types.clear();
  }
}

static ValType Ast2ValType(AST::ValType v) {
  ValType res;
  switch (v) {
  case AST::ValType::I32:
    res = ValType::I32;
    break;
  case AST::ValType::I64:
    res = ValType::I64;
    break;
  case AST::ValType::F32:
    res = ValType::F32;
    break;
  case AST::ValType::F64:
    res = ValType::F64;
    break;
  default:
    // TODO throw expect
    throw "Ast2ValType";
  }
  return res;
}

void ValidatMachine::addloacl(unsigned int idx, AST::ValType v) {
  local[idx] = Ast2ValType(v);
}

ValType ValidatMachine::getlocal(unsigned int idx) {
  if (local.count(idx) == 0)
    throw "getlocal";
  return local[idx];
}

void ValidatMachine::setlocal(unsigned int idx, ValType v) {
  if (local.count(idx) == 0)
    throw "setlocal";
  local[idx] = v;
}

void ValidatMachine::addglobal(AST::GlobalType v) {
  // Check type
  (void)Ast2ValType(v.getValueType());
  global.emplace_back(v);
}

ValType ValidatMachine::getglobal(unsigned int idx) {
  if (global.size() <= idx)
    throw "getglobal";
  return Ast2ValType(global[idx].getValueType());
}

void ValidatMachine::setglobal(unsigned int idx, ValType v) {
  if (global.size() <= idx)
    throw "setglobal idx";
  if (global[idx].getValueMutation() != AST::ValMut::Var)
    throw "setglobal getValueMutation";
  if (Ast2ValType(global[idx].getValueType()) != v)
    throw "setglobal Ast2ValType";
}

void ValidatMachine::addfunc(AST::FunctionType *func) {
  std::vector<ValType> param, ret;

  for (auto val : func->getParamTypes())
    param.emplace_back(Ast2ValType(val));
  for (auto val : func->getReturnTypes())
    ret.emplace_back(Ast2ValType(val));
  funcs.emplace_back(param, ret);
}

void ValidatMachine::addtype(AST::FunctionType *func) {
  std::vector<ValType> param, ret;

  for (auto val : func->getParamTypes())
    param.emplace_back(Ast2ValType(val));
  for (auto val : func->getReturnTypes())
    ret.emplace_back(Ast2ValType(val));
  types.emplace_back(param, ret);
}

void ValidatMachine::push_opd(ValType v) { ValStack.emplace_front(v); }

ValType ValidatMachine::pop_opd() {
  if (ValStack.size() == CtrlStack[0].height && CtrlStack[0].unreachable)
    return ValType::Unknown;

  if (ValStack.size() == CtrlStack[0].height) {
    throw "pop_opd Reach Block End.";
  }

  auto res = ValStack.front();
  ValStack.pop_front();
  return res;
}

ValType ValidatMachine::pop_opd(ValType expect) {
  auto res = pop_opd();
  if (res == ValType::Unknown)
    return expect;
  if (expect == ValType::Unknown)
    return res;
  if (res != expect) {
    throw "Stack Val Not Match..";
  }
  return res;
}

void ValidatMachine::pop_opds(const std::vector<ValType> &input) {
  auto vals = input;
  reverse(vals.begin(), vals.end());

  for (auto val : vals)
    pop_opd(val);
}

void ValidatMachine::push_opds(const std::vector<ValType> &input) {
  for (auto val : input)
    push_opd(val);
}

void ValidatMachine::push_ctrl(const std::vector<ValType> &label,
                               const std::vector<ValType> &out) {
  CtrlFrame frame;

  frame.label_types = label;
  frame.end_types = out;
  frame.height = ValStack.size();
  frame.unreachable = false;

  CtrlStack.emplace_front(std::move(frame));
}

std::vector<ValType> ValidatMachine::pop_ctrl() {
  if (CtrlStack.empty())
    throw "ValidatMachine::pop_ctr CtrlStack.empty";
  auto head = CtrlStack.front();

  pop_opds(head.end_types);

  if (ValStack.size() != head.height)
    throw "ValStack.size() != head.height";

  CtrlStack.pop_front();
  return head.end_types;
}

void ValidatMachine::unreachable() {
  while (ValStack.size() > CtrlStack[0].height)
    pop_opd();
  CtrlStack[0].unreachable = true;
}

ErrCode ValidatMachine::validateWarp(const AST::InstrVec *insts) {
  for (auto &op : *insts)
    runop(op.get());
  return ErrCode::Success;
}

ErrCode ValidatMachine::validate(const AST::InstrVec &insts,
                                 const std::vector<AST::ValType> &ret) {
  try {
    for (AST::ValType val : ret)
      ReturnVals.push_back(Ast2ValType(val));
    push_ctrl({}, ReturnVals);
    validateWarp(&insts);
  } catch (const char *str) {
    cout << "Error:" << str << endl;
    return ErrCode::Invalid;
  } catch (...) {
    return ErrCode::Invalid;
  }
  return ErrCode::Success;
}

void ValidatMachine::runop(AST::Instruction *instr) {
  auto stack_trans = [&](const std::vector<ValType> &take,
                         const std::vector<ValType> &put) -> void {
    pop_opds(take);
    push_opds(put);
  };

  auto opcode = instr->getOpCode();
  AST::VariableInstruction *VarInstr = nullptr;
  //printf("op: 0x%x\n", opcode);
  switch (opcode) {
  /// 0x00
  case OpCode::Unreachable:
    unreachable();
    break;
  case OpCode::Nop:
    break;
  case OpCode::Block: {
    AST::BlockControlInstruction *BlockInstr =
        dynamic_cast<AST::BlockControlInstruction *>(instr);
    auto res = BlockInstr->getResultType();
    std::vector<ValType> res_vec;
    if (res != SSVM::AST::ValType::None)
      res_vec.emplace_back(Ast2ValType(res));
    push_ctrl(res_vec, res_vec);
    validateWarp(BlockInstr->getBody());
    push_opds(pop_ctrl());
    break;
  }
  case OpCode::Loop: {
    AST::BlockControlInstruction *BlockInstr =
        dynamic_cast<AST::BlockControlInstruction *>(instr);

    auto res = BlockInstr->getResultType();
    std::vector<ValType> res_vec;
    if (res != SSVM::AST::ValType::None)
      res_vec.emplace_back(Ast2ValType(res));
    push_ctrl({}, res_vec);
    validateWarp(BlockInstr->getBody());
    push_opds(pop_ctrl());
    break;
  }
  case OpCode::If: {
    // case OpCode::Else: Unused opcode in AST
    AST::IfElseControlInstruction *IfInstr =
        dynamic_cast<AST::IfElseControlInstruction *>(instr);
    pop_opd(ValType::I32);

    auto res = IfInstr->getResultType();
    std::vector<ValType> res_vec;
    if (res != SSVM::AST::ValType::None)
      res_vec.emplace_back(Ast2ValType(res));
    push_ctrl(res_vec, res_vec);
    validateWarp(IfInstr->getIfStatement());
    if (IfInstr->getElseStatement())
    {
      auto result = pop_ctrl();
      push_ctrl(result, result);
      validateWarp(IfInstr->getElseStatement());
    }
    push_opds(pop_ctrl());
    break;
  }

  // case OpCode::End: Unused opcode in AST
  case OpCode::Br: {
    AST::BrControlInstruction *BrInstr =
        dynamic_cast<AST::BrControlInstruction *>(instr);
    auto N = BrInstr->getLabelIndex();
    if (CtrlStack.size() <= N)
      throw "Br CtrlStack.size() <= N";
    pop_opds(CtrlStack[N].label_types);
    unreachable();
    break;
  }
  case OpCode::Br_if: {
    AST::BrControlInstruction *BrInstr =
        dynamic_cast<AST::BrControlInstruction *>(instr);
    auto N = BrInstr->getLabelIndex();
    if (CtrlStack.size() <= N)
      throw "Br_if CtrlStack.size() <= N";
    pop_opd(ValType::I32);
    pop_opds(CtrlStack[N].label_types);
    push_opds(CtrlStack[N].label_types);
    break;
  }
  case OpCode::Br_table: {
    AST::BrTableControlInstruction *BrTInstr =
      dynamic_cast<AST::BrTableControlInstruction *>(instr);
    auto M = BrTInstr->getLabelIndex();
    if (CtrlStack.size() <= M)
       throw "Br_table CtrlStack.size() <= M";
    for (auto N:*BrTInstr->getLabelTable())
    {
      //rror_if(ctrls.size() < n || ctrls[n].label_types =/= ctrls[m].label_types)
      if (CtrlStack.size() <= N)
        throw "Br_table CtrlStack.size() <= N";
      if (CtrlStack[N].label_types != CtrlStack[M].label_types)
        throw "CtrlStack[N].label_types != CtrlStack[M].label_types";
    }
    pop_opd(ValType::I32);
    pop_opds(CtrlStack[M].label_types);
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
        dynamic_cast<AST::CallControlInstruction *>(instr);
    auto N = CallInstr->getFuncIndex();
    if (funcs.size() <= N)
      throw "Call funcs.size() <= N";
    stack_trans({funcs[N].first}, {funcs[N].second});
    break;
  }
  case OpCode::Call_indirect: {
    AST::CallControlInstruction *CallInstr =
        dynamic_cast<AST::CallControlInstruction *>(instr);
    auto N = CallInstr->getFuncIndex();
    if (types.size() <= N)
      throw "Call funcs.size() <= N";
    pop_opd(ValType::I32);
    stack_trans({types[N].first}, {types[N].second});
    break;
  }
  case OpCode::Drop:
    stack_trans({ValType::Unknown}, {});
    break;
  case OpCode::Select: {
    pop_opd(ValType::I32);
    auto t1 = pop_opd();
    auto t2 = pop_opd(t1);
    push_opd(t2);
    break;
  }

  /// 0x20
  case OpCode::Local__get:
    VarInstr = dynamic_cast<AST::VariableInstruction *>(instr);
    stack_trans({}, {getlocal(VarInstr->getVariableIndex())});
    break;
  case OpCode::Local__set: {
    VarInstr = dynamic_cast<AST::VariableInstruction *>(instr);
    auto t = pop_opd();
    setlocal(VarInstr->getVariableIndex(), t);
    break;
  }
  case OpCode::Local__tee: {
    VarInstr = dynamic_cast<AST::VariableInstruction *>(instr);
    auto t = pop_opd();
    setlocal(VarInstr->getVariableIndex(), t);
    push_opd(t);
    break;
  }
  case OpCode::Global__get:
    VarInstr = dynamic_cast<AST::VariableInstruction *>(instr);
    stack_trans({}, {getglobal(VarInstr->getVariableIndex())});
    break;
  case OpCode::Global__set: {
    VarInstr = dynamic_cast<AST::VariableInstruction *>(instr);
    auto t = pop_opd();
    setglobal(VarInstr->getVariableIndex(), t);
    break;
  }

  /// TODO: Check memory arg
  case OpCode::I32__load:
    stack_trans({ValType::I32}, {ValType::I32});
    break;
  case OpCode::I64__load:
    stack_trans({ValType::I32}, {ValType::I64});
    break;
  case OpCode::F32__load:
    stack_trans({ValType::I32}, {ValType::F32});
    break;
  case OpCode::F64__load:
    stack_trans({ValType::I32}, {ValType::F64});
    break;
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
    stack_trans({ValType::I32}, {ValType::I32});
    break;

  /// 0x30
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
    stack_trans({ValType::I32}, {ValType::I64});
    break;
  /// TODO: check store:
  /// https://webassembly.github.io/spec/core/appendix/properties.html
  case OpCode::I32__store:
    stack_trans({ValType::I32, ValType::I32}, {});
    break;
  case OpCode::I64__store:
    stack_trans({ValType::I32, ValType::I64}, {});
    break;
  case OpCode::F32__store:
    stack_trans({ValType::I32, ValType::F32}, {});
    break;
  case OpCode::F64__store:
    stack_trans({ValType::I32, ValType::F64}, {});
    break;
  case OpCode::I32__store8:
  case OpCode::I32__store16:
    stack_trans({ValType::I32, ValType::I32}, {});
    break;
  case OpCode::I64__store8:
  case OpCode::I64__store16:
  case OpCode::I64__store32:
    stack_trans({ValType::I32, ValType::I64}, {});
    break;
  case OpCode::Memory__size:
    /// TODO: check memory[0]
    stack_trans({}, {ValType::I32});
    break;

    /// 0x40
  case OpCode::Memory__grow:
    /// TODO: check memory[0]
    stack_trans({ValType::I32}, {ValType::I32});
    break;
  case OpCode::I32__const:
    stack_trans({}, {ValType::I32});
    break;
  case OpCode::I64__const:
    stack_trans({}, {ValType::I64});
    break;
  case OpCode::F32__const:
    stack_trans({}, {ValType::F32});
    break;
  case OpCode::F64__const:
    stack_trans({}, {ValType::F64});
    break;
  case OpCode::I32__eqz:
    stack_trans({ValType::I32}, {ValType::I32});
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
    stack_trans({ValType::I32, ValType::I32}, {ValType::I32});
    break;

  /// 0x50
  case OpCode::I64__eqz:
    stack_trans({ValType::I64}, {ValType::I32});
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
    stack_trans({ValType::I64, ValType::I64}, {ValType::I32});
    break;
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
    stack_trans({ValType::F32, ValType::F32}, {ValType::I32});
    break;

  /// 0x60
  case OpCode::F32__ge:
    stack_trans({ValType::F32, ValType::F32}, {ValType::I32});
    break;
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:
    stack_trans({ValType::F64, ValType::F64}, {ValType::I32});
    break;
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
    stack_trans({ValType::I32}, {ValType::I32});
    break;
  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I32__mul:
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
    stack_trans({ValType::I32, ValType::I32}, {ValType::I32});
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
    stack_trans({ValType::I32, ValType::I32}, {ValType::I32});
    break;
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
    stack_trans({ValType::I64}, {ValType::I64});
    break;
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_s:
    stack_trans({ValType::I64, ValType::I64}, {ValType::I64});
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
    stack_trans({ValType::I64, ValType::I64}, {ValType::I64});
    break;
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
    stack_trans({ValType::F32}, {ValType::F32});
    break;

  /// 0x90
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
    stack_trans({ValType::F32}, {ValType::F32});
    break;
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
    stack_trans({ValType::F32, ValType::F32}, {ValType::F32});
    break;
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
    stack_trans({ValType::F64}, {ValType::F64});
    break;

  /// 0xA0
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    stack_trans({ValType::F64, ValType::F64}, {ValType::F64});
    break;
  case OpCode::I32__wrap_i64:
    stack_trans({ValType::I64}, {ValType::I32});
    break;
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
    stack_trans({ValType::F32}, {ValType::I32});
    break;
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
    stack_trans({ValType::F64}, {ValType::I32});
    break;
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
    stack_trans({ValType::I32}, {ValType::I64});
    break;
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
    stack_trans({ValType::F32}, {ValType::I64});
    break;

  /// 0XB0
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
    stack_trans({ValType::F64}, {ValType::I64});
    break;
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
    stack_trans({ValType::I32}, {ValType::F32});
    break;
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
    stack_trans({ValType::I64}, {ValType::F32});
    break;
  case OpCode::F32__demote_f64:
    stack_trans({ValType::F64}, {ValType::F32});
    break;
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
    stack_trans({ValType::I32}, {ValType::F64});
    break;
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
    stack_trans({ValType::I64}, {ValType::F64});
    break;
  case OpCode::F64__promote_f32:
    stack_trans({ValType::F32}, {ValType::F64});
    break;
  case OpCode::I32__reinterpret_f32:
    stack_trans({ValType::F32}, {ValType::I32});
    break;
  case OpCode::I64__reinterpret_f64:
    stack_trans({ValType::F64}, {ValType::I64});
    break;
  case OpCode::F32__reinterpret_i32:
    stack_trans({ValType::I32}, {ValType::F32});
    break;
  case OpCode::F64__reinterpret_i64:
    stack_trans({ValType::I64}, {ValType::F64});
    break;

  default:
    cout << "unimp opcode:0x" << std::hex << (uint)opcode << std::dec << endl;
    throw;
  }
}

ErrCode Validator::validate(const AST::Limit *limit, unsigned int K) {
  bool cond1 = limit->getMin() <= K;
  bool cond2 = true;

  if (limit->hasMax()) {
    cond2 = limit->getMax() <= K && limit->getMin() <= limit->getMax();
  }

  if (cond1 && cond2)
    return ErrCode::Success;
  return ErrCode::Invalid;
}

ErrCode Validator::validate(AST::FunctionType *Func) {
  /// The restriction to at most one result may be removed in future versions of
  /// WebAssembly.
  if (Func->getReturnTypes().size() > 1)
    return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::TableType *Tab) {
  return validate(Tab->getLimit(), LIMIT_TABLETYPE);
}

ErrCode Validator::validate(AST::MemoryType *Mem) {
  return validate(Mem->getLimit(), LIMIT_MEMORYTYPE);
}

ErrCode Validator::validate(AST::GlobalType *) {
  /// The global type is valid any way
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::FunctionSection *FuncSec,
                            AST::CodeSection *CodeSec,
                            AST::TypeSection *TypeSec) {
  if (!FuncSec)
    return ErrCode::Success;

  if (FuncSec->getContent().size() != CodeSec->getContent().size())
    return ErrCode::Invalid;

  size_t TotoalFunctions = FuncSec->getContent().size();

  for (size_t id = 0; id < TotoalFunctions; ++id) {
    auto tid = FuncSec->getContent().at(id);

    if (tid >= TypeSec->getContent().size())
      return ErrCode::Invalid;

    vm.addfunc(TypeSec->getContent().at(tid).get());
  }

  for (size_t id = 0; id < TotoalFunctions; ++id) {
    auto tid = FuncSec->getContent().at(id);

    if (validate(CodeSec->getContent().at(id).get(),
                 TypeSec->getContent().at(tid).get()) != ErrCode::Success)
      return ErrCode::Invalid;
  }
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::CodeSegment *CodeSeg,
                            AST::FunctionType *Func) {
  vm.reset();
  int idx = 0;

  for (auto val : Func->getParamTypes()) {
    vm.addloacl(idx++, val);
  }

  for (auto val : CodeSeg->getLocals()) {
    for (unsigned int cnt = 0; cnt < val.first; ++cnt)
      vm.addloacl(idx++, val.second);
  }
  return vm.validate(CodeSeg->getInstrs(), Func->getReturnTypes());
}

ErrCode Validator::validate(AST::MemorySection *MemSec) {
  if (!MemSec)
    return ErrCode::Success;

  for (auto &mem : MemSec->getContent())
    if (validate(mem.get()) != ErrCode::Success)
      return ErrCode::Invalid;

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::TableSection *TabSec) {
  if (!TabSec)
    return ErrCode::Success;

  for (auto &tab:TabSec->getContent())
  {
    if (validate(tab.get()) != ErrCode::Success)
      return ErrCode::Invalid;
    
    switch (tab->getElementType())
    {
      case AST::ElemType::FuncRef:
        break;
      default:
        // In future versions of WebAssembly, additional element types may be introduced.
        return ErrCode::Invalid;
    }
  }

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::GlobalSection *GloSec) {
  if (!GloSec)
    return ErrCode::Success;

  for (auto &val : GloSec->getContent())
    if (validate(val.get()) != ErrCode::Success) {
      return ErrCode::Invalid;
    } else {
      vm.addglobal(*val.get()->getGlobalType());
    }

  cout << "globals=" << GloSec->getContent().size() << endl;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::GlobalSegment *) {
  // TODO: Check GloSeg->getInstrs(); is a const expr
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ElementSegment *EleSeg) {
  // In the current version of WebAssembly, at most one table is allowed in a
  // module. Consequently, the only valid tableidx is 0
  if (EleSeg->getIdx() != 0)
    return ErrCode::Invalid;

  // TODO check EleSeg->getInstrs(); is const expr
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ElementSection *EleSec) {
  if (!EleSec)
    return ErrCode::Success;

  for (auto &element : EleSec->getContent())
    if (validate(element.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::StartSection *StartSec) {
  if (!StartSec)
    return ErrCode::Success;
  cout << "unimp StartSection";
  assert(false && "unimp StartSection");
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportSection *ExportSec) {
  if (!ExportSec)
    return ErrCode::Success;

  for (auto &exportedc : ExportSec->getContent())
    if (validate(exportedc.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportDesc *ExportDesc) {
  /// TODO: Ast not provide where the data is.
  ///      It need to enumerate func/table/mem/global to check the name are
  ///      defined.
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ImportSection *ImportSec,
                            AST::TypeSection *TypeSec) {
  if (!ImportSec)
    return ErrCode::Success;

  for (auto &import : ImportSec->getContent())
    if (validate(import.get(), TypeSec) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ImportDesc *Import,
                            AST::TypeSection *TypeSec) {
  switch (Import->getExternalType()) {
  case SSVM::AST::Desc::ExternalType::Function:
    unsigned int *tid;
    if (Import->getExternalContent(tid) != SSVM::Executor::ErrCode::Success)
      return ErrCode::Invalid;
    vm.addfunc(TypeSec->getContent().at(*tid).get());
    break;
  case SSVM::AST::Desc::ExternalType::Global:
    AST::GlobalType *global;
    if (Import->getExternalContent(global) != SSVM::Executor::ErrCode::Success)
      return ErrCode::Invalid;
    vm.addglobal(*global);
    break;
  default:
    cout << "Unimp ImportDesc:" << (int)Import->getExternalType() << endl;
    return ErrCode::Invalid;
  }

  return ErrCode::Success;
}

ErrCode Validator::validate(std::unique_ptr<AST::Module> &Mod) {
  cout << "start Validator" << endl;
  reset();

  // Every import defines an index in the respective index space. In each index
  // space, the indices of imports go before the first index of any definition
  // contained in the module itself.
  if (validate((*Mod).getImportSection(), (*Mod).getTypeSection()) !=
      ErrCode::Success)
    return ErrCode::Invalid;

  if ((*Mod).getTypeSection())
    for (auto &type:(*Mod).getTypeSection()->getContent())
      vm.addtype(type.get());

  if (validate((*Mod).getTableSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  // https://webassembly.github.io/spec/core/valid/modules.html
  if (validate((*Mod).getMemorySection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getGlobalSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getElementSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getStartSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getExportSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getFunctionSection(), (*Mod).getCodeSection(),
               (*Mod).getTypeSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  cout << "Validator OK" << endl;
  cout << std::flush;
  return ErrCode::Success;
}

void Validator::reset() { vm.reset(true); }

} // namespace Validator
} // namespace SSVM