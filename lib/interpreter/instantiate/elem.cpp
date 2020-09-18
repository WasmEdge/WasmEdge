// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"
#include "support/log.h"

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

    /// Make a new element instance.
    auto NewElemInst = std::make_unique<Runtime::Instance::ElementInstance>(
        ElemSeg->getRefType(), InitVals);

    /// Insert element instance to store manager.
    uint32_t NewElemInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewElemInstAddr = StoreMgr.pushElement(NewElemInst);
    } else {
      NewElemInstAddr = StoreMgr.importElement(NewElemInst);
    }
    ModInst.addElemAddr(NewElemInstAddr);
  }
  return {};
}

/// Initialize table with Element Instances.
Expect<void> Interpreter::initTable(Runtime::StoreManager &StoreMgr,
                                    Runtime::Instance::ModuleInstance &ModInst,
                                    const AST::ElementSection &ElemSec) {
  /// A frame with module is pushed into stack outside.
  /// Initialize tables.
  uint32_t Idx = 0;
  for (const auto &ElemSeg : ElemSec.getContent()) {
    auto *ElemInst = getElemInstByIdx(StoreMgr, Idx);
    if (ElemSeg->getMode() == AST::ElementSegment::ElemMode::Active) {
      /// Table index should be 0. Checked in validation phase.
      auto *TabInst = getTabInstByIdx(StoreMgr, ElemSeg->getIdx());

      /// Run initialize expression.
      if (auto Res = runExpression(StoreMgr, ElemSeg->getInstrs()); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
        LOG(ERROR) << ErrInfo::InfoAST(ElemSeg->NodeAttr);
        return Unexpect(Res);
      }
      uint32_t Off = retrieveValue<uint32_t>(StackMgr.pop());

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
