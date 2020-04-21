// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"

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
    if (!TabInst->checkAccessBound(Offset + ElemSeg->getFuncIdxes().size())) {
      return Unexpect(ErrCode::AccessForbidMemory);
    }
    Offsets.push_back(Offset);
  }
  return std::move(Offsets);
}

/// Initialize table instance. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiate(
    Runtime::StoreManager &StoreMgr, Runtime::Instance::ModuleInstance &ModInst,
    const AST::ElementSection &ElemSec, const std::vector<uint32_t> &Offsets) {
  auto ItElemSeg = ElemSec.getContent().cbegin();
  auto ItOffset = Offsets.cbegin();
  while (ItOffset != Offsets.cend()) {
    /// Get table instance.
    uint32_t TabAddr = *ModInst.getTableAddr((*ItElemSeg)->getIdx());
    auto *TabInst = *StoreMgr.getTable(TabAddr);

    /// Transfer function index to address and copy data to table instance.
    auto FuncIdxList = (*ItElemSeg)->getFuncIdxes();
    for (auto &Idx : FuncIdxList) {
      uint32_t FuncAddr = *ModInst.getFuncAddr(Idx);
      Idx = FuncAddr;
    }
    if (auto Res = TabInst->setInitList(*ItOffset, FuncIdxList); !Res) {
      return Unexpect(Res);
    }

    ++ItElemSeg;
    ++ItOffset;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
