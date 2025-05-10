// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/type.h"
#include "common/errcode.h"
#include "loader/loader.h"
#include "spdlog/spdlog.h"

#include <optional>

using namespace std::literals;

namespace WasmEdge {
namespace Loader {

using namespace WasmEdge::AST::Component;

Expect<void> Loader::loadLabel(std::string &Label) {
  auto RName = FMgr.readName();
  if (!RName) {
    return logLoadError(ErrCode::Value::MalformedRecordType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  Label = *RName;
  return {};
}

Expect<void> Loader::loadType(ValueType &Ty) {
  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
    return E;
  }));

  switch (Tag) {
  case 0x7f:
  case 0x7e:
  case 0x7d:
  case 0x7c:
  case 0x7b:
  case 0x7a:
  case 0x79:
  case 0x78:
  case 0x77:
  case 0x76:
  case 0x75:
  case 0x74:
  case 0x73:
    Ty.emplace<PrimValType>(static_cast<PrimValType>(Tag));
    break;
  default:
    Ty.emplace<TypeIndex>(Tag);
    break;
  }

  return {};
}

Expect<void> Loader::loadType(LabelValType &Ty) {
  // labelvaltype ::= l:<label'> t:<valtype>
  EXPECTED_TRY(loadLabel(Ty.getLabel()));
  return loadType(Ty.getValType());
}

Expect<void> Loader::loadType(Record &RecTy) {
  // syntax:
  //     lt*:vec(<labelvaltype>)
  //
  // output: (record (field lt)*)  (if |lt*|>0)
  EXPECTED_TRY(loadVec<TypeSection>(
      RecTy.getLabelTypes(),
      [this](LabelValType LT) -> Expect<void> { return loadType(LT); }));
  if (RecTy.getLabelTypes().size() == 0) {
    return logLoadError(ErrCode::Value::MalformedRecordType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  return {};
}

Expect<void> Loader::loadCase(Case &C) {
  // case ::= l:<label'> t?:<valtype>? 0x00
  EXPECTED_TRY(loadLabel(C.getLabel()));
  EXPECTED_TRY(C.getValType(), loadOption<ValueType>([this](ValueType Ty) {
                 return loadType(Ty);
               }));
  EXPECTED_TRY(auto Type, FMgr.readU32());
  if (Type != 0x00) {
    return logLoadError(ErrCode::Value::MalformedVariantType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  return {};
}

Expect<void> Loader::loadType(VariantTy &Ty) {
  return loadVec<TypeSection>(Ty.getCases(),
                              [this](Case C) { return loadCase(C); });
}

Expect<void> Loader::loadType(ListTy &Ty) { return loadType(Ty.getValType()); }

Expect<void> Loader::loadType(Tuple &Ty) {
  EXPECTED_TRY(loadVec<TypeSection>(
      Ty.getTypes(), [this](ValueType T) { return loadType(T); }));
  if (unlikely(Ty.getTypes().size() == 0)) {
    return logLoadError(ErrCode::Value::MalformedTupleType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  return {};
}
Expect<void> Loader::loadType(Flags &Ty) {
  EXPECTED_TRY(loadVec<TypeSection>(
      Ty.getLabels(), [this](std::string Label) { return loadLabel(Label); }));
  if (unlikely(Ty.getLabels().size() == 0)) {
    return logLoadError(ErrCode::Value::MalformedFlagsType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  return {};
}

Expect<void> Loader::loadType(Enum &Ty) {
  return loadVec<TypeSection>(
      Ty.getLabels(), [this](std::string Label) { return loadLabel(Label); });
}

Expect<void> Loader::loadType(Option &Ty) { return loadType(Ty.getValType()); }

Expect<void> Loader::loadType(Result &Ty) {
  EXPECTED_TRY(Ty.getValType(), loadOption<ValueType>([this](ValueType VTy) {
                 return loadType(VTy);
               }));
  EXPECTED_TRY(Ty.getErrorType(), loadOption<ValueType>([this](ValueType VTy) {
                 return loadType(VTy);
               }));
  return {};
}

Expect<void> Loader::loadType(Own &Ty) {
  EXPECTED_TRY(Ty.getIndex(), FMgr.readU32());
  return {};
}

Expect<void> Loader::loadType(Borrow &Ty) {
  EXPECTED_TRY(Ty.getIndex(), FMgr.readU32());
  return {};
}

Expect<void> Loader::loadType(DefType &Ty) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
    return E;
  };

  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error(ReportError));
  switch (Tag) {
  case 0x7f:
  case 0x7e:
  case 0x7d:
  case 0x7c:
  case 0x7b:
  case 0x7a:
  case 0x79:
  case 0x78:
  case 0x77:
  case 0x76:
  case 0x75:
  case 0x74:
  case 0x73:
    Ty.emplace<DefValType>().emplace<PrimValType>(
        static_cast<PrimValType>(Tag));
    break;
  case 0x72:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Record>())
                     .map_error(ReportError));
    break;
  case 0x71:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<VariantTy>())
                     .map_error(ReportError));
    break;
  case 0x70:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<ListTy>())
                     .map_error(ReportError));
    break;
  case 0x6f:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Tuple>())
                     .map_error(ReportError));
    break;
  case 0x6e:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Flags>())
                     .map_error(ReportError));
    break;
  case 0x6d:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Enum>())
                     .map_error(ReportError));
    break;
  case 0x6b:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Option>())
                     .map_error(ReportError));
    break;
  case 0x6a:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Result>())
                     .map_error(ReportError));
    break;
  case 0x69:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Own>())
                     .map_error(ReportError));
    break;
  case 0x68:
    EXPECTED_TRY(loadType(Ty.emplace<DefValType>().emplace<Borrow>())
                     .map_error(ReportError));
    break;
  case 0x40:
    EXPECTED_TRY(loadType(Ty.emplace<FuncType>()).map_error(ReportError));
    break;
  case 0x41:
    EXPECTED_TRY(loadType(Ty.emplace<ComponentType>()).map_error(ReportError));
    break;
  case 0x42:
    EXPECTED_TRY(loadType(Ty.emplace<InstanceType>()).map_error(ReportError));
    break;
  case 0x3f:
    EXPECTED_TRY(
        loadType(Ty.emplace<ResourceType>(false)).map_error(ReportError));
    break;
  case 0x3e:
    EXPECTED_TRY(
        loadType(Ty.emplace<ResourceType>(true)).map_error(ReportError));
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::DefType);
  }

  return {};
}

Expect<void> Loader::loadType(ComponentType &Ty) {
  // componenttype ::= 0x41 cd*:vec(<componentdecl>)
  // => (component cd*)
  return loadVec<TypeSection>(Ty.getContent(), [this](ComponentDecl Decl) {
    return loadComponentDecl(Decl);
  });
}

Expect<void> Loader::loadComponentDecl(ComponentDecl &Decl) {
  EXPECTED_TRY(auto B, FMgr.peekByte());
  if (B != 0x03U) {
    return loadInstanceDecl(Decl.emplace<InstanceDecl>());
  } else {
    FMgr.readByte();
    return loadImportDecl(Decl.emplace<ImportDecl>());
  }
}

Expect<void> Loader::loadImportDecl(ImportDecl &Decl) {
  EXPECTED_TRY(loadImportName(Decl.getImportName()));
  return loadExternDesc(Decl.getExternDesc());
}

Expect<void> Loader::loadType(ResultList &Ty) {
  EXPECTED_TRY(auto Tag, FMgr.readByte());
  switch (Tag) {
  case 0x00:
    EXPECTED_TRY(loadType(Ty.emplace<ValueType>()));
    break;
  case 0x01:
    EXPECTED_TRY(
        loadVec<TypeSection>(Ty.emplace<std::vector<LabelValType>>(),
                             [this](LabelValType LV) { return loadType(LV); }));
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::DefType);
  }
  return {};
}

Expect<void> Loader::loadType(FuncType &Ty) {
  // ps:<paramlist> rs:<resultlist>
  // => (func ps rs)
  EXPECTED_TRY(loadVec<TypeSection>(
      Ty.getParamList(), [this](LabelValType &LV) { return loadType(LV); }));
  return loadType(Ty.getResultList());
}

Expect<void> Loader::loadType(InstanceType &Ty) {
  // instancetype  ::= 0x42 id*:vec(<instancedecl>)
  // => (instance id*)
  return loadVec<TypeSection>(Ty.getContent(), [this](InstanceDecl &Decl) {
    return loadInstanceDecl(Decl);
  });
}

Expect<void> Loader::loadType(ResourceType &Ty) {
  EXPECTED_TRY(auto B, FMgr.readByte());
  if (B != 0x7f) {
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::DefType);
  }

  if (Ty.IsAsync()) {
    EXPECTED_TRY(auto Idx, FMgr.readU32());
    Ty.getDestructor().emplace(Idx);
    EXPECTED_TRY(loadOption<FuncIdx>([&](FuncIdx &) -> Expect<void> {
      EXPECTED_TRY(auto RCallback, FMgr.readU32());
      Ty.getCallback().emplace(RCallback);
      return {};
    }));
  } else {
    EXPECTED_TRY(loadOption<FuncIdx>([&](FuncIdx &) -> Expect<void> {
      EXPECTED_TRY(auto RDestructor, FMgr.readU32());
      Ty.getDestructor().emplace(RDestructor);
      return {};
    }));
  }

  return {};
}

Expect<void> Loader::loadInstanceDecl(InstanceDecl &Decl) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::InstanceDecl));
    return E;
  };
  EXPECTED_TRY(auto Tag, FMgr.readByte());
  switch (Tag) {
  case 0x00:
    EXPECTED_TRY(loadType(Decl.emplace<CoreType>()).map_error(ReportError));
    break;
  case 0x01: {
    DefType Ty;
    EXPECTED_TRY(loadType(Ty).map_error(ReportError));
    Decl.emplace<std::shared_ptr<Type>>(std::make_shared<Type>(Ty));
    break;
  }
  case 0x02:
    EXPECTED_TRY(loadAlias(Decl.emplace<Alias>()).map_error(ReportError));
    break;
  case 0x04: {
    ExportDecl &Ed = Decl.emplace<ExportDecl>();
    EXPECTED_TRY(loadExportName(Ed.getExportName()).map_error(ReportError));
    EXPECTED_TRY(loadExternDesc(Ed.getExternDesc()).map_error(ReportError));
    break;
  }
  default:
    spdlog::error(
        "unknown instance decl, {} is not one of 0x00|0x01|0x02|0x04"sv, Tag);
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::DefType);
  }
  return {};
}

Expect<void> Loader::loadImportName(std::string &Name) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Name));
    return E;
  };
  EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
  if (B != 0x00) {
    return logLoadError(ErrCode::Value::MalformedName, FMgr.getLastOffset(),
                        ASTNodeAttr::Name);
  }
  EXPECTED_TRY(auto N, FMgr.readName().map_error(ReportError));
  Name = std::move(N);
  return {};
}

Expect<void> Loader::loadExportName(std::string &Name) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Name));
    return E;
  };
  EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
  if (B != 0x00) {
    return logLoadError(ErrCode::Value::MalformedName, FMgr.getLastOffset(),
                        ASTNodeAttr::Name);
  }
  EXPECTED_TRY(auto N, FMgr.readName().map_error(ReportError));
  Name = std::move(N);
  return {};
}

Expect<void> Loader::loadExternDesc(ExternDesc &Desc) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
    return E;
  };
  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error(ReportError));

  switch (Tag) {
  case 0x00: {
    EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
    if (B != 0x11U) {
      return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                          ASTNodeAttr::ExternDesc);
    }
    EXPECTED_TRY(auto Idx, FMgr.readU32().map_error(ReportError));
    DescTypeIndex &T = Desc.emplace<DescTypeIndex>();
    T.getKind() = static_cast<IndexKind>(Tag);
    T.getIndex() = Idx;
    break;
  }
  case 0x01: {
    // | 0x01 i:<typeidx>                        => (func (type i))
    EXPECTED_TRY(auto Idx, FMgr.readU32().map_error(ReportError));
    DescTypeIndex &T = Desc.emplace<DescTypeIndex>();
    T.getKind() = static_cast<IndexKind>(Tag);
    T.getIndex() = Idx;
    break;
  }
  case 0x02:
    // | 0x02 t:<valtype>                        => (value t) 🪙
    EXPECTED_TRY(loadType(Desc.emplace<ValueType>()).map_error(ReportError));
    break;
  case 0x03: {
    // | 0x03 b:<typebound>                      => (type b)
    //
    // typebound     ::=
    //   | 0x00 i:<typeidx>        => (eq i)
    //   | 0x01                    => (sub resource)
    EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
    if (B == 0x00) {
      EXPECTED_TRY(Desc.emplace<TypeBound>(),
                   FMgr.readU32().map_error(ReportError));
    } else if (B == 0x01) {
      Desc.emplace<TypeBound>() = std::nullopt;
    } else {
      return Unexpect(ReportError(ErrCode::Value::IllegalGrammar));
    }
    break;
  }
  case 0x04: {
    // | 0x04 i:<typeidx>                        => (component (type i))
    EXPECTED_TRY(auto Idx, FMgr.readU32().map_error(ReportError));
    DescTypeIndex &T = Desc.emplace<DescTypeIndex>();
    T.getKind() = static_cast<IndexKind>(Tag);
    T.getIndex() = Idx;
    break;
  }
  case 0x05: {
    // | 0x05 i:<typeidx>                        => (instance (type i))
    EXPECTED_TRY(auto Idx, FMgr.readU32().map_error(ReportError));
    DescTypeIndex &T = Desc.emplace<DescTypeIndex>();
    T.getKind() = static_cast<IndexKind>(Tag);
    T.getIndex() = Idx;
    break;
  }
  default:
    break;
  }

  return {};
}

Expect<void> Loader::loadType(CoreType &Ty) { return loadType(Ty.getType()); }
Expect<void> Loader::loadType(CoreDefType &Ty) {
  return loadType(Ty.emplace<AST::FunctionType>()).or_else([&](auto) {
    return loadType(Ty.emplace<ModuleType>());
  });
}

Expect<void> Loader::loadType(ModuleType &Ty) {
  EXPECTED_TRY(auto B, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Module);
  }));
  if (B != 0x50U) {
    return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Module);
  }
  return loadVec<TypeSection>(Ty.getContent(), [this](ModuleDecl Decl) {
    return loadModuleDecl(Decl);
  });
}

Expect<void> Loader::loadModuleDecl(ModuleDecl &Decl) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Module)
        .value();
  };
  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error(ReportError));
  switch (Tag) {
  case 0x00:
    return loadDesc(Decl.emplace<AST::ImportDesc>());
  case 0x01:
    return loadType(Decl.emplace<std::shared_ptr<CoreType>>()->getType());
  case 0x02:
    return loadAlias(Decl.emplace<Alias>());
  case 0x03:
    return loadExportDecl(Decl.emplace<CoreExportDecl>());
  default:
    return Unexpect(ReportError(ErrCode::Value::IllegalGrammar));
  }
}

Expect<void> Loader::loadExportDecl(CoreExportDecl &Decl) {
  EXPECTED_TRY(loadExportName(Decl.getName()).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Type_Module)
        .value();
  }));
  return loadDesc(Decl.getImportDesc());
}

} // namespace Loader
} // namespace WasmEdge
