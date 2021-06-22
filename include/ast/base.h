// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/base.h - AST node base class definition --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the AST node base class, which is the
/// base class for all of the AST nodes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/astdef.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/log.h"
#include "loader/filemgr.h"

namespace WasmEdge {
namespace AST {

/// Base class of AST node.
class Base {
public:
  Base() noexcept = default;
  virtual ~Base() noexcept = default;

  /// Binary loading from file manager.
  virtual Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) = 0;

  /// AST node attribute.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Module;
};

/// Helper function of logging error when loading.
inline auto logLoadError(ErrCode Code, uint64_t Off, ASTNodeAttr Node) {
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoLoading(Off));
  spdlog::error(ErrInfo::InfoAST(Node));
  return Unexpect(Code);
}

/// Helper function of logging error when needing enabling proposal.
inline auto logNeedProposal(ErrCode Code, Proposal Prop, uint64_t Off,
                            ASTNodeAttr Node) {
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoProposal(Prop));
  spdlog::error(ErrInfo::InfoLoading(Off));
  spdlog::error(ErrInfo::InfoAST(Node));
  return Unexpect(Code);
}

/// Helper function of checking the valid value types.
inline Expect<ValType> checkValTypeProposals(const Configure &Conf,
                                             ValType VType, uint64_t Off,
                                             ASTNodeAttr Node) {
  if (VType == ValType::V128 && !Conf.hasProposal(Proposal::SIMD)) {
    return logNeedProposal(ErrCode::MalformedValType, Proposal::SIMD, Off,
                           Node);
  }
  if ((VType == ValType::FuncRef &&
       !Conf.hasProposal(Proposal::ReferenceTypes) &&
       !Conf.hasProposal(Proposal::BulkMemoryOperations)) ||
      (VType == ValType::ExternRef &&
       !Conf.hasProposal(Proposal::ReferenceTypes))) {
    return logNeedProposal(ErrCode::MalformedElemType, Proposal::ReferenceTypes,
                           Off, Node);
  }
  switch (VType) {
  case ValType::None:
  case ValType::I32:
  case ValType::I64:
  case ValType::F32:
  case ValType::F64:
  case ValType::V128:
  case ValType::ExternRef:
  case ValType::FuncRef:
    return VType;
  default:
    return logLoadError(ErrCode::MalformedValType, Off, Node);
  }
}

/// Helper function of checking the valid reference types.
inline Expect<RefType> checkRefTypeProposals(const Configure &Conf,
                                             RefType RType, uint64_t Off,
                                             ASTNodeAttr Node) {
  switch (RType) {
  case RefType::ExternRef:
    if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::MalformedElemType,
                             Proposal::ReferenceTypes, Off, Node);
    }
    [[fallthrough]];
  case RefType::FuncRef:
    return RType;
  default:
    if (Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logLoadError(ErrCode::MalformedRefType, Off, Node);
    } else {
      return logLoadError(ErrCode::MalformedElemType, Off, Node);
    }
  }
}

} // namespace AST
} // namespace WasmEdge
