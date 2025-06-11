// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

using namespace std::literals;

namespace WasmEdge {
namespace Loader {

using namespace WasmEdge::AST::Component;

Expect<void> Loader::loadType(AST::Component::CoreType &Ty) {
  // TODO: COMPONENT - combine the CoreType and CoreDefType.

  // core:type ::= dt:<core:deftype> => (type dt)
  return loadType(Ty.getType());
}

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

  // core:deftype ::= rt:<core:rectype>
  //                => rt (WebAssembly 3.0)
  //                | 0x00 0x50 x*:vec(<core:typeidx>) ct:<core:comptype>
  //                => sub x* ct (WebAssembly 3.0)
  //                | mt:<core:moduletype>
  //                => mt

  // Peek the first byte to determine the type to load.
  EXPECTED_TRY(uint8_t Flag, FMgr.peekByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_CoreDefType);
  }));

  // TODO: COMPONENT - Apply the GC proposal: load the AST::SubType.
  switch (Flag) {
  case 0x50: // module type case
    return loadType(Ty.emplace<AST::Component::CoreModuleType>())
        .map_error(ReportError);
  case 0x60: // function type case
    // TODO: COMPONENT - combine this into recursive type case
    return loadType(Ty.emplace<AST::FunctionType>()).map_error(ReportError);
  case 0x00: // sub non-final case
    // TODO: COMPONENT - implement this
    break;
  default: // recursive type case
    // TODO: COMPONENT - implement this
    break;
  }
  return logLoadError(ErrCode::Value::ComponentNotImplLoader,
                      FMgr.getLastOffset(), ASTNodeAttr::Comp_CoreDefType);
}

Expect<void> Loader::loadType(AST::Component::CoreModuleType &Ty) {
  // core:moduletype ::= 0x50 md*:vec(<core:moduledecl>) => (module md*)

  EXPECTED_TRY(auto B, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreModuleType);
  }));
  if (B != 0x50U) {
    return logLoadError(ErrCode::Value::MalformedModuleType,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_CoreModuleType);
  }
  return loadVec<AST::Component::CoreModuleType>(
      Ty.getContent(),
      [this](CoreModuleDecl &Decl) { return loadModuleDecl(Decl); });
}

Expect<void> Loader::loadModuleDecl(AST::Component::CoreModuleDecl &Decl) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreModuleDecl));
    return E;
  };
  // core:moduledecl ::= 0x00 i:<core:import>     => i
  //                   | 0x01 t:<core:type>       => t
  //                   | 0x02 a:<core:alias>      => a
  //                   | 0x03 e:<core:exportdecl> => e

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreModuleDecl);
  }));
  switch (Flag) {
  case 0x00:
    return loadDesc(Decl.emplace<AST::ImportDesc>()).map_error(ReportError);
  case 0x01:
    return loadType(Decl.emplace<std::shared_ptr<CoreType>>()->getType())
        .map_error(ReportError);
  case 0x02:
    return loadAlias(Decl.emplace<Alias>()).map_error(ReportError);
  case 0x03:
    return loadExportDecl(Decl.emplace<CoreExportDecl>())
        .map_error(ReportError);
  default:
    return logLoadError(ErrCode::Value::IllegalGrammar, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreModuleDecl);
  }
}

Expect<void> Loader::loadExportDecl(AST::Component::CoreExportDecl &Decl) {
  // core:exportdecl ::= n:<core:name> d:<core:importdesc> => (export n d)

  EXPECTED_TRY(Decl.getName(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreExportDecl);
  }));
  return loadDesc(Decl.getImportDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreExportDecl));
    return E;
  });
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
  case 0x40:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::FuncType>())
                     .map_error(ReportError));
    return {};
  case 0x41:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::ComponentType>())
                     .map_error(ReportError));
    return {};
  case 0x42:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::InstanceType>())
                     .map_error(ReportError));
    return {};
  case 0x3F:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::ResourceType>(false))
                     .map_error(ReportError));
    return {};
  case 0x3E:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::ResourceType>(true))
                     .map_error(ReportError));
    return {};
  default:
    return loadType(Ty.emplace<AST::Component::DefValType>(), Flag)
        .map_error(ReportError);
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
    Ty.emplace<AST::Component::PrimValType>(
        static_cast<AST::Component::PrimValType>(Code));
    return {};
  case 0x72:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::RecordTy>())
                     .map_error(ReportError));
    return {};
  case 0x71:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::VariantTy>())
                     .map_error(ReportError));
    return {};
  case 0x70:
    EXPECTED_TRY(
        loadType(Ty.emplace<AST::Component::ListTy>()).map_error(ReportError));
    return {};
  case 0x67:
    // TODO: COMPONENT - implement t:<valtype> len:<u32>
    return logLoadError(ErrCode::Value::ComponentNotImplLoader,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_DefValType);
  case 0x6F:
    EXPECTED_TRY(
        loadType(Ty.emplace<AST::Component::TupleTy>()).map_error(ReportError));
    return {};
  case 0x6E:
    EXPECTED_TRY(
        loadType(Ty.emplace<AST::Component::FlagsTy>()).map_error(ReportError));
    return {};
  case 0x6D:
    EXPECTED_TRY(
        loadType(Ty.emplace<AST::Component::EnumTy>()).map_error(ReportError));
    return {};
  case 0x6B:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::OptionTy>())
                     .map_error(ReportError));
    return {};
  case 0x6A:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::ResultTy>())
                     .map_error(ReportError));
    return {};
  case 0x69:
    EXPECTED_TRY(
        loadType(Ty.emplace<AST::Component::OwnTy>()).map_error(ReportError));
    return {};
  case 0x68:
    EXPECTED_TRY(loadType(Ty.emplace<AST::Component::BorrowTy>())
                     .map_error(ReportError));
    return {};
  case 0x66:
    // TODO: COMPONENT - implement (stream t?) 🔀
    return logLoadError(ErrCode::Value::ComponentNotImplLoader,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_DefValType);
  case 0x65:
    // TODO: COMPONENT - implement (future t?) 🔀
    return logLoadError(ErrCode::Value::ComponentNotImplLoader,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_DefValType);
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_DefValType);
  }
}

Expect<void> Loader::loadType(AST::Component::RecordTy &Ty) {
  // record ::= lt*:vec(<labelvaltype>) => (record (field lt)*) (if |lt*| > 0)

  EXPECTED_TRY(loadVec<AST::Component::RecordTy>(
      Ty.getLabelTypes(),
      [this](AST::Component::LabelValType &LT) { return loadType(LT); }));
  if (Ty.getLabelTypes().size() == 0) {
    return logLoadError(ErrCode::Value::MalformedRecordType,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Record);
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::VariantTy &Ty) {
  // variant ::= case*:vec(<case>) => (variant case+) (if |case*| > 0)

  return loadVec<AST::Component::VariantTy>(
      Ty.getCases(), [this](Case &C) { return loadCase(C); });
}

Expect<void> Loader::loadType(AST::Component::ListTy &Ty) {
  // list ::= t:<valtype> => (list t)

  // TODO: COMPONENT - lack of fixed length list type:
  // list ::= t:<valtype> len:<u32> => (list t len) (if len > 0) 🔧

  return loadType(Ty.getValType()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Type_List));
    return E;
  });
}

Expect<void> Loader::loadType(AST::Component::TupleTy &Ty) {
  // tuple ::= t*:vec(<valtype>) => (tuple t+) (if |t*| > 0)

  EXPECTED_TRY(loadVec<AST::Component::TupleTy>(
      Ty.getTypes(),
      [this](AST::Component::ValueType &T) { return loadType(T); }));
  if (unlikely(Ty.getTypes().size() == 0)) {
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
      Ty.getLabels(),
      [LoadName](std::string &Label) { return LoadName(Label); }));
  if (unlikely(Ty.getLabels().size() == 0)) {
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
      Ty.getLabels(),
      [LoadName](std::string &Label) { return LoadName(Label); });
}

Expect<void> Loader::loadType(AST::Component::OptionTy &Ty) {
  // option ::= t:<valtype> => (option t)

  return loadType(Ty.getValType()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Type_Option));
    return E;
  });
}

Expect<void> Loader::loadType(AST::Component::ResultTy &Ty) {
  // result ::= t?:<valtype>? u?:<valtype>? => (result t? (error u)?)

  EXPECTED_TRY(
      Ty.getValType(),
      loadOption<AST::Component::ResultTy, AST::Component::ValueType>(
          [this](AST::Component::ValueType &VTy) { return loadType(VTy); }));
  EXPECTED_TRY(
      Ty.getErrorType(),
      loadOption<AST::Component::ResultTy, AST::Component::ValueType>(
          [this](AST::Component::ValueType &VTy) { return loadType(VTy); }));
  return {};
}

Expect<void> Loader::loadType(AST::Component::OwnTy &Ty) {
  // own ::= i:<typeidx> => (own i)

  EXPECTED_TRY(Ty.getIndex(), FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Own);
  }));
  return {};
}

Expect<void> Loader::loadType(AST::Component::BorrowTy &Ty) {
  // borrow ::= i:<typeidx> => (borrow i)

  EXPECTED_TRY(Ty.getIndex(), FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Type_Borrow);
  }));
  return {};
}

Expect<void> Loader::loadType(AST::Component::LabelValType &Ty) {
  // labelvaltype ::= l:<label'> t:<valtype>
  // label'       ::= len:<u32> l:<label>    => l (if len = |l|)

  EXPECTED_TRY(Ty.getLabel(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_LabelValType);
  }));
  return loadType(Ty.getValType()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_LabelValType));
    return E;
  });
}

Expect<void> Loader::loadType(AST::Component::ValueType &Ty) {
  // valtype ::= i:<typeidx>       => i
  //           | pvt:<primvaltype> => pvt

  EXPECTED_TRY(int64_t Val, FMgr.readS33().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_ValueType);
  }));
  if (Val < 0) {
    // PrimValType case.
    if (Val < -64) {
      // For checking the invalid s33 value which is larger than 1 byte.
      return logLoadError(ErrCode::Value::MalformedValType,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_ValueType);
    }
    AST::Component::PrimValType PVT = static_cast<AST::Component::PrimValType>(
        static_cast<uint8_t>(Val & INT64_C(0x7F)));
    switch (PVT) {
    case AST::Component::PrimValType::Bool:
    case AST::Component::PrimValType::S8:
    case AST::Component::PrimValType::U8:
    case AST::Component::PrimValType::S16:
    case AST::Component::PrimValType::U16:
    case AST::Component::PrimValType::S32:
    case AST::Component::PrimValType::U32:
    case AST::Component::PrimValType::S64:
    case AST::Component::PrimValType::U64:
    case AST::Component::PrimValType::F32:
    case AST::Component::PrimValType::F64:
    case AST::Component::PrimValType::Char:
    case AST::Component::PrimValType::String:
    case AST::Component::PrimValType::ErrorContext:
      Ty.emplace<AST::Component::PrimValType>(PVT);
      break;
    default:
      return logLoadError(ErrCode::Value::MalformedValType,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_ValueType);
    }
  } else {
    // Type index case.
    Ty.emplace<uint32_t>(static_cast<uint32_t>(Val));
  }
  return {};
}

Expect<void> Loader::loadCase(AST::Component::Case &C) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Case);
  };
  // case ::= l:<label'> t?:<valtype>? 0x00

  EXPECTED_TRY(C.getLabel(), FMgr.readName().map_error(ReportError));
  EXPECTED_TRY(
      C.getValType(),
      loadOption<AST::Component::Case, AST::Component::ValueType>(
          [this](AST::Component::ValueType &Ty) { return loadType(Ty); }));
  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  if (Flag != 0x00) {
    return logLoadError(ErrCode::Value::MalformedVariantType,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Case);
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::FuncType &Ty) {
  // functype  ::= 0x40 ps:<paramlist> rs:<resultlist> => (func ps rs)
  // paramlist ::= lt*:vec(<labelvaltype>)             => (param lt)*

  // The prefix `0x40` has been loaded in the parent scope.
  EXPECTED_TRY(loadVec<AST::Component::FuncType>(
      Ty.getParamList(), [this](LabelValType &LV) { return loadType(LV); }));
  return loadType(Ty.getResultList());
}

Expect<void> Loader::loadType(AST::Component::ResultList &Ty) {
  // TODO: COMPONENT - combine into FuncType.

  // resultlist ::= 0x00 t:<valtype> => (result t)
  //              | 0x01 0x00        => ϵ

  /// NOTE:
  /// The original resultlist grammar:
  ///
  /// resultlist ::= 0x00 t:<valtype>             => (result t)
  ///              | 0x01 lt*:vec(<labelvaltype>) => (result lt)*

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte());
  switch (Flag) {
  case 0x00:
    EXPECTED_TRY(
        loadType(Ty.emplace<AST::Component::ValueType>()).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_FuncType));
          return E;
        }));
    break;
  case 0x01:
    EXPECTED_TRY(loadVec<AST::Component::ResultList>(
        Ty.emplace<std::vector<AST::Component::LabelValType>>(),
        [this](LabelValType &LV) { return loadType(LV); }));
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_FuncType);
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::ComponentType &Ty) {
  // componenttype ::= 0x41 cd*:vec(<componentdecl>) => (component cd*)

  // The prefix `0x41` has been loaded in the parent scope.
  return loadVec<AST::Component::ComponentType>(
      Ty.getContent(),
      [this](ComponentDecl &Decl) { return loadComponentDecl(Decl); });
}

Expect<void> Loader::loadComponentDecl(AST::Component::ComponentDecl &Decl) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ComponentDecl));
    return E;
  };
  // componentdecl ::= 0x03 id:<importdecl> => id
  //                 | id:<instancedecl>    => id

  EXPECTED_TRY(auto B, FMgr.peekByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ComponentDecl);
  }));
  if (B == 0x03U) {
    FMgr.readByte();
    return loadImportDecl(Decl.emplace<AST::Component::ImportDecl>())
        .map_error(ReportError);
  } else {
    return loadInstanceDecl(Decl.emplace<AST::Component::InstanceDecl>())
        .map_error(ReportError);
  }
}

Expect<void> Loader::loadImportDecl(AST::Component::ImportDecl &Decl) {
  // importdecl  ::= in:<importname'> ed:<externdesc> => (import in ed)
  // importname' ::= 0x00 len:<u32> in:<importname>   => in (if len = |in|)

  EXPECTED_TRY(loadExternName(Decl.getImportName()).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_ImportDecl);
  }));
  return loadExternDesc(Decl.getExternDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ImportDecl));
    return E;
  });
}

Expect<void> Loader::loadType(AST::Component::InstanceType &Ty) {
  // instancetype ::= 0x42 id*:vec(<instancedecl>) => (instance id*)

  // The prefix `0x42` has been loaded in the parent scope.
  return loadVec<AST::Component::InstanceType>(
      Ty.getContent(),
      [this](InstanceDecl &Decl) { return loadInstanceDecl(Decl); });
}

Expect<void> Loader::loadInstanceDecl(AST::Component::InstanceDecl &Decl) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_InstanceDecl));
    return E;
  };
  // instancedecl ::= 0x00 t:<core:type>   => t
  //                | 0x01 t:<type>        => t
  //                | 0x02 a:<alias>       => a
  //                | 0x04 ed:<exportdecl> => ed

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_InstanceDecl);
  }));
  switch (Flag) {
  case 0x00:
    return loadType(Decl.emplace<CoreType>()).map_error(ReportError);
  case 0x01: {
    DefType Ty;
    EXPECTED_TRY(loadType(Ty).map_error(ReportError));
    Decl.emplace<std::shared_ptr<Type>>(std::make_shared<Type>(Ty));
    return {};
  }
  case 0x02:
    return loadAlias(Decl.emplace<Alias>()).map_error(ReportError);
  case 0x04: {
    return loadExportDecl(Decl.emplace<AST::Component::ExportDecl>())
        .map_error(ReportError);
  }
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_InstanceDecl);
  }
}

Expect<void> Loader::loadExportDecl(AST::Component::ExportDecl &Decl) {
  // exportdecl  ::= en:<exportname'> ed:<externdesc> => (export en ed)
  // exportname' ::= 0x00 len:<u32> en:<exportname>   => en (if len = |en|)

  EXPECTED_TRY(loadExternName(Decl.getExportName()).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_ExportDecl);
  }));
  return loadExternDesc(Decl.getExternDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_ExportDecl));
    return E;
  });
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

Expect<void> Loader::loadExternName(std::string &Name) {
  // importname' ::= 0x00 len:<u32> in:<importname> => in (if len = |in|)
  // exportname' ::= 0x00 len:<u32> en:<exportname> => en (if len = |en|)

  // Error messages will be handled in the parent scope.
  EXPECTED_TRY(auto B, FMgr.readByte());
  if (B != 0x00) {
    return Unexpect(ErrCode::Value::MalformedName);
  }
  EXPECTED_TRY(Name, FMgr.readName());
  return {};
}

Expect<void> Loader::loadExternDesc(AST::Component::ExternDesc &Desc) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_ExternDesc);
  };
  // externdesc ::= 0x00 0x11 i:<core:typeidx> => (core module (type i))
  //              | 0x01 i:<typeidx>           => (func (type i))
  //              | 0x02 b:<valuebound>        => (value b) 🪙
  //              | 0x03 b:<typebound>         => (type b)
  //              | 0x04 i:<typeidx>           => (component (type i))
  //              | 0x05 i:<typeidx>           => (instance (type i))
  // valuebound ::= 0x00 i:<valueidx>          => (eq i) 🪙
  //              | 0x01 t:<valtype>           => t 🪙
  // typebound  ::= 0x00 i:<typeidx>           => (eq i)
  //              | 0x01                       => (sub resource)

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (B != 0x11U) {
      return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ExternDesc);
    }
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    DescTypeIndex &T = Desc.emplace<DescTypeIndex>();
    T.getKind() = static_cast<IndexKind>(Flag);
    T.getIndex() = Idx;
    return {};
  }
  case 0x01:
  case 0x04:
  case 0x05: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    DescTypeIndex &T = Desc.emplace<DescTypeIndex>();
    T.getKind() = static_cast<IndexKind>(Flag);
    T.getIndex() = Idx;
    return {};
  }
  case 0x02:
    // TODO: COMPONENT - ValueType is the old spec, should modify to ValueBound.
    EXPECTED_TRY(loadType(Desc.emplace<ValueType>()).map_error(ReportError));
    return {};
  case 0x03: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (B == 0x00) {
      EXPECTED_TRY(Desc.emplace<TypeBound>(),
                   FMgr.readU32().map_error(ReportError));
    } else if (B == 0x01) {
      Desc.emplace<TypeBound>() = std::nullopt;
    } else {
      return ReportError(ErrCode::Value::IllegalGrammar);
    }
    return {};
  }
  default:
    return ReportError(ErrCode::Value::IllegalGrammar);
  }
}

} // namespace Loader
} // namespace WasmEdge
