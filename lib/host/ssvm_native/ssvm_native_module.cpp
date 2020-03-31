// SPDX-License-Identifier: Apache-2.0
#include "host/ssvm_native/ssvm_native_module.h"
#include "host/ssvm_native/ssvm_native_func.h"

#include <memory>

namespace SSVM {
namespace Host {

SSVMNativeModule::SSVMNativeModule() : ImportObject("ssvm_native") {
  addHostFunc("ssvm_storage_createUUID",
              std::make_unique<SSVMNativeStorageCreateUUID>());
  addHostFunc("ssvm_storage_beginStoreTx",
              std::make_unique<SSVMNativeStorageBeginStoreTx>());
  addHostFunc("ssvm_storage_beginLoadTx",
              std::make_unique<SSVMNativeStorageBeginLoadTx>());
  addHostFunc("ssvm_storage_storeI32",
              std::make_unique<SSVMNativeStorageStoreI32>());
  addHostFunc("ssvm_storage_loadI32",
              std::make_unique<SSVMNativeStorageLoadI32>());
  addHostFunc("ssvm_storage_storeI64",
              std::make_unique<SSVMNativeStorageStoreI64>());
  addHostFunc("ssvm_storage_loadI64",
              std::make_unique<SSVMNativeStorageLoadI64>());
  addHostFunc("ssvm_storage_endStoreTx",
              std::make_unique<SSVMNativeStorageEndStoreTx>());
  addHostFunc("ssvm_storage_endLoadTx",
              std::make_unique<SSVMNativeStorageEndLoadTx>());
}

} // namespace Host
} // namespace SSVM