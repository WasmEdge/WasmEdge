// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

// Instantiate data instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StackManager &StackMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::DataSection &DataSec) {
  // A frame with the current module has been pushed into the stack outside.

  // Iterate through the data segments to instantiate data instances.
  for (const auto &DataSeg : DataSec.getContent()) {
    addr_t Offset = 0;
    // Initialize memory if the data mode is active.
    if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
      // Run initialize expression.
      EXPECTED_TRY(runExpression(StackMgr, DataSeg.getExpr().getInstrs())
                       .map_error([](auto E) {
                         spdlog::error(
                             ErrInfo::InfoAST(ASTNodeAttr::Expression));
                         spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
                         return E;
                       }));
      // Get memory instance and address type.
      // Memory64 proposal is checked in validation phase.
      auto *MemInst = getMemInstByIdx(StackMgr, DataSeg.getIdx());
      assuming(MemInst);
      Offset = extractAddr(StackMgr.pop(),
                           MemInst->getMemoryType().getLimit().getAddrType());

      // Check boundary unless ReferenceTypes or BulkMemoryOperations proposal
      // enabled.
      if (unlikely(!Conf.hasProposal(Proposal::ReferenceTypes) &&
                   !Conf.hasProposal(Proposal::BulkMemoryOperations))) {
        // Check data fits.
        if (!MemInst->checkAccessBound(Offset, DataSeg.getData().size())) {
          spdlog::error(ErrCode::Value::DataSegDoesNotFit);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
          return Unexpect(ErrCode::Value::DataSegDoesNotFit);
        }
      }
    }

    // Create and add the data instance into the module instance.
    ModInst.addData(Offset, DataSeg.getData());
  }
  return {};
}

// Initialize memory with Data section. See "include/executor/executor.h".
Expect<void> Executor::initMemory(Runtime::StackManager &StackMgr,
                                  const AST::DataSection &DataSec) {
  // initialize memory.
  uint32_t Idx = 0;
  for (const auto &DataSeg : DataSec.getContent()) {
    // Initialize memory if data mode is active.
    if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
      // Memory and data index are checked in validation phase.
      auto *MemInst = getMemInstByIdx(StackMgr, DataSeg.getIdx());
      assuming(MemInst);
      auto *DataInst = getDataInstByIdx(StackMgr, Idx);
      assuming(DataInst);
      const uint64_t Off = DataInst->getOffset();

      // Replace mem[Off : Off + n] with data[0 : n].
      EXPECTED_TRY(MemInst
                       ->setBytes(DataInst->getData(), Off, 0,
                                  DataInst->getData().size())
                       .map_error([](auto E) {
                         spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
                         return E;
                       }));

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
