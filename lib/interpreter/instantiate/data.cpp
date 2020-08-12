// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "support/log.h"

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
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
      LOG(ERROR) << ErrInfo::InfoAST(DataSeg->NodeAttr);
      return Unexpect(Res);
    }

    /// Pop result from stack.
    ValVariant PopVal = StackMgr.pop();
    uint32_t Offset = retrieveValue<uint32_t>(PopVal);

    /// Get memory instance.
    uint32_t MemAddr = *ModInst.getMemAddr(DataSeg->getIdx());
    auto *MemInst = *StoreMgr.getMemory(MemAddr);

    /// Check offset bound.
    if (!MemInst->checkAccessBound(Offset, DataSeg->getData().size())) {
      LOG(ERROR) << ErrCode::DataSegDoesNotFit;
      LOG(ERROR) << ErrInfo::InfoBoundary(Offset, DataSeg->getData().size(),
                                          MemInst->getBoundIdx());
      LOG(ERROR) << ErrInfo::InfoAST(DataSeg->NodeAttr);
      return Unexpect(ErrCode::DataSegDoesNotFit);
    }
    Offsets.push_back(Offset);
  }
  return Offsets;
}

/// Initialize memory instance. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiate(
    Runtime::StoreManager &StoreMgr, Runtime::Instance::ModuleInstance &ModInst,
    const AST::DataSection &DataSec, Span<const uint32_t> Offsets) {
  auto ItDataSeg = DataSec.getContent().begin();
  auto ItOffset = Offsets.begin();
  while (ItOffset != Offsets.end()) {
    /// Get memory instance.
    uint32_t MemAddr = *ModInst.getMemAddr((*ItDataSeg)->getIdx());
    auto *MemInst = *StoreMgr.getMemory(MemAddr);

    /// Copy data to memory instance. Boundary checked in resolving expression.
    const auto &Data = (*ItDataSeg)->getData();
    MemInst->setBytes(Data, *ItOffset, 0, Data.size());

    ++ItDataSeg;
    ++ItOffset;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
