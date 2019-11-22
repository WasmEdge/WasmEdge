#include "executor/instance/memory.h"
#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Instantiate memory instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::MemorySection *MemSec,
                              AST::DataSection *DataSec) {
  if (MemSec == nullptr) {
    return ErrCode::Success;
  }
  ErrCode Status = ErrCode::Success;

  /// Iterate and istantiate memory types.
  auto &MemTypes = MemSec->getContent();
  for (auto MemType = MemTypes.begin(); MemType != MemTypes.end(); MemType++) {
    /// Make a new memory instance.
    auto NewMemInst = std::make_unique<Instance::MemoryInstance>();
    unsigned int NewMemInstId = 0;

    /// Set memory instance data.
    auto *Limit = (*MemType)->getLimit();
    if ((Status = NewMemInst->setLimit(Limit->getMin(), Limit->hasMax(),
                                       Limit->getMax())) != ErrCode::Success) {
      return Status;
    }

    /// Insert memory instance to store manager.
    if ((Status = StoreMgr.insertMemoryInst(NewMemInst, NewMemInstId)) !=
        ErrCode::Success) {
      return Status;
    }

    /// Set external value (memory address) to module instance.
    if ((Status = ModInst->addMemAddr(NewMemInstId)) != ErrCode::Success) {
      return Status;
    }
  }

  /// Iterate and evaluate data segments.
  if (DataSec == nullptr) {
    return ErrCode::Success;
  }
  auto &DataSegs = DataSec->getContent();
  for (auto DataSeg = DataSegs.begin(); DataSeg != DataSegs.end(); DataSeg++) {
    /// Evaluate instrs in data segment for offset.
    if ((Status = Engine.runExpression((*DataSeg)->getInstrs())) !=
        ErrCode::Success) {
      return Status;
    }

    /// Pop the result for offset.
    Value PopVal;
    StackMgr.pop(PopVal);
    uint32_t Offset = retrieveValue<uint32_t>(PopVal);

    /// Get memory instance
    Instance::MemoryInstance *MemInst = nullptr;
    unsigned int MemAddr = 0;
    if ((Status = ModInst->getMemAddr((*DataSeg)->getIdx(), MemAddr)) !=
        ErrCode::Success) {
      return Status;
    }
    if ((Status = StoreMgr.getMemory(MemAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }

    /// Copy data to memory instance
    std::vector<unsigned char> &Data = (*DataSeg)->getData();
    if ((Status = MemInst->setBytes(Data, Offset, 0, Data.size())) !=
        ErrCode::Success) {
      return Status;
    }
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM
