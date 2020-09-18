// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "support/log.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate data instance. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::DataSection &DataSec) {
  /// A frame with module is pushed into stack outside.
  /// Instantiate data instances and initialize memory.
  for (const auto &DataSeg : DataSec.getContent()) {
    /// Make a new data instance.
    auto NewDataInst =
        std::make_unique<Runtime::Instance::DataInstance>(DataSeg->getData());

    /// Insert data instance to store manager.
    uint32_t NewDataInstAddr;
    if (InsMode == InstantiateMode::Instantiate) {
      NewDataInstAddr = StoreMgr.pushData(NewDataInst);
    } else {
      NewDataInstAddr = StoreMgr.importData(NewDataInst);
    }
    ModInst.addDataAddr(NewDataInstAddr);

    /// Initialize memory if data mode is active.
    if (DataSeg->getMode() == AST::DataSegment::DataMode::Active) {
      /// Memory index should be 0. Checked in validation phase.
      auto *MemInst = getMemInstByIdx(StoreMgr, DataSeg->getIdx());
      auto *DataInst = *StoreMgr.getData(NewDataInstAddr);

      /// Run initialize expression.
      if (auto Res = runExpression(StoreMgr, DataSeg->getInstrs()); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Expression);
        LOG(ERROR) << ErrInfo::InfoAST(DataSeg->NodeAttr);
        return Unexpect(Res);
      }
      uint32_t Off = retrieveValue<uint32_t>(StackMgr.pop());

      /// Replace mem[Off : Off + n] with data[0 : n].
      if (auto Res = MemInst->setBytes(DataInst->getData(), Off, 0,
                                       DataInst->getData().size());
          !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(DataSeg->NodeAttr);
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
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
