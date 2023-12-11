// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

using namespace WasmEdge::AST;

Expect<void> Loader::loadLabel(std::string &Label) {
  // label' ::= len:<u32> l:<label>
  // the length of loaded name must has same value as predicate value
  auto RLen = FMgr.readU32();
  if (!RLen) {
    return logLoadError(ErrCode::Value::MalformedRecordType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  auto RName = FMgr.readName();
  if (!RName) {
    return logLoadError(ErrCode::Value::MalformedRecordType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  Label = *RName;
  if (Label.size() != *RLen) {
    return logLoadError(ErrCode::Value::MalformedRecordType,
                        FMgr.getLastOffset(), ASTNodeAttr::DefType);
  }
  return {};
}

Expect<void> Loader::loadType(ValueType &Ty) {
  auto RTag = FMgr.readByte();
  if (!RTag) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
    return Unexpect(RTag);
  }

  switch (*RTag) {
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
  case 0x73: {
    PrimValType PrimTy;
    if (auto Res = loadType(*RTag, PrimTy); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    Ty.emplace<PrimValType>(PrimTy);
    break;
  }
  default:
    Ty.emplace<TypeIndex>(*RTag);
    break;
  }

  return {};
}

Expect<void> Loader::loadType(LabelValType &Ty) {
  // labelvaltype ::= l:<label'> t:<valtype>
  if (auto Res = loadLabel(Ty.getLabel()); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = loadType(Ty.getValType()); !Res) {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Loader::loadType(Record &RecTy) {
  // syntax:
  //     lt*:vec(<labelvaltype>)
  //
  // output: (record (field lt)*)  (if |lt*|>0)
  auto Res = loadVec<CompTypeSection>(
      RecTy.getLabelTypes(),
      [this](LabelValType LT) -> Expect<void> { return loadType(LT); });

  if (Res) {
    if (RecTy.getLabelTypes().size() > 0) {
      return {};
    } else {
      return logLoadError(ErrCode::Value::MalformedRecordType,
                          FMgr.getLastOffset(), ASTNodeAttr::DefType);
    }
  } else {
    return Unexpect(Res);
  }
}

Expect<void> Loader::loadCase(AST::Case &C) {
  // case ::= l:<label'> t?:<valtype>? 0x00
  if (auto Res = loadLabel(C.getLabel()); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = loadOption<ValueType>(
          [this](ValueType Ty) -> Expect<void> { return loadType(Ty); })) {
    C.getValType() = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = FMgr.readU32()) {
    if (*Res != 0x00) {
      return logLoadError(ErrCode::Value::MalformedVariantType,
                          FMgr.getLastOffset(), ASTNodeAttr::DefType);
    }
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Loader::loadType(VariantTy &Ty) {
  if (auto Res = loadVec<CompTypeSection>(
          Ty.getCases(),
          [this](Case C) -> Expect<void> { return loadCase(C); })) {
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<void> Loader::loadType(List &Ty) { return loadType(Ty.getValType()); }

Expect<void> Loader::loadType(Tuple &Ty) {
  if (auto Res = loadVec<CompTypeSection>(
          Ty.getTypes(),
          [this](ValueType T) -> Expect<void> { return loadType(T); })) {
    if (Ty.getTypes().size() == 0) {
      return logLoadError(ErrCode::Value::MalformedTupleType,
                          FMgr.getLastOffset(), ASTNodeAttr::DefType);
    }
    return {};
  } else {
    return Unexpect(Res);
  }
}
Expect<void> Loader::loadType(Flags &Ty) {
  if (auto Res = loadVec<CompTypeSection>(
          Ty.getLabels(), [this](std::string Label) -> Expect<void> {
            return loadLabel(Label);
          })) {
    if (Ty.getLabels().size() == 0) {
      return logLoadError(ErrCode::Value::MalformedFlagsType,
                          FMgr.getLastOffset(), ASTNodeAttr::DefType);
    }
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<void> Loader::loadType(Enum &Ty) {
  if (auto Res = loadVec<CompTypeSection>(
          Ty.getLabels(), [this](std::string Label) -> Expect<void> {
            return loadLabel(Label);
          })) {
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<void> Loader::loadType(Option &Ty) { return loadType(Ty.getValType()); }

Expect<void> Loader::loadType(Result &Ty) {
  if (auto Res = loadOption<ValueType>(
          [this](ValueType VTy) -> Expect<void> { return loadType(VTy); })) {
    Ty.getValType() = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = loadOption<ValueType>(
          [this](ValueType VTy) -> Expect<void> { return loadType(VTy); })) {
    Ty.getErrorType() = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Loader::loadType(Own &Ty) {
  if (auto Res = FMgr.readU32()) {
    Ty.getIndex() = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Loader::loadType(Borrow &Ty) {
  if (auto Res = FMgr.readU32()) {
    Ty.getIndex() = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Loader::loadType(Byte Tag, PrimValType &Ty) {
  switch (Tag) {
  case 0x7f: // bool
    Ty = PrimValType::Bool;
    break;
  case 0x7e: // s8
    Ty = PrimValType::S8;
    break;
  case 0x7d: // u8
    Ty = PrimValType::U8;
    break;
  case 0x7c: // s16
    Ty = PrimValType::S16;
    break;
  case 0x7b: // u16
    Ty = PrimValType::U16;
    break;
  case 0x7a: // s32
    Ty = PrimValType::S32;
    break;
  case 0x79: // u32
    Ty = PrimValType::U32;
    break;
  case 0x78: // s64
    Ty = PrimValType::S64;
    break;
  case 0x77: // u64
    Ty = PrimValType::U64;
    break;
  case 0x76: // float32
    Ty = PrimValType::Float32;
    break;
  case 0x75: // float64
    Ty = PrimValType::Float64;
    break;
  case 0x74: // char
    Ty = PrimValType::Char;
    break;
  case 0x73: // string
    Ty = PrimValType::String;
    break;
  default:
    break;
  }
  return {};
}

Expect<void> Loader::loadType(AST::DefType &Ty) {
  auto RTag = FMgr.readByte();
  if (!RTag) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
    return Unexpect(RTag);
  }
  Byte Tag = *RTag;

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
    if (auto Res =
            loadType(Tag, Ty.emplace<DefValType>().emplace<PrimValType>());
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    };
    break;
  case 0x72:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Record>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x71:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<VariantTy>());
        !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x70:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<List>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x6f:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Tuple>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x6e:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Flags>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x6d:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Enum>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x6b:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Option>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x6a:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Result>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x69:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Own>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x68:
    if (auto Res = loadType(Ty.emplace<DefValType>().emplace<Borrow>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x40:
    if (auto Res = loadType(Ty.emplace<FuncType>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x41:
    if (auto Res = loadType(Ty.emplace<ComponentType>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  case 0x42: {
    if (auto Res = loadType(Ty.emplace<InstanceType>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
      return Unexpect(Res);
    }
    break;
  }
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::DefType);
  }

  return {};
}

Expect<void> Loader::loadType(ComponentType &Ty) {
  // componenttype ::= 0x41 cd*:vec(<componentdecl>)
  // => (component cd*)
  return loadVec<CompTypeSection>(Ty.getContent(), [this](ComponentDecl Decl) {
    return loadComponentDecl(Decl);
  });
}

Expect<void> Loader::loadComponentDecl(ComponentDecl &Decl) {
  if (auto Res = FMgr.readByte(0x03)) {
    return loadImportDecl(Decl.emplace<ImportDecl>());
  } else {
    return loadInstanceDecl(Decl.emplace<InstanceDecl>());
  }
}

Expect<void> Loader::loadImportDecl(ImportDecl &Decl) {
  if (auto Res = loadImportExportName(Decl.getImportName()); !Res) {
    return Unexpect(Res);
  }
  return loadExternDesc(Decl.getExternDesc());
}

Expect<void> Loader::loadType(ResultList &Ty) {
  if (auto RTag = FMgr.readByte(); !RTag) {
    return Unexpect(RTag);
  } else {
    switch (*RTag) {
    case 0x00: {
      ValueType V;
      if (auto Res = loadType(V); !Res) {
        return Unexpect(Res);
      }
      Ty.emplace<ValueType>(V);
      break;
    }
    case 0x01: {
      std::vector<LabelValType> RList;
      if (auto Res = loadVec<CompTypeSection>(
              RList, [this](LabelValType LV) { return loadType(LV); });
          !Res) {
        return Unexpect(Res);
      }
      Ty.emplace<std::vector<LabelValType>>(RList);
      break;
    }
    default:
      return logLoadError(ErrCode::Value::MalformedDefType,
                          FMgr.getLastOffset(), ASTNodeAttr::DefType);
    }
    return {};
  }
}

Expect<void> Loader::loadType(FuncType &Ty) {
  // ps:<paramlist> rs:<resultlist>
  // => (func ps rs)
  if (auto Res = loadVec<CompTypeSection>(
          Ty.getParamList(), [this](LabelValType LV) { return loadType(LV); });
      !Res) {
    return Unexpect(Res);
  }
  return loadType(Ty.getResultList());
}

Expect<void> Loader::loadType(InstanceType &Ty) {
  // instancetype  ::= 0x42 id*:vec(<instancedecl>)
  // => (instance id*)
  return loadVec<CompTypeSection>(Ty.getContent(), [this](InstanceDecl Decl) {
    return loadInstanceDecl(Decl);
  });
}

Expect<void> Loader::loadInstanceDecl(InstanceDecl &Decl) {
  auto RTag = FMgr.readByte();
  if (!RTag) {
    return Unexpect(RTag);
  }
  switch (*RTag) {
  case 0x00: {
    // TODO core:type
    spdlog::error("component model core:type in type section is incomplete");
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::InstanceDecl);
  }
  case 0x01: {
    DefType Ty;
    if (auto Res = loadType(Ty); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::InstanceDecl));
      return Unexpect(Res);
    }
    Decl.emplace<std::shared_ptr<Type>>(std::make_shared<Type>(Ty));
    break;
  }
  case 0x02: {
    if (auto Res = loadAlias(Decl.emplace<Alias>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::InstanceDecl));
      return Unexpect(Res);
    }
    break;
  }
  case 0x04: {
    ExportDecl &Ed = Decl.emplace<ExportDecl>();
    if (auto Res = loadImportExportName(Ed.getExportName()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::InstanceDecl));
      return Unexpect(Res);
    }
    if (auto Res = loadExternDesc(Ed.getExternDesc()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::InstanceDecl));
      return Unexpect(Res);
    };
    break;
  }
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::DefType);
  }
  return {};
}

Expect<void> Loader::loadImportExportName(std::string &Name) {
  if (auto Res = FMgr.readName(); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Name));
    return Unexpect(Res);
  } else {
    Name = *Res;
    return {};
  }
}

Expect<void> Loader::loadExternDesc(AST::ExternDesc &Desc) {
  auto RTag = FMgr.readByte();
  if (!RTag) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
    return Unexpect(RTag);
  }

  switch (*RTag) {
  case 0x00:
    if (auto Res = FMgr.readByte(0x11); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    }
    if (auto Res = FMgr.readU32(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    } else {
      Desc.emplace<TypeIndex>(*Res);
    }
    break;
  case 0x01:
    // | 0x01 i:<typeidx>                        => (func (type i))
    if (auto Res = FMgr.readU32(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    } else {
      Desc.emplace<TypeIndex>(*Res);
    }
    break;
  case 0x02:
    // | 0x02 t:<valtype>                        => (value t) 🪙
    if (auto Res = loadType(Desc.emplace<ValueType>()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    }
    break;
  case 0x03:
    // | 0x03 b:<typebound>                      => (type b)
    if (auto Res = FMgr.readU32(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    } else {
      Desc = *Res;
    }
    break;
  case 0x04:
    // | 0x04 i:<typeidx>                        => (component (type i))
    // typebound     ::= 0x00 i:<typeidx>        => (eq i)
    // | 0x01                                    => (sub resource)
    if (auto Res = FMgr.readByte(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    } else if (*Res == 0x00) {
      Desc.emplace<TypeIndex>(*Res);
      break;
    } else if (*Res == 0x01) {
      break;
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    }
  case 0x05:
    // | 0x05 i:<typeidx>                        => (instance (type i))
    if (auto Res = FMgr.readU32(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::ExternDesc));
      return Unexpect(Res);
    } else {
      Desc.emplace<TypeIndex>(*Res);
    }
    break;
  default:
    break;
  }

  return {};
}

Expect<void> Loader::loadType(CoreType &Ty) { return loadType(Ty.getType()); }
Expect<void> Loader::loadType(CoreDefType &Ty) {
  Expect<void> Res;
  if (Res = loadType(Ty.emplace<FunctionType>()); Res) {
    return {};
  } else if (Res = loadType(Ty.emplace<ModuleType>()); Res) {
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<void> Loader::loadType(ModuleType &Ty) {
  if (auto Res = FMgr.readByte(0x50U); !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Module);
  }
  return loadVec<CompTypeSection>(
      Ty.getContent(),
      [this](ModuleDecl Decl) -> Expect<void> { return loadModuleDecl(Decl); });
}

Expect<void> Loader::loadModuleDecl(ModuleDecl &Decl) {
  auto RTag = FMgr.readByte();
  if (!RTag) {
    return logLoadError(RTag.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Module);
  }
  switch (*RTag) {
  case 0x00:
    return loadDesc(Decl.emplace<ImportDesc>());
  case 0x01:
    return loadType(Decl.emplace<std::shared_ptr<CoreType>>()->getType());
  case 0x02:
    return loadAlias(Decl.emplace<Alias>());
  case 0x03:
    return loadExportDecl(Decl.emplace<CoreExportDecl>());
  default:
    return logLoadError(RTag.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Module);
  }
}

Expect<void> Loader::loadExportDecl(CoreExportDecl &Decl) {
  if (auto Res = loadImportExportName(Decl.getName()); !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Type_Module);
  }
  return loadDesc(Decl.getImportDesc());
}

} // namespace Loader
} // namespace WasmEdge
