// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "validator/formchecker.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <tuple>

namespace WasmEdge {
namespace Validator {

namespace {

// Helper function for printing error log of index out of range.
auto logOutOfRange(ErrCode Code, ErrInfo::IndexCategory Cate, uint32_t Idx,
                   uint32_t Bound) {
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoForbidIndex(Cate, Idx, Bound));
  return Unexpect(Code);
}

} // namespace

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
    Tags.clear();
    NumImportFuncs = 0;
    NumImportGlobals = 0;
  }
}

Expect<void> FormChecker::validate(AST::InstrView Instrs,
                                   Span<const ValType> RetVals) {
  for (const ValType &Val : RetVals) {
    Returns.push_back(Val);
  }
  return checkExpr(Instrs);
}

Expect<void> FormChecker::validate(const ValType &VT) const noexcept {
  // The value type should be validated for the type index case.
  if (VT.isRefType() && VT.getHeapTypeCode() == TypeCode::TypeIndex) {
    if (VT.getTypeIndex() >= Types.size()) {
      spdlog::error(ErrCode::Value::InvalidFuncTypeIdx);
      spdlog::error(ErrInfo::InfoForbidIndex(
          ErrInfo::IndexCategory::FunctionType, VT.getTypeIndex(),
          static_cast<uint32_t>(Types.size())));
      return Unexpect(ErrCode::Value::InvalidFuncTypeIdx);
    }
  }
  return {};
}

void FormChecker::addType(const AST::SubType &Type) { Types.push_back(&Type); }

void FormChecker::addFunc(const uint32_t TypeIdx, const bool IsImport) {
  if (Types.size() > TypeIdx) {
    Funcs.emplace_back(TypeIdx);
  }
  if (IsImport) {
    NumImportFuncs++;
  }
}

void FormChecker::addTable(const AST::TableType &Tab) {
  Tables.push_back(Tab.getRefType());
}

void FormChecker::addMemory(const AST::MemoryType &) { Mems++; }

void FormChecker::addGlobal(const AST::GlobalType &Glob, const bool IsImport) {
  // Type in global is confirmed in loading phase.
  Globals.emplace_back(Glob.getValType(), Glob.getValMut());
  if (IsImport) {
    NumImportGlobals++;
  }
}

void FormChecker::addData(const AST::DataSegment &) {
  Datas.emplace_back(static_cast<uint32_t>(Datas.size()));
}

void FormChecker::addElem(const AST::ElementSegment &Elem) {
  Elems.emplace_back(Elem.getRefType());
}

void FormChecker::addRef(const uint32_t FuncIdx) { Refs.emplace(FuncIdx); }

void FormChecker::addLocal(const ValType &V, bool Initialized) {
  Locals.emplace_back(V);
  if (Initialized || V.isDefaultable()) {
    LocalInits.push_back(static_cast<uint32_t>(Locals.size() - 1));
    Locals.back().IsInit = true;
  }
}

void FormChecker::addTag(const uint32_t TypeIdx) { Tags.push_back(TypeIdx); }

ValType FormChecker::VTypeToAST(const VType &V) {
  if (!V) {
    return TypeCode::I32;
  }
  return *V;
}

Expect<void> FormChecker::checkExpr(AST::InstrView Instrs) {
  if (Instrs.size() > 0) {
    // Push ctrl frame ([] -> [Returns])
    pushCtrl({}, Returns, &*Instrs.rbegin());
    return checkInstrs(Instrs);
  }
  return {};
}

Expect<void> FormChecker::checkInstrs(AST::InstrView Instrs) {
  // Validate instructions
  for (auto &Instr : Instrs) {
    if (auto Res = checkInstr(Instr); !Res) {
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(Res);
    }
  }
  return {};
}

Expect<void> FormChecker::checkInstr(const AST::Instruction &Instr) {
  // Note: The instructions and their immediates have passed proposal
  // configuration checking in loader phase.

  // Helper lambda for checking the defined type.
  auto checkDefinedType =
      [this](uint32_t TIdx, TypeCode TC) -> Expect<const AST::CompositeType *> {
    if (TIdx >= Types.size()) {
      return logOutOfRange(ErrCode::Value::InvalidFuncTypeIdx,
                           ErrInfo::IndexCategory::FunctionType, TIdx,
                           static_cast<uint32_t>(Types.size()));
    }
    const auto &CType = Types[TIdx]->getCompositeType();
    if (CType.getContentTypeCode() == TC) {
      return &CType;
    } else {
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
  };

  // Helper lambda for checking and resolve the block type.
  auto checkBlockType = [this, checkDefinedType](std::vector<ValType> &Buffer,
                                                 const BlockType &BType)
      -> Expect<std::pair<Span<const ValType>, Span<const ValType>>> {
    using ReturnType = std::pair<Span<const ValType>, Span<const ValType>>;
    if (BType.isEmpty()) {
      // Empty case. t2* = none
      return ReturnType{{}, {}};
    } else if (BType.isValType()) {
      // ValType case. t2* = valtype
      if (auto Res = validate(BType.getValType()); !Res) {
        return Unexpect(Res);
      }
      Buffer[0] = BType.getValType();
      return ReturnType{{}, Buffer};
    } else {
      // Type index case. t2* = functype.returns.
      if (auto Res = checkDefinedType(BType.getTypeIndex(), TypeCode::Func)) {
        const auto &FType = (*Res)->getFuncType();
        return ReturnType{FType.getParamTypes(), FType.getReturnTypes()};
      } else {
        return Unexpect(Res);
      }
    }
  };

  // Helper lambda for checking control stack depth and return index.
  auto checkCtrlStackDepth = [this](uint32_t N) -> Expect<uint32_t> {
    // Check the control stack for at least N + 1 frames.
    if (N >= CtrlStack.size()) {
      // Branch out of stack.
      return logOutOfRange(ErrCode::Value::InvalidLabelIdx,
                           ErrInfo::IndexCategory::Label, N,
                           static_cast<uint32_t>(CtrlStack.size()));
    }
    // Return the index of the last N element.
    return static_cast<uint32_t>(CtrlStack.size()) - UINT32_C(1) - N;
  };

  // Helper lambda for checking memory index and perform transformation.
  auto checkMemAndTrans = [this,
                           &Instr](Span<const ValType> Take,
                                   Span<const ValType> Put) -> Expect<void> {
    if (Instr.getTargetIndex() >= Mems) {
      return logOutOfRange(ErrCode::Value::InvalidMemoryIdx,
                           ErrInfo::IndexCategory::Memory,
                           Instr.getTargetIndex(), Mems);
    }
    return StackTrans(Take, Put);
  };

  // Helper lambda for checking lane index and perform transformation.
  auto checkLaneAndTrans = [this,
                            &Instr](uint32_t N, Span<const ValType> Take,
                                    Span<const ValType> Put) -> Expect<void> {
    if (Instr.getMemoryLane() >= N) {
      return logOutOfRange(ErrCode::Value::InvalidLaneIdx,
                           ErrInfo::IndexCategory::Lane, Instr.getMemoryLane(),
                           N);
    }
    return StackTrans(Take, Put);
  };

  // Helper lambda for checking memory alignment and perform transformation.
  auto checkAlignAndTrans = [this, checkLaneAndTrans,
                             &Instr](uint32_t N, Span<const ValType> Take,
                                     Span<const ValType> Put,
                                     bool CheckLane = false) -> Expect<void> {
    if (Instr.getTargetIndex() >= Mems) {
      return logOutOfRange(ErrCode::Value::InvalidMemoryIdx,
                           ErrInfo::IndexCategory::Memory,
                           Instr.getTargetIndex(), Mems);
    }
    if (Instr.getMemoryAlign() > 31 ||
        (1UL << Instr.getMemoryAlign()) > (N >> 3UL)) {
      // 2 ^ align needs to <= N / 8
      spdlog::error(ErrCode::Value::InvalidAlignment);
      spdlog::error(ErrInfo::InfoMismatch(static_cast<uint8_t>(N >> 3),
                                          Instr.getMemoryAlign()));
      return Unexpect(ErrCode::Value::InvalidAlignment);
    }
    if (CheckLane) {
      return checkLaneAndTrans(128 / N, Take, Put);
    }
    return StackTrans(Take, Put);
  };

  // Helper lambda for checking value types matching.
  auto checkTypesMatching = [this](Span<const ValType> Exp,
                                   Span<const ValType> Got) -> Expect<void> {
    if (!AST::TypeMatcher::matchTypes(Types, Exp, Got)) {
      std::vector<ValType> ExpV(Exp.begin(), Exp.end()),
          GotV(Got.begin(), Got.end());
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(ExpV, GotV));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
    return {};
  };

  // Helper lambda for recording jump data.
  auto recordJump = [this, &Instr](AST::Instruction::JumpDescriptor &Jump,
                                   uint32_t Arity, uint32_t D) -> void {
    const uint32_t Remain =
        static_cast<uint32_t>(ValStack.size() - CtrlStack[D].Height);
    Jump.StackEraseBegin = Remain + Arity;
    Jump.StackEraseEnd = Arity;
    Jump.PCOffset = static_cast<int32_t>(CtrlStack[D].Jump - &Instr);
  };

  // Helper lambda for unpacking a value type.
  auto unpackType = [](const ValType &T) -> ValType {
    if (T.isPackType()) {
      return ValType(TypeCode::I32);
    }
    return T;
  };

  // Helper lambda for downcasting into the top heap type.
  auto toTopHeapType = [this](const ValType &T) -> ValType {
    assuming(T.isRefType());
    if (T.isAbsHeapType()) {
      switch (T.getHeapTypeCode()) {
      case TypeCode::NullFuncRef:
      case TypeCode::FuncRef:
        return TypeCode::FuncRef;
      case TypeCode::NullExternRef:
      case TypeCode::ExternRef:
        return TypeCode::ExternRef;
      case TypeCode::NullRef:
      case TypeCode::AnyRef:
      case TypeCode::EqRef:
      case TypeCode::I31Ref:
      case TypeCode::StructRef:
      case TypeCode::ArrayRef:
        return TypeCode::AnyRef;
      default:
        assumingUnreachable();
      }
    } else {
      const auto &CompType = Types[T.getTypeIndex()]->getCompositeType();
      if (CompType.isFunc()) {
        return TypeCode::FuncRef;
      } else {
        return TypeCode::AnyRef;
      }
    }
  };

  switch (Instr.getOpCode()) {
  // Control instructions.
  case OpCode::Unreachable:
    return unreachable();
  case OpCode::Nop:
    return {};

  case OpCode::Block:
  case OpCode::Loop:
  case OpCode::If:
  // LEGACY-EH: remove the `Try` case after deprecating legacy EH.
  case OpCode::Try:
  case OpCode::Try_table: {
    // Get blocktype [t1*] -> [t2*] and check valtype first.
    std::vector<ValType> Buffer(1);
    Span<const ValType> T1, T2;
    // LEGACY-EH: remove the `Try` case after deprecating legacy EH.
    const auto &BType = (Instr.getOpCode() == OpCode::Try ||
                         Instr.getOpCode() == OpCode::Try_table)
                            ? Instr.getTryCatch().ResType
                            : Instr.getBlockType();
    if (auto Res = checkBlockType(Buffer, BType)) {
      std::tie(T1, T2) = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
    // For the if instruction, pop I32 first.
    if (Instr.getOpCode() == OpCode::If) {
      if (auto Res = popType(TypeCode::I32); !Res) {
        return Unexpect(Res);
      }
    }
    // Pop and check [t1*]
    if (auto Res = popTypes(T1); !Res) {
      return Unexpect(Res);
    }
    // For the try_table instruction, validate the handlers.
    if (Instr.getOpCode() == OpCode::Try_table) {
      const auto &TryDesc = Instr.getTryCatch();
      const_cast<AST::Instruction::TryDescriptor &>(TryDesc).BlockParamNum =
          static_cast<uint32_t>(T1.size());
      // Validate catch clause.
      for (const auto &C : TryDesc.Catch) {
        if (!C.IsAll) {
          // Check tag index.
          if (unlikely(C.TagIndex >= Tags.size())) {
            return logOutOfRange(ErrCode::Value::InvalidTagIdx,
                                 ErrInfo::IndexCategory::Tag, C.TagIndex,
                                 static_cast<uint32_t>(Tags.size()));
          }
          // Result type of tag index are checked in tag section.
        }
        if (auto D = checkCtrlStackDepth(C.LabelIndex)) {
          pushCtrl({}, getLabelTypes(CtrlStack[*D]), &Instr + TryDesc.JumpEnd,
                   Instr.getOpCode());
          std::vector<ValType> NTypes;
          if (!C.IsAll) {
            // The type is checked as a function type.
            NTypes = Types[Tags[C.TagIndex]]
                         ->getCompositeType()
                         .getFuncType()
                         .getParamTypes();
          }
          if (C.IsRef) {
            NTypes.emplace_back(ValType(TypeCode::ExnRef));
          }
          pushTypes(NTypes);
          if (auto Res = popCtrl(); !Res) {
            return Unexpect(Res);
          }
          recordJump(const_cast<AST::Instruction::JumpDescriptor &>(C.Jump),
                     static_cast<uint32_t>(NTypes.size()), *D);
        } else {
          return Unexpect(D);
        }
      }
    } else if (Instr.getOpCode() == OpCode::Try) {
      // LEGACY-EH: remove the `Try` case after deprecating legacy EH.
      const auto &TryDesc = Instr.getTryCatch();
      const_cast<AST::Instruction::TryDescriptor &>(TryDesc).BlockParamNum =
          static_cast<uint32_t>(T1.size());
    }
    // Push ctrl frame ([t1*], [t2*])
    const AST::Instruction *From = Instr.getOpCode() == OpCode::Loop
                                       ? &Instr
                                       : &Instr + Instr.getJumpEnd();
    pushCtrl(T1, T2, From, Instr.getOpCode());
    if (Instr.getOpCode() == OpCode::If &&
        Instr.getJumpElse() == Instr.getJumpEnd()) {
      // No else case in if-else statement.
      if (auto Res = checkTypesMatching(T2, T1); !Res) {
        return Unexpect(Res);
      }
    }
    return {};
  }

  case OpCode::Else:
    if (auto Res = popCtrl()) {
      pushCtrl(Res->StartTypes, Res->EndTypes, Res->Jump, Instr.getOpCode());
    } else {
      return Unexpect(Res);
    }
    return {};

  // LEGACY-EH: remove the `Catch` after deprecating legacy EH.
  case OpCode::Catch: {
    const auto &CatchDesc = Instr.getCatchLegacy();
    // Check tag index.
    if (unlikely(CatchDesc.TagIndex >= Tags.size())) {
      return logOutOfRange(ErrCode::Value::InvalidTagIdx,
                           ErrInfo::IndexCategory::Tag, CatchDesc.TagIndex,
                           static_cast<uint32_t>(Tags.size()));
    }
    const auto &NTypes = Types[Tags[CatchDesc.TagIndex]]
                             ->getCompositeType()
                             .getFuncType()
                             .getParamTypes();
    const auto &TryInstr = *(&Instr - CatchDesc.CatchPCOffset);
    const auto &Catch = TryInstr.getTryCatch().Catch[CatchDesc.CatchIndex];
    if (auto Res = popCtrl()) {
      // The continue block PC offset is the next of this instruction.
      auto &Jump = const_cast<AST::Instruction::JumpDescriptor &>(Catch.Jump);
      Jump.StackEraseBegin =
          static_cast<uint32_t>(ValStack.size() - Res->Height) +
          static_cast<uint32_t>(NTypes.size());
      Jump.StackEraseEnd = static_cast<uint32_t>(NTypes.size());
      Jump.PCOffset = static_cast<int32_t>(CatchDesc.CatchPCOffset + 1);
      pushCtrl(NTypes, Res->EndTypes, Res->Jump, Instr.getOpCode());
    } else {
      return Unexpect(Res);
    }
    return {};
  }

  case OpCode::Throw:
    if (unlikely(Instr.getTargetIndex() >= Tags.size())) {
      return logOutOfRange(ErrCode::Value::InvalidTagIdx,
                           ErrInfo::IndexCategory::Tag, Instr.getTargetIndex(),
                           static_cast<uint32_t>(Tags.size()));
    }
    if (auto CompType =
            checkDefinedType(Tags[Instr.getTargetIndex()], TypeCode::Func)) {
      std::vector<ValType> Input = (*CompType)->getFuncType().getParamTypes();
      if (auto Res = popTypes(Input); !Res) {
        return Unexpect(Res);
      }
      return unreachable();
    } else {
      return Unexpect(CompType);
    }

  // LEGACY-EH: remove the `Rethrow` after deprecating legacy EH.
  case OpCode::Rethrow:
    spdlog::error(ErrCode::Value::TypeCheckFailed);
    spdlog::error("    Deprecated `rethrow` instruction.");
    return Unexpect(ErrCode::Value::TypeCheckFailed);

  case OpCode::Throw_ref:
    if (auto Res = popType(TypeCode::ExnRef); !Res) {
      return Unexpect(Res);
    }
    return unreachable();

  case OpCode::End:
    if (auto Res = popCtrl()) {
      pushTypes(Res->EndTypes);
    } else {
      return Unexpect(Res);
    }
    return {};

  case OpCode::Br:
    if (auto D = checkCtrlStackDepth(Instr.getJump().TargetIndex); !D) {
      return Unexpect(D);
    } else {
      // D is the last D element of control stack.
      const auto NTypes = getLabelTypes(CtrlStack[*D]);
      if (auto Res = popTypes(NTypes); !Res) {
        return Unexpect(Res);
      }
      recordJump(const_cast<AST::Instruction &>(Instr).getJump(),
                 static_cast<uint32_t>(NTypes.size()), *D);
      return unreachable();
    }
  case OpCode::Br_if:
    if (auto D = checkCtrlStackDepth(Instr.getJump().TargetIndex); !D) {
      return Unexpect(D);
    } else {
      // D is the last D element of control stack.
      if (auto Res = popType(TypeCode::I32); !Res) {
        return Unexpect(Res);
      }
      const auto NTypes = getLabelTypes(CtrlStack[*D]);
      if (auto Res = popTypes(NTypes); !Res) {
        return Unexpect(Res);
      }
      recordJump(const_cast<AST::Instruction &>(Instr).getJump(),
                 static_cast<uint32_t>(NTypes.size()), *D);
      pushTypes(NTypes);
      return {};
    }
  case OpCode::Br_table: {
    if (auto Res = popType(TypeCode::I32); !Res) {
      return Unexpect(Res);
    }
    auto LabelTable = const_cast<AST::Instruction &>(Instr).getLabelList();
    const auto LabelTableSize = static_cast<uint32_t>(LabelTable.size() - 1);
    if (auto M = checkCtrlStackDepth(LabelTable[LabelTableSize].TargetIndex)) {
      // M is the last M element of control stack.
      auto MTypes = getLabelTypes(CtrlStack[*M]);
      for (uint32_t LabelIdx = 0; LabelIdx < LabelTableSize; ++LabelIdx) {
        const uint32_t L = LabelTable[LabelIdx].TargetIndex;
        if (auto N = checkCtrlStackDepth(L)) {
          // N is the last N element of control stack.
          const auto NTypes = getLabelTypes(CtrlStack[*N]);
          if (MTypes.size() != NTypes.size()) {
            return checkTypesMatching(MTypes, NTypes);
          }
          // Push the popped types.
          std::vector<VType> TypeBuf(NTypes.size());
          for (uint32_t IdxN = static_cast<uint32_t>(NTypes.size()); IdxN >= 1;
               --IdxN) {
            const uint32_t Idx = IdxN - 1;
            // Cannot use popTypes() here because we need the popped value.
            if (auto Res = popType(NTypes[Idx])) {
              // Have to check is `unreachableVType` occurred for the case of
              // `unreachable` instruction appeared before the `br_table`
              // instruction.
              if (CtrlStack.back().IsUnreachable) {
                TypeBuf[Idx] = unreachableVType();
              } else {
                TypeBuf[Idx] = *Res;
              }
            } else {
              return Unexpect(Res);
            }
          }
          recordJump(LabelTable[LabelIdx], static_cast<uint32_t>(NTypes.size()),
                     *N);
          pushTypes(TypeBuf);
        } else {
          return Unexpect(N);
        }
      }
      const auto NTypes = getLabelTypes(CtrlStack[*M]);
      if (auto Res = popTypes(NTypes); !Res) {
        return Unexpect(Res);
      }
      recordJump(LabelTable[LabelTableSize],
                 static_cast<uint32_t>(NTypes.size()), *M);
      return unreachable();
    } else {
      return Unexpect(M);
    }
  }

  case OpCode::Br_on_null:
    // D is the last D element of control stack.
    if (auto D = checkCtrlStackDepth(Instr.getJump().TargetIndex)) {
      const auto NTypes = getLabelTypes(CtrlStack[*D]);
      if (auto ResT = popType()) {
        if ((*ResT).has_value() && !(*ResT)->isRefType()) {
          spdlog::error(ErrCode::Value::InvalidBrRefType);
          return Unexpect(ErrCode::ErrCode::Value::InvalidBrRefType);
        }
        if (auto Res = popTypes(NTypes); !Res) {
          return Unexpect(Res);
        }
        recordJump(const_cast<AST::Instruction &>(Instr).getJump(),
                   static_cast<uint32_t>(NTypes.size()), *D);
        pushTypes(NTypes);
        if ((*ResT).has_value()) {
          pushType((*ResT)->toNonNullableRef());
        } else {
          pushType(unreachableVType());
        }
        return {};
      } else {
        return Unexpect(ResT);
      }
    } else {
      return Unexpect(D);
    }

  case OpCode::Br_on_non_null:
    if (auto D = checkCtrlStackDepth(Instr.getJump().TargetIndex)) {
      // Get the result type of the label. (Should be [t* rt].)
      auto LabelTypes = getLabelTypes(CtrlStack[*D]);
      std::vector<ValType> NTypes(LabelTypes.begin(), LabelTypes.end());
      if (unlikely(NTypes.empty())) {
        spdlog::error(ErrCode::Value::InvalidBrRefType);
        return Unexpect(ErrCode::Value::InvalidBrRefType);
      }
      // Pop types [t* (ref.null rt)].
      ValType &RT = NTypes.back();
      if (!RT.isRefType()) {
        spdlog::error(ErrCode::Value::InvalidBrRefType);
        return Unexpect(ErrCode::Value::InvalidBrRefType);
      }
      RT.toNullableRef();
      if (auto Res = popTypes(NTypes); !Res) {
        return Unexpect(Res);
      }
      recordJump(const_cast<AST::Instruction &>(Instr).getJump(),
                 static_cast<uint32_t>(NTypes.size()), *D);
      // Push types [t*].
      NTypes.pop_back();
      pushTypes(NTypes);
      return {};
    } else {
      return Unexpect(D);
    }

  case OpCode::Return:
    if (auto Res = popTypes(Returns); !Res) {
      return Unexpect(Res);
    }
    return unreachable();

  case OpCode::Call: {
    auto N = Instr.getTargetIndex();
    if (unlikely(N >= Funcs.size())) {
      return logOutOfRange(ErrCode::Value::InvalidFuncIdx,
                           ErrInfo::IndexCategory::Function, N,
                           static_cast<uint32_t>(Funcs.size()));
    }
    // Due to validation when adding functions, Type[Funcs[N]] must be a
    // function type.
    auto &FuncType = Types[Funcs[N]]->getCompositeType().getFuncType();
    return StackTrans(FuncType.getParamTypes(), FuncType.getReturnTypes());
  }
  case OpCode::Call_indirect: {
    auto N = Instr.getTargetIndex();
    auto T = Instr.getSourceIndex();
    // Check source table index.
    if (unlikely(T >= Tables.size())) {
      return logOutOfRange(ErrCode::Value::InvalidTableIdx,
                           ErrInfo::IndexCategory::Table, T,
                           static_cast<uint32_t>(Tables.size()));
    }
    if (unlikely(!Tables[T].isFuncRefType())) {
      spdlog::error(ErrCode::Value::InvalidTableIdx);
      return Unexpect(ErrCode::Value::InvalidTableIdx);
    }
    // Check target function type index.
    if (auto CompType = checkDefinedType(N, TypeCode::Func)) {
      if (auto Res = popType(TypeCode::I32); !Res) {
        return Unexpect(Res);
      }
      const auto &FType = (*CompType)->getFuncType();
      return StackTrans(FType.getParamTypes(), FType.getReturnTypes());
    } else {
      return Unexpect(CompType);
    }
  }
  case OpCode::Return_call: {
    auto N = Instr.getTargetIndex();
    if (unlikely(N >= Funcs.size())) {
      // Call function index out of range
      spdlog::error(ErrCode::Value::InvalidFuncIdx);
      spdlog::error(
          ErrInfo::InfoForbidIndex(ErrInfo::IndexCategory::Function, N,
                                   static_cast<uint32_t>(Funcs.size())));
      return Unexpect(ErrCode::Value::InvalidFuncIdx);
    }
    // Due to validation when adding functions, Type[Funcs[N]] must be a
    // function type.
    auto &FType = Types[Funcs[N]]->getCompositeType().getFuncType();
    if (auto Res = checkTypesMatching(Returns, FType.getReturnTypes()); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = popTypes(FType.getParamTypes()); !Res) {
      return Unexpect(Res);
    }
    return unreachable();
  }
  case OpCode::Return_call_indirect: {
    auto N = Instr.getTargetIndex();
    auto T = Instr.getSourceIndex();
    // Check source table index.
    if (unlikely(T >= Tables.size())) {
      return logOutOfRange(ErrCode::Value::InvalidTableIdx,
                           ErrInfo::IndexCategory::Table, T,
                           static_cast<uint32_t>(Tables.size()));
    }
    if (unlikely(!Tables[T].isFuncRefType())) {
      spdlog::error(ErrCode::Value::InvalidTableIdx);
      return Unexpect(ErrCode::Value::InvalidTableIdx);
    }
    // Check target function type index.
    if (auto CompType = checkDefinedType(N, TypeCode::Func)) {
      const auto &FType = (*CompType)->getFuncType();
      if (auto Res = checkTypesMatching(Returns, FType.getReturnTypes());
          !Res) {
        return Unexpect(Res);
      }
      if (auto Res = popType(TypeCode::I32); !Res) {
        return Unexpect(Res);
      }
      if (auto Res = popTypes(FType.getParamTypes()); !Res) {
        return Unexpect(Res);
      }
      return unreachable();
    } else {
      return Unexpect(CompType);
    }
  }
  case OpCode::Call_ref:
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Func)) {
      const auto &FType = (*Res)->getFuncType();
      std::vector<ValType> Input = FType.getParamTypes();
      Input.push_back(ValType(TypeCode::RefNull, Instr.getTargetIndex()));
      return StackTrans(Input, FType.getReturnTypes());
    } else {
      return Unexpect(Res);
    }
  case OpCode::Return_call_ref: {
    if (auto CompType =
            checkDefinedType(Instr.getTargetIndex(), TypeCode::Func)) {
      const auto &FType = (*CompType)->getFuncType();
      if (auto Res = checkTypesMatching(Returns, FType.getReturnTypes());
          !Res) {
        return Unexpect(Res);
      }
      if (auto Res =
              popType(ValType(TypeCode::RefNull, Instr.getTargetIndex()));
          !Res) {
        return Unexpect(Res);
      }
      if (auto Res = popTypes(FType.getParamTypes()); !Res) {
        return Unexpect(Res);
      }
      return unreachable();
    } else {
      return Unexpect(CompType);
    }
  }

  // LEGACY-EH: remove the `Catch_all` after deprecating legacy EH.
  case OpCode::Catch_all: {
    const auto &CatchDesc = Instr.getCatchLegacy();
    const auto &TryInstr = *(&Instr - CatchDesc.CatchPCOffset);
    const auto &Catch = TryInstr.getTryCatch().Catch[CatchDesc.CatchIndex];
    if (auto Res = popCtrl()) {
      // The continue block PC offset is the next of this instruction.
      auto &Jump = const_cast<AST::Instruction::JumpDescriptor &>(Catch.Jump);
      Jump.StackEraseBegin =
          static_cast<uint32_t>(ValStack.size() - Res->Height);
      Jump.StackEraseEnd = 0;
      Jump.PCOffset = static_cast<int32_t>(CatchDesc.CatchPCOffset + 1);
      pushCtrl({}, Res->EndTypes, Res->Jump, Instr.getOpCode());
    } else {
      return Unexpect(Res);
    }
    return {};
  }

  // LEGACY-EH: remove the `Delegate` after deprecating legacy EH.
  case OpCode::Delegate:
    spdlog::error(ErrCode::Value::TypeCheckFailed);
    spdlog::error("    Deprecated `delegate` instruction.");
    return Unexpect(ErrCode::Value::TypeCheckFailed);

  // Reference Instructions.
  case OpCode::Ref__null:
    if (auto Res = validate(Instr.getValType())) {
      return StackTrans({}, {Instr.getValType()});
    } else {
      return Unexpect(Res);
    }
  case OpCode::Ref__is_null:
    if (auto Res = popType()) {
      if ((*Res).has_value() && !(*Res)->isRefType()) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(
            ErrInfo::InfoMismatch(TypeCode::FuncRef, VTypeToAST(*Res)));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
    } else {
      return Unexpect(Res);
    }
    return StackTrans({}, {ValType(TypeCode::I32)});
  case OpCode::Ref__func: {
    auto FuncIdx = Instr.getTargetIndex();
    if (Refs.find(FuncIdx) == Refs.cend()) {
      // Undeclared function reference.
      spdlog::error(ErrCode::Value::InvalidRefIdx);
      return Unexpect(ErrCode::Value::InvalidRefIdx);
    }
    assuming(FuncIdx < Funcs.size());
    auto TypeIdx = Funcs[FuncIdx];
    assuming(TypeIdx < Types.size());
    return StackTrans({}, {ValType(TypeCode::Ref, TypeIdx)});
  }
  case OpCode::Ref__eq: {
    return StackTrans({ValType(TypeCode::RefNull, TypeCode::EqRef),
                       ValType(TypeCode::RefNull, TypeCode::EqRef)},
                      {ValType(TypeCode::I32)});
  }
  case OpCode::Ref__as_non_null: {
    if (auto Res = popType()) {
      if (*Res == unreachableVType()) {
        pushType(unreachableVType());
        return {};
      }
      if (!(*Res)->isRefType()) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(ErrInfo::InfoMismatch(
            ValType(TypeCode::RefNull, TypeCode::FuncRef), VTypeToAST(*Res)));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      return StackTrans({}, {(*Res)->toNonNullableRef()});
    } else {
      return Unexpect(Res);
    }
  }

  case OpCode::Struct__new:
  case OpCode::Struct__new_default: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Struct)) {
      std::vector<ValType> Fields;
      if (Instr.getOpCode() == OpCode::Struct__new) {
        Fields.reserve((*Res)->getFieldTypes().size());
      }
      for (auto &FType : (*Res)->getFieldTypes()) {
        if (Instr.getOpCode() == OpCode::Struct__new) {
          Fields.emplace_back(unpackType(FType.getStorageType()));
        } else if (!FType.getStorageType().isDefaultable()) {
          spdlog::error(ErrCode::Value::TypeCheckFailed);
          spdlog::error("    Value type should be defaultable.");
          return Unexpect(ErrCode::Value::TypeCheckFailed);
        }
      }
      return StackTrans(Fields,
                        {ValType(TypeCode::Ref, Instr.getTargetIndex())});
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Struct__get:
  case OpCode::Struct__get_s:
  case OpCode::Struct__get_u: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Struct)) {
      if (Instr.getSourceIndex() >= (*Res)->getFieldTypes().size()) {
        return logOutOfRange(
            ErrCode::Value::InvalidFieldIdx, ErrInfo::IndexCategory::Field,
            Instr.getSourceIndex(),
            static_cast<uint32_t>((*Res)->getFieldTypes().size()));
      }
      const auto &FType = (*Res)->getFieldTypes()[Instr.getSourceIndex()];
      if (unlikely(Instr.getOpCode() == OpCode::Struct__get &&
                   FType.getStorageType().isPackType())) {
        // For a packed type, the `_s` or `_u` in instruction is required.
        spdlog::error(ErrCode::Value::InvalidPackedField);
        return Unexpect(ErrCode::Value::InvalidPackedField);
      } else if (unlikely(Instr.getOpCode() != OpCode::Struct__get &&
                          !FType.getStorageType().isPackType())) {
        // The `_s` or `_u` in instruction only accepts packed field.
        spdlog::error(ErrCode::Value::InvalidUnpackedField);
        return Unexpect(ErrCode::Value::InvalidUnpackedField);
      }
      return StackTrans({ValType(TypeCode::RefNull, Instr.getTargetIndex())},
                        {unpackType(FType.getStorageType())});
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Struct__set: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Struct)) {
      if (Instr.getSourceIndex() >= (*Res)->getFieldTypes().size()) {
        return logOutOfRange(
            ErrCode::Value::InvalidFieldIdx, ErrInfo::IndexCategory::Field,
            Instr.getSourceIndex(),
            static_cast<uint32_t>((*Res)->getFieldTypes().size()));
      }
      const auto &FType = (*Res)->getFieldTypes()[Instr.getSourceIndex()];
      if (FType.getValMut() != ValMut::Var) {
        spdlog::error(ErrCode::Value::ImmutableField);
        return Unexpect(ErrCode::Value::ImmutableField);
      }
      return StackTrans({ValType(TypeCode::RefNull, Instr.getTargetIndex()),
                         unpackType(FType.getStorageType())},
                        {});
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Array__new:
  case OpCode::Array__new_default:
  case OpCode::Array__new_fixed: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Array)) {
      const auto &SType = (*Res)->getFieldTypes()[0].getStorageType();
      if (Instr.getOpCode() == OpCode::Array__new) {
        return StackTrans({unpackType(SType), ValType(TypeCode::I32)},
                          {ValType(TypeCode::Ref, Instr.getTargetIndex())});
      } else if (Instr.getOpCode() == OpCode::Array__new_default) {
        if (!SType.isDefaultable()) {
          spdlog::error(ErrCode::Value::TypeCheckFailed);
          spdlog::error("    Value type should be defaultable.");
          return Unexpect(ErrCode::Value::TypeCheckFailed);
        }
        return StackTrans({ValType(TypeCode::I32)},
                          {ValType(TypeCode::Ref, Instr.getTargetIndex())});
      } else {
        std::vector<ValType> Fields(Instr.getSourceIndex(), unpackType(SType));
        return StackTrans(Fields,
                          {ValType(TypeCode::Ref, Instr.getTargetIndex())});
      }
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Array__new_data:
  case OpCode::Array__init_data: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Array)) {
      const auto &FType = (*Res)->getFieldTypes()[0];
      if (Instr.getOpCode() == OpCode::Array__init_data &&
          FType.getValMut() != ValMut::Var) {
        spdlog::error(ErrCode::Value::ImmutableArray);
        return Unexpect(ErrCode::Value::ImmutableArray);
      }
      if (!unpackType(FType.getStorageType()).isNumType()) {
        spdlog::error(ErrCode::Value::ArrayTypesNumtypeRequired);
        return Unexpect(ErrCode::Value::ArrayTypesNumtypeRequired);
      }
      if (Instr.getSourceIndex() >= Datas.size()) {
        return logOutOfRange(
            ErrCode::Value::InvalidDataIdx, ErrInfo::IndexCategory::Data,
            Instr.getSourceIndex(), static_cast<uint32_t>(Datas.size()));
      }
      if (Instr.getOpCode() == OpCode::Array__new_data) {
        return StackTrans({ValType(TypeCode::I32), ValType(TypeCode::I32)},
                          {ValType(TypeCode::Ref, Instr.getTargetIndex())});
      } else {
        return StackTrans({ValType(TypeCode::RefNull, Instr.getTargetIndex()),
                           ValType(TypeCode::I32), ValType(TypeCode::I32),
                           ValType(TypeCode::I32)},
                          {});
      }
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Array__new_elem:
  case OpCode::Array__init_elem: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Array)) {
      const auto &FType = (*Res)->getFieldTypes()[0];
      if (Instr.getOpCode() == OpCode::Array__init_elem &&
          FType.getValMut() != ValMut::Var) {
        spdlog::error(ErrCode::Value::ImmutableArray);
        return Unexpect(ErrCode::Value::ImmutableArray);
      }
      if (!FType.getStorageType().isRefType()) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      if (Instr.getSourceIndex() >= Elems.size()) {
        return logOutOfRange(
            ErrCode::Value::InvalidElemIdx, ErrInfo::IndexCategory::Element,
            Instr.getSourceIndex(), static_cast<uint32_t>(Elems.size()));
      }
      if (!AST::TypeMatcher::matchType(Types, FType.getStorageType(),
                                       Elems[Instr.getSourceIndex()])) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(ErrInfo::InfoMismatch(FType.getStorageType(),
                                            Elems[Instr.getSourceIndex()]));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      if (Instr.getOpCode() == OpCode::Array__new_elem) {
        return StackTrans({ValType(TypeCode::I32), ValType(TypeCode::I32)},
                          {ValType(TypeCode::Ref, Instr.getTargetIndex())});
      } else {
        return StackTrans({ValType(TypeCode::RefNull, Instr.getTargetIndex()),
                           ValType(TypeCode::I32), ValType(TypeCode::I32),
                           ValType(TypeCode::I32)},
                          {});
      }
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Array__get:
  case OpCode::Array__get_s:
  case OpCode::Array__get_u: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Array)) {
      const auto &FType = (*Res)->getFieldTypes()[0];
      if (unlikely(Instr.getOpCode() == OpCode::Array__get &&
                   FType.getStorageType().isPackType())) {
        // For a packed type, the `_s` or `_u` in instruction is required.
        spdlog::error(ErrCode::Value::InvalidPackedArray);
        return Unexpect(ErrCode::Value::InvalidPackedArray);
      } else if (unlikely(Instr.getOpCode() != OpCode::Array__get &&
                          !FType.getStorageType().isPackType())) {
        // The `_s` or `_u` in instruction only accepts packed array.
        spdlog::error(ErrCode::Value::InvalidUnpackedArray);
        return Unexpect(ErrCode::Value::InvalidUnpackedArray);
      }
      return StackTrans({ValType(TypeCode::RefNull, Instr.getTargetIndex()),
                         ValType(TypeCode::I32)},
                        {unpackType(FType.getStorageType())});
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Array__set:
  case OpCode::Array__fill: {
    if (auto Res = checkDefinedType(Instr.getTargetIndex(), TypeCode::Array)) {
      const auto &FType = (*Res)->getFieldTypes()[0];
      if (FType.getValMut() != ValMut::Var) {
        spdlog::error(ErrCode::Value::ImmutableArray);
        return Unexpect(ErrCode::Value::ImmutableArray);
      }
      std::vector<ValType> Fields = {
          ValType(TypeCode::RefNull, Instr.getTargetIndex()),
          ValType(TypeCode::I32), unpackType(FType.getStorageType())};
      if (Instr.getOpCode() == OpCode::Array__fill) {
        Fields.emplace_back(ValType(TypeCode::I32));
      }
      return StackTrans(Fields, {});
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Array__len:
    return StackTrans({ValType(TypeCode::ArrayRef)}, {ValType(TypeCode::I32)});
  case OpCode::Array__copy: {
    if (auto Dst = checkDefinedType(Instr.getTargetIndex(), TypeCode::Array)) {
      const auto &DstFType = (*Dst)->getFieldTypes()[0];
      if (DstFType.getValMut() != ValMut::Var) {
        spdlog::error(ErrCode::Value::ImmutableArray);
        return Unexpect(ErrCode::Value::ImmutableArray);
      }
      if (auto Src =
              checkDefinedType(Instr.getSourceIndex(), TypeCode::Array)) {
        const auto &SrcFType = (*Src)->getFieldTypes()[0];
        if (!AST::TypeMatcher::matchType(Types, DstFType.getStorageType(),
                                         SrcFType.getStorageType())) {
          spdlog::error(ErrCode::Value::ArrayTypesMismatch);
          spdlog::error(ErrInfo::InfoMismatch(DstFType.getStorageType(),
                                              SrcFType.getStorageType()));
          return Unexpect(ErrCode::Value::ArrayTypesMismatch);
        }
        return StackTrans({ValType(TypeCode::RefNull, Instr.getTargetIndex()),
                           ValType(TypeCode::I32),
                           ValType(TypeCode::RefNull, Instr.getSourceIndex()),
                           ValType(TypeCode::I32), ValType(TypeCode::I32)},
                          {});
      } else {
        return Unexpect(Src);
      }
    } else {
      return Unexpect(Dst);
    }
  }

  case OpCode::Ref__test:
  case OpCode::Ref__test_null:
  case OpCode::Ref__cast:
  case OpCode::Ref__cast_null: {
    if (auto Res = validate(Instr.getValType()); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = popType()) {
      if (!(*Res).has_value() || !(*Res)->isRefType()) {
        // For getting bottom valtype here, matching must fail.
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(
            ErrInfo::InfoMismatch(Instr.getValType(), VTypeToAST(*Res)));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      if (!AST::TypeMatcher::matchType(Types, toTopHeapType(**Res),
                                       Instr.getValType())) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(ErrInfo::InfoMismatch(**Res, Instr.getValType()));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      if (Instr.getOpCode() == OpCode::Ref__test ||
          Instr.getOpCode() == OpCode::Ref__test_null) {
        return StackTrans({}, {ValType(TypeCode::I32)});
      } else {
        return StackTrans({}, {Instr.getValType()});
      }
    } else {
      return Unexpect(Res);
    }
  }
  case OpCode::Br_on_cast:
  case OpCode::Br_on_cast_fail: {
    // The reference types should be valid.
    auto &RT1 = Instr.getBrCast().RType1;
    auto &RT2 = Instr.getBrCast().RType2;
    if (auto Res = validate(RT1); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = validate(RT2); !Res) {
      return Unexpect(Res);
    }
    // The reference type RT2 should match RT1.
    if (unlikely(!AST::TypeMatcher::matchType(Types, RT1, RT2))) {
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(RT1, RT2));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
    if (auto D = checkCtrlStackDepth(Instr.getBrCast().Jump.TargetIndex)) {
      // Get the result type of the label. (Should be [t* rt'].)
      auto LabelTypes = getLabelTypes(CtrlStack[*D]);
      std::vector<ValType> NTypes(LabelTypes.begin(), LabelTypes.end());
      if (unlikely(NTypes.empty())) {
        spdlog::error(ErrCode::Value::InvalidBrRefType);
        return Unexpect(ErrCode::Value::InvalidBrRefType);
      }
      // Get the type difference between rt1 \ rt2. (rt1' = rt1 \ rt2)
      ValType RT1P = RT2.isNullableRefType() ? RT1.getNonNullableRef() : RT1;
      // For br_on_cast, rt2 must match rt'.
      // For Br_on_cast_fail, rt1' must match rt'.
      ValType &RTP = NTypes.back();
      const ValType &RTRHS =
          Instr.getOpCode() == OpCode::Br_on_cast ? RT2 : RT1P;
      if (unlikely(!AST::TypeMatcher::matchType(Types, RTP, RTRHS))) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(ErrInfo::InfoMismatch(RTP, RTRHS));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      // Pop types [t* rt1].
      RTP = RT1;
      if (auto Res = popTypes(NTypes); !Res) {
        return Unexpect(Res);
      }
      recordJump(const_cast<AST::Instruction &>(Instr).getBrCast().Jump,
                 static_cast<uint32_t>(NTypes.size()), *D);
      // For br_on_cast, push types [t* rt1'].
      // For Br_on_cast_fail, push types [t* rt2].
      RTP = Instr.getOpCode() == OpCode::Br_on_cast ? RT1P : RT2;
      pushTypes(NTypes);
      return {};
    } else {
      return Unexpect(D);
    }
  }
  case OpCode::Any__convert_extern:
    if (auto Res = popType(TypeCode::ExternRef)) {
      return StackTrans({}, {ValType((*Res)->getCode(), TypeCode::AnyRef)});
    } else {
      return Unexpect(Res);
    }
  case OpCode::Extern__convert_any:
    if (auto Res = popType(TypeCode::AnyRef)) {
      return StackTrans({}, {ValType((*Res)->getCode(), TypeCode::ExternRef)});
    } else {
      return Unexpect(Res);
    }
  case OpCode::Ref__i31:
    return StackTrans({ValType(TypeCode::I32)},
                      {ValType(TypeCode::Ref, TypeCode::I31Ref)});
  case OpCode::I31__get_s:
  case OpCode::I31__get_u:
    return StackTrans({ValType(TypeCode::RefNull, TypeCode::I31Ref)},
                      {ValType(TypeCode::I32)});

  // Parametric Instructions.
  case OpCode::Drop:
    return StackPopAny();
  case OpCode::Select: {
    // Pop I32.
    if (auto Res = popType(TypeCode::I32); !Res) {
      return Unexpect(Res);
    }
    // Pop T1 and T2.
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
    // T1 and T2 should be number type.
    if (T1.has_value() && !T1->isNumType()) {
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(TypeCode::I32, VTypeToAST(T1)));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
    if (T2.has_value() && !T2->isNumType()) {
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(VTypeToAST(T1), VTypeToAST(T2)));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
    // Error if t1 != t2 && t1 =/= Unknown && t2 =/= Unknown
    if (T1 != T2 && T1 != unreachableVType() && T2 != unreachableVType()) {
      spdlog::error(ErrCode::Value::TypeCheckFailed);
      spdlog::error(ErrInfo::InfoMismatch(VTypeToAST(T1), VTypeToAST(T2)));
      return Unexpect(ErrCode::Value::TypeCheckFailed);
    }
    // Push value.
    if (T1 == unreachableVType()) {
      pushType(T2);
    } else {
      pushType(T1);
    }
    return {};
  }
  case OpCode::Select_t: {
    // Note: There may be multiple values choice in the future.
    if (Instr.getValTypeList().size() != 1) {
      spdlog::error(ErrCode::Value::InvalidResultArity);
      return Unexpect(ErrCode::Value::InvalidResultArity);
    }
    ValType ExpT = Instr.getValTypeList()[0];
    if (auto Res = validate(ExpT); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = popTypes({ExpT, ExpT, ValType(TypeCode::I32)}); !Res) {
      return Unexpect(Res);
    }
    pushType(ExpT);
    return {};
  }

  // Variable Instructions.
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee: {
    if (Instr.getTargetIndex() >= Locals.size()) {
      return logOutOfRange(
          ErrCode::Value::InvalidLocalIdx, ErrInfo::IndexCategory::Local,
          Instr.getTargetIndex(), static_cast<uint32_t>(Locals.size()));
    }
    auto &TExpect = Locals[Instr.getTargetIndex()];
    const_cast<AST::Instruction &>(Instr).getStackOffset() =
        static_cast<uint32_t>(ValStack.size() +
                              (Locals.size() - Instr.getTargetIndex()));
    if (Instr.getOpCode() == OpCode::Local__get) {
      if (!TExpect.IsInit) {
        spdlog::error(ErrCode::Value::InvalidUninitLocal);
        return Unexpect(ErrCode::Value::InvalidUninitLocal);
      }
      return StackTrans({}, {TExpect.VType});
    } else if (Instr.getOpCode() == OpCode::Local__set) {
      if (!TExpect.IsInit) {
        TExpect.IsInit = true;
        LocalInits.push_back(Instr.getTargetIndex());
      }
      return StackTrans({TExpect.VType}, {});
    } else if (Instr.getOpCode() == OpCode::Local__tee) {
      if (!TExpect.IsInit) {
        TExpect.IsInit = true;
        LocalInits.push_back(Instr.getTargetIndex());
      }
      return StackTrans({TExpect.VType}, {TExpect.VType});
    } else {
      assumingUnreachable();
    }
  }
  case OpCode::Global__set:
    // Global case, check mutation.
    if (Instr.getTargetIndex() < Globals.size() &&
        Globals[Instr.getTargetIndex()].second != ValMut::Var) {
      // Global is immutable
      spdlog::error(ErrCode::Value::ImmutableGlobal);
      return Unexpect(ErrCode::Value::ImmutableGlobal);
    }
    [[fallthrough]];
  case OpCode::Global__get: {
    if (Instr.getTargetIndex() >= Globals.size()) {
      return logOutOfRange(
          ErrCode::Value::InvalidGlobalIdx, ErrInfo::IndexCategory::Global,
          Instr.getTargetIndex(), static_cast<uint32_t>(Globals.size()));
    }
    ValType ExpT = Globals[Instr.getTargetIndex()].first;
    if (Instr.getOpCode() == OpCode::Global__set) {
      return StackTrans({ExpT}, {});
    } else {
      return StackTrans({}, {ExpT});
    }
  }

  // Table Instructions.
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
  case OpCode::Table__init:
  case OpCode::Table__copy: {
    // Check target table index to perform.
    if (Instr.getTargetIndex() >= Tables.size()) {
      return logOutOfRange(
          ErrCode::Value::InvalidTableIdx, ErrInfo::IndexCategory::Table,
          Instr.getTargetIndex(), static_cast<uint32_t>(Tables.size()));
    }
    ValType ExpT = Tables[Instr.getTargetIndex()];
    if (Instr.getOpCode() == OpCode::Table__get) {
      return StackTrans({ValType(TypeCode::I32)}, {ExpT});
    } else if (Instr.getOpCode() == OpCode::Table__set) {
      return StackTrans({ValType(TypeCode::I32), ExpT}, {});
    } else if (Instr.getOpCode() == OpCode::Table__grow) {
      return StackTrans({ExpT, ValType(TypeCode::I32)},
                        {ValType(TypeCode::I32)});
    } else if (Instr.getOpCode() == OpCode::Table__size) {
      return StackTrans({}, {ValType(TypeCode::I32)});
    } else if (Instr.getOpCode() == OpCode::Table__fill) {
      return StackTrans({ValType(TypeCode::I32), ExpT, ValType(TypeCode::I32)},
                        {});
    } else if (Instr.getOpCode() == OpCode::Table__init) {
      // Check source element index for initialization.
      if (Instr.getSourceIndex() >= Elems.size()) {
        return logOutOfRange(
            ErrCode::Value::InvalidElemIdx, ErrInfo::IndexCategory::Element,
            Instr.getSourceIndex(), static_cast<uint32_t>(Elems.size()));
      }
      // Check is the reference types matched.
      if (!AST::TypeMatcher::matchType(Types, Tables[Instr.getTargetIndex()],
                                       Elems[Instr.getSourceIndex()])) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(ErrInfo::InfoMismatch(Tables[Instr.getTargetIndex()],
                                            Elems[Instr.getSourceIndex()]));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      return StackTrans({ValType(TypeCode::I32), ValType(TypeCode::I32),
                         ValType(TypeCode::I32)},
                        {});
    } else if (Instr.getOpCode() == OpCode::Table__copy) {
      // Check source table index for copying.
      if (Instr.getSourceIndex() >= Tables.size()) {
        return logOutOfRange(
            ErrCode::Value::InvalidTableIdx, ErrInfo::IndexCategory::Table,
            Instr.getSourceIndex(), static_cast<uint32_t>(Tables.size()));
      }
      // Check is the reference types matched.
      if (!AST::TypeMatcher::matchType(Types, Tables[Instr.getTargetIndex()],
                                       Tables[Instr.getSourceIndex()])) {
        spdlog::error(ErrCode::Value::TypeCheckFailed);
        spdlog::error(ErrInfo::InfoMismatch(Tables[Instr.getTargetIndex()],
                                            Tables[Instr.getSourceIndex()]));
        return Unexpect(ErrCode::Value::TypeCheckFailed);
      }
      return StackTrans({ValType(TypeCode::I32), ValType(TypeCode::I32),
                         ValType(TypeCode::I32)},
                        {});
    } else {
      assumingUnreachable();
    }
  }
  case OpCode::Elem__drop:
    // Check target element index to drop.
    if (Instr.getTargetIndex() >= Elems.size()) {
      return logOutOfRange(
          ErrCode::Value::InvalidElemIdx, ErrInfo::IndexCategory::Element,
          Instr.getTargetIndex(), static_cast<uint32_t>(Elems.size()));
    }
    return {};

  // Memory Instructions.
  case OpCode::I32__load:
    return checkAlignAndTrans(32, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::I32)});
  case OpCode::I64__load:
    return checkAlignAndTrans(64, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::I64)});
  case OpCode::F32__load:
    return checkAlignAndTrans(32, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::F32)});
  case OpCode::F64__load:
    return checkAlignAndTrans(64, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::F64)});
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
    return checkAlignAndTrans(8, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::I32)});
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
    return checkAlignAndTrans(16, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::I32)});
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
    return checkAlignAndTrans(8, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::I64)});
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
    return checkAlignAndTrans(16, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::I64)});
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
    return checkAlignAndTrans(32, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::I64)});
  case OpCode::I32__store:
    return checkAlignAndTrans(
        32, {ValType(TypeCode::I32), ValType(TypeCode::I32)}, {});
  case OpCode::I64__store:
    return checkAlignAndTrans(
        64, {ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::F32__store:
    return checkAlignAndTrans(
        32, {ValType(TypeCode::I32), ValType(TypeCode::F32)}, {});
  case OpCode::F64__store:
    return checkAlignAndTrans(
        64, {ValType(TypeCode::I32), ValType(TypeCode::F64)}, {});
  case OpCode::I32__store8:
    return checkAlignAndTrans(
        8, {ValType(TypeCode::I32), ValType(TypeCode::I32)}, {});
  case OpCode::I32__store16:
    return checkAlignAndTrans(
        16, {ValType(TypeCode::I32), ValType(TypeCode::I32)}, {});
  case OpCode::I64__store8:
    return checkAlignAndTrans(
        8, {ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::I64__store16:
    return checkAlignAndTrans(
        16, {ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::I64__store32:
    return checkAlignAndTrans(
        32, {ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::Memory__size:
    return checkMemAndTrans({}, {ValType(TypeCode::I32)});
  case OpCode::Memory__grow:
    return checkMemAndTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::I32)});
  case OpCode::Memory__init:
    // Check the target memory index. Memory index should be checked first.
    if (Instr.getTargetIndex() >= Mems) {
      return logOutOfRange(ErrCode::Value::InvalidMemoryIdx,
                           ErrInfo::IndexCategory::Memory,
                           Instr.getTargetIndex(), Mems);
    }
    // Check the source data index.
    if (Instr.getSourceIndex() >= Datas.size()) {
      return logOutOfRange(ErrCode::Value::InvalidDataIdx,
                           ErrInfo::IndexCategory::Data, Instr.getSourceIndex(),
                           static_cast<uint32_t>(Datas.size()));
    }
    return StackTrans({ValType(TypeCode::I32), ValType(TypeCode::I32),
                       ValType(TypeCode::I32)},
                      {});
  case OpCode::Memory__copy:
    /// Check the source memory index.
    if (Instr.getSourceIndex() >= Mems) {
      return logOutOfRange(ErrCode::Value::InvalidMemoryIdx,
                           ErrInfo::IndexCategory::Memory,
                           Instr.getSourceIndex(), Mems);
    }
    [[fallthrough]];
  case OpCode::Memory__fill:
    return checkMemAndTrans({ValType(TypeCode::I32), ValType(TypeCode::I32),
                             ValType(TypeCode::I32)},
                            {});
  case OpCode::Data__drop:
    // Check the target data index.
    if (Instr.getTargetIndex() >= Datas.size()) {
      return logOutOfRange(ErrCode::Value::InvalidDataIdx,
                           ErrInfo::IndexCategory::Data, Instr.getTargetIndex(),
                           static_cast<uint32_t>(Datas.size()));
    }
    return {};

  // Const Instructions.
  case OpCode::I32__const:
    return StackTrans({}, {ValType(TypeCode::I32)});
  case OpCode::I64__const:
    return StackTrans({}, {ValType(TypeCode::I64)});
  case OpCode::F32__const:
    return StackTrans({}, {ValType(TypeCode::F32)});
  case OpCode::F64__const:
    return StackTrans({}, {ValType(TypeCode::F64)});

  // Unary Numeric Instructions.
  case OpCode::I32__eqz:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::I32)});
  case OpCode::I64__eqz:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::I32)});
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::I32)});
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::I64)});
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::F32)});
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::F64)});
  case OpCode::I32__wrap_i64:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::I32)});
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::I32)});
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::I32)});
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::I64)});
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::I64)});
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::I64)});
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::F32)});
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::F32)});
  case OpCode::F32__demote_f64:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::F32)});
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::F64)});
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::F64)});
  case OpCode::F64__promote_f32:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::F64)});
  case OpCode::I32__reinterpret_f32:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::I32)});
  case OpCode::I64__reinterpret_f64:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::I64)});
  case OpCode::F32__reinterpret_i32:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::F32)});
  case OpCode::F64__reinterpret_i64:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::F64)});
  case OpCode::I32__extend8_s:
  case OpCode::I32__extend16_s:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::I32)});
  case OpCode::I64__extend8_s:
  case OpCode::I64__extend16_s:
  case OpCode::I64__extend32_s:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::I64)});
  case OpCode::I32__trunc_sat_f32_s:
  case OpCode::I32__trunc_sat_f32_u:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::I32)});
  case OpCode::I32__trunc_sat_f64_s:
  case OpCode::I32__trunc_sat_f64_u:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::I32)});
  case OpCode::I64__trunc_sat_f32_s:
  case OpCode::I64__trunc_sat_f32_u:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::I64)});
  case OpCode::I64__trunc_sat_f64_s:
  case OpCode::I64__trunc_sat_f64_u:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::I64)});

  // Binary Numeric Instructions.
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
    return StackTrans({ValType(TypeCode::I32), ValType(TypeCode::I32)},
                      {ValType(TypeCode::I32)});
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
    return StackTrans({ValType(TypeCode::I64), ValType(TypeCode::I64)},
                      {ValType(TypeCode::I32)});
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
  case OpCode::F32__ge:
    return StackTrans({ValType(TypeCode::F32), ValType(TypeCode::F32)},
                      {ValType(TypeCode::I32)});
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:
    return StackTrans({ValType(TypeCode::F64), ValType(TypeCode::F64)},
                      {ValType(TypeCode::I32)});
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
    return StackTrans({ValType(TypeCode::I32), ValType(TypeCode::I32)},
                      {ValType(TypeCode::I32)});
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
    return StackTrans({ValType(TypeCode::I64), ValType(TypeCode::I64)},
                      {ValType(TypeCode::I64)});
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
    return StackTrans({ValType(TypeCode::F32), ValType(TypeCode::F32)},
                      {ValType(TypeCode::F32)});
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    return StackTrans({ValType(TypeCode::F64), ValType(TypeCode::F64)},
                      {ValType(TypeCode::F64)});

  // SIMD Memory Instruction.
  case OpCode::V128__load:
    return checkAlignAndTrans(128, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::V128)});
  case OpCode::V128__load8x8_s:
  case OpCode::V128__load8x8_u:
  case OpCode::V128__load16x4_s:
  case OpCode::V128__load16x4_u:
  case OpCode::V128__load32x2_s:
  case OpCode::V128__load32x2_u:
  case OpCode::V128__load64_splat:
  case OpCode::V128__load64_zero:
    return checkAlignAndTrans(64, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::V128)});
  case OpCode::V128__load8_splat:
    return checkAlignAndTrans(8, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::V128)});
  case OpCode::V128__load16_splat:
    return checkAlignAndTrans(16, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::V128)});
  case OpCode::V128__load32_splat:
  case OpCode::V128__load32_zero:
    return checkAlignAndTrans(32, {ValType(TypeCode::I32)},
                              {ValType(TypeCode::V128)});
  case OpCode::V128__store:
    return checkAlignAndTrans(
        128, {ValType(TypeCode::I32), ValType(TypeCode::V128)}, {});
  case OpCode::V128__load8_lane:
    return checkAlignAndTrans(8,
                              {ValType(TypeCode::I32), ValType(TypeCode::V128)},
                              {ValType(TypeCode::V128)}, true);
  case OpCode::V128__load16_lane:
    return checkAlignAndTrans(16,
                              {ValType(TypeCode::I32), ValType(TypeCode::V128)},
                              {ValType(TypeCode::V128)}, true);
  case OpCode::V128__load32_lane:
    return checkAlignAndTrans(32,
                              {ValType(TypeCode::I32), ValType(TypeCode::V128)},
                              {ValType(TypeCode::V128)}, true);
  case OpCode::V128__load64_lane:
    return checkAlignAndTrans(64,
                              {ValType(TypeCode::I32), ValType(TypeCode::V128)},
                              {ValType(TypeCode::V128)}, true);
  case OpCode::V128__store8_lane:
    return checkAlignAndTrans(
        8, {ValType(TypeCode::I32), ValType(TypeCode::V128)}, {}, true);
  case OpCode::V128__store16_lane:
    return checkAlignAndTrans(
        16, {ValType(TypeCode::I32), ValType(TypeCode::V128)}, {}, true);
  case OpCode::V128__store32_lane:
    return checkAlignAndTrans(
        32, {ValType(TypeCode::I32), ValType(TypeCode::V128)}, {}, true);
  case OpCode::V128__store64_lane:
    return checkAlignAndTrans(
        64, {ValType(TypeCode::I32), ValType(TypeCode::V128)}, {}, true);

  // SIMD Const Instruction.
  case OpCode::V128__const:
    return StackTrans({}, {ValType(TypeCode::V128)});

  // SIMD Shuffle Instruction.
  case OpCode::I8x16__shuffle: {
    // Check all lane index < 32 by masking
    const uint128_t Mask = (uint128_t(0xe0e0e0e0e0e0e0e0U) << 64U) |
                           uint128_t(0xe0e0e0e0e0e0e0e0U);
    const uint128_t Result = Instr.getNum().get<uint128_t>() & Mask;
    if (Result) {
      spdlog::error(ErrCode::Value::InvalidLaneIdx);
      return Unexpect(ErrCode::Value::InvalidLaneIdx);
    }
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::V128)},
                      {ValType(TypeCode::V128)});
  }

  // SIMD Lane Instructions.
  case OpCode::I8x16__extract_lane_s:
  case OpCode::I8x16__extract_lane_u:
    return checkLaneAndTrans(16, {ValType(TypeCode::V128)},
                             {ValType(TypeCode::I32)});
  case OpCode::I8x16__replace_lane:
    return checkLaneAndTrans(16,
                             {ValType(TypeCode::V128), ValType(TypeCode::I32)},
                             {ValType(TypeCode::V128)});
  case OpCode::I16x8__extract_lane_s:
  case OpCode::I16x8__extract_lane_u:
    return checkLaneAndTrans(8, {ValType(TypeCode::V128)},
                             {ValType(TypeCode::I32)});
  case OpCode::I16x8__replace_lane:
    return checkLaneAndTrans(8,
                             {ValType(TypeCode::V128), ValType(TypeCode::I32)},
                             {ValType(TypeCode::V128)});
  case OpCode::I32x4__extract_lane:
    return checkLaneAndTrans(4, {ValType(TypeCode::V128)},
                             {ValType(TypeCode::I32)});
  case OpCode::I32x4__replace_lane:
    return checkLaneAndTrans(4,
                             {ValType(TypeCode::V128), ValType(TypeCode::I32)},
                             {ValType(TypeCode::V128)});
  case OpCode::I64x2__extract_lane:
    return checkLaneAndTrans(2, {ValType(TypeCode::V128)},
                             {ValType(TypeCode::I64)});
  case OpCode::I64x2__replace_lane:
    return checkLaneAndTrans(2,
                             {ValType(TypeCode::V128), ValType(TypeCode::I64)},
                             {ValType(TypeCode::V128)});
  case OpCode::F32x4__extract_lane:
    return checkLaneAndTrans(4, {ValType(TypeCode::V128)},
                             {ValType(TypeCode::F32)});
  case OpCode::F32x4__replace_lane:
    return checkLaneAndTrans(4,
                             {ValType(TypeCode::V128), ValType(TypeCode::F32)},
                             {ValType(TypeCode::V128)});
  case OpCode::F64x2__extract_lane:
    return checkLaneAndTrans(2, {ValType(TypeCode::V128)},
                             {ValType(TypeCode::F64)});
  case OpCode::F64x2__replace_lane:
    return checkLaneAndTrans(2,
                             {ValType(TypeCode::V128), ValType(TypeCode::F64)},
                             {ValType(TypeCode::V128)});

  // SIMD Numeric Instructions.
  case OpCode::I8x16__splat:
  case OpCode::I16x8__splat:
  case OpCode::I32x4__splat:
    return StackTrans({ValType(TypeCode::I32)}, {ValType(TypeCode::V128)});
  case OpCode::I64x2__splat:
    return StackTrans({ValType(TypeCode::I64)}, {ValType(TypeCode::V128)});
  case OpCode::F32x4__splat:
    return StackTrans({ValType(TypeCode::F32)}, {ValType(TypeCode::V128)});
  case OpCode::F64x2__splat:
    return StackTrans({ValType(TypeCode::F64)}, {ValType(TypeCode::V128)});
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
    return StackTrans({ValType(TypeCode::V128)}, {ValType(TypeCode::V128)});
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
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::V128)},
                      {ValType(TypeCode::V128)});
  case OpCode::V128__bitselect:
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::V128),
                       ValType(TypeCode::V128)},
                      {ValType(TypeCode::V128)});
  case OpCode::V128__any_true:
  case OpCode::I8x16__all_true:
  case OpCode::I8x16__bitmask:
  case OpCode::I16x8__all_true:
  case OpCode::I16x8__bitmask:
  case OpCode::I32x4__all_true:
  case OpCode::I32x4__bitmask:
  case OpCode::I64x2__all_true:
  case OpCode::I64x2__bitmask:
    return StackTrans({ValType(TypeCode::V128)}, {ValType(TypeCode::I32)});
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
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::I32)},
                      {ValType(TypeCode::V128)});

  case OpCode::I8x16__relaxed_swizzle:
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::V128)},
                      {ValType(TypeCode::V128)});
  case OpCode::I32x4__relaxed_trunc_f32x4_s:
  case OpCode::I32x4__relaxed_trunc_f32x4_u:
  case OpCode::I32x4__relaxed_trunc_f64x2_s_zero:
  case OpCode::I32x4__relaxed_trunc_f64x2_u_zero:
    return StackTrans({ValType(TypeCode::V128)}, {ValType(TypeCode::V128)});
  case OpCode::F32x4__relaxed_madd:
  case OpCode::F32x4__relaxed_nmadd:
  case OpCode::F64x2__relaxed_madd:
  case OpCode::F64x2__relaxed_nmadd:
  case OpCode::I8x16__relaxed_laneselect:
  case OpCode::I16x8__relaxed_laneselect:
  case OpCode::I32x4__relaxed_laneselect:
  case OpCode::I64x2__relaxed_laneselect:
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::V128),
                       ValType(TypeCode::V128)},
                      {ValType(TypeCode::V128)});
  case OpCode::F32x4__relaxed_min:
  case OpCode::F32x4__relaxed_max:
  case OpCode::F64x2__relaxed_min:
  case OpCode::F64x2__relaxed_max:
  case OpCode::I16x8__relaxed_q15mulr_s:
  case OpCode::I16x8__relaxed_dot_i8x16_i7x16_s:
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::V128)},
                      {ValType(TypeCode::V128)});
  case OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s:
    return StackTrans({ValType(TypeCode::V128), ValType(TypeCode::V128),
                       ValType(TypeCode::V128)},
                      {ValType(TypeCode::V128)});

  case OpCode::Atomic__fence:
    return {};

  case OpCode::Memory__atomic__notify:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::Memory__atomic__wait32:
    return checkAlignAndTrans(32,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I32),
                                         ValType(TypeCode::I64)},
                              std::array{ValType(TypeCode::I32)});
  case OpCode::Memory__atomic__wait64:
    return checkAlignAndTrans(64,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I64),
                                         ValType(TypeCode::I64)},
                              std::array{ValType(TypeCode::I32)});

  case OpCode::I32__atomic__load:
    return checkAlignAndTrans(32, std::array{ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__load:
    return checkAlignAndTrans(64, std::array{ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__load8_u:
    return checkAlignAndTrans(8, std::array{ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__load16_u:
    return checkAlignAndTrans(16, std::array{ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__load8_u:
    return checkAlignAndTrans(8, std::array{ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__load16_u:
    return checkAlignAndTrans(16, std::array{ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__load32_u:
    return checkAlignAndTrans(32, std::array{ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__store:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)}, {});
  case OpCode::I64__atomic__store:
    return checkAlignAndTrans(
        64, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::I32__atomic__store8:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)}, {});
  case OpCode::I32__atomic__store16:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)}, {});
  case OpCode::I64__atomic__store8:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::I64__atomic__store16:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::I64__atomic__store32:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)}, {});
  case OpCode::I32__atomic__rmw__add:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw__add:
    return checkAlignAndTrans(
        64, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw8__add_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__rmw16__add_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw8__add_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw16__add_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw32__add_u:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw__sub:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw__sub:
    return checkAlignAndTrans(
        64, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw8__sub_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__rmw16__sub_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw8__sub_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw16__sub_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw32__sub_u:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw__and:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw__and:
    return checkAlignAndTrans(
        64, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw8__and_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__rmw16__and_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw8__and_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw16__and_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw32__and_u:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw__or:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw__or:
    return checkAlignAndTrans(
        64, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw8__or_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__rmw16__or_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw8__or_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw16__or_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw32__or_u:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw__xor:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw__xor:
    return checkAlignAndTrans(
        64, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw8__xor_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__rmw16__xor_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw8__xor_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw16__xor_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw32__xor_u:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw__xchg:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw__xchg:
    return checkAlignAndTrans(
        64, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw8__xchg_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__rmw16__xchg_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I32)},
        std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw8__xchg_u:
    return checkAlignAndTrans(
        8, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw16__xchg_u:
    return checkAlignAndTrans(
        16, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw32__xchg_u:
    return checkAlignAndTrans(
        32, std::array{ValType(TypeCode::I32), ValType(TypeCode::I64)},
        std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw__cmpxchg:
    return checkAlignAndTrans(32,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I32),
                                         ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw__cmpxchg:
    return checkAlignAndTrans(64,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I64),
                                         ValType(TypeCode::I64)},
                              std::array{ValType(TypeCode::I64)});
  case OpCode::I32__atomic__rmw8__cmpxchg_u:
    return checkAlignAndTrans(8,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I32),
                                         ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I32)});
  case OpCode::I32__atomic__rmw16__cmpxchg_u:
    return checkAlignAndTrans(16,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I32),
                                         ValType(TypeCode::I32)},
                              std::array{ValType(TypeCode::I32)});
  case OpCode::I64__atomic__rmw8__cmpxchg_u:
    return checkAlignAndTrans(8,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I64),
                                         ValType(TypeCode::I64)},
                              std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw16__cmpxchg_u:
    return checkAlignAndTrans(16,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I64),
                                         ValType(TypeCode::I64)},
                              std::array{ValType(TypeCode::I64)});
  case OpCode::I64__atomic__rmw32__cmpxchg_u:
    return checkAlignAndTrans(32,
                              std::array{ValType(TypeCode::I32),
                                         ValType(TypeCode::I64),
                                         ValType(TypeCode::I64)},
                              std::array{ValType(TypeCode::I64)});

  default:
    assumingUnreachable();
  }
}

void FormChecker::pushType(VType V) { ValStack.emplace_back(V); }

void FormChecker::pushTypes(Span<const VType> Input) {
  for (auto Val : Input) {
    pushType(Val);
  }
}

void FormChecker::pushTypes(Span<const ValType> Input) {
  for (auto Val : Input) {
    pushType(Val);
  }
}

Expect<VType> FormChecker::popType() {
  if (ValStack.size() == CtrlStack.back().Height) {
    if (CtrlStack.back().IsUnreachable) {
      return unreachableVType();
    }
    // Value stack underflow
    spdlog::error(ErrCode::Value::TypeCheckFailed);
    spdlog::error("    Value stack underflow.");
    return Unexpect(ErrCode::Value::TypeCheckFailed);
  }
  auto Res = ValStack.back();
  ValStack.pop_back();
  return Res;
}

Expect<VType> FormChecker::popType(ValType E) {
  auto Res = popType();
  if (!Res) {
    return Unexpect(Res);
  }
  if (*Res == unreachableVType()) {
    return E;
  }

  if (!AST::TypeMatcher::matchType(Types, E, **Res)) {
    // Expect value on value stack is not matched
    spdlog::error(ErrCode::Value::TypeCheckFailed);
    spdlog::error(ErrInfo::InfoMismatch(VTypeToAST(E), VTypeToAST(*Res)));
    return Unexpect(ErrCode::Value::TypeCheckFailed);
  }
  return *Res;
}

Expect<void> FormChecker::popTypes(Span<const ValType> Input) {
  for (auto Val = Input.rbegin(); Val != Input.rend(); ++Val) {
    if (auto Res = popType(*Val); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

void FormChecker::pushCtrl(Span<const ValType> In, Span<const ValType> Out,
                           const AST::Instruction *Jump, OpCode Code) {
  CtrlStack.emplace_back(In, Out, Jump, ValStack.size(), LocalInits.size(),
                         Code);
  pushTypes(In);
}

Expect<FormChecker::CtrlFrame> FormChecker::popCtrl() {
  if (CtrlStack.empty()) {
    // Ctrl stack is empty when popping.
    spdlog::error(ErrCode::Value::TypeCheckFailed);
    spdlog::error("    Control stack underflow.");
    return Unexpect(ErrCode::Value::TypeCheckFailed);
  }
  if (auto Res = popTypes(CtrlStack.back().EndTypes); !Res) {
    return Unexpect(Res);
  }
  if (ValStack.size() != CtrlStack.back().Height) {
    // Value stack size not matched.
    spdlog::error(ErrCode::Value::TypeCheckFailed);
    spdlog::error("    Value stack underflow.");
    return Unexpect(ErrCode::Value::TypeCheckFailed);
  }
  // When popping a frame, reset the inited locals during this frame.
  for (size_t I = CtrlStack.back().InitedLocal; I < LocalInits.size(); I++) {
    Locals[LocalInits[I]].IsInit = false;
  }
  LocalInits.erase(LocalInits.begin() +
                       static_cast<uint32_t>(CtrlStack.back().InitedLocal),
                   LocalInits.end());
  auto Head = std::move(CtrlStack.back());
  CtrlStack.pop_back();
  return Head;
}

Span<const ValType>
FormChecker::getLabelTypes(const FormChecker::CtrlFrame &F) {
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

Expect<void> FormChecker::StackTrans(Span<const ValType> Take,
                                     Span<const ValType> Put) {
  if (auto Res = popTypes(Take); !Res) {
    return Unexpect(Res);
  }
  pushTypes(Put);
  return {};
}

Expect<void> FormChecker::StackPopAny() {
  if (auto Res = popType(); !Res) {
    return Unexpect(Res);
  }
  return {};
}

} // namespace Validator
} // namespace WasmEdge
