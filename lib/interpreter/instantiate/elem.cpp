// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"

namespace SSVM {
namespace Interpreter {

/// Initialize table instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::ElementSection &ElemSec) {
  /// Iterate and evaluate offsets.
  for (const auto &ElemSeg : ElemSec.getContent()) {
    /// Run initialize expression.
    if (auto Res = runExpression(StoreMgr, ElemSeg->getInstrs()); !Res) {
      return Unexpect(Res);
    }

    /// Pop result from stack.
    ValVariant PopVal = StackMgr.pop();
    uint32_t Offset = retrieveValue<uint32_t>(PopVal);

    /// Get table instance
    uint32_t TabAddr = *ModInst.getTableAddr(ElemSeg->getIdx());
    auto *TabInst = *StoreMgr.getTable(TabAddr);

    /// Transfer function index to address and copy data to table instance
    auto FuncIdxList = ElemSeg->getFuncIdxes();
    for (auto &Idx : FuncIdxList) {
      uint32_t FuncAddr = *ModInst.getFuncAddr(Idx);
      Idx = FuncAddr;
    }
    if (auto Res = TabInst->setInitList(Offset, FuncIdxList); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
