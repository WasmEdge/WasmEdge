// SPDX-License-Identifier: Apache-2.0
#include "validator/formchecker.h"
#include "ast/module.h"

namespace {
template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;
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
    Datas.clear();
    Elems.clear();
    Refs.clear();
    NumImportFuncs = 0;
    NumImportGlobals = 0;
  }
}

Expect<void> FormChecker::validate(const AST::InstrVec &Instrs,
                                   Span<const ValType> RetVals) {
  for (const ValType &Val : RetVals) {
    Returns.push_back(ASTToVType(Val));
  }
  return checkExpr(Instrs);
}

Expect<void> FormChecker::validate(const AST::InstrVec &Instrs,
                                   Span<const VType> RetVals) {
  for (const VType &Val : RetVals) {
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
  Tables.push_back(Tab.getReferenceType());
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

void FormChecker::addData(const AST::DataSegment &Data) {
  Datas.emplace_back(Datas.size());
}

void FormChecker::addElem(const AST::ElementSegment &Elem) {
  Elems.emplace_back(Elem.getRefType());
}

void FormChecker::addRef(const uint32_t FuncIdx) { Refs.emplace(FuncIdx); }

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
  case ValType::V128:
    return VType::V128;
  case ValType::FuncRef:
    return VType::FuncRef;
  case ValType::ExternRef:
    return VType::ExternRef;
  default:
    return VType::Unknown;
  }
}

VType FormChecker::ASTToVType(const NumType &V) {
  switch (V) {
  case NumType::I32:
    return VType::I32;
  case NumType::I64:
    return VType::I64;
  case NumType::F32:
    return VType::F32;
  case NumType::F64:
    return VType::F64;
  case NumType::V128:
    return VType::V128;
  default:
    return VType::Unknown;
  }
}

VType FormChecker::ASTToVType(const RefType &V) {
  switch (V) {
  case RefType::FuncRef:
    return VType::FuncRef;
  case RefType::ExternRef:
    return VType::ExternRef;
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
  case VType::V128:
    return ValType::V128;
  case VType::FuncRef:
    return ValType::FuncRef;
  case VType::ExternRef:
    return ValType::ExternRef;
  default:
    return ValType::I32;
  }
}

Expect<void> FormChecker::checkExpr(const AST::InstrVec &Instrs) {
  /// Push ctrl frame ([] -> [Returns])
  pushCtrl({}, Returns);
  return checkInstrs(Instrs);
}

Expect<void> FormChecker::checkInstrs(const AST::InstrVec &Instrs) {
  /// Validate instructions
  for (auto &Instr : Instrs) {
    if (auto Res = AST::dispatchInstruction(
            Instr->getOpCode(),
            [this, &Instr](auto &&Arg) -> Expect<void> {
              using InstrT = typename std::decay_t<decltype(Arg)>::type;
              if constexpr (std::is_void_v<InstrT>) {
                /// If the Code not matched, validation failed.
                LOG(ERROR) << ErrCode::InvalidOpCode;
                LOG(ERROR) << ErrInfo::InfoInstruction(Instr->getOpCode(),
                                                       Instr->getOffset());
                return Unexpect(ErrCode::InvalidOpCode);
              } else {
                /// Check the corresponding instruction.
                auto Check =
                    checkInstr(*static_cast<const InstrT *>(Instr.get()));
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
  case OpCode::End:
    if (auto Res = popCtrl()) {
      pushTypes((*Res).EndTypes);
    } else {
      return Unexpect(Res);
    }
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
    if (Instr.getElseStatement().size() > 0) {
      if (auto Res = popTypes(T2); !Res) {
        return Unexpect(Res);
      }
      pushCtrl(T1, T2);
      return checkInstrs(Instr.getElseStatement());
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
    if (Tables.size() <= Instr.getTableIndex()) {
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Table, Instr.getTableIndex(), Tables.size());
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    if (Tables[Instr.getTableIndex()] != RefType::FuncRef) {
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

Expect<void> FormChecker::checkInstr(const AST::ReferenceInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Ref__null:
  case OpCode::Ref__is_null:
    /// These instruction is for ReferenceTypes and BulkMemoryOperations
    /// proposal.
    if (!PConf.hasProposal(Proposal::ReferenceTypes) &&
        !PConf.hasProposal(Proposal::BulkMemoryOperations)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    break;
  case OpCode::Ref__func:
    /// Skip this instruction for internal used.
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }

  switch (Instr.getOpCode()) {
  case OpCode::Ref__null:
    return StackTrans({}, std::array{ASTToVType(Instr.getReferenceType())});
  case OpCode::Ref__is_null:
    if (auto Res = popType()) {
      if (!isRefType(*Res)) {
        LOG(ERROR) << ErrCode::TypeCheckFailed;
        LOG(ERROR) << ErrInfo::InfoMismatch(ValType::FuncRef, VTypeToAST(*Res));
        return Unexpect(ErrCode::TypeCheckFailed);
      }
    } else {
      return Unexpect(Res);
    }
    return StackTrans({}, std::array{VType::I32});
  case OpCode::Ref__func:
    if (Refs.find(Instr.getTargetIndex()) == Refs.cend()) {
      /// Undeclared function reference.
      LOG(ERROR) << ErrCode::InvalidRefIdx;
      return Unexpect(ErrCode::InvalidRefIdx);
    }
    return StackTrans({}, std::array{VType::FuncRef});
  default:
    __builtin_unreachable();
  }
  return {};
}

Expect<void> FormChecker::checkInstr(const AST::ParametricInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Drop:
    return StackTrans(std::array{VType::Unknown}, {});
  case OpCode::Select: {
    /// Pop I32.
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    /// Pop T1 and T2.
    VType T1, T2;
    if (auto Res = popType()) {
      T1 = *Res;
    } else {
      return Unexpect(Res);
    }
    if (auto Res = popType()) {
      T2 = *Res;
    } else {
      return Unexpect(Res);
    }
    /// T1 and T2 should be number type.
    if (!isNumType(T1)) {
      LOG(ERROR) << ErrCode::TypeCheckFailed;
      LOG(ERROR) << ErrInfo::InfoMismatch(ValType::I32, VTypeToAST(T1));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    if (!isNumType(T2)) {
      LOG(ERROR) << ErrCode::TypeCheckFailed;
      LOG(ERROR) << ErrInfo::InfoMismatch(VTypeToAST(T1), VTypeToAST(T2));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    /// Error if t1 != t2 && t1 =/= Unknown && t2 =/= Unknown
    if (T1 != T2 && T1 != VType::Unknown && T2 != VType::Unknown) {
      LOG(ERROR) << ErrCode::TypeCheckFailed;
      LOG(ERROR) << ErrInfo::InfoMismatch(VTypeToAST(T1), VTypeToAST(T2));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    /// Push value.
    if (T1 == VType::Unknown) {
      pushType(T2);
    } else {
      pushType(T1);
    }
    return {};
  }
  case OpCode::Select_t: {
    /// This instruction is for ReferenceTypes proposal.
    if (!PConf.hasProposal(Proposal::ReferenceTypes)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    /// Note: There may be multiple values choise in the future.
    if (Instr.getValTypeList().size() != 1) {
      LOG(ERROR) << ErrCode::InvalidResultArity;
      return Unexpect(ErrCode::InvalidResultArity);
    }
    VType ExpT = ASTToVType(Instr.getValTypeList()[0]);
    if (auto Res = popTypes(std::array{ExpT, ExpT, VType::I32}); !Res) {
      return Unexpect(Res);
    }
    pushType(ExpT);
    return {};
  }
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }
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
    [[fallthrough]];
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

Expect<void> FormChecker::checkInstr(const AST::TableInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
    /// These instruction is for ReferenceTypes proposal.
    if (!PConf.hasProposal(Proposal::ReferenceTypes)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    break;
  case OpCode::Table__init:
  case OpCode::Elem__drop:
  case OpCode::Table__copy:
    /// These instruction is for ReferenceTypes or BulkMemoryOperations
    /// proposal.
    if (!PConf.hasProposal(Proposal::ReferenceTypes) &&
        !PConf.hasProposal(Proposal::BulkMemoryOperations)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }
  /// Check target table/element index.
  VType DstRefType = VType::FuncRef;
  switch (Instr.getOpCode()) {
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__init:
  case OpCode::Table__copy:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
    if (Tables.size() <= Instr.getTargetIndex()) {
      /// Table index out of range.
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Table, Instr.getTargetIndex(), Tables.size());
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    DstRefType = ASTToVType(Tables[Instr.getTargetIndex()]);
    break;
  case OpCode::Elem__drop:
    if (Elems.size() <= Instr.getElemIndex()) {
      /// Element index out of range.
      LOG(ERROR) << ErrCode::InvalidElemIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Element, Instr.getElemIndex(), Elems.size());
      return Unexpect(ErrCode::InvalidElemIdx);
    }
    break;
  default:
    __builtin_unreachable();
  }

  /// Check source table/element index and do matching.
  switch (Instr.getOpCode()) {
  case OpCode::Table__copy:
    if (Tables.size() <= Instr.getSourceIndex()) {
      /// Table index out of range.
      LOG(ERROR) << ErrCode::InvalidTableIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Table, Instr.getSourceIndex(), Tables.size());
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    if (Tables[Instr.getSourceIndex()] != Tables[Instr.getTargetIndex()]) {
      /// Reference type not matched.
      LOG(ERROR) << ErrCode::TypeCheckFailed;
      LOG(ERROR) << ErrInfo::InfoMismatch(
          ToValType(Tables[Instr.getTargetIndex()]),
          ToValType(Tables[Instr.getSourceIndex()]));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    break;
  case OpCode::Table__init:
    if (Elems.size() <= Instr.getElemIndex()) {
      /// Element index out of range.
      LOG(ERROR) << ErrCode::InvalidElemIdx;
      LOG(ERROR) << ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Element, Instr.getElemIndex(), Elems.size());
      return Unexpect(ErrCode::InvalidElemIdx);
    }
    if (Elems[Instr.getElemIndex()] != Tables[Instr.getTargetIndex()]) {
      /// Reference type not matched.
      LOG(ERROR) << ErrCode::TypeCheckFailed;
      LOG(ERROR) << ErrInfo::InfoMismatch(
          ToValType(Tables[Instr.getTargetIndex()]),
          ToValType(Elems[Instr.getElemIndex()]));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    break;
  default:
    break;
  }

  /// Transformation.
  switch (Instr.getOpCode()) {
  case OpCode::Table__get:
    return StackTrans(std::array{VType::I32}, std::array{DstRefType});
  case OpCode::Table__set:
    return StackTrans(std::array{VType::I32, DstRefType}, {});
  case OpCode::Table__grow:
    return StackTrans(std::array{DstRefType, VType::I32},
                      std::array{VType::I32});
  case OpCode::Table__size:
    return StackTrans({}, std::array{VType::I32});
  case OpCode::Table__fill:
    return StackTrans(std::array{VType::I32, DstRefType, VType::I32}, {});
  case OpCode::Table__copy:
  case OpCode::Table__init:
    return StackTrans(std::array{VType::I32, VType::I32, VType::I32}, {});
  case OpCode::Elem__drop:
    return {};
  default:
    break;
  }
  return {};
}

Expect<void> FormChecker::checkInstr(const AST::MemoryInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::Memory__init:
  case OpCode::Data__drop:
  case OpCode::Memory__copy:
  case OpCode::Memory__fill:
    /// These instruction is for ReferenceTypes or BulkMemoryOperations
    /// proposal.
    if (!PConf.hasProposal(Proposal::ReferenceTypes) &&
        !PConf.hasProposal(Proposal::BulkMemoryOperations)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    break;
  default:
    break;
  }

  /// Memory[0] must exist except data.drop instruction.
  if (Instr.getOpCode() != OpCode::Data__drop && Mems.size() == 0) {
    LOG(ERROR) << ErrCode::InvalidMemoryIdx;
    LOG(ERROR) << ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, 0,
                                           Mems.size());
    return Unexpect(ErrCode::InvalidMemoryIdx);
  }
  /// Data[x] must exist in memory.init and data.drop instructions.
  switch (Instr.getOpCode()) {
  case OpCode::Memory__init:
  case OpCode::Data__drop:
    if (Instr.getDataIndex() >= Datas.size()) {
      /// Data index out of range.
      LOG(ERROR) << ErrCode::InvalidDataIdx;
      return Unexpect(ErrCode::InvalidDataIdx);
    }
    break;
  default:
    break;
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
  case OpCode::Memory__init:
  case OpCode::Data__drop:
  case OpCode::Memory__copy:
  case OpCode::Memory__fill:
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
  case OpCode::Memory__init:
  case OpCode::Memory__copy:
  case OpCode::Memory__fill:
    return StackTrans(std::array{VType::I32, VType::I32, VType::I32}, {});
  case OpCode::Data__drop:
    return {};
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

Expect<void> FormChecker::checkInstr(const AST::SIMDMemoryInstruction &Instr) {
  /// Get bit width
  uint32_t N = 8;
  switch (Instr.getOpCode()) {
  case OpCode::V128__load:
  case OpCode::V128__store:
    /// N will be 128 in the end
    N <<= 1;
    [[fallthrough]];

  case OpCode::I16x8__load8x8_s:
  case OpCode::I16x8__load8x8_u:
  case OpCode::I32x4__load16x4_s:
  case OpCode::I32x4__load16x4_u:
  case OpCode::I64x2__load32x2_s:
  case OpCode::I64x2__load32x2_u:
  case OpCode::I64x2__load_splat:
  case OpCode::V128__load64_zero:
    /// N will be 64 in the end
    N <<= 1;
    [[fallthrough]];

  case OpCode::I32x4__load_splat:
  case OpCode::V128__load32_zero:
    /// N will be 32 in the end
    N <<= 1;
    [[fallthrough]];

  case OpCode::I16x8__load_splat:
    /// N will be 16 in the end
    N <<= 1;
    [[fallthrough]];

  case OpCode::I8x16__load_splat:
    /// N will be 8 in the end

    /// These instruction is for SIMD proposal.
    if (!PConf.hasProposal(Proposal::SIMD)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::SIMD);
      return Unexpect(ErrCode::InvalidOpCode);
    }

    if (Instr.getMemoryAlign() > 31 ||
        (1UL << Instr.getMemoryAlign()) > (N >> 3UL)) {
      /// 2 ^ align needs to <= N / 8
      LOG(ERROR) << ErrCode::InvalidAlignment;
      LOG(ERROR) << ErrInfo::InfoMismatch(uint8_t(N >> 3),
                                          Instr.getMemoryAlign());
      return Unexpect(ErrCode::InvalidAlignment);
    }

    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
    break;
  }

  switch (Instr.getOpCode()) {
  case OpCode::V128__load:
  case OpCode::I16x8__load8x8_s:
  case OpCode::I16x8__load8x8_u:
  case OpCode::I32x4__load16x4_s:
  case OpCode::I32x4__load16x4_u:
  case OpCode::I64x2__load32x2_s:
  case OpCode::I64x2__load32x2_u:
  case OpCode::I8x16__load_splat:
  case OpCode::I16x8__load_splat:
  case OpCode::I32x4__load_splat:
  case OpCode::I64x2__load_splat:
  case OpCode::V128__load32_zero:
  case OpCode::V128__load64_zero:
    return StackTrans(std::array{VType::I32}, std::array{VType::V128});
  case OpCode::V128__store:
    return StackTrans(std::array{VType::I32, VType::V128}, {});
  default:
    __builtin_unreachable();
    break;
  }
}

Expect<void> FormChecker::checkInstr(const AST::SIMDConstInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::V128__const:
    /// These instruction is for SIMD proposal.
    if (!PConf.hasProposal(Proposal::SIMD)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::SIMD);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    return StackTrans({}, std::array{VType::V128});
  default:
    break;
  }
  LOG(ERROR) << ErrCode::InvalidOpCode;
  return Unexpect(ErrCode::InvalidOpCode);
}

Expect<void> FormChecker::checkInstr(const AST::SIMDShuffleInstruction &Instr) {
  switch (Instr.getOpCode()) {
  case OpCode::I8x16__shuffle: {
    /// These instruction is for SIMD proposal.
    if (!PConf.hasProposal(Proposal::SIMD)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::SIMD);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    /// Check all lane index < 32 by masking
    const uint128_t Mask = reinterpret_cast<uint128_t>(
        uint64x2_t{0xe0e0e0e0e0e0e0e0, 0xe0e0e0e0e0e0e0e0});
    const uint128_t Result = Instr.getShuffleValue() & Mask;
    if (Result != 0) {
      LOG(ERROR) << ErrCode::InvalidLaneIdx;
      return Unexpect(ErrCode::InvalidLaneIdx);
    }
    return StackTrans(std::array{VType::V128, VType::V128},
                      std::array{VType::V128});
  }
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }
}

Expect<void> FormChecker::checkInstr(const AST::SIMDLaneInstruction &Instr) {
  uint8_t MaxLaneIndex;
  switch (Instr.getOpCode()) {
  case OpCode::I8x16__extract_lane_s:
  case OpCode::I8x16__extract_lane_u:
  case OpCode::I8x16__replace_lane:
    MaxLaneIndex = 16;
    break;
  case OpCode::I16x8__extract_lane_s:
  case OpCode::I16x8__extract_lane_u:
  case OpCode::I16x8__replace_lane:
    MaxLaneIndex = 8;
    break;
  case OpCode::I32x4__extract_lane:
  case OpCode::F32x4__extract_lane:
  case OpCode::I32x4__replace_lane:
  case OpCode::F32x4__replace_lane:
    MaxLaneIndex = 4;
    break;
  case OpCode::I64x2__extract_lane:
  case OpCode::F64x2__extract_lane:
  case OpCode::I64x2__replace_lane:
  case OpCode::F64x2__replace_lane:
    MaxLaneIndex = 2;
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }

  /// These instruction is for SIMD proposal.
  if (!PConf.hasProposal(Proposal::SIMD)) {
    LOG(ERROR) << ErrCode::InvalidOpCode;
    LOG(ERROR) << ErrInfo::InfoProposal(Proposal::SIMD);
    return Unexpect(ErrCode::InvalidOpCode);
  }

  /// Check invalid lane index.
  if (Instr.getLaneIndex() >= MaxLaneIndex) {
    LOG(ERROR) << ErrCode::InvalidLaneIdx;
    return Unexpect(ErrCode::InvalidLaneIdx);
  }

  switch (Instr.getOpCode()) {
  case OpCode::I8x16__extract_lane_s:
  case OpCode::I8x16__extract_lane_u:
  case OpCode::I16x8__extract_lane_s:
  case OpCode::I16x8__extract_lane_u:
  case OpCode::I32x4__extract_lane:
    return StackTrans(std::array{VType::V128}, std::array{VType::I32});
  case OpCode::I64x2__extract_lane:
    return StackTrans(std::array{VType::V128}, std::array{VType::I64});
  case OpCode::F32x4__extract_lane:
    return StackTrans(std::array{VType::V128}, std::array{VType::F32});
  case OpCode::F64x2__extract_lane:
    return StackTrans(std::array{VType::V128}, std::array{VType::F64});
  case OpCode::I8x16__replace_lane:
  case OpCode::I16x8__replace_lane:
  case OpCode::I32x4__replace_lane:
    return StackTrans(std::array{VType::V128, VType::I32},
                      std::array{VType::V128});
  case OpCode::I64x2__replace_lane:
    return StackTrans(std::array{VType::V128, VType::I64},
                      std::array{VType::V128});
  case OpCode::F32x4__replace_lane:
    return StackTrans(std::array{VType::V128, VType::F32},
                      std::array{VType::V128});
  case OpCode::F64x2__replace_lane:
    return StackTrans(std::array{VType::V128, VType::F64},
                      std::array{VType::V128});
  default:
    __builtin_unreachable();
  }
}

Expect<void> FormChecker::checkInstr(const AST::SIMDNumericInstruction &Instr) {
  switch (Instr.getOpCode()) {
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

  case OpCode::I8x16__abs:
  case OpCode::I8x16__neg:
  case OpCode::I8x16__any_true:
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
  case OpCode::I16x8__any_true:
  case OpCode::I16x8__all_true:
  case OpCode::I16x8__bitmask:
  case OpCode::I16x8__narrow_i32x4_s:
  case OpCode::I16x8__narrow_i32x4_u:
  case OpCode::I16x8__widen_low_i8x16_s:
  case OpCode::I16x8__widen_high_i8x16_s:
  case OpCode::I16x8__widen_low_i8x16_u:
  case OpCode::I16x8__widen_high_i8x16_u:
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

  case OpCode::I32x4__abs:
  case OpCode::I32x4__neg:
  case OpCode::I32x4__any_true:
  case OpCode::I32x4__all_true:
  case OpCode::I32x4__bitmask:
  case OpCode::I32x4__widen_low_i16x8_s:
  case OpCode::I32x4__widen_high_i16x8_s:
  case OpCode::I32x4__widen_low_i16x8_u:
  case OpCode::I32x4__widen_high_i16x8_u:
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

  case OpCode::I64x2__neg:
  case OpCode::I64x2__shl:
  case OpCode::I64x2__shr_s:
  case OpCode::I64x2__shr_u:
  case OpCode::I64x2__add:
  case OpCode::I64x2__sub:
  case OpCode::I64x2__mul:

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

  case OpCode::I32x4__trunc_sat_f32x4_s:
  case OpCode::I32x4__trunc_sat_f32x4_u:
  case OpCode::F32x4__convert_i32x4_s:
  case OpCode::F32x4__convert_i32x4_u:

  case OpCode::I8x16__mul:
  case OpCode::I32x4__dot_i16x8_s:
  case OpCode::I64x2__any_true:
  case OpCode::I64x2__all_true:
  case OpCode::F32x4__qfma:
  case OpCode::F32x4__qfms:
  case OpCode::F64x2__qfma:
  case OpCode::F64x2__qfms:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
  case OpCode::I64x2__trunc_sat_f64x2_s:
  case OpCode::I64x2__trunc_sat_f64x2_u:
  case OpCode::F64x2__convert_i64x2_s:
  case OpCode::F64x2__convert_i64x2_u:
    /// These instruction is for SIMD proposal.
    if (!PConf.hasProposal(Proposal::SIMD)) {
      LOG(ERROR) << ErrCode::InvalidOpCode;
      LOG(ERROR) << ErrInfo::InfoProposal(Proposal::SIMD);
      return Unexpect(ErrCode::InvalidOpCode);
    }
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidOpCode;
    return Unexpect(ErrCode::InvalidOpCode);
  }

  switch (Instr.getOpCode()) {
  case OpCode::I8x16__splat:
  case OpCode::I16x8__splat:
  case OpCode::I32x4__splat:
    return StackTrans(std::array{VType::I32}, std::array{VType::V128});
  case OpCode::I64x2__splat:
    return StackTrans(std::array{VType::I64}, std::array{VType::V128});
  case OpCode::F32x4__splat:
    return StackTrans(std::array{VType::F32}, std::array{VType::V128});
  case OpCode::F64x2__splat:
    return StackTrans(std::array{VType::F64}, std::array{VType::V128});

  case OpCode::V128__not:
  case OpCode::I8x16__abs:
  case OpCode::I8x16__neg:
  case OpCode::I16x8__abs:
  case OpCode::I16x8__neg:
  case OpCode::I16x8__widen_low_i8x16_s:
  case OpCode::I16x8__widen_high_i8x16_s:
  case OpCode::I16x8__widen_low_i8x16_u:
  case OpCode::I16x8__widen_high_i8x16_u:
  case OpCode::I32x4__abs:
  case OpCode::I32x4__neg:
  case OpCode::I32x4__widen_low_i16x8_s:
  case OpCode::I32x4__widen_high_i16x8_s:
  case OpCode::I32x4__widen_low_i16x8_u:
  case OpCode::I32x4__widen_high_i16x8_u:
  case OpCode::I64x2__neg:
  case OpCode::F32x4__abs:
  case OpCode::F32x4__neg:
  case OpCode::F32x4__sqrt:
  case OpCode::F64x2__abs:
  case OpCode::F64x2__neg:
  case OpCode::F64x2__sqrt:
  case OpCode::I32x4__trunc_sat_f32x4_s:
  case OpCode::I32x4__trunc_sat_f32x4_u:
  case OpCode::F32x4__convert_i32x4_s:
  case OpCode::F32x4__convert_i32x4_u:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
  case OpCode::I64x2__trunc_sat_f64x2_s:
  case OpCode::I64x2__trunc_sat_f64x2_u:
  case OpCode::F64x2__convert_i64x2_s:
  case OpCode::F64x2__convert_i64x2_u:
    return StackTrans(std::array{VType::V128}, std::array{VType::V128});

  case OpCode::I8x16__swizzle:
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
  case OpCode::V128__and:
  case OpCode::V128__andnot:
  case OpCode::V128__or:
  case OpCode::V128__xor:
  case OpCode::I8x16__narrow_i16x8_s:
  case OpCode::I8x16__narrow_i16x8_u:
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
  case OpCode::I16x8__narrow_i32x4_s:
  case OpCode::I16x8__narrow_i32x4_u:
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
  case OpCode::I32x4__add:
  case OpCode::I32x4__sub:
  case OpCode::I32x4__mul:
  case OpCode::I32x4__min_s:
  case OpCode::I32x4__min_u:
  case OpCode::I32x4__max_s:
  case OpCode::I32x4__max_u:
  case OpCode::I64x2__add:
  case OpCode::I64x2__sub:
  case OpCode::I64x2__mul:
  case OpCode::F32x4__add:
  case OpCode::F32x4__sub:
  case OpCode::F32x4__mul:
  case OpCode::F32x4__div:
  case OpCode::F32x4__min:
  case OpCode::F32x4__max:
  case OpCode::F32x4__pmin:
  case OpCode::F32x4__pmax:
  case OpCode::F64x2__add:
  case OpCode::F64x2__sub:
  case OpCode::F64x2__mul:
  case OpCode::F64x2__div:
  case OpCode::F64x2__min:
  case OpCode::F64x2__max:
  case OpCode::F64x2__pmin:
  case OpCode::F64x2__pmax:
  case OpCode::I8x16__mul:
  case OpCode::I32x4__dot_i16x8_s:
    return StackTrans(std::array{VType::V128, VType::V128},
                      std::array{VType::V128});

  case OpCode::V128__bitselect:
  case OpCode::F32x4__qfma:
  case OpCode::F32x4__qfms:
  case OpCode::F64x2__qfma:
  case OpCode::F64x2__qfms:
    return StackTrans(std::array{VType::V128, VType::V128, VType::V128},
                      std::array{VType::V128});

  case OpCode::I8x16__any_true:
  case OpCode::I8x16__all_true:
  case OpCode::I8x16__bitmask:
  case OpCode::I16x8__any_true:
  case OpCode::I16x8__all_true:
  case OpCode::I16x8__bitmask:
  case OpCode::I32x4__any_true:
  case OpCode::I32x4__all_true:
  case OpCode::I32x4__bitmask:
  case OpCode::I64x2__any_true:
  case OpCode::I64x2__all_true:
    return StackTrans(std::array{VType::V128}, std::array{VType::I32});

  case OpCode::I8x16__shl:
  case OpCode::I8x16__shr_s:
  case OpCode::I8x16__shr_u:
  case OpCode::I16x8__shl:
  case OpCode::I16x8__shr_s:
  case OpCode::I16x8__shr_u:
  case OpCode::I32x4__shl:
  case OpCode::I32x4__shr_s:
  case OpCode::I32x4__shr_u:
  case OpCode::I64x2__shl:
  case OpCode::I64x2__shr_s:
  case OpCode::I64x2__shr_u:
    return StackTrans(std::array{VType::V128, VType::I32},
                      std::array{VType::V128});

  default:
    __builtin_unreachable();
  }
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
