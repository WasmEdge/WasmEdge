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
  auto RTag = FMgr.readU32();
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
    return Res;
  }
}

Expect<void> Loader::loadType(uint32_t Tag, PrimValType &Ty) {
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
  auto RTag = FMgr.readU32();
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
    Ty.emplace<DefValType>(PrimTy);
    break;
  }

  case 0x72: {
    Record R;
    if (auto Res = loadType(R); !Res) {
      return Unexpect(Res);
    }
    Ty.emplace<DefValType>(R);
    break;
  }
  case 0x71: // case*:vec(<case>)       => (variant case*)
    Ty.emplace<DefValType>(VariantTy());
    break;
  case 0x70: // t:<valtype>             => (list t)
    Ty.emplace<DefValType>(List());
    break;
  case 0x6f: // t*:vec(<valtype>)    => (tuple t+)    (if |t*| >0)
    Ty.emplace<DefValType>(Tuple());
    break;
  case 0x6e: // l*:vec(<label'>)     => (flags l+)    (if |l*| >0)
    Ty.emplace<DefValType>(Flags());
    break;
  case 0x6d: // l*:vec(<label'>)        => (enum l*)
    Ty.emplace<DefValType>(Enum());
    break;
  case 0x6b: // t:<valtype>             => (option t)
    Ty.emplace<DefValType>(Option());
    break;
  case 0x6a: // t?:<valtype>? u?:<valtype>? => (result t? (error u)?)
    Ty.emplace<DefValType>(Result());
    break;
  case 0x69: // i:<typeidx>             => (own i)
    Ty.emplace<DefValType>(Own());
    break;
  case 0x68: // i:<typeidx>             => (borrow i)
    Ty.emplace<DefValType>(Borrow());
    break;
  case 0x40:
    // functype      ::= 0x40 ps:<paramlist> rs:<resultlist> => (func ps rs)
    Ty.emplace<FuncType>();
    break;
  case 0x41:
    // componenttype ::= 0x41 cd*:vec(<componentdecl>)       => (component cd*)
    Ty.emplace<ComponentType>();
    break;
  case 0x42:
    // instancetype  ::= 0x42 id*:vec(<instancedecl>)        => (instance id*)
    Ty.emplace<InstanceType>();
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::DefType);
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
