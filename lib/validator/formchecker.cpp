// SPDX-License-Identifier: Apache-2.0
#include "validator/formchecker.h"
#include "common/ast/module.h"

namespace {
template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overloaded(Ts...)->overloaded<Ts...>;
} // namespace

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
    NumImportFuncs = 0;
    NumImportGlobals = 0;
  }
}

Expect<void> FormChecker::validate(const AST::InstrVec &Instrs,
                                   Span<const ValType> RetVals) {
  for (ValType Val : RetVals) {
    Returns.push_back(ASTToVType(Val));
  }
  return checkExpr(Instrs);
}

Expect<void> FormChecker::validate(const AST::InstrVec &Instrs,
                                   Span<const VType> RetVals) {
  for (VType Val : RetVals) {
    Returns.push_back(Val);
  }
  return checkExpr(Instrs);
}

void FormChecker::addType(const AST::FunctionType &Func) {
  std::vector<VType> Param, Ret;
  Param.reserve(Func.getParamTypes().size());
  Ret.reserve(Func.getReturnTypes().size());
  for (auto Val : Func.getParamTypes()) {
    Param.push_back(ASTToVType(Val));
  }
  for (auto Val : Func.getReturnTypes()) {
    Ret.push_back(ASTToVType(Val));
  }
  Types.emplace_back(std::move(Param), std::move(Ret));
}

void FormChecker::addFunc(const uint32_t TypeIdx, const bool IsImport) {
  if (Types.size() > TypeIdx) {
    Funcs.emplace_back(TypeIdx);
  }
  if (IsImport) {
    NumImportFuncs++;
  }
}

void FormChecker::addTable(const AST::TableType &Tab) {
  Tables.push_back(Tab.getElementType());
}

void FormChecker::addMemory(const AST::MemoryType &Mem) {
  Mems.push_back(Mems.size());
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
  Locals.push_back(ASTToVType(V));
}

void FormChecker::addLocal(const VType &V) { Locals.push_back(V); }

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

ValType FormChecker::VTypeToAST(const VType &V) {
  switch (V) {
  case VType::I32:
    return ValType::I32;
  case VType::I64:
    return ValType::I64;
  case VType::F32:
    return ValType::F32;
  case VType::F64:
    return ValType::F64;
  default:
    return ValType::I32;
  }
}

Expect<void> FormChecker::checkExpr(const AST::InstrVec &Instrs) {
  /// Push ctrl frame ([] -> [Returns])
  pushCtrl({}, Returns);
  if (auto Res = checkInstrs(Instrs); !Res) {
    return Unexpect(Res);
  }
  /// Pop ctrl frame
  if (auto Res = popCtrl(); !Res) {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> FormChecker::checkInstrs(const AST::InstrVec &Instrs) {
  /// Validate instructions
  for (auto &Instr : Instrs) {
    if (auto Res = AST::dispatchInstruction(
            Instr->getOpCode(),
            [this, &Instr](auto &&Arg) -> Expect<void> {
              if constexpr (std::is_void_v<
                                typename std::decay_t<decltype(Arg)>::type>) {
                /// If the Code not matched, validation failed.
                LOG(ERROR) << ErrCode::InvalidOpCode;
                LOG(ERROR) << ErrInfo::InfoInstruction(Instr->getOpCode(),
                                                       Instr->getOffset());
                return Unexpect(ErrCode::InvalidOpCode);
              } else {
                /// Check the corresponding instruction.
                auto Check = checkInstr(
                    *static_cast<typename std::decay_t<decltype(Arg)>::type *>(
                        Instr.get()));
                if (!Check) {
                  LOG(ERROR) << ErrInfo::InfoInstruction(Instr->getOpCode(),
                                                         Instr->getOffset());
                }
                return Check;
              }
            });
        !Res) {
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
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
}

Expect<void>
FormChecker::checkInstr(const AST::BlockControlInstruction &Instr) {
  /// Get blocktype [t1*] -> [t2*]
  std::vector<VType> Buffer;
  Span<const VType> T1, T2;
  if (auto Res = resolveBlockType(Buffer, Instr.getBlockType())) {
    std::tie(T1, T2) = std::move(*Res);
  } else {
    return Unexpect(Res);
  }

  /// Check type transformation
  switch (Instr.getOpCode()) {
  case OpCode::Block:
  case OpCode::Loop:
    /// Pop and check [t1*]
    if (auto Res = popTypes(T1); !Res) {
      return Unexpect(Res);
    }
    /// Push ctrl frame ([t1*], [t2*])
    pushCtrl(T1, T2, (Instr.getOpCode() == OpCode::Loop));
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }

  /// Check block body
  if (auto Res = checkInstrs(Instr.getBody()); !Res) {
    return Unexpect(Res);
  }

  /// End of block body
  if (auto Res = popCtrl()) {
    pushTypes((*Res).EndTypes);
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void>
FormChecker::checkInstr(const AST::IfElseControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::If: {
    /// Get blocktype [t1*] -> [t2*]
    std::vector<VType> Buffer;
    Span<const VType> T1, T2;
    if (auto Res = resolveBlockType(Buffer, Instr.getBlockType())) {
      std::tie(T1, T2) = std::move(*Res);
    } else {
      return Unexpect(Res);
    }

    /// Pop I32
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    /// Pop and check [t1*]
    if (auto Res = popTypes(T1); !Res) {
      return Unexpect(Res);
    }
    /// Push ctrl frame ([t1*], [t2*])
    pushCtrl(T1, T2);

    /// Check `if` body.
    if (auto Res = checkInstrs(Instr.getIfStatement()); !Res) {
      return Unexpect(Res);
    }

    /// Check `else` body.
    if (auto Frame = popCtrl()) {
      pushCtrl((*Frame).StartTypes, (*Frame).EndTypes);
      if (auto Res = checkInstrs(Instr.getElseStatement()); !Res) {
        return Unexpect(Res);
      }
    } else {
      return Unexpect(Frame);
    }

    /// End of `if-else` statement.
    if (auto Res = popCtrl()) {
      pushTypes((*Res).EndTypes);
    } else {
      return Unexpect(Res);
    }
    return {};
  }
  default:
    break;
  }
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
}

Expect<void> FormChecker::checkInstr(const AST::BrControlInstruction &Instr) {
  const uint32_t N = Instr.getLabelIndex();
  if (CtrlStack.size() <= N) {
    /// Branch out of stack
    LOG(ERROR) << ErrCode::InvalidLabelIdx;
    LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Label, N,
                                           CtrlStack.size());
    return Unexpect(ErrCode::InvalidLabelIdx);
  }
  const uint32_t N_ = CtrlStack.size() - 1 - N;
  switch (Instr.getOpCode()) {
  case OpCode::Br: {
    if (auto Res = popTypes(getLabelTypes(CtrlStack[N_])); !Res) {
      return Unexpect(Res);
    }
    return unreachable();
  }
  case OpCode::Br_if: {
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = popTypes(getLabelTypes(CtrlStack[N_])); !Res) {
      return Unexpect(Res);
    }
    pushTypes(getLabelTypes(CtrlStack[N_]));
    return {};
  }
  default:
    break;
  }
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
}

Expect<void>
FormChecker::checkInstr(const AST::BrTableControlInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Br_table: {
    const uint32_t M = Instr.getLabelIndex();
    if (CtrlStack.size() <= M) {
      /// Branch out of table index
      LOG(ERROR) << ErrCode::InvalidLabelIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Label, M,
                                             CtrlStack.size());
      return Unexpect(ErrCode::InvalidLabelIdx);
    }
    const uint32_t M_ = CtrlStack.size() - 1 - M;
    for (const uint32_t &N : Instr.getLabelList()) {
      if (CtrlStack.size() <= N) {
        /// Branch out of stack
        LOG(ERROR) << ErrCode::InvalidLabelIdx;
        LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Label, N,
                                               CtrlStack.size());
        return Unexpect(ErrCode::InvalidLabelIdx);
      }
      const uint32_t N_ = CtrlStack.size() - 1 - N;
      Span<const VType> NSpan = getLabelTypes(CtrlStack[N_]);
      Span<const VType> MSpan = getLabelTypes(CtrlStack[M_]);
      if (NSpan.size() != MSpan.size() ||
          !std::equal(NSpan.begin(), NSpan.end(), MSpan.begin())) {
        /// CtrlStack[N_].label_types != CtrlStack[M_].label_types
        std::vector<ValType> NList, MList;
        NList.reserve(NSpan.size());
        for (auto &I : NSpan) {
          NList.push_back(VTypeToAST(I));
        }
        MList.reserve(MSpan.size());
        for (auto &I : MSpan) {
          MList.push_back(VTypeToAST(I));
        }
        LOG(ERROR) << ErrCode::TypeCheckFailed;
        LOG(ERROR) << ErrInfo::InfoMismatch(MList, NList);
        return Unexpect(ErrCode::TypeCheckFailed);
      }
    }
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = popTypes(getLabelTypes(CtrlStack[M_])); !Res) {
      return Unexpect(Res);
    }
    return unreachable();
  }
  default:
    break;
  }
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
}

Expect<void> FormChecker::checkInstr(const AST::CallControlInstruction &Instr) {
  auto N = Instr.getTargetIndex();
  switch (Instr.getOpCode()) {
  case OpCode::Call: {
    if (Funcs.size() <= N) {
      /// Call function index out of range
      LOG(ERROR) << ErrCode::InvalidFuncIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function,
                                             N, Funcs.size());
      return Unexpect(ErrCode::InvalidFuncIdx);
    }
    return StackTrans(Types[Funcs[N]].first, Types[Funcs[N]].second);
  }
  case OpCode::Call_indirect: {
    if (Tables.size() == 0) {
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Table, 0,
                                             Tables.size());
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    if (Tables[0] != ElemType::FuncRef) {
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    if (Types.size() <= N) {
      /// Function type index out of range
      LOG(ERROR) << ErrCode::InvalidFuncTypeIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::FunctionType, N, Types.size());
      return Unexpect(ErrCode::InvalidFuncTypeIdx);
    }
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    return StackTrans(Types[N].first, Types[N].second);
  }
  default:
    break;
  }
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
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
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
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
      LOG(ERROR) << ErrCode::InvalidLocalIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Local,
                                             Instr.getVariableIndex(),
                                             Locals.size());
      return Unexpect(ErrCode::InvalidLocalIdx);
    }
    TExpect = Locals[Instr.getVariableIndex()];
    break;
  case OpCode::Global__get:
  case OpCode::Global__set:
    if (Instr.getVariableIndex() >= Globals.size()) {
      /// Global index out of range
      LOG(ERROR) << ErrCode::InvalidGlobalIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Global,
                                             Instr.getVariableIndex(),
                                             Locals.size());
      return Unexpect(ErrCode::InvalidGlobalIdx);
    }
    TExpect = Globals[Instr.getVariableIndex()].first;
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }

  /// Pop type and check is type matching
  switch (Instr.getOpCode()) {
  case OpCode::Global__set:
    /// Global case, check mutation.
    if (Globals[Instr.getVariableIndex()].second != ValMut::Var) {
      /// Global is immutable
      LOG(ERROR) << ErrCode::ImmutableGlobal;
      return Unexpect(ErrCode::ImmutableGlobal);
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
    LOG(ERROR) << ErrCode::InvalidMemoryIdx;
    LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, 0,
                                           Mems.size());
    return Unexpect(ErrCode::InvalidMemoryIdx);
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
    [[fallthrough]];

  case OpCode::I32__load:
  case OpCode::F32__load:
  case OpCode::I32__store:
  case OpCode::F32__store:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
  case OpCode::I64__store32:
    /// N will be 32 in the end
    N <<= 1;
    [[fallthrough]];

  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I32__store16:
  case OpCode::I64__store16:
    /// N will be 16 in the end
    N <<= 1;
    [[fallthrough]];

  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I32__store8:
  case OpCode::I64__store8:
    /// N will be 8 in the end

    if (Instr.getMemoryAlign() > 31 ||
        (1UL << Instr.getMemoryAlign()) > (N >> 3UL)) {
      /// 2 ^ align needs to <= N / 8
      LOG(ERROR) << ErrCode::InvalidAlignment;
      LOG(ERROR) << ErrInfo::InfoMismatch(uint8_t(N >> 3),
                                          Instr.getMemoryAlign());
      return Unexpect(ErrCode::InvalidAlignment);
    }
    break;
  case OpCode::Memory__size:
  case OpCode::Memory__grow:
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
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
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
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
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
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
  case OpCode::I32__extend8_s:
  case OpCode::I32__extend16_s:
    return StackTrans(std::array{VType::I32}, std::array{VType::I32});
  case OpCode::I64__extend8_s:
  case OpCode::I64__extend16_s:
  case OpCode::I64__extend32_s:
    return StackTrans(std::array{VType::I64}, std::array{VType::I64});
  case OpCode::I32__trunc_sat_f32_s:
  case OpCode::I32__trunc_sat_f32_u:
    return StackTrans(std::array{VType::F32}, std::array{VType::I32});
  case OpCode::I32__trunc_sat_f64_s:
  case OpCode::I32__trunc_sat_f64_u:
    return StackTrans(std::array{VType::F64}, std::array{VType::I32});
  case OpCode::I64__trunc_sat_f32_s:
  case OpCode::I64__trunc_sat_f32_u:
    return StackTrans(std::array{VType::F32}, std::array{VType::I64});
  case OpCode::I64__trunc_sat_f64_s:
  case OpCode::I64__trunc_sat_f64_u:
    return StackTrans(std::array{VType::F64}, std::array{VType::I64});
  default:
    break;
  }
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
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
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
}

void FormChecker::pushType(VType V) { ValStack.emplace_back(V); }

void FormChecker::pushTypes(Span<const VType> Input) {
  for (auto Val : Input) {
    pushType(Val);
  }
}

Expect<VType> FormChecker::popType() {
  if (ValStack.size() == CtrlStack.back().Height) {
    if (CtrlStack.back().IsUnreachable) {
      return VType::Unknown;
    }
    /// Value stack underflow
    LOG(ERROR) << ErrCode::TypeCheckFailed;
    LOG(ERROR) << "    Value stack underflow.";
    return Unexpect(ErrCode::TypeCheckFailed);
  }
  auto Res = ValStack.back();
  ValStack.pop_back();
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
    LOG(ERROR) << ErrCode::TypeCheckFailed;
    LOG(ERROR) << ErrInfo::InfoMismatch(VTypeToAST(E), VTypeToAST(*Res));
    return Unexpect(ErrCode::TypeCheckFailed);
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

void FormChecker::pushCtrl(Span<const VType> In, Span<const VType> Out,
                           bool IsLoopOp) {
  CtrlStack.emplace_back(In, Out, ValStack.size(), IsLoopOp);
  pushTypes(In);
}

Expect<FormChecker::CtrlFrame> FormChecker::popCtrl() {
  if (CtrlStack.empty()) {
    /// Ctrl stack is empty when popping.
    LOG(ERROR) << ErrCode::TypeCheckFailed;
    LOG(ERROR) << "    Control stack underflow.";
    return Unexpect(ErrCode::TypeCheckFailed);
  }
  if (auto Res = popTypes(CtrlStack.back().EndTypes); !Res) {
    return Unexpect(Res);
  }
  if (ValStack.size() != CtrlStack.back().Height) {
    /// Value stack size not matched.
    LOG(ERROR) << ErrCode::TypeCheckFailed;
    LOG(ERROR) << "    Value stack underflow.";
    return Unexpect(ErrCode::TypeCheckFailed);
  }
  auto Head = std::move(CtrlStack.back());
  CtrlStack.pop_back();
  return Head;
}

Span<const VType> FormChecker::getLabelTypes(const FormChecker::CtrlFrame &F) {
  if (F.IsLoop) {
    return F.StartTypes;
  }
  return F.EndTypes;
}

Expect<void> FormChecker::unreachable() {
  while (ValStack.size() > CtrlStack.back().Height) {
    if (auto Res = popType(); !Res) {
      return Unexpect(Res);
    }
  }
  CtrlStack.back().IsUnreachable = true;
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

Expect<std::pair<Span<const VType>, Span<const VType>>>
FormChecker::resolveBlockType(std::vector<VType> &Buffer, BlockType Type) {
  using ReturnType = std::pair<Span<const VType>, Span<const VType>>;
  return std::visit(
      overloaded{
          [this, &Buffer](ValType RetType) -> Expect<ReturnType> {
            /// ValType case. t2* = valtype | none
            if (RetType != ValType::None) {
              Buffer.clear();
              Buffer.reserve(1);
              Buffer.push_back(ASTToVType(RetType));
            }
            return ReturnType{{}, Buffer};
          },
          [this](uint32_t TypeIdx) -> Expect<ReturnType> {
            /// Type index case. t2* = type[index].returns
            if (Types.size() <= TypeIdx) {
              /// Function type index out of range.
              LOG(ERROR) << ErrCode::InvalidFuncTypeIdx;
              LOG(ERROR) << ErrInfo::InfoForbidIndex(
                  ErrInfo::IndexCategory::FunctionType, TypeIdx, Types.size());
              return Unexpect(ErrCode::InvalidFuncTypeIdx);
            }
            return ReturnType{Types[TypeIdx].first, Types[TypeIdx].second};
          }},
      Type);
}

} // namespace Validator
} // namespace SSVM
