// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

#include "common/errinfo.h"
#include "common/log.h"

namespace WasmEdge {
namespace Interpreter {

/// Instantiate data instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::DataSection &DataSec) {
  /// A frame with module is pushed into stack outside.
  /// Instantiate data instances.
  for (const auto &DataSeg : DataSec.getContent()) {
    uint32_t Offset = 0;
    /// Initialize memory if data mode is active.
    if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
      /// Run initialize expression.
      if (auto Res = runExpression(StoreMgr, DataSeg.getExpr().getInstrs());
          !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
        return Unexpect(Res);
      }
      Offset = StackMgr.pop().get<uint32_t>();

      /// Check boundary unless ReferenceTypes or BulkMemoryOperations proposal
      /// enabled.
      if (!Conf.hasProposal(Proposal::ReferenceTypes) &&
          !Conf.hasProposal(Proposal::BulkMemoryOperations)) {
        /// Memory index should be 0. Checked in validation phase.
        auto *MemInst = getMemInstByIdx(StoreMgr, DataSeg.getIdx());
        /// Check data fits.
        if (!MemInst->checkAccessBound(
                Offset, static_cast<uint32_t>(DataSeg.getData().size()))) {
          spdlog::error(ErrCode::DataSegDoesNotFit);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
          return Unexpect(ErrCode::DataSegDoesNotFit);
        }
      }
    }

    /// Insert data instance to store manager.
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

/// Initialize memory with Data Instances. See
/// "include/interpreter/interpreter.h".
Expect<void> Interpreter::initMemory(Runtime::StoreManager &StoreMgr,
                                     Runtime::Instance::ModuleInstance &,
                                     const AST::DataSection &DataSec) {
  /// initialize memory.
  uint32_t Idx = 0;
  for (const auto &DataSeg : DataSec.getContent()) {
    auto *DataInst = getDataInstByIdx(StoreMgr, Idx);

    /// Initialize memory if data mode is active.
    if (DataSeg.getMode() == AST::DataSegment::DataMode::Active) {
      /// Memory index should be 0. Checked in validation phase.
      auto *MemInst = getMemInstByIdx(StoreMgr, DataSeg.getIdx());
      const uint32_t Off = DataInst->getOffset();

      /// Replace mem[Off : Off + n] with data[0 : n].
      if (auto Res = MemInst->setBytes(
              DataInst->getData(), Off, 0,
              static_cast<uint32_t>(DataInst->getData().size()));
          !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
        return Unexpect(Res);
      }

      /// Drop the data instance.
      DataInst->clear();

      /// Operation above is equal to the following instruction sequence:
      ///   expr(init) -> i32.const off
      ///   i32.const 0
      ///   i32.const n
      ///   memory.init idx
      ///   data.drop idx
    }
    Idx++;
  }
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
