// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadCanonicalOption(AST::Component::CanonOpt &C) {
  auto RTag = FMgr.readByte();
  if (!RTag) {
    return Unexpect(RTag);
  }

  switch (*RTag) {
  case 0x00:
  case 0x01:
  case 0x02:
    C.emplace<StringEncoding>() = static_cast<StringEncoding>(*RTag);
    break;
  case 0x03:
    if (auto Res = FMgr.readU32()) {
      C.emplace<Memory>().getMemIndex() = *Res;
    } else {
      return Unexpect(Res);
    }
    break;
  case 0x04:
    if (auto Res = FMgr.readU32()) {
      C.emplace<Realloc>().getFuncIndex() = *Res;
    } else {
      return Unexpect(Res);
    }
    break;
  case 0x05:
    if (auto Res = FMgr.readU32()) {
      C.emplace<PostReturn>().getFuncIndex() = *Res;
    } else {
      return Unexpect(Res);
    }
    break;
  default:
    return logLoadError(ErrCode::Value::UnknownCanonicalOption,
                        FMgr.getLastOffset(), ASTNodeAttr::Canonical);
  }

  return {};
}

Expect<void> Loader::loadCanonical(AST::Component::Lift &C) {
  if (auto Res = FMgr.readU32()) {
    C.getCoreFuncIndex() = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = loadVec<CanonSection>(C.getOptions(),
                                       [this](CanonOpt &Opt) -> Expect<void> {
                                         return loadCanonicalOption(Opt);
                                       });
      !Res) {
    return Unexpect(Res);
  }
  if (auto Res = FMgr.readU32()) {
    C.getFuncTypeIndex() = *Res;
  } else {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Loader::loadCanonical(AST::Component::Lower &C) {
  if (auto Res = FMgr.readU32()) {
    C.getFuncIndex() = *Res;
  } else {
    return Unexpect(Res);
  }
  return loadVec<CanonSection>(C.getOptions(),
                               [this](CanonOpt &Opt) -> Expect<void> {
                                 return loadCanonicalOption(Opt);
                               });
}

Expect<void> Loader::loadCanonical(AST::Component::Canon &C) {
  auto RTag = FMgr.readByte();
  if (!RTag) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Canonical));
    return Unexpect(RTag);
  }
  switch (*RTag) {
  case 0x00:
    if (auto Res = FMgr.readByte()) {
      if (unlikely(*Res != 0x00)) {
        return logLoadError(ErrCode::Value::MalformedCanonical,
                            FMgr.getLastOffset(), ASTNodeAttr::Canonical);
      }
      loadCanonical(C.emplace<Lift>());
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Canonical));
      return Unexpect(Res);
    }
    break;
  case 0x01:
    if (auto Res = FMgr.readByte()) {
      if (unlikely(*Res != 0x00)) {
        return logLoadError(ErrCode::Value::MalformedCanonical,
                            FMgr.getLastOffset(), ASTNodeAttr::Canonical);
      }
      loadCanonical(C.emplace<Lower>());
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Canonical));
      return Unexpect(Res);
    }
    break;
  case 0x02:
    if (auto Res = FMgr.readU32()) {
      C.emplace<ResourceNew>().getTypeIndex() = *Res;
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Canonical));
      return Unexpect(Res);
    }
    break;
  case 0x03:
    if (auto Res = FMgr.readU32()) {
      C.emplace<ResourceDrop>().getTypeIndex() = *Res;
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Canonical));
      return Unexpect(Res);
    }
    break;
  case 0x04:
    if (auto Res = FMgr.readU32()) {
      C.emplace<ResourceRep>().getTypeIndex() = *Res;
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Canonical));
      return Unexpect(Res);
    }
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedCanonical,
                        FMgr.getLastOffset(), ASTNodeAttr::Canonical);
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
