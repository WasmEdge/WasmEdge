#include "vm/hostfunc/ethereum/getcalldatasize.h"
#include "executor/common.h"
#include "stdint.h"

namespace SSVM {
namespace Executor {

EEIGetCallDataSize::EEIGetCallDataSize(VM::Environment &Env) : EEI(Env) {
  appendReturnDef(AST::ValType::I32);
}

ErrCode EEIGetCallDataSize::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                                std::vector<std::unique_ptr<ValueEntry>> &Res,
                                StoreManager &Store,
                                Instance::ModuleInstance *ModInst) {
  /// Arg: void
  if (Args.size() != 0) {
    return ErrCode::CallFunctionError;
  }

  /// Return: Length(u32)
  Res.push_back(std::make_unique<ValueEntry>(
      static_cast<uint32_t>(Env.getCallData().size())));
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM