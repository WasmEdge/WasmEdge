// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

using namespace WasmEdge::AST;

Expect<void> Loader::loadType(AST::DefType &Ty) {
  auto RTag = FMgr.readU32();
  if (!RTag) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::DefType));
    return Unexpect(RTag);
  }

  switch (*RTag) {
  case 0x7f: // bool
    Ty.emplace<DefValType>(PrimValType::Bool);
    break;
  case 0x7e: // s8
    Ty.emplace<DefValType>(PrimValType::S8);
    break;
  case 0x7d: // u8
    Ty.emplace<DefValType>(PrimValType::U8);
    break;
  case 0x7c: // s16
    Ty.emplace<DefValType>(PrimValType::S16);
    break;
  case 0x7b: // u16
    Ty.emplace<DefValType>(PrimValType::U16);
    break;
  case 0x7a: // s32
    Ty.emplace<DefValType>(PrimValType::S32);
    break;
  case 0x79: // u32
    Ty.emplace<DefValType>(PrimValType::U32);
    break;
  case 0x78: // s64
    Ty.emplace<DefValType>(PrimValType::S64);
    break;
  case 0x77: // u64
    Ty.emplace<DefValType>(PrimValType::U64);
    break;
  case 0x76: // float32
    Ty.emplace<DefValType>(PrimValType::Float32);
    break;
  case 0x75: // float64
    Ty.emplace<DefValType>(PrimValType::Float64);
    break;
  case 0x74: // char
    Ty.emplace<DefValType>(PrimValType::Char);
    break;
  case 0x73: // string
    Ty.emplace<DefValType>(PrimValType::String);
    break;
  case 0x72: // lt*:vec(<labelvaltype>) => (record (field lt)*)  (if |lt*|>0)
    Ty.emplace<DefValType>(Record());
    break;
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
