// SPDX-License-Identifier: Apache-2.0
#include "ast/section.h"
#include "common/log.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"

namespace SSVM {
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
      if (auto Res = runExpression(StoreMgr, DataSeg.getInstrs()); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
        LOG(ERROR) << ErrInfo::InfoAST(DataSeg.NodeAttr);
        return Unexpect(Res);
      }
      Offset = retrieveValue<uint32_t>(StackMgr.pop());

      /// Check boundary unless ReferenceTypes or BulkMemoryOperations proposal
      /// enabled.
      if (!PConf.hasProposal(Proposal::ReferenceTypes) &&
          !PConf.hasProposal(Proposal::BulkMemoryOperations)) {
        /// Memory index should be 0. Checked in validation phase.
        auto *MemInst = getMemInstByIdx(StoreMgr, DataSeg.getIdx());
        /// Check data fits.
        if (!MemInst->checkAccessBound(Offset, DataSeg.getData().size())) {
          LOG(ERROR) << ErrCode::DataSegDoesNotFit;
          LOG(ERROR) << ErrInfo::InfoAST(DataSeg.NodeAttr);
          return Unexpect(ErrCode::DataSegDoesNotFit);
        }
      }
    }

    /// Make a new data instance.
    auto NewDataInst = std::make_unique<Runtime::Instance::DataInstance>(
        Offset, DataSeg.getData());

    /// Insert data instance to store manager.
    uint32_t NewDataInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewDataInstAddr = StoreMgr.pushData(std::move(NewDataInst));
    } else {
      NewDataInstAddr = StoreMgr.importData(std::move(NewDataInst));
    }
    ModInst.addDataAddr(NewDataInstAddr);
  }
  return {};
}

/// Initialize memory with Data Instances. See
/// "include/interpreter/interpreter.h".
Expect<void> Interpreter::initMemory(Runtime::StoreManager &StoreMgr,
                                     Runtime::Instance::ModuleInstance &ModInst,
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
      if (auto Res = MemInst->setBytes(DataInst->getData(), Off, 0,
                                       DataInst->getData().size());
          !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(DataSeg.NodeAttr);
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
} // namespace SSVM
