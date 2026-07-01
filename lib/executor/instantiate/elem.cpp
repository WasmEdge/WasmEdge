// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
    // Evaluate every init expression, leaving each result on the (GC-rooted)
    // value stack, not an unrooted local: a later init expr may run
    // struct.new/array.new and trigger a collection that sweeps earlier
    // results.
    const auto &InitExprs = ElemSeg.getInitExprs();
    for (const auto &Expr : InitExprs) {
      // Run the init expr; leave its result reference on the stack.
      EXPECTED_TRY(
          runExpression(StackMgr, Expr.getInstrs()).map_error([](auto E) {
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
            return E;
          }));
    }
    // Snapshot the still-rooted results into InitVals, then drop them.
    // Detaching is safe mid-collection: the refs were on the GC-scanned stack
    // at every root scan so far, so SATB already shaded them. The active-offset
    // expression below is a non-allocating const expression: no new cycle.
    const uint32_t InitCount = static_cast<uint32_t>(InitExprs.size());
    std::vector<RefVariant> InitVals;
    InitVals.reserve(InitCount);
    for (const auto &Val : StackMgr.getTopSpan(InitCount)) {
      InitVals.push_back(Val.get<RefVariant>());
    }
    StackMgr.eraseValueStack(InitCount, 0);

    uint64_t Offset = 0;
    if (ElemSeg.getMode() == AST::ElementSegment::ElemMode::Active) {
      // Run the initialization expression.
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
      Offset = extractAddr(StackMgr.pop<ValVariant>(),
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

    // Add the element instance; addElem registers it with the GC allocator so
    // its references are scanned as roots.
    ModInst.addElem(Allocator, Offset, ElemSeg.getRefType(), InitVals);
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
          TabInst->setRefs(ElemInst->getRefs(), Off).map_error([](auto E) {
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
