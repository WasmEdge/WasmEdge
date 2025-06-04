// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadCanonicalOption(AST::Component::CanonOpt &C) {
  EXPECTED_TRY(auto Tag, FMgr.readByte());
  switch (Tag) {
  case 0x00:
  case 0x01:
  case 0x02:
    C.emplace<StringEncoding>() = static_cast<StringEncoding>(Tag);
    break;
  case 0x03: {
    EXPECTED_TRY(C.emplace<Memory>().getMemIndex(), FMgr.readU32());
  } break;
  case 0x04: {
    EXPECTED_TRY(C.emplace<Realloc>().getFuncIndex(), FMgr.readU32());
  } break;
  case 0x05: {
    EXPECTED_TRY(C.emplace<PostReturn>().getFuncIndex(), FMgr.readU32());
  } break;
  default:
    return logLoadError(ErrCode::Value::UnknownCanonicalOption,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  }

  return {};
}

Expect<void> Loader::loadCanonical(AST::Component::Lift &C) {
  EXPECTED_TRY(C.getCoreFuncIndex(), FMgr.readU32());
  EXPECTED_TRY(loadVec<CanonSection>(C.getOptions(),
                                     [this](CanonOpt &Opt) -> Expect<void> {
                                       return loadCanonicalOption(Opt);
                                     }));
  EXPECTED_TRY(C.getFuncTypeIndex(), FMgr.readU32());

  return {};
}

Expect<void> Loader::loadCanonical(AST::Component::Lower &C) {
  EXPECTED_TRY(C.getFuncIndex(), FMgr.readU32());
  return loadVec<CanonSection>(C.getOptions(),
                               [this](CanonOpt &Opt) -> Expect<void> {
                                 return loadCanonicalOption(Opt);
                               });
}

Expect<void> Loader::loadCanonical(AST::Component::Canon &C) {
  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return E;
  }));
  switch (Tag) {
  case 0x00: {
    EXPECTED_TRY(auto B, FMgr.readByte().map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return E;
    }));
    if (unlikely(B != 0x00)) {
      return logLoadError(ErrCode::Value::MalformedCanonical,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
    }
    EXPECTED_TRY(loadCanonical(C.emplace<Lift>()));
  } break;
  case 0x01: {
    EXPECTED_TRY(auto B, FMgr.readByte().map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return E;
    }));
    if (unlikely(B != 0x00)) {
      return logLoadError(ErrCode::Value::MalformedCanonical,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
    }
    EXPECTED_TRY(loadCanonical(C.emplace<Lower>()));
  } break;
  case 0x02: {
    EXPECTED_TRY(auto Idx, FMgr.readU32().map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return E;
    }));
    C.emplace<ResourceNew>().getTypeIndex() = Idx;
  } break;
  case 0x03: {
    EXPECTED_TRY(auto Idx, FMgr.readU32().map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return E;
    }));
    C.emplace<ResourceDrop>().getTypeIndex() = Idx;
  } break;
  case 0x04: {
    EXPECTED_TRY(auto Idx, FMgr.readU32().map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return E;
    }));
    C.emplace<ResourceRep>().getTypeIndex() = Idx;
  } break;
  default:
    return logLoadError(ErrCode::Value::MalformedCanonical,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
