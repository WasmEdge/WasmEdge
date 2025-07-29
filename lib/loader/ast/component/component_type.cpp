// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

using namespace std::literals;

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadType(AST::Component::CoreDefType &Ty) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
    return E;
  };
  /// FROM:
  /// https://github.com/WebAssembly/component-model/blob/main/design/mvp/Binary.md#type-definitions
  ///
  /// Unfortunately, the `core:deftype` rule results in an encoding ambiguity:
  /// the `0x50` opcode is used by both `core:moduletype` and a non-final
  /// `core:subtype`, which can be decoded as a top-level form of
  /// `core:rectype`.
  ///
  /// To resolve this, prior to v1.0 of this specification, we require
  /// `core:subtype` to be prefixed by `0x00` in this context (i.e., a non-final
  /// sub as a component core type is `0x00 0x50`; elsewhere, `0x50`). By the
  /// v1.0 release of this specification, `core:moduletype` will receive a new,
  /// non-overlapping opcode.

  // core:type       ::= dt:<core:deftype> => (type dt)
  // core:deftype    ::= rt:<core:rectype>
  //                   => rt (WebAssembly 3.0)
  //                   | 0x00 0x50 x*:vec(<core:typeidx>) ct:<core:comptype>
  //                   => sub x* ct (WebAssembly 3.0)
  //                   | mt:<core:moduletype>
  //                   => mt
  // core:moduletype ::= 0x50 md*:vec(<core:moduledecl>) => (module md*)

  // Peek the first byte to determine the type to load.
  EXPECTED_TRY(uint8_t Flag, FMgr.peekByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_CoreDefType);
  }));
  switch (Flag) {
  case 0x50: {
    FMgr.readByte();
    std::vector<AST::Component::CoreModuleDecl> Decls;
    EXPECTED_TRY(loadVec<AST::Component::CoreDefType>(
        Decls, [this](AST::Component::CoreModuleDecl &Decl) {
          return loadDecl(Decl);
        }));
    Ty.setModuleType(std::move(Decls));
    return {};
  }
  case 0x00: // sub non-final case
    FMgr.readByte();
    [[fallthrough]];
  default: { // recursive type case
    std::vector<AST::SubType> STypes;
    if (static_cast<TypeCode>(Flag) == TypeCode::Rec) {
      // Case: 0x4E vec(subtype).
      FMgr.readByte();
      EXPECTED_TRY(uint32_t RecVecCnt, loadVecCnt().map_error([this](auto E) {
        return logLoadError(E, FMgr.getLastOffset(),
                            ASTNodeAttr::Comp_CoreDefType);
      }));
      for (uint32_t I = 0; I < RecVecCnt; ++I) {
        STypes.emplace_back();
        EXPECTED_TRY(loadType(STypes.back()).map_error(ReportError));
        STypes.back().setRecursiveInfo(I, RecVecCnt);
        STypes.back().setTypeIndex(I);
      }
    } else {
      // Case: subtype.
      STypes.emplace_back();
      STypes.back().setTypeIndex(0);
      EXPECTED_TRY(loadType(STypes.back()).map_error(ReportError));
    }
    return {};
  }
  }
}

Expect<void> Loader::loadType(AST::Component::DefType &Ty) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
    return E;
  };
  // type    ::= dt:<deftype>       => (type dt)
  // deftype ::= dvt:<defvaltype>   => dvt
  //           | ft:<functype>      => ft
  //           | ct:<componenttype> => ct
  //           | it:<instancetype>  => it
  //           | rt:<resourcetype>  => rt

  // Read the first byte to determine the type to load.
  EXPECTED_TRY(auto Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_DefType);
  }));
  switch (Flag) {
  case 0x40: {
    AST::Component::FuncType FT;
    EXPECTED_TRY(loadType(FT).map_error(ReportError));
    Ty.setFuncType(std::move(FT));
    return {};
  }
  case 0x41: {
    AST::Component::ComponentType CT;
    EXPECTED_TRY(loadType(CT).map_error(ReportError));
    Ty.setComponentType(std::move(CT));
    return {};
  }
  case 0x42: {
    AST::Component::InstanceType IT;
    EXPECTED_TRY(loadType(IT).map_error(ReportError));
    Ty.setInstanceType(std::move(IT));
    return {};
  }
  case 0x3F:
  case 0x3E: {
    AST::Component::ResourceType RT(Flag == 0x3F);
    EXPECTED_TRY(loadType(RT).map_error(ReportError));
    Ty.setResourceType(std::move(RT));
    return {};
  }
  default: {
    AST::Component::DefValType DVT;
    EXPECTED_TRY(loadType(DVT, Flag).map_error(ReportError));
    Ty.setDefValType(std::move(DVT));
    return {};
  }
  }
}

Expect<void> Loader::loadType(AST::Component::DefValType &Ty, uint8_t Code) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
    return E;
  };

  // defvaltype ::= pvt:<primvaltype>          => pvt
  //              | 0x72 lt*:vec(<labelvaltype>)
  //                => (record (field lt)*) (if |lt*| > 0)
  //              | 0x71 case*:vec(<case>) => (variant case+) (if |case*| > 0)/
  //              | 0x70 t:<valtype>           => (list t)
  //              | 0x67 t:<valtype> len:<u32> => (list t len) (if len > 0) 🔧
  //              | 0x6f t*:vec(<valtype>)     => (tuple t+) (if |t*| > 0)
  //              | 0x6e l*:vec(<label'>)      => (flags l+) (if 0 < |l*| <= 32)
  //              | 0x6d l*:vec(<label'>)      => (enum l+) (if |l*| > 0)
  //              | 0x6b t:<valtype>           => (option t)
  //              | 0x6a t?:<valtype>? u?:<valtype>? => (result t? (error u)?)
  //              | 0x69 i:<typeidx>           => (own i)
  //              | 0x68 i:<typeidx>           => (borrow i)
  //              | 0x66 t?:<valtype>?         => (stream t?) 🔀
  //              | 0x65 t?:<valtype>?         => (future t?) 🔀

  switch (Code) {
  case 0x7F:
  case 0x7E:
  case 0x7D:
  case 0x7C:
  case 0x7B:
  case 0x7A:
  case 0x79:
  case 0x78:
  case 0x77:
  case 0x76:
  case 0x75:
  case 0x74:
  case 0x73:
  case 0x64:
    Ty.setPrimValType(static_cast<AST::Component::PrimValType>(Code));
    return {};
  case 0x72: {
    AST::Component::RecordTy RTy;
    EXPECTED_TRY(loadType(RTy).map_error(ReportError));
    Ty.setRecord(std::move(RTy));
    return {};
  }
  case 0x71: {
    AST::Component::VariantTy VTy;
    EXPECTED_TRY(loadType(VTy).map_error(ReportError));
    Ty.setVariant(std::move(VTy));
    return {};
  }
  case 0x70:
  case 0x67: {
    AST::Component::ListTy LTy;
    EXPECTED_TRY(loadType(LTy, Code == 0x67).map_error(ReportError));
    Ty.setList(std::move(LTy));
    return {};
  }
  case 0x6F: {
    AST::Component::TupleTy TTy;
    EXPECTED_TRY(loadType(TTy).map_error(ReportError));
    Ty.setTuple(std::move(TTy));
    return {};
  }
  case 0x6E: {
    AST::Component::FlagsTy FTy;
    EXPECTED_TRY(loadType(FTy).map_error(ReportError));
    Ty.setFlags(std::move(FTy));
    return {};
  }
  case 0x6D: {
    AST::Component::EnumTy ETy;
    EXPECTED_TRY(loadType(ETy).map_error(ReportError));
    Ty.setEnum(std::move(ETy));
    return {};
  }
  case 0x6B: {
    AST::Component::OptionTy OTy;
    EXPECTED_TRY(loadType(OTy).map_error(ReportError));
    Ty.setOption(std::move(OTy));
    return {};
  }
  case 0x6A: {
    AST::Component::ResultTy RTy;
    EXPECTED_TRY(loadType(RTy).map_error(ReportError));
    Ty.setResult(std::move(RTy));
    return {};
  }
  case 0x69: {
    AST::Component::OwnTy OTy;
    EXPECTED_TRY(loadType(OTy).map_error(ReportError));
    Ty.setOwn(std::move(OTy));
    return {};
  }
  case 0x68: {
    AST::Component::BorrowTy BTy;
    EXPECTED_TRY(loadType(BTy).map_error(ReportError));
    Ty.setBorrow(std::move(BTy));
    return {};
  }
  case 0x66: {
    AST::Component::StreamTy STy;
    EXPECTED_TRY(loadType(STy).map_error(ReportError));
    Ty.setStream(std::move(STy));
    return {};
  }
  case 0x65: {
    AST::Component::FutureTy FTy;
    EXPECTED_TRY(loadType(FTy).map_error(ReportError));
    Ty.setFuture(std::move(FTy));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_DefValType);
  }
}

Expect<void> Loader::loadType(AST::Component::FuncType &Ty) {
  /// FROM:
  /// https://github.com/WebAssembly/component-model/blob/main/design/mvp/CanonicalABI.md#flattening
  ///
  /// The number of flattened results is currently limited to 1 due to various
  /// parts of the toolchain (notably the C ABI) not yet being able to express
  /// multi-value returns. Hopefully this limitation is temporary and can be
  /// lifted before the Component Model is fully standardized.
  ///
  /// NOTE:
  /// The original resultlist grammar:
  ///
  /// resultlist ::= 0x00 t:<valtype>             => (result t)
  ///              | 0x01 lt*:vec(<labelvaltype>) => (result lt)*

  // functype   ::= 0x40 ps:<paramlist> rs:<resultlist> => (func ps rs)
  // paramlist  ::= lt*:vec(<labelvaltype>)             => (param lt)*
  // resultlist ::= 0x00 t:<valtype> => (result t)
  //              | 0x01 0x00        => ϵ

  // The prefix `0x40` has been loaded in the parent scope.

  // Load the param list.
  std::vector<AST::Component::LabelValType> ParamList;
  EXPECTED_TRY(loadVec<AST::Component::FuncType>(
      ParamList,
      [this](AST::Component::LabelValType &LV) { return loadType(LV); }));
  Ty.setParamList(std::move(ParamList));

  // Load the result list.
  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_FuncType);
  }));
  switch (Flag) {
  case 0x00: {
    AST::Component::ValueType VT;
    EXPECTED_TRY(loadType(VT).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_FuncType));
      return E;
    }));
    Ty.setResultType(VT);
    return {};
  }
  case 0x01: {
    std::vector<AST::Component::LabelValType> ResultList;
    EXPECTED_TRY(loadVec<AST::Component::FuncType>(
        ResultList,
        [this](AST::Component::LabelValType &LV) { return loadType(LV); }));
    Ty.setResultList(std::move(ResultList));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_FuncType);
  }
}

Expect<void> Loader::loadType(AST::Component::ComponentType &Ty) {
  // componenttype ::= 0x41 cd*:vec(<componentdecl>) => (component cd*)

  // The prefix `0x41` has been loaded in the parent scope.
  std::vector<AST::Component::ComponentDecl> Decls;
  EXPECTED_TRY(loadVec<AST::Component::ComponentType>(
      Decls,
      [this](AST::Component::ComponentDecl &Decl) { return loadDecl(Decl); }));
  Ty.setDecl(std::move(Decls));
  return {};
}

Expect<void> Loader::loadType(AST::Component::InstanceType &Ty) {
  // instancetype ::= 0x42 id*:vec(<instancedecl>) => (instance id*)

  // The prefix `0x42` has been loaded in the parent scope.
  std::vector<AST::Component::InstanceDecl> Decls;
  EXPECTED_TRY(loadVec<AST::Component::InstanceType>(
      Decls,
      [this](AST::Component::InstanceDecl &Decl) { return loadDecl(Decl); }));
  Ty.setDecl(std::move(Decls));
  return {};
}

Expect<void> Loader::loadType(AST::Component::ResourceType &Ty) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ResourceType);
  };
  auto LoadIdx = [this](uint32_t &CBIdx) -> Expect<void> {
    EXPECTED_TRY(CBIdx, FMgr.readU32().map_error([this](auto E) {
      spdlog::error(E);
      spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
      return E;
    }));
    return {};
  };
  // resourcetype ::= 0x3f 0x7f f?:<funcidx>?
  //                => (resource (rep i32) (dtor f)?)
  //                | 0x3e 0x7f f:<funcidx> cb?:<funcidx>?
  //                => (resource (rep i32) (dtor async f (callback cb)?))

  // The prefix `0x3F` or `0x3E` has been loaded in the parent scope.
  EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
  if (B != 0x7f) {
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ResourceType);
  }
  if (Ty.IsSync()) {
    EXPECTED_TRY(Ty.getDestructor(),
                 loadOption<AST::Component::ResourceType, uint32_t>(LoadIdx));
  } else {
    EXPECTED_TRY(uint32_t FIdx, FMgr.readU32().map_error(ReportError));
    Ty.getDestructor().emplace(FIdx);
    EXPECTED_TRY(Ty.getCallback(),
                 loadOption<AST::Component::ResourceType, uint32_t>(LoadIdx));
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::RecordTy &Ty) {
  // record ::= lt*:vec(<labelvaltype>) => (record (field lt)*) (if |lt*| > 0)

  EXPECTED_TRY(loadVec<AST::Component::RecordTy>(
      Ty.LabelTypes,
      [this](AST::Component::LabelValType &LT) { return loadType(LT); }));
  if (Ty.LabelTypes.size() == 0) {
    return logLoadError(ErrCode::Value::MalformedRecordType,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Record);
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::VariantTy &Ty) {
  // variant ::= case*:vec(<case>) => (variant case+) (if |case*| > 0)
  // case    ::= l:<label'> t?:<valtype>? 0x00

  auto LoadCase =
      [this](std::pair<std::string, std::optional<AST::Component::ValueType>>
                 &Case) -> Expect<void> {
    EXPECTED_TRY(std::string Label, FMgr.readName().map_error([this](auto E) {
      spdlog::error(E);
      spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
      return E;
    }));
    EXPECTED_TRY(
        std::optional<AST::Component::ValueType> VT,
        loadOption<AST::Component::VariantTy, AST::Component::ValueType>(
            [this](AST::Component::ValueType &VTy) { return loadType(VTy); }));
    EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
      spdlog::error(E);
      spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
      return E;
    }));
    if (Flag != 0x00) {
      spdlog::error(ErrCode::Value::MalformedVariantType);
      spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
      return Unexpect(ErrCode::Value::MalformedVariantType);
    }
    Case = std::make_pair(Label, VT);
    return {};
  };
  return loadVec<AST::Component::VariantTy>(
      Ty.Cases,
      [LoadCase](
          std::pair<std::string, std::optional<AST::Component::ValueType>> &C) {
        return LoadCase(C);
      });
}

Expect<void> Loader::loadType(AST::Component::ListTy &Ty, bool IsFixedLen) {
  // list ::= t:<valtype>           => (list t)
  //        | t:<valtype> len:<u32> => (list t len) (if len > 0) 🔧

  EXPECTED_TRY(loadType(Ty.ValTy).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Type_List));
    return E;
  }));
  if (IsFixedLen) {
    EXPECTED_TRY(Ty.Len, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_List);
    }));
  } else {
    Ty.Len = 0;
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::TupleTy &Ty) {
  // tuple ::= t*:vec(<valtype>) => (tuple t+) (if |t*| > 0)

  EXPECTED_TRY(loadVec<AST::Component::TupleTy>(
      Ty.Types, [this](AST::Component::ValueType &T) { return loadType(T); }));
  if (unlikely(Ty.Types.size() == 0)) {
    return logLoadError(ErrCode::Value::MalformedTupleType,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Tuple);
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::FlagsTy &Ty) {
  // flags  ::= l*:vec(<label'>)    => (flags l+) (if 0 < |l*| <= 32)
  // label' ::= len:<u32> l:<label> => l (if len = |l|)

  auto LoadName = [this](std::string &Name) -> Expect<void> {
    EXPECTED_TRY(Name, FMgr.readName().map_error([this](auto E) {
      spdlog::error(E);
      spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
      return E;
    }));
    return {};
  };
  EXPECTED_TRY(loadVec<AST::Component::FlagsTy>(
      Ty.Labels, [LoadName](std::string &Label) { return LoadName(Label); }));
  if (unlikely(Ty.Labels.size() == 0)) {
    return logLoadError(ErrCode::Value::MalformedFlagsType,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Flags);
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::EnumTy &Ty) {
  // enum   ::= l*:vec(<label'>)    => (enum l+) (if |l*| > 0)
  // label' ::= len:<u32> l:<label> => l (if len = |l|)

  auto LoadName = [this](std::string &Name) -> Expect<void> {
    EXPECTED_TRY(Name, FMgr.readName().map_error([this](auto E) {
      spdlog::error(E);
      spdlog::error(ErrInfo::InfoLoading(FMgr.getLastOffset()));
      return E;
    }));
    return {};
  };
  return loadVec<AST::Component::EnumTy>(
      Ty.Labels, [LoadName](std::string &Label) { return LoadName(Label); });
}

Expect<void> Loader::loadType(AST::Component::OptionTy &Ty) {
  // option ::= t:<valtype> => (option t)

  return loadType(Ty.ValTy).map_error([](auto E) {
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Type_Option));
    return E;
  });
}

Expect<void> Loader::loadType(AST::Component::ResultTy &Ty) {
  // result ::= t?:<valtype>? u?:<valtype>? => (result t? (error u)?)

  EXPECTED_TRY(
      Ty.ValTy,
      loadOption<AST::Component::ResultTy, AST::Component::ValueType>(
          [this](AST::Component::ValueType &VTy) { return loadType(VTy); }));
  EXPECTED_TRY(
      Ty.ErrTy,
      loadOption<AST::Component::ResultTy, AST::Component::ValueType>(
          [this](AST::Component::ValueType &VTy) { return loadType(VTy); }));
  return {};
}

Expect<void> Loader::loadType(AST::Component::OwnTy &Ty) {
  // own ::= i:<typeidx> => (own i)

  EXPECTED_TRY(Ty.Idx, FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Own);
  }));
  return {};
}

Expect<void> Loader::loadType(AST::Component::BorrowTy &Ty) {
  // borrow ::= i:<typeidx> => (borrow i)

  EXPECTED_TRY(Ty.Idx, FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Borrow);
  }));
  return {};
}

Expect<void> Loader::loadType(AST::Component::StreamTy &Ty) {
  // stream ::= t?:<valtype>? => (stream t?) 🔀

  EXPECTED_TRY(
      Ty.ValTy,
      loadOption<AST::Component::StreamTy, AST::Component::ValueType>(
          [this](AST::Component::ValueType &VTy) { return loadType(VTy); }));
  return {};
}

Expect<void> Loader::loadType(AST::Component::FutureTy &Ty) {
  // future ::= t?:<valtype>? => (future t?) 🔀

  EXPECTED_TRY(
      Ty.ValTy,
      loadOption<AST::Component::FutureTy, AST::Component::ValueType>(
          [this](AST::Component::ValueType &VTy) { return loadType(VTy); }));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
