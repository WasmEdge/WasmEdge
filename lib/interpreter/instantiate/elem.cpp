// SPDX-License-Identifier: Apache-2.0
#include "ast/section.h"
#include "common/log.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate element instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::ElementSection &ElemSec) {
  /// A frame with temp. module is pushed into stack outside.
  /// Instantiate and initialize elements.
  for (const auto &ElemSeg : ElemSec.getContent()) {
    std::vector<ValVariant> InitVals;
    for (const auto &Expr : ElemSeg->getInitExprs()) {
      /// Run init expr of every elements and get the result reference.
      if (auto Res = runExpression(StoreMgr, Expr->getInstrs()); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
        LOG(ERROR) << ErrInfo::InfoAST(ElemSeg->NodeAttr);
        return Unexpect(Res);
      }
      /// Pop result from stack.
      InitVals.push_back(StackMgr.pop());
    }

    uint32_t Offset = 0;
    if (ElemSeg->getMode() == AST::ElementSegment::ElemMode::Active) {
      /// Run initialize expression.
      if (auto Res = runExpression(StoreMgr, ElemSeg->getInstrs()); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
        LOG(ERROR) << ErrInfo::InfoAST(ElemSeg->NodeAttr);
        return Unexpect(Res);
      }
      Offset = retrieveValue<uint32_t>(StackMgr.pop());

      /// Table index should be 0. Checked in validation phase.
      auto *TabInst = getTabInstByIdx(StoreMgr, ElemSeg->getIdx());
      /// Check elements fits.
      if (!TabInst->checkAccessBound(Offset, InitVals.size())) {
        LOG(ERROR) << ErrCode::ElemSegDoesNotFit;
        LOG(ERROR) << ErrInfo::InfoAST(ElemSeg->NodeAttr);
        return Unexpect(ErrCode::ElemSegDoesNotFit);
      }
    }

    /// Make a new element instance.
    auto NewElemInst = std::make_unique<Runtime::Instance::ElementInstance>(
        Offset, ElemSeg->getRefType(), InitVals);

    /// Insert element instance to store manager.
    uint32_t NewElemInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewElemInstAddr = StoreMgr.pushElement(std::move(NewElemInst));
    } else {
      NewElemInstAddr = StoreMgr.importElement(std::move(NewElemInst));
    }
    ModInst.addElemAddr(NewElemInstAddr);
  }
  return {};
}

/// Initialize table with Element Instances. See
/// "include/interpreter/interpreter.h".
Expect<void> Interpreter::initTable(Runtime::StoreManager &StoreMgr,
                                    Runtime::Instance::ModuleInstance &ModInst,
                                    const AST::ElementSection &ElemSec) {
  /// Initialize tables.
  uint32_t Idx = 0;
  for (const auto &ElemSeg : ElemSec.getContent()) {
    auto *ElemInst = getElemInstByIdx(StoreMgr, Idx);
    if (ElemSeg->getMode() == AST::ElementSegment::ElemMode::Active) {
      /// Table index should be 0. Checked in validation phase.
      auto *TabInst = getTabInstByIdx(StoreMgr, ElemSeg->getIdx());
      const uint32_t Off = ElemInst->getOffset();

      /// Replace table[Off : Off + n] with elem[0 : n].
      if (auto Res = TabInst->setRefs(ElemInst->getRefs(), Off, 0,
                                      ElemInst->getRefs().size());
          !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(ElemSeg->NodeAttr);
        return Unexpect(Res);
      }

      /// Drop the element instance.
      ElemInst->clear();

      /// Operation above is equal to the following instruction sequence:
      ///   expr(init) -> i32.const off
      ///   i32.const 0
      ///   i32.const n
      ///   table.init idx
      ///   elem.drop idx
    } else if (ElemSeg->getMode() ==
               AST::ElementSegment::ElemMode::Declarative) {
      /// Drop the element instance.
      ElemInst->clear();
    }
    Idx++;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
