// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"

namespace SSVM {
namespace Interpreter {

/// Initialize memory instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::DataSection &DataSec) {
  /// Iterate and evaluate offsets.
  for (const auto &DataSeg : DataSec.getContent()) {
    /// Run initialize expression.
    if (auto Res = runExpression(StoreMgr, DataSeg->getInstrs()); !Res) {
      return Unexpect(Res);
    }

    /// Pop result from stack.
    ValVariant PopVal = StackMgr.pop();
    uint32_t Offset = retrieveValue<uint32_t>(PopVal);

    /// Get table instance
    uint32_t MemAddr = *ModInst.getMemAddr(DataSeg->getIdx());
    auto *MemInst = *StoreMgr.getMemory(MemAddr);

    /// Copy data to memory instance
    const auto &Data = DataSeg->getData();
    if (auto Res = MemInst->setBytes(Data, Offset, 0, Data.size()); !Res) {
      return Unexpect(Res);
    }
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
