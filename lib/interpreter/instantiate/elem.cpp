// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"
#include "support/log.h"

namespace SSVM {
namespace Interpreter {

/// Calculate offsets of table initializations.
Expect<std::vector<uint32_t>>
Interpreter::resolveExpression(Runtime::StoreManager &StoreMgr,
                               Runtime::Instance::ModuleInstance &ModInst,
                               const AST::ElementSection &ElemSec) {
  std::vector<uint32_t> Offsets;
  /// Iterate and evaluate offsets.
  for (const auto &ElemSeg : ElemSec.getContent()) {
    /// Run initialize expression.
    if (auto Res = runExpression(StoreMgr, ElemSeg->getInstrs()); !Res) {
      return Unexpect(Res);
    }

    /// Pop result from stack.
    ValVariant PopVal = StackMgr.pop();
    uint32_t Offset = retrieveValue<uint32_t>(PopVal);

    /// Get table instance.
    uint32_t TabAddr = *ModInst.getTableAddr(ElemSeg->getIdx());
    auto *TabInst = *StoreMgr.getTable(TabAddr);

    /// Check offset bound.
    if (!TabInst->checkAccessBound(Offset, ElemSeg->getFuncIdxes().size())) {
      LOG(ERROR) << ErrCode::ElemSegDoesNotFit;
      LOG(ERROR) << ErrInfo::InfoBoundary(
          Offset, ElemSeg->getFuncIdxes().size(), TabInst->getMin() - 1);
      LOG(ERROR) << ErrInfo::InfoAST(ElemSeg->NodeAttr);
      LOG(ERROR) << ErrInfo::InfoAST(ElemSec.NodeAttr);
      return Unexpect(ErrCode::ElemSegDoesNotFit);
    }
    Offsets.push_back(Offset);
  }
  return Offsets;
}

/// Initialize table instance. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiate(
    Runtime::StoreManager &StoreMgr, Runtime::Instance::ModuleInstance &ModInst,
    const AST::ElementSection &ElemSec, Span<const uint32_t> Offsets) {
  auto ItElemSeg = ElemSec.getContent().begin();
  auto ItOffset = Offsets.begin();
  while (ItOffset != Offsets.end()) {
    /// Get table instance.
    uint32_t TabAddr = *ModInst.getTableAddr((*ItElemSeg)->getIdx());
    auto *TabInst = *StoreMgr.getTable(TabAddr);

    /// Transfer function index to address and copy data to table instance.
    std::vector<uint32_t> FuncIdxList;
    FuncIdxList.reserve((*ItElemSeg)->getFuncIdxes().size());
    for (auto &Idx : (*ItElemSeg)->getFuncIdxes()) {
      FuncIdxList.push_back(*ModInst.getFuncAddr(Idx));
    }
    TabInst->setInitList(*ItOffset, FuncIdxList);

    ++ItElemSeg;
    ++ItOffset;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
