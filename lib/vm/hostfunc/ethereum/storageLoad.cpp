// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/storageLoad.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEIStorageLoad::body(VM::EnvironmentManager &EnvMgr,
                             Instance::MemoryInstance &MemInst,
                             uint32_t PathOffset, uint32_t ValueOffset) {
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }

  /// Get Path data by path offset.
  std::vector<unsigned char> Data;
  if (ErrCode Status = MemInst.getBytes(Data, PathOffset, 32);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Get Value data in storage by key of path.
  std::string Path;
  std::string Value;
  std::map<std::string, std::string> &Storage = Env.getStorage();
  Support::convertHexToString(Data, Path, 64);
  if (auto Iter = Storage.find(Path); Iter != Storage.end()) {
    Value = Iter->second;
  } else {
    Value.resize(64, '0');
  }

  /// Set Value data to memory.
  Data.clear();
  Support::convertStringToHex(Value, Data, 64);
  if (ErrCode Status = MemInst.setBytes(Data, ValueOffset, 0, 32);
      Status != ErrCode::Success) {
    return Status;
  }

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
