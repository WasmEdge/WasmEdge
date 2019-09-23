#include "executor/instance/table.h"
#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Instantiate table instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::TableSection *TabSec,
                              AST::ElementSection *ElemSec) {
  if (TabSec == nullptr) {
    return ErrCode::Success;
  }
  ErrCode Status = ErrCode::Success;

  /// Iterate and istantiate table types.
  auto &TabTypes = TabSec->getContent();
  for (auto TabType = TabTypes.begin(); TabType != TabTypes.end(); TabType++) {
    /// Make a new table instance.
    auto NewTabInst = std::make_unique<Instance::TableInstance>();
    unsigned int NewTabInstId = 0;

    /// Set table instance data.
    auto ElemType = (*TabType)->getElementType();
    auto &Limit = (*TabType)->getLimit();
    if ((Status = NewTabInst->setElemType(ElemType)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = NewTabInst->setLimit(Limit->getMin(), Limit->hasMax(),
                                       Limit->getMax())) != ErrCode::Success) {
      return Status;
    }

    /// Insert table instance to store manager.
    if ((Status = StoreMgr.insertTableInst(NewTabInst, NewTabInstId)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Set external value (table address) to module instance.
    if ((Status = ModInst->addTableAddr(NewTabInstId)) != ErrCode::Success) {
      return Status;
    }
  }

  /// Iterate and evaluate element segments.
  auto &ElemSegs = ElemSec->getContent();
  for (auto ElemSeg = ElemSegs.begin(); ElemSeg != ElemSegs.end(); ElemSeg++) {
    /// Evaluate instrs in element segment for offset.
    if ((Status = Engine.runExpression((*ElemSeg)->getInstrs())) !=
        ErrCode::Success) {
      return Status;
    }

    /// Pop the result for offset.
    std::unique_ptr<ValueEntry> PopVal;
    StackMgr.pop(PopVal);
    uint32_t Offset = 0;
    if ((Status = PopVal->getValue(Offset)) != ErrCode::Success) {
      return Status;
    }

    /// Get table instance
    Instance::TableInstance *TabInst = nullptr;
    unsigned int TabAddr = 0;
    if ((Status = ModInst->getMemAddr((*ElemSeg)->getIdx(), TabAddr)) !=
        ErrCode::Success) {
      return Status;
    }
    if ((Status = StoreMgr.getTable(TabAddr, TabInst)) != ErrCode::Success) {
      return Status;
    }

    /// Copy data to table instance
    TabInst->setInitList(Offset, (*ElemSeg)->getFuncIdxes());
  }

  return Status;
}

} // namespace Executor
} // namespace SSVM