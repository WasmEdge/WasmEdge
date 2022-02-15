// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/log.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate data instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::StackManager &StackMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::DataSection &DataSec) {
  // A frame with module is pushed into stack outside.
  // Instantiate data instances.
  for (const auto &DataSeg : DataSec.getContent()) {
    uint32_t Offset = 0;
    // Initialize memory if data mode is active.
    if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
      // Run initialize expression.
      if (auto Res =
              runExpression(StoreMgr, StackMgr, DataSeg.getExpr().getInstrs());
          unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
        return Unexpect(Res);
      }
      Offset = StackMgr.pop().get<uint32_t>();

      // Check boundary unless ReferenceTypes or BulkMemoryOperations proposal
      // enabled.
      if (!Conf.hasProposal(Proposal::ReferenceTypes) &&
          !Conf.hasProposal(Proposal::BulkMemoryOperations)) {
        // Memory index should be 0. Checked in validation phase.
        auto *MemInst = getMemInstByIdx(StoreMgr, StackMgr, DataSeg.getIdx());
        // Check data fits.
        assuming(MemInst);
        if (!MemInst->checkAccessBound(
                Offset, static_cast<uint32_t>(DataSeg.getData().size()))) {
          spdlog::error(ErrCode::DataSegDoesNotFit);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
          return Unexpect(ErrCode::DataSegDoesNotFit);
        }
      }
    }

    // Insert data instance to store manager.
    uint32_t NewDataInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewDataInstAddr = StoreMgr.pushData(Offset, DataSeg.getData());
    } else {
      NewDataInstAddr = StoreMgr.importData(Offset, DataSeg.getData());
    }
    ModInst.addDataAddr(NewDataInstAddr);
  }
  return {};
}

// Initialize memory with Data Instances. See
// "include/executor/executor.h".
Expect<void> Executor::initMemory(Runtime::StoreManager &StoreMgr,
                                  Runtime::StackManager &StackMgr,
                                  Runtime::Instance::ModuleInstance &,
                                  const AST::DataSection &DataSec) {
  // initialize memory.
  uint32_t Idx = 0;
  for (const auto &DataSeg : DataSec.getContent()) {
    // Initialize memory if data mode is active.
    if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
      // Memory index should be 0. Checked in validation phase.
      auto *MemInst = getMemInstByIdx(StoreMgr, StackMgr, DataSeg.getIdx());
      assuming(MemInst);

      auto *DataInst = getDataInstByIdx(StoreMgr, StackMgr, Idx);
      assuming(DataInst);
      const uint32_t Off = DataInst->getOffset();

      // Replace mem[Off : Off + n] with data[0 : n].
      if (auto Res = MemInst->setBytes(
              DataInst->getData(), Off, 0,
              static_cast<uint32_t>(DataInst->getData().size()));
          !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
        return Unexpect(Res);
      }

      // Drop the data instance.
      DataInst->clear();

      // Operation above is equal to the following instruction sequence:
      //   expr(init) -> i32.const off
      //   i32.const 0
      //   i32.const n
      //   memory.init idx
      //   data.drop idx
    }
    Idx++;
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
