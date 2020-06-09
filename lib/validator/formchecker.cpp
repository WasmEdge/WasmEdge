// SPDX-License-Identifier: Apache-2.0
#include "validator/formchecker.h"
#include "common/ast/module.h"

namespace SSVM {
namespace Validator {

void FormChecker::reset(bool CleanGlobal) {
  ValStack.clear();
  CtrlStack.clear();
  Locals.clear();
  Returns.clear();

  if (CleanGlobal) {
    Types.clear();
    Funcs.clear();
    Tables.clear();
    Mems.clear();
    Globals.clear();
    NumImportGlobals = 0;
  }
}

Expect<void> FormChecker::validate(const AST::InstrVec &Instrs,
                                   Span<const ValType> RetVals) {
  for (ValType Val : RetVals) {
    Returns.push_back(ASTToVType(Val));
  }
  pushCtrl(Returns, Returns);
  return checkInstrs(Instrs);
}

Expect<void> FormChecker::validate(const AST::InstrVec &Instrs,
                                   Span<const VType> RetVals) {
  for (VType Val : RetVals) {
    Returns.push_back(Val);
  }
  pushCtrl(Returns, Returns);
  return checkInstrs(Instrs);
}

void FormChecker::addType(const AST::FunctionType &Func) {
  std::vector<VType> Param, Ret;
  for (auto Val : Func.getParamTypes()) {
    Param.emplace_back(ASTToVType(Val));
  }
  for (auto Val : Func.getReturnTypes()) {
    Ret.emplace_back(ASTToVType(Val));
  }
  Types.emplace_back(Param, Ret);
}

void FormChecker::addFunc(const uint32_t &TypeIdx) {
  if (Types.size() > TypeIdx) {
    Funcs.emplace_back(TypeIdx);
  }
}

void FormChecker::addTable(const AST::TableType &Tab) {
  Tables.emplace_back(Tab.getElementType());
}

void FormChecker::addMemory(const AST::MemoryType &Mem) {
  Mems.emplace_back(Mems.size());
}

void FormChecker::addGlobal(const AST::GlobalType &Glob, const bool IsImport) {
  /// Type in global is comfirmed in loading phase.
  Globals.emplace_back(ASTToVType(Glob.getValueType()),
                       Glob.getValueMutation());
  if (IsImport) {
    NumImportGlobals++;
  }
}

void FormChecker::addLocal(const ValType &V) {
  Locals.emplace_back(ASTToVType(V));
}

void FormChecker::addLocal(const VType &V) { Locals.emplace_back(V); }

VType FormChecker::ASTToVType(const ValType &V) {
  switch (V) {
  case ValType::I32:
    return VType::I32;
  case ValType::I64:
    return VType::I64;
  case ValType::F32:
    return VType::F32;
  case ValType::F64:
    return VType::F64;
  default:
    return VType::Unknown;
  }
}

Expect<void> FormChecker::checkInstrs(const AST::InstrVec &Instrs) {
  for (auto &Instr : Instrs) {
    OpCode Code = Instr->getOpCode();
    auto Res =
        dispatchInstruction(Code, [this, &Instr](auto &&Arg) -> Expect<void> {
          if constexpr (std::is_void_v<
                            typename std::decay_t<decltype(Arg)>::type>) {
            /// If the Code not matched, validation failed.
            return Unexpect(ErrCode::ValidationFailed);
          } else {
            /// Check the corresponding instruction.
            return checkInstr(
                *static_cast<typename std::decay_t<decltype(Arg)>::type *>(
                    Instr.get()));
          }
        });
    if (!Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

Expect<void> FormChecker::checkInstr(const AST::ControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Unreachable:
    return unreachable();
  case OpCode::Nop:
    return {};
  case OpCode::Return:
    if (auto Res = popTypes(Returns); !Res) {
      return Unexpect(Res);
    }
    return unreachable();
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void>
FormChecker::checkInstr(const AST::BlockControlInstruction &Instr) {
  /// Get blocktype [t*]
  std::vector<VType> ResVec;
  if (Instr.getResultType() != ValType::None) {
    ResVec.emplace_back(ASTToVType(Instr.getResultType()));
  }
  switch (Instr.getOpCode()) {
  case OpCode::Block: {
    /// Push ctrl frame ([t*], [t*])
    pushCtrl(ResVec, ResVec);
    break;
  }
  case OpCode::Loop: {
    /// Push ctrl frame ([], [t*])
    pushCtrl({}, ResVec);
    break;
  }
  default:
    return Unexpect(ErrCode::ValidationFailed);
  }
  /// Check block body
  if (auto Res = checkInstrs(Instr.getBody()); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = popCtrl()) {
    pushTypes(*Res);
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void>
FormChecker::checkInstr(const AST::IfElseControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::If: {
    /// Get blocktype [t*]
    std::vector<VType> ResVec;
    if (Instr.getResultType() != ValType::None) {
      ResVec.emplace_back(ASTToVType(Instr.getResultType()));
    }
    /// Pop I32
    popType(VType::I32);
    /// Push ctrl frame ([t*], [t*]) and check body
    pushCtrl(ResVec, ResVec);
    if (auto Res = checkInstrs(Instr.getIfStatement()); !Res) {
      return Unexpect(Res);
    }
    /// Else case, push ctrl frame (Results, Results) and check body
    if (Instr.getElseStatement().size() > 0) {
      if (auto Results = popCtrl()) {
        pushCtrl(*Results, *Results);
        if (auto Res = checkInstrs(Instr.getElseStatement()); !Res) {
          return Unexpect(Res);
        }
      } else {
        return Unexpect(Results);
      }
    }
    if (auto Res = popCtrl()) {
      pushTypes(*Res);
    } else {
      return Unexpect(Res);
    }
    return {};
  }
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void> FormChecker::checkInstr(const AST::BrControlInstruction &Instr) {
  auto N = Instr.getLabelIndex();
  if (CtrlStack.size() <= N) {
    /// Branch out of stack
    return Unexpect(ErrCode::ValidationFailed);
  }
  switch (Instr.getOpCode()) {
  case OpCode::Br: {
    if (auto Res = popTypes(CtrlStack[N].LabelTypes); !Res) {
      return Unexpect(Res);
    }
    return unreachable();
  }
  case OpCode::Br_if: {
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = popTypes(CtrlStack[N].LabelTypes); !Res) {
      return Unexpect(Res);
    }
    pushTypes(CtrlStack[N].LabelTypes);
    return {};
  }
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void>
FormChecker::checkInstr(const AST::BrTableControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Br_table: {
    auto M = Instr.getLabelIndex();
    if (CtrlStack.size() <= M) {
      /// Branch out of table index
      return Unexpect(ErrCode::ValidationFailed);
    }
    for (auto &N : Instr.getLabelTable()) {
      // Error_if(ctrls.size() < n || ctrls[n].label_types =/=
      // ctrls[m].label_types)
      if (CtrlStack.size() <= N) {
        /// Branch out of stack
        return Unexpect(ErrCode::ValidationFailed);
      }
      if (CtrlStack[N].LabelTypes != CtrlStack[M].LabelTypes) {
        /// CtrlStack[N].label_types != CtrlStack[M].label_types
        return Unexpect(ErrCode::ValidationFailed);
      }
    }
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = popTypes(CtrlStack[M].LabelTypes); !Res) {
      return Unexpect(Res);
    }
    return unreachable();
  }
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void> FormChecker::checkInstr(const AST::CallControlInstruction &Instr) {
  auto N = Instr.getFuncIndex();
  switch (Instr.getOpCode()) {
  case OpCode::Call: {
    if (Funcs.size() <= N) {
      /// Call function index out of range
      return Unexpect(ErrCode::ValidationFailed);
    }
    return StackTrans(Types[Funcs[N]].first, Types[Funcs[N]].second);
  }
  case OpCode::Call_indirect: {
    if (Tables.size() == 0) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    if (Tables[0] != ElemType::FuncRef) {
      return Unexpect(ErrCode::ValidationFailed);
    }
    if (Types.size() <= N) {
      /// Function type index out of range
      return Unexpect(ErrCode::ValidationFailed);
    }
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    return StackTrans(Types[N].first, Types[N].second);
  }
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void> FormChecker::checkInstr(const AST::ParametricInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Drop:
    return StackTrans(std::array{VType::Unknown}, {});
  case OpCode::Select: {
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    VType T1, T2;
    if (auto Res = popType()) {
      T1 = *Res;
    } else {
      return Unexpect(Res);
    }
    if (auto Res = popType(T1)) {
      T2 = *Res;
    } else {
      return Unexpect(Res);
    }
    pushType(T2);
    return {};
  }
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void> FormChecker::checkInstr(const AST::VariableInstruction &Instr) {
  /// Check accessing index and expect variable type
  VType TExpect;
  switch (Instr.getOpCode()) {
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee:
    if (Instr.getVariableIndex() >= Locals.size()) {
      /// Local index out of range
      return Unexpect(ErrCode::ValidationFailed);
    }
    TExpect = Locals[Instr.getVariableIndex()];
    break;
  case OpCode::Global__get:
  case OpCode::Global__set:
    if (Instr.getVariableIndex() >= Globals.size()) {
      /// Global index out of range
      return Unexpect(ErrCode::ValidationFailed);
    }
    TExpect = Globals[Instr.getVariableIndex()].first;
    break;
  default:
    return Unexpect(ErrCode::ValidationFailed);
  }

  /// Pop type and check is type matching
  switch (Instr.getOpCode()) {
  case OpCode::Global__set:
    /// Global case, check mutation.
    if (Globals[Instr.getVariableIndex()].second != ValMut::Var) {
      /// Global is immutable
      return Unexpect(ErrCode::ValidationFailed);
    }
    // fall through
  case OpCode::Local__set:
  case OpCode::Local__tee:
    if (auto Res = popType(TExpect); !Res) {
      return Unexpect(Res);
    }
    break;
  default:
    break;
  }

  /// Push type
  switch (Instr.getOpCode()) {
  case OpCode::Local__get:
  case OpCode::Local__tee:
  case OpCode::Global__get:
    pushType(TExpect);
  default:
    break;
  }
  return {};
}

Expect<void> FormChecker::checkInstr(const AST::MemoryInstruction &Instr) {
  /// Memory[0] must exist
  if (Mems.size() == 0) {
    return Unexpect(ErrCode::ValidationFailed);
  }

  /// Get bit width
  uint32_t N = 8;
  switch (Instr.getOpCode()) {
  case OpCode::I64__load:
  case OpCode::F64__load:
  case OpCode::I64__store:
  case OpCode::F64__store:
    /// N will be 64 in the end
    N <<= 1;
    // fall through

  case OpCode::I32__load:
  case OpCode::F32__load:
  case OpCode::I32__store:
  case OpCode::F32__store:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
  case OpCode::I64__store32:
    /// N will be 32 in the end
    N <<= 1;
    // fall through

  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I32__store16:
  case OpCode::I64__store16:
    /// N will be 16 in the end
    N <<= 1;
    // fall through

  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I32__store8:
  case OpCode::I64__store8:
    /// N will be 8 in the end
    if ((1UL << Instr.getMemoryAlign()) > (N >> 3UL)) {
      /// 2 ^ align needs to <= N / 8
      return Unexpect(ErrCode::ValidationFailed);
    }
    break;
  case OpCode::Memory__size:
  case OpCode::Memory__grow:
    break;
  default:
    return Unexpect(ErrCode::ValidationFailed);
  }

  switch (Instr.getOpCode()) {
  case OpCode::I32__load:
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
    return StackTrans(std::array{VType::I32}, std::array{VType::I32});
  case OpCode::I64__load:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
    return StackTrans(std::array{VType::I32}, std::array{VType::I64});
  case OpCode::F32__load:
    return StackTrans(std::array{VType::I32}, std::array{VType::F32});
  case OpCode::F64__load:
    return StackTrans(std::array{VType::I32}, std::array{VType::F64});
  case OpCode::I32__store:
  case OpCode::I32__store8:
  case OpCode::I32__store16:
    return StackTrans(std::array{VType::I32, VType::I32}, {});
  case OpCode::I64__store:
  case OpCode::I64__store8:
  case OpCode::I64__store16:
  case OpCode::I64__store32:
    return StackTrans(std::array{VType::I32, VType::I64}, {});
  case OpCode::F32__store:
    return StackTrans(std::array{VType::I32, VType::F32}, {});
  case OpCode::F64__store:
    return StackTrans(std::array{VType::I32, VType::F64}, {});
  case OpCode::Memory__size:
    return StackTrans({}, std::array{VType::I32});
  case OpCode::Memory__grow:
    return StackTrans(std::array{VType::I32}, std::array{VType::I32});
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void> FormChecker::checkInstr(const AST::ConstInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::I32__const:
    return StackTrans({}, std::array{VType::I32});
  case OpCode::I64__const:
    return StackTrans({}, std::array{VType::I64});
  case OpCode::F32__const:
    return StackTrans({}, std::array{VType::F32});
  case OpCode::F64__const:
    return StackTrans({}, std::array{VType::F64});
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void>
FormChecker::checkInstr(const AST::UnaryNumericInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::I32__eqz:
    return StackTrans(std::array{VType::I32}, std::array{VType::I32});
  case OpCode::I64__eqz:
    return StackTrans(std::array{VType::I64}, std::array{VType::I32});
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
    return StackTrans(std::array{VType::I32}, std::array{VType::I32});
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
    return StackTrans(std::array{VType::I64}, std::array{VType::I64});
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
    return StackTrans(std::array{VType::F32}, std::array{VType::F32});
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
    return StackTrans(std::array{VType::F64}, std::array{VType::F64});
  case OpCode::I32__wrap_i64:
    return StackTrans(std::array{VType::I64}, std::array{VType::I32});
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
    return StackTrans(std::array{VType::F32}, std::array{VType::I32});
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
    return StackTrans(std::array{VType::F64}, std::array{VType::I32});
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
    return StackTrans(std::array{VType::I32}, std::array{VType::I64});
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
    return StackTrans(std::array{VType::F32}, std::array{VType::I64});
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
    return StackTrans(std::array{VType::F64}, std::array{VType::I64});
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
    return StackTrans(std::array{VType::I32}, std::array{VType::F32});
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
    return StackTrans(std::array{VType::I64}, std::array{VType::F32});
  case OpCode::F32__demote_f64:
    return StackTrans(std::array{VType::F64}, std::array{VType::F32});
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
    return StackTrans(std::array{VType::I32}, std::array{VType::F64});
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
    return StackTrans(std::array{VType::I64}, std::array{VType::F64});
  case OpCode::F64__promote_f32:
    return StackTrans(std::array{VType::F32}, std::array{VType::F64});
  case OpCode::I32__reinterpret_f32:
    return StackTrans(std::array{VType::F32}, std::array{VType::I32});
  case OpCode::I64__reinterpret_f64:
    return StackTrans(std::array{VType::F64}, std::array{VType::I64});
  case OpCode::F32__reinterpret_i32:
    return StackTrans(std::array{VType::I32}, std::array{VType::F32});
  case OpCode::F64__reinterpret_i64:
    return StackTrans(std::array{VType::I64}, std::array{VType::F64});
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

Expect<void>
FormChecker::checkInstr(const AST::BinaryNumericInstruction &Instr) {
  switch (Instr.getOpCode()) {
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
    return StackTrans(std::array{VType::I32, VType::I32},
                      std::array{VType::I32});
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
    return StackTrans(std::array{VType::I64, VType::I64},
                      std::array{VType::I32});
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
  case OpCode::F32__ge:
    return StackTrans(std::array{VType::F32, VType::F32},
                      std::array{VType::I32});
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:
    return StackTrans(std::array{VType::F64, VType::F64},
                      std::array{VType::I32});
  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I32__mul:
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
  case OpCode::I32__rem_u:
  case OpCode::I32__and:
  case OpCode::I32__or:
  case OpCode::I32__xor:
  case OpCode::I32__shl:
  case OpCode::I32__shr_s:
  case OpCode::I32__shr_u:
  case OpCode::I32__rotl:
  case OpCode::I32__rotr:
    return StackTrans(std::array{VType::I32, VType::I32},
                      std::array{VType::I32});
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_s:
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
    return StackTrans(std::array{VType::I64, VType::I64},
                      std::array{VType::I64});
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
    return StackTrans(std::array{VType::F32, VType::F32},
                      std::array{VType::F32});
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    return StackTrans(std::array{VType::F64, VType::F64},
                      std::array{VType::F64});
  default:
    break;
  }
  return Unexpect(ErrCode::ValidationFailed);
}

void FormChecker::pushType(VType V) { ValStack.emplace_front(V); }

void FormChecker::pushTypes(Span<const VType> Input) {
  for (auto Val : Input) {
    pushType(Val);
  }
}

Expect<VType> FormChecker::popType() {
  if (ValStack.size() == CtrlStack[0].Height) {
    if (CtrlStack[0].IsUnreachable) {
      return VType::Unknown;
    }
    /// Value stack underflow
    return Unexpect(ErrCode::ValidationFailed);
  }
  auto Res = ValStack.front();
  ValStack.pop_front();
  return Res;
}

Expect<VType> FormChecker::popType(VType E) {
  auto Res = popType();
  if (!Res) {
    return Unexpect(Res);
  }
  if (*Res == VType::Unknown) {
    return E;
  }
  if (E == VType::Unknown) {
    return *Res;
  }
  if (*Res != E) {
    /// Expect value on value stack is not matched
    return Unexpect(ErrCode::ValidationFailed);
  }
  return *Res;
}

Expect<void> FormChecker::popTypes(Span<const VType> Input) {
  for (auto Val = Input.rbegin(); Val != Input.rend(); ++Val) {
    if (auto Res = popType(*Val); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

void FormChecker::pushCtrl(Span<const VType> Label, Span<const VType> Out) {
  CtrlFrame Frame = {.LabelTypes = {Label.begin(), Label.end()},
                     .EndTypes = {Out.begin(), Out.end()},
                     .Height = ValStack.size(),
                     .IsUnreachable = false};
  CtrlStack.emplace_front(Frame);
}

Expect<std::vector<VType>> FormChecker::popCtrl() {
  if (CtrlStack.empty()) {
    /// Ctrl stack is empty when pop.
    return Unexpect(ErrCode::ValidationFailed);
  }
  auto Head = CtrlStack.front();
  auto ResTypes = Head.EndTypes;
  if (auto Res = popTypes(ResTypes); !Res) {
    return Unexpect(Res);
  }
  if (ValStack.size() != Head.Height) {
    /// Value stack size not matched.
    return Unexpect(ErrCode::ValidationFailed);
  }
  CtrlStack.pop_front();
  return Head.EndTypes;
}

Expect<void> FormChecker::unreachable() {
  while (ValStack.size() > CtrlStack[0].Height) {
    if (auto Res = popType(); !Res) {
      return Unexpect(Res);
    }
  }
  CtrlStack[0].IsUnreachable = true;
  return {};
}

Expect<void> FormChecker::StackTrans(Span<const VType> Take,
                                     Span<const VType> Put) {
  if (auto Res = popTypes(Take); !Res) {
    return Unexpect(Res);
  }
  pushTypes(Put);
  return {};
}

} // namespace Validator
} // namespace SSVM
