// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Executor {

// Instantiate element instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::ElementSection &ElemSec) {
  // A frame with the current module has been pushed into the stack outside.

  // Iterate through the element segments to instantiate element instances.
  for (const auto &ElemSeg : ElemSec.getContent()) {
    std::vector<RefVariant> InitVals;
    for (const auto &Expr : ElemSeg.getInitExprs()) {
      // Run init expr of every elements and get the result reference.
      EXPECTED_TRY(
          runExpression(StackMgr, Expr.getInstrs()).map_error([](auto E) {
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
            return E;
          }));
      // Pop result from stack.
      InitVals.push_back(StackMgr.pop().get<RefVariant>());
    }

    uint64_t Offset = 0;
    if (ElemSeg.getMode() == AST::ElementSegment::ElemMode::Active) {
      // Run initialize expression.
      EXPECTED_TRY(
          runExpression(StackMgr, ElemSeg.getExpr().getInstrs())
              .map_error([](auto E) {
                spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
                spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
                return E;
              }));
      // Get table instance and address type.
      // Memory64 proposal is checked in validation phase.
      auto *TabInst = getTabInstByIdx(StackMgr, ElemSeg.getIdx());
      assuming(TabInst);
      Offset = extractAddr(StackMgr.pop(),
                           TabInst->getTableType().getLimit().getAddrType());

      // Check boundary unless ReferenceTypes or BulkMemoryOperations proposal
      // enabled.
      if (unlikely(!Conf.hasProposal(Proposal::ReferenceTypes) &&
                   !Conf.hasProposal(Proposal::BulkMemoryOperations))) {
        // Check elements fits.
        if (!TabInst->checkAccessBound(Offset, InitVals.size())) {
          spdlog::error(ErrCode::Value::ElemSegDoesNotFit);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
          return Unexpect(ErrCode::Value::ElemSegDoesNotFit);
        }
      }
    }

    // Create and add the element instance into the module instance.
    ModInst.addElem(Offset, ElemSeg.getRefType(), InitVals);
  }
  return {};
}

// Initialize table with Element section. See "include/executor/executor.h".
Expect<void> Executor::initTable(Runtime::StackManager &StackMgr,
                                 const AST::ElementSection &ElemSec) {
  // Initialize tables.
  uint32_t Idx = 0;
  for (const auto &ElemSeg : ElemSec.getContent()) {
    // Element index is checked in validation phase.
    auto *ElemInst = getElemInstByIdx(StackMgr, Idx);
    assuming(ElemInst);
    if (ElemSeg.getMode() == AST::ElementSegment::ElemMode::Active) {
      // Table index is checked in validation phase.
      auto *TabInst = getTabInstByIdx(StackMgr, ElemSeg.getIdx());
      assuming(TabInst);
      const uint64_t Off = ElemInst->getOffset();

      // Replace table[Off : Off + n] with elem[0 : n].
      EXPECTED_TRY(
          TabInst
              ->setRefs(ElemInst->getRefs(), Off, 0, ElemInst->getRefs().size())
              .map_error([](auto E) {
                spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
                return E;
              }));

      // Drop the element instance.
      ElemInst->clear();

      // Operation above is equal to the following instruction sequence:
      //   expr(init) -> i32.const off
      //   i32.const 0
      //   i32.const n
      //   table.init idx
      //   elem.drop idx
    } else if (ElemSeg.getMode() ==
               AST::ElementSegment::ElemMode::Declarative) {
      // Drop the element instance.
      ElemInst->clear();
    }
    Idx++;
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
