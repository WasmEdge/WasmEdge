// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"

namespace SSVM {
namespace Interpreter {

/// Calculate offsets of table initializations.
Expect<std::vector<uint32_t>>
Interpreter::resolveExpression(Runtime::StoreManager &StoreMgr,
                               Runtime::Instance::ModuleInstance &ModInst,
                               const AST::DataSection &DataSec) {
  std::vector<uint32_t> Offsets;
  /// Iterate and evaluate offsets.
  for (const auto &DataSeg : DataSec.getContent()) {
    /// Run initialize expression.
    if (auto Res = runExpression(StoreMgr, DataSeg->getInstrs()); !Res) {
      return Unexpect(Res);
    }

    /// Pop result from stack.
    ValVariant PopVal = StackMgr.pop();
    uint32_t Offset = retrieveValue<uint32_t>(PopVal);

    /// Get memory instance.
    uint32_t MemAddr = *ModInst.getMemAddr(DataSeg->getIdx());
    auto *MemInst = *StoreMgr.getMemory(MemAddr);

    /// Check offset bound.
    if (!MemInst->checkAccessBound(Offset + DataSeg->getData().size())) {
      return Unexpect(ErrCode::DataSegDoesNotFit);
    }
    Offsets.push_back(Offset);
  }
  return std::move(Offsets);
}

/// Initialize memory instance. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiate(
    Runtime::StoreManager &StoreMgr, Runtime::Instance::ModuleInstance &ModInst,
    const AST::DataSection &DataSec, const std::vector<uint32_t> &Offsets) {
  auto ItDataSeg = DataSec.getContent().cbegin();
  auto ItOffset = Offsets.cbegin();
  while (ItOffset != Offsets.cend()) {
    /// Get memory instance.
    uint32_t MemAddr = *ModInst.getMemAddr((*ItDataSeg)->getIdx());
    auto *MemInst = *StoreMgr.getMemory(MemAddr);

    /// Copy data to memory instance.
    const auto &Data = (*ItDataSeg)->getData();
    if (auto Res = MemInst->setBytes(Data, *ItOffset, 0, Data.size()); !Res) {
      return Unexpect(ErrCode::DataSegDoesNotFit);
    }

    ++ItDataSeg;
    ++ItOffset;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
