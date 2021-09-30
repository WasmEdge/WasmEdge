// SPDX-License-Identifier: Apache-2.0

#include "validator/formchecker.h"

namespace {
template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace

namespace WasmEdge {
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
    Mems = 0;
    Globals.clear();
    Datas.clear();
    Elems.clear();
    Refs.clear();
    NumImportFuncs = 0;
    NumImportGlobals = 0;
  }
}

Expect<void> FormChecker::validate(AST::InstrView Instrs,
                                   Span<const ValType> RetVals) {
  for (const ValType &Val : RetVals) {
    Returns.push_back(ASTToVType(Val));
  }
  return checkExpr(Instrs);
}

Expect<void> FormChecker::validate(AST::InstrView Instrs,
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

void FormChecker::addMemory(const AST::MemoryType &) { Mems++; }

void FormChecker::addGlobal(const AST::GlobalType &Glob, const bool IsImport) {
  /// Type in global is comfirmed in loading phase.
  Globals.emplace_back(ASTToVType(Glob.getValueType()),
                       Glob.getValueMutation());
  if (IsImport) {
    NumImportGlobals++;
  }
}

void FormChecker::addData(const AST::DataSegment &) {
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

Expect<void> FormChecker::checkExpr(AST::InstrView Instrs) {
  /// Push ctrl frame ([] -> [Returns])
  pushCtrl({}, Returns);
  return checkInstrs(Instrs);
}

Expect<void> FormChecker::checkInstrs(AST::InstrView Instrs) {
  /// Validate instructions
  for (auto &Instr : Instrs) {
    if (auto Res = checkInstr(Instr); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}
Expect<void> FormChecker::checkInstr(const AST::Instruction &Instr) {
  /// Note: The instructions and their immediates have passed proposal
  /// configuration checking in loader phase.

  /// Helper lambda for checking control stack depth and return index.
  auto checkCtrlStackDepth = [this](uint32_t N) -> Expect<uint32_t> {
    /// Check the control stack for at least N + 1 frames.
    if (CtrlStack.size() <= N) {
      /// Branch out of stack
      spdlog::error(ErrCode::InvalidLabelIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Label, N,
                                   static_cast<uint32_t>(CtrlStack.size())));
      return Unexpect(ErrCode::InvalidLabelIdx);
    }
    /// Return the index of the last N element.
    return static_cast<uint32_t>(CtrlStack.size()) - UINT32_C(1) - N;
  };

  /// Helper lambda for checking memory index and perform transformation.
  auto checkMemAndTrans = [this](uint32_t N, Span<const VType> Take,
                                 Span<const VType> Put) -> Expect<void> {
    if (Mems <= N) {
      spdlog::error(ErrCode::InvalidMemoryIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, N, Mems));
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    return StackTrans(Take, Put);
  };

  /// Helper lambda for checking memory alignment and perform transformation.
  auto checkAlignAndTrans = [this,
                             &Instr](uint32_t N, Span<const VType> Take,
                                     Span<const VType> Put) -> Expect<void> {
    if (Mems == 0) {
      spdlog::error(ErrCode::InvalidMemoryIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, 0, Mems));
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    if (Instr.getMemoryAlign() > 31 ||
        (1UL << Instr.getMemoryAlign()) > (N >> 3UL)) {
      /// 2 ^ align needs to <= N / 8
      spdlog::error(ErrCode::InvalidAlignment);
      spdlog::error(ErrInfo::InfoMismatch(static_cast<uint8_t>(N >> 3),
                                          Instr.getMemoryAlign()));
      return Unexpect(ErrCode::InvalidAlignment);
    }
    return StackTrans(Take, Put);
  };

  /// Helper lambda for checking vtypes matching.
  auto checkTypesMatching = [this](Span<const VType> Exp,
                                   Span<const VType> Got) -> Expect<void> {
    if (Exp.size() != Got.size() ||
        !std::equal(Exp.begin(), Exp.end(), Got.begin())) {
      std::vector<ValType> ExpV, GotV;
      ExpV.reserve(Exp.size());
      for (auto &I : Exp) {
        ExpV.push_back(VTypeToAST(I));
      }
      GotV.reserve(Got.size());
      for (auto &I : Got) {
        GotV.push_back(VTypeToAST(I));
      }
      spdlog::error(ErrCode::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(ExpV, GotV));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    return {};
  };

  /// Helper lambda for checking lane index and perform transformation.
  auto checkLaneAndTrans = [this,
                            &Instr](uint32_t N, Span<const VType> Take,
                                    Span<const VType> Put) -> Expect<void> {
    if (Instr.getTargetIndex() >= N) {
      spdlog::error(ErrCode::InvalidLaneIdx);
      return Unexpect(ErrCode::InvalidLaneIdx);
    }
    return StackTrans(Take, Put);
  };

  /// Helper lambda for checking memory alignment, lane index and perform
  /// transformation.
  auto checkAlignLaneAndTrans =
      [this, &Instr](const uint32_t N, Span<const VType> Take,
                     Span<const VType> Put) -> Expect<void> {
    if (Mems == 0) {
      spdlog::error(ErrCode::InvalidMemoryIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, 0, Mems));
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    if (Instr.getMemoryAlign() > 31 ||
        (1UL << Instr.getMemoryAlign()) > (N >> 3UL)) {
      /// 2 ^ align needs to <= N / 8
      spdlog::error(ErrCode::InvalidAlignment);
      spdlog::error(ErrInfo::InfoMismatch(static_cast<uint8_t>(N >> 3),
                                          Instr.getMemoryAlign()));
      return Unexpect(ErrCode::InvalidAlignment);
    }
    const uint32_t I = 128 / N;
    if (Instr.getTargetIndex() >= I) {
      spdlog::error(ErrCode::InvalidLaneIdx);
      return Unexpect(ErrCode::InvalidLaneIdx);
    }
    return StackTrans(Take, Put);
  };

  switch (Instr.getOpCode()) {
  /// Control instructions.
  case OpCode::Unreachable:
    return unreachable();
  case OpCode::Nop:
    return {};

  case OpCode::If:
    /// Pop I32
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    [[fallthrough]];
  case OpCode::Block:
  case OpCode::Loop: {
    /// Get blocktype [t1*] -> [t2*]
    std::vector<VType> Buffer;
    Span<const VType> T1, T2;
    if (auto Res = resolveBlockType(Buffer, Instr.getBlockType())) {
      std::tie(T1, T2) = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
    /// Pop and check [t1*]
    if (auto Res = popTypes(T1); !Res) {
      return Unexpect(Res);
    }
    /// Push ctrl frame ([t1*], [t2*])
    pushCtrl(T1, T2, Instr.getOpCode());
    if (Instr.getOpCode() == OpCode::If &&
        Instr.getJumpElse() == Instr.getJumpEnd()) {
      /// No else case in if-else statement.
      if (auto Res = checkTypesMatching(T2, T1); !Res) {
        return Unexpect(Res);
      }
    }
    return {};
  }

  case OpCode::Else:
    if (auto Res = popCtrl()) {
      pushCtrl((*Res).StartTypes, (*Res).EndTypes, Instr.getOpCode());
    } else {
      return Unexpect(Res);
    }
    return {};
  case OpCode::End:
    if (auto Res = popCtrl()) {
      pushTypes((*Res).EndTypes);
    } else {
      return Unexpect(Res);
    }
    return {};

  case OpCode::Br:
    if (auto D = checkCtrlStackDepth(Instr.getTargetIndex())) {
      /// D is the last D element of control stack.
      if (auto Res = popTypes(getLabelTypes(CtrlStack[*D]))) {
        return unreachable();
      } else {
        return Unexpect(Res);
      }
    } else {
      return Unexpect(D);
    }
  case OpCode::Br_if:
    if (auto D = checkCtrlStackDepth(Instr.getTargetIndex())) {
      /// D is the last D element of control stack.
      if (auto Res = popType(VType::I32); !Res) {
        return Unexpect(Res);
      }
      if (auto Res = popTypes(getLabelTypes(CtrlStack[*D]))) {
        pushTypes(getLabelTypes(CtrlStack[*D]));
        return {};
      } else {
        return Unexpect(Res);
      }
    } else {
      return Unexpect(D);
    }
  case OpCode::Br_table:
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    if (auto M = checkCtrlStackDepth(Instr.getTargetIndex())) {
      /// M is the last M element of control stack.
      auto MTypes = getLabelTypes(CtrlStack[*M]);
      for (const uint32_t &L : Instr.getLabelList()) {
        if (auto N = checkCtrlStackDepth(L)) {
          /// N is the last N element of control stack.
          auto NTypes = getLabelTypes(CtrlStack[*N]);
          if (MTypes.size() != NTypes.size()) {
            return checkTypesMatching(MTypes, NTypes);
          }
          /// Push the popped types.
          std::vector<VType> TypeBuf(NTypes.size());
          for (uint32_t IdxN = static_cast<uint32_t>(NTypes.size()); IdxN >= 1;
               --IdxN) {
            const uint32_t Idx = IdxN - 1;
            /// Cannot use popTypes() here because we need the popped value.
            if (auto Res = popType(NTypes[Idx])) {
              /// Have to check is `VType::Unknown` occured for the case of
              /// `unreachable` instruction appeared before the `br_table`
              /// instruction.
              if (CtrlStack.back().IsUnreachable) {
                TypeBuf[Idx] = VType::Unknown;
              } else {
                TypeBuf[Idx] = *Res;
              }
            } else {
              return Unexpect(Res);
            }
          }
          pushTypes(TypeBuf);
        } else {
          return Unexpect(N);
        }
      }
      if (auto Res = popTypes(getLabelTypes(CtrlStack[*M])); !Res) {
        return Unexpect(Res);
      }
      return unreachable();
    } else {
      return Unexpect(M);
    }

  case OpCode::Return:
    if (auto Res = popTypes(Returns); !Res) {
      return Unexpect(Res);
    }
    return unreachable();

  case OpCode::Call: {
    auto N = Instr.getTargetIndex();
    if (Funcs.size() <= N) {
      /// Call function index out of range
      spdlog::error(ErrCode::InvalidFuncIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function, N,
                                   static_cast<uint32_t>(Funcs.size())));
      return Unexpect(ErrCode::InvalidFuncIdx);
    }
    return StackTrans(Types[Funcs[N]].first, Types[Funcs[N]].second);
  }
  case OpCode::Call_indirect: {
    auto N = Instr.getTargetIndex();
    auto T = Instr.getSourceIndex();
    /// Check source table index.
    if (Tables.size() <= T) {
      spdlog::error(ErrCode::InvalidTableIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Table, T,
                                   static_cast<uint32_t>(Tables.size())));
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    if (Tables[T] != RefType::FuncRef) {
      spdlog::error(ErrCode::InvalidTableIdx);
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    /// Check target function type index.
    if (Types.size() <= N) {
      spdlog::error(ErrCode::InvalidFuncTypeIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::FunctionType, N,
                                   static_cast<uint32_t>(Types.size())));
      return Unexpect(ErrCode::InvalidFuncTypeIdx);
    }
    if (auto Res = popType(VType::I32); !Res) {
      return Unexpect(Res);
    }
    return StackTrans(Types[N].first, Types[N].second);
  }

  /// Reference Instructions.
  case OpCode::Ref__null:
    return StackTrans({}, std::array{ASTToVType(Instr.getReferenceType())});
  case OpCode::Ref__is_null:
    if (auto Res = popType()) {
      if (!isRefType(*Res)) {
        spdlog::error(ErrCode::TypeCheckFailed);
        spdlog::error(
            ErrInfo::InfoMismatch(ValType::FuncRef, VTypeToAST(*Res)));
        return Unexpect(ErrCode::TypeCheckFailed);
      }
    } else {
      return Unexpect(Res);
    }
    return StackTrans({}, std::array{VType::I32});
  case OpCode::Ref__func:
    if (Refs.find(Instr.getTargetIndex()) == Refs.cend()) {
      /// Undeclared function reference.
      spdlog::error(ErrCode::InvalidRefIdx);
      return Unexpect(ErrCode::InvalidRefIdx);
    }
    return StackTrans({}, std::array{VType::FuncRef});

  /// Parametric Instructions.
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
      spdlog::error(ErrCode::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(ValType::I32, VTypeToAST(T1)));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    if (!isNumType(T2)) {
      spdlog::error(ErrCode::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(VTypeToAST(T1), VTypeToAST(T2)));
      return Unexpect(ErrCode::TypeCheckFailed);
    }
    /// Error if t1 != t2 && t1 =/= Unknown && t2 =/= Unknown
    if (T1 != T2 && T1 != VType::Unknown && T2 != VType::Unknown) {
      spdlog::error(ErrCode::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(VTypeToAST(T1), VTypeToAST(T2)));
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
    /// Note: There may be multiple values choise in the future.
    if (Instr.getValTypeList().size() != 1) {
      spdlog::error(ErrCode::InvalidResultArity);
      return Unexpect(ErrCode::InvalidResultArity);
    }
    VType ExpT = ASTToVType(Instr.getValTypeList()[0]);
    if (auto Res = popTypes(std::array{ExpT, ExpT, VType::I32}); !Res) {
      return Unexpect(Res);
    }
    pushType(ExpT);
    return {};
  }

  /// Variable Instructions.
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee: {
    if (Instr.getTargetIndex() >= Locals.size()) {
      /// Local index out of range
      spdlog::error(ErrCode::InvalidLocalIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Local, Instr.getTargetIndex(),
          static_cast<uint32_t>(Locals.size())));
      return Unexpect(ErrCode::InvalidLocalIdx);
    }
    VType TExpect = Locals[Instr.getTargetIndex()];
    if (Instr.getOpCode() == OpCode::Local__get) {
      return StackTrans({}, std::array{TExpect});
    } else if (Instr.getOpCode() == OpCode::Local__set) {
      return StackTrans(std::array{TExpect}, {});
    } else {
      return StackTrans(std::array{TExpect}, std::array{TExpect});
    }
  }
  case OpCode::Global__set:
    /// Global case, check mutation.
    if (Instr.getTargetIndex() < Globals.size() &&
        Globals[Instr.getTargetIndex()].second != ValMut::Var) {
      /// Global is immutable
      spdlog::error(ErrCode::ImmutableGlobal);
      return Unexpect(ErrCode::ImmutableGlobal);
    }
    [[fallthrough]];
  case OpCode::Global__get: {
    if (Instr.getTargetIndex() >= Globals.size()) {
      /// Global index out of range
      spdlog::error(ErrCode::InvalidGlobalIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Global, Instr.getTargetIndex(),
          static_cast<uint32_t>(Locals.size())));
      return Unexpect(ErrCode::InvalidGlobalIdx);
    }
    VType ExpT = Globals[Instr.getTargetIndex()].first;
    if (Instr.getOpCode() == OpCode::Global__set) {
      return StackTrans(std::array{ExpT}, {});
    } else {
      return StackTrans({}, std::array{ExpT});
    }
  }

  /// Table Instructions.
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
  case OpCode::Table__init:
  case OpCode::Table__copy: {
    /// Check target table index to perform.
    if (Tables.size() <= Instr.getTargetIndex()) {
      spdlog::error(ErrCode::InvalidTableIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Table, Instr.getTargetIndex(),
          static_cast<uint32_t>(Tables.size())));
      return Unexpect(ErrCode::InvalidTableIdx);
    }
    VType ExpT = ASTToVType(Tables[Instr.getTargetIndex()]);
    if (Instr.getOpCode() == OpCode::Table__get) {
      return StackTrans(std::array{VType::I32}, std::array{ExpT});
    } else if (Instr.getOpCode() == OpCode::Table__set) {
      return StackTrans(std::array{VType::I32, ExpT}, {});
    } else if (Instr.getOpCode() == OpCode::Table__grow) {
      return StackTrans(std::array{ExpT, VType::I32}, std::array{VType::I32});
    } else if (Instr.getOpCode() == OpCode::Table__size) {
      return StackTrans({}, std::array{VType::I32});
    } else if (Instr.getOpCode() == OpCode::Table__fill) {
      return StackTrans(std::array{VType::I32, ExpT, VType::I32}, {});
    } else if (Instr.getOpCode() == OpCode::Table__init) {
      /// Check source element index for initialization.
      if (Elems.size() <= Instr.getSourceIndex()) {
        spdlog::error(ErrCode::InvalidElemIdx);
        spdlog::error(ErrInfo::InfoForbidIndex(
            ErrInfo::IndexCategory::Element, Instr.getSourceIndex(),
            static_cast<uint32_t>(Elems.size())));
        return Unexpect(ErrCode::InvalidElemIdx);
      }
      /// Check is the reference types matched.
      if (Elems[Instr.getSourceIndex()] != Tables[Instr.getTargetIndex()]) {
        spdlog::error(ErrCode::TypeCheckFailed);
        spdlog::error(
            ErrInfo::InfoMismatch(ToValType(Tables[Instr.getTargetIndex()]),
                                  ToValType(Elems[Instr.getSourceIndex()])));
        return Unexpect(ErrCode::TypeCheckFailed);
      }
      return StackTrans(std::array{VType::I32, VType::I32, VType::I32}, {});
    } else {
      /// Check source table index for copying.
      if (Tables.size() <= Instr.getSourceIndex()) {
        spdlog::error(ErrCode::InvalidTableIdx);
        spdlog::error(ErrInfo::InfoForbidIndex(
            ErrInfo::IndexCategory::Table, Instr.getSourceIndex(),
            static_cast<uint32_t>(Tables.size())));
        return Unexpect(ErrCode::InvalidTableIdx);
      }
      /// Check is the reference types matched.
      if (Tables[Instr.getSourceIndex()] != Tables[Instr.getTargetIndex()]) {
        spdlog::error(ErrCode::TypeCheckFailed);
        spdlog::error(
            ErrInfo::InfoMismatch(ToValType(Tables[Instr.getTargetIndex()]),
                                  ToValType(Tables[Instr.getSourceIndex()])));
        return Unexpect(ErrCode::TypeCheckFailed);
      }
      return StackTrans(std::array{VType::I32, VType::I32, VType::I32}, {});
    }
  }
  case OpCode::Elem__drop:
    /// Check target element index to drop.
    if (Elems.size() <= Instr.getTargetIndex()) {
      spdlog::error(ErrCode::InvalidElemIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Element, Instr.getTargetIndex(),
          static_cast<uint32_t>(Elems.size())));
      return Unexpect(ErrCode::InvalidElemIdx);
    }
    return {};

  /// Memory Instructions.
  case OpCode::I32__load:
    return checkAlignAndTrans(32, std::array{VType::I32},
                              std::array{VType::I32});
  case OpCode::I64__load:
    return checkAlignAndTrans(64, std::array{VType::I32},
                              std::array{VType::I64});
  case OpCode::F32__load:
    return checkAlignAndTrans(32, std::array{VType::I32},
                              std::array{VType::F32});
  case OpCode::F64__load:
    return checkAlignAndTrans(64, std::array{VType::I32},
                              std::array{VType::F64});
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
    return checkAlignAndTrans(8, std::array{VType::I32},
                              std::array{VType::I32});
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
    return checkAlignAndTrans(16, std::array{VType::I32},
                              std::array{VType::I32});
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
    return checkAlignAndTrans(8, std::array{VType::I32},
                              std::array{VType::I64});
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
    return checkAlignAndTrans(16, std::array{VType::I32},
                              std::array{VType::I64});
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
    return checkAlignAndTrans(32, std::array{VType::I32},
                              std::array{VType::I64});
  case OpCode::I32__store:
    return checkAlignAndTrans(32, std::array{VType::I32, VType::I32}, {});
  case OpCode::I64__store:
    return checkAlignAndTrans(64, std::array{VType::I32, VType::I64}, {});
  case OpCode::F32__store:
    return checkAlignAndTrans(32, std::array{VType::I32, VType::F32}, {});
  case OpCode::F64__store:
    return checkAlignAndTrans(64, std::array{VType::I32, VType::F64}, {});
  case OpCode::I32__store8:
    return checkAlignAndTrans(8, std::array{VType::I32, VType::I32}, {});
  case OpCode::I32__store16:
    return checkAlignAndTrans(16, std::array{VType::I32, VType::I32}, {});
  case OpCode::I64__store8:
    return checkAlignAndTrans(8, std::array{VType::I32, VType::I64}, {});
  case OpCode::I64__store16:
    return checkAlignAndTrans(16, std::array{VType::I32, VType::I64}, {});
  case OpCode::I64__store32:
    return checkAlignAndTrans(32, std::array{VType::I32, VType::I64}, {});
  case OpCode::Memory__size:
    return checkMemAndTrans(0, {}, std::array{VType::I32});
  case OpCode::Memory__grow:
    return checkMemAndTrans(0, std::array{VType::I32}, std::array{VType::I32});
  case OpCode::Memory__init:
    /// Check target memory index to initialize. Memory[0] must exist.
    if (Mems == 0) {
      spdlog::error(ErrCode::InvalidMemoryIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Memory, 0, Mems));
      return Unexpect(ErrCode::InvalidMemoryIdx);
    }
    /// Check source data index for initialization.
    if (Instr.getSourceIndex() >= Datas.size()) {
      spdlog::error(ErrCode::InvalidDataIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Data, Instr.getSourceIndex(),
          static_cast<uint32_t>(Datas.size())));
      return Unexpect(ErrCode::InvalidDataIdx);
    }
    [[fallthrough]];
  case OpCode::Memory__copy:
  case OpCode::Memory__fill:
    return checkMemAndTrans(0, std::array{VType::I32, VType::I32, VType::I32},
                            {});
  case OpCode::Data__drop:
    /// Check target data index to drop.
    if (Instr.getTargetIndex() >= Datas.size()) {
      spdlog::error(ErrCode::InvalidDataIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::Data, Instr.getTargetIndex(),
          static_cast<uint32_t>(Datas.size())));
      return Unexpect(ErrCode::InvalidDataIdx);
    }
    return {};

  /// Const Instructions.
  case OpCode::I32__const:
    return StackTrans({}, std::array{VType::I32});
  case OpCode::I64__const:
    return StackTrans({}, std::array{VType::I64});
  case OpCode::F32__const:
    return StackTrans({}, std::array{VType::F32});
  case OpCode::F64__const:
    return StackTrans({}, std::array{VType::F64});

  /// Unary Numeric Instructions.
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

  /// Binary Numeric Instructions.
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

  /// SIMD Memory Instruction.
  case OpCode::V128__load:
    return checkAlignAndTrans(128, std::array{VType::I32},
                              std::array{VType::V128});
  case OpCode::V128__load8x8_s:
  case OpCode::V128__load8x8_u:
  case OpCode::V128__load16x4_s:
  case OpCode::V128__load16x4_u:
  case OpCode::V128__load32x2_s:
  case OpCode::V128__load32x2_u:
  case OpCode::V128__load64_splat:
  case OpCode::V128__load64_zero:
    return checkAlignAndTrans(64, std::array{VType::I32},
                              std::array{VType::V128});
  case OpCode::V128__load8_splat:
    return checkAlignAndTrans(8, std::array{VType::I32},
                              std::array{VType::V128});
  case OpCode::V128__load16_splat:
    return checkAlignAndTrans(16, std::array{VType::I32},
                              std::array{VType::V128});
  case OpCode::V128__load32_splat:
  case OpCode::V128__load32_zero:
    return checkAlignAndTrans(32, std::array{VType::I32},
                              std::array{VType::V128});
  case OpCode::V128__store:
    return checkAlignAndTrans(128, std::array{VType::I32, VType::V128}, {});
  case OpCode::V128__load8_lane:
    return checkAlignLaneAndTrans(8, std::array{VType::I32, VType::V128},
                                  std::array{VType::V128});
  case OpCode::V128__load16_lane:
    return checkAlignLaneAndTrans(16, std::array{VType::I32, VType::V128},
                                  std::array{VType::V128});
  case OpCode::V128__load32_lane:
    return checkAlignLaneAndTrans(32, std::array{VType::I32, VType::V128},
                                  std::array{VType::V128});
  case OpCode::V128__load64_lane:
    return checkAlignLaneAndTrans(64, std::array{VType::I32, VType::V128},
                                  std::array{VType::V128});
  case OpCode::V128__store8_lane:
    return checkAlignLaneAndTrans(8, std::array{VType::I32, VType::V128}, {});
  case OpCode::V128__store16_lane:
    return checkAlignLaneAndTrans(16, std::array{VType::I32, VType::V128}, {});
  case OpCode::V128__store32_lane:
    return checkAlignLaneAndTrans(32, std::array{VType::I32, VType::V128}, {});
  case OpCode::V128__store64_lane:
    return checkAlignLaneAndTrans(64, std::array{VType::I32, VType::V128}, {});

  /// SIMD Const Instruction.
  case OpCode::V128__const:
    return StackTrans({}, std::array{VType::V128});

  /// SIMD Shuffle Instruction.
  case OpCode::I8x16__shuffle: {
    /// Check all lane index < 32 by masking
    const uint128_t Mask = (uint128_t(0xe0e0e0e0e0e0e0e0U) << 64U) |
                           uint128_t(0xe0e0e0e0e0e0e0e0U);
    const uint128_t Result = Instr.getNum().get<uint128_t>() & Mask;
    if (Result) {
      spdlog::error(ErrCode::InvalidLaneIdx);
      return Unexpect(ErrCode::InvalidLaneIdx);
    }
    return StackTrans(std::array{VType::V128, VType::V128},
                      std::array{VType::V128});
  }

  /// SIMD Lane Instructions.
  case OpCode::I8x16__extract_lane_s:
  case OpCode::I8x16__extract_lane_u:
    return checkLaneAndTrans(16, std::array{VType::V128},
                             std::array{VType::I32});
  case OpCode::I8x16__replace_lane:
    return checkLaneAndTrans(16, std::array{VType::V128, VType::I32},
                             std::array{VType::V128});
  case OpCode::I16x8__extract_lane_s:
  case OpCode::I16x8__extract_lane_u:
    return checkLaneAndTrans(8, std::array{VType::V128},
                             std::array{VType::I32});
  case OpCode::I16x8__replace_lane:
    return checkLaneAndTrans(8, std::array{VType::V128, VType::I32},
                             std::array{VType::V128});
  case OpCode::I32x4__extract_lane:
    return checkLaneAndTrans(4, std::array{VType::V128},
                             std::array{VType::I32});
  case OpCode::I32x4__replace_lane:
    return checkLaneAndTrans(4, std::array{VType::V128, VType::I32},
                             std::array{VType::V128});
  case OpCode::I64x2__extract_lane:
    return checkLaneAndTrans(2, std::array{VType::V128},
                             std::array{VType::I64});
  case OpCode::I64x2__replace_lane:
    return checkLaneAndTrans(2, std::array{VType::V128, VType::I64},
                             std::array{VType::V128});
  case OpCode::F32x4__extract_lane:
    return checkLaneAndTrans(4, std::array{VType::V128},
                             std::array{VType::F32});
  case OpCode::F32x4__replace_lane:
    return checkLaneAndTrans(4, std::array{VType::V128, VType::F32},
                             std::array{VType::V128});
  case OpCode::F64x2__extract_lane:
    return checkLaneAndTrans(2, std::array{VType::V128},
                             std::array{VType::F64});
  case OpCode::F64x2__replace_lane:
    return checkLaneAndTrans(2, std::array{VType::V128, VType::F64},
                             std::array{VType::V128});

  /// SIMD Numeric Instructions.
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
  case OpCode::I8x16__popcnt:
  case OpCode::I16x8__abs:
  case OpCode::I16x8__neg:
  case OpCode::I16x8__extend_low_i8x16_s:
  case OpCode::I16x8__extend_high_i8x16_s:
  case OpCode::I16x8__extend_low_i8x16_u:
  case OpCode::I16x8__extend_high_i8x16_u:
  case OpCode::I16x8__extadd_pairwise_i8x16_s:
  case OpCode::I16x8__extadd_pairwise_i8x16_u:
  case OpCode::I32x4__abs:
  case OpCode::I32x4__neg:
  case OpCode::I32x4__extend_low_i16x8_s:
  case OpCode::I32x4__extend_high_i16x8_s:
  case OpCode::I32x4__extend_low_i16x8_u:
  case OpCode::I32x4__extend_high_i16x8_u:
  case OpCode::I32x4__extadd_pairwise_i16x8_s:
  case OpCode::I32x4__extadd_pairwise_i16x8_u:
  case OpCode::I64x2__abs:
  case OpCode::I64x2__neg:
  case OpCode::I64x2__extend_low_i32x4_s:
  case OpCode::I64x2__extend_high_i32x4_s:
  case OpCode::I64x2__extend_low_i32x4_u:
  case OpCode::I64x2__extend_high_i32x4_u:
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
  case OpCode::I32x4__trunc_sat_f64x2_s_zero:
  case OpCode::I32x4__trunc_sat_f64x2_u_zero:
  case OpCode::F64x2__convert_low_i32x4_s:
  case OpCode::F64x2__convert_low_i32x4_u:
  case OpCode::F32x4__demote_f64x2_zero:
  case OpCode::F64x2__promote_low_f32x4:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
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
  case OpCode::I16x8__extmul_low_i8x16_s:
  case OpCode::I16x8__extmul_high_i8x16_s:
  case OpCode::I16x8__extmul_low_i8x16_u:
  case OpCode::I16x8__extmul_high_i8x16_u:
  case OpCode::I16x8__q15mulr_sat_s:
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
  case OpCode::I64x2__add:
  case OpCode::I64x2__sub:
  case OpCode::I64x2__mul:
  case OpCode::I64x2__extmul_low_i32x4_s:
  case OpCode::I64x2__extmul_high_i32x4_s:
  case OpCode::I64x2__extmul_low_i32x4_u:
  case OpCode::I64x2__extmul_high_i32x4_u:
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
  case OpCode::I32x4__dot_i16x8_s:
    return StackTrans(std::array{VType::V128, VType::V128},
                      std::array{VType::V128});
  case OpCode::V128__bitselect:
    return StackTrans(std::array{VType::V128, VType::V128, VType::V128},
                      std::array{VType::V128});
  case OpCode::V128__any_true:
  case OpCode::I8x16__all_true:
  case OpCode::I8x16__bitmask:
  case OpCode::I16x8__all_true:
  case OpCode::I16x8__bitmask:
  case OpCode::I32x4__all_true:
  case OpCode::I32x4__bitmask:
  case OpCode::I64x2__all_true:
  case OpCode::I64x2__bitmask:
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
    spdlog::error(ErrCode::TypeCheckFailed);
    spdlog::error("    Value stack underflow.");
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
    spdlog::error(ErrCode::TypeCheckFailed);
    spdlog::error(ErrInfo::InfoMismatch(VTypeToAST(E), VTypeToAST(*Res)));
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
                           OpCode Code) {
  CtrlStack.emplace_back(In, Out, ValStack.size(), Code);
  pushTypes(In);
}

Expect<FormChecker::CtrlFrame> FormChecker::popCtrl() {
  if (CtrlStack.empty()) {
    /// Ctrl stack is empty when popping.
    spdlog::error(ErrCode::TypeCheckFailed);
    spdlog::error("    Control stack underflow.");
    return Unexpect(ErrCode::TypeCheckFailed);
  }
  if (auto Res = popTypes(CtrlStack.back().EndTypes); !Res) {
    return Unexpect(Res);
  }
  if (ValStack.size() != CtrlStack.back().Height) {
    /// Value stack size not matched.
    spdlog::error(ErrCode::TypeCheckFailed);
    spdlog::error("    Value stack underflow.");
    return Unexpect(ErrCode::TypeCheckFailed);
  }
  auto Head = std::move(CtrlStack.back());
  CtrlStack.pop_back();
  return Head;
}

Span<const VType> FormChecker::getLabelTypes(const FormChecker::CtrlFrame &F) {
  if (F.Code == OpCode::Loop) {
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
              spdlog::error(ErrCode::InvalidFuncTypeIdx);
              spdlog::error(ErrInfo::InfoForbidIndex(
                  ErrInfo::IndexCategory::FunctionType, TypeIdx,
                  static_cast<uint32_t>(Types.size())));
              return Unexpect(ErrCode::InvalidFuncTypeIdx);
            }
            return ReturnType{Types[TypeIdx].first, Types[TypeIdx].second};
          }},
      Type);
}

} // namespace Validator
} // namespace WasmEdge
