// SPDX-License-Identifier: Apache-2.0
#include "host/ethereum/eeimodule.h"
#include "host/ethereum/eeifunc.h"

#include <memory>

namespace SSVM {
namespace Host {

EEIModule::EEIModule(uint64_t &CostLimit, uint64_t &CostSum)
    : ImportObject("ethereum"), Env(CostLimit, CostSum) {
  addHostFunc("call", std::make_unique<EEICall>(Env));
  addHostFunc("callCode", std::make_unique<EEICallCode>(Env));
  addHostFunc("callDataCopy", std::make_unique<EEICallDataCopy>(Env));
  addHostFunc("callDelegate", std::make_unique<EEICallDelegate>(Env));
  addHostFunc("callStatic", std::make_unique<EEICallStatic>(Env));
  addHostFunc("codeCopy", std::make_unique<EEICodeCopy>(Env));
  addHostFunc("create", std::make_unique<EEICreate>(Env));
  addHostFunc("externalCodeCopy", std::make_unique<EEIExternalCodeCopy>(Env));
  addHostFunc("finish", std::make_unique<EEIFinish>(Env));
  addHostFunc("getAddress", std::make_unique<EEIGetAddress>(Env));
  addHostFunc("getBlockCoinbase", std::make_unique<EEIGetBlockCoinbase>(Env));
  addHostFunc("getBlockDifficulty",
              std::make_unique<EEIGetBlockDifficulty>(Env));
  addHostFunc("getBlockGasLimit", std::make_unique<EEIGetBlockGasLimit>(Env));
  addHostFunc("getBlockHash", std::make_unique<EEIGetBlockHash>(Env));
  addHostFunc("getBlockNumber", std::make_unique<EEIGetBlockNumber>(Env));
  addHostFunc("getBlockTimestamp", std::make_unique<EEIGetBlockTimestamp>(Env));
  addHostFunc("getCallDataSize", std::make_unique<EEIGetCallDataSize>(Env));
  addHostFunc("getCallValue", std::make_unique<EEIGetCallValue>(Env));
  addHostFunc("getCaller", std::make_unique<EEIGetCaller>(Env));
  addHostFunc("getCodeSize", std::make_unique<EEIGetCodeSize>(Env));
  addHostFunc("getExternalBalance",
              std::make_unique<EEIGetExternalBalance>(Env));
  addHostFunc("getExternalCodeSize",
              std::make_unique<EEIGetExternalCodeSize>(Env));
  addHostFunc("getGasLeft", std::make_unique<EEIGetGasLeft>(Env));
  addHostFunc("getReturnDataSize", std::make_unique<EEIGetReturnDataSize>(Env));
  addHostFunc("getTxGasPrice", std::make_unique<EEIGetTxGasPrice>(Env));
  addHostFunc("getTxOrigin", std::make_unique<EEIGetTxOrigin>(Env));
  addHostFunc("log", std::make_unique<EEILog>(Env));
  addHostFunc("returnDataCopy", std::make_unique<EEIReturnDataCopy>(Env));
  addHostFunc("revert", std::make_unique<EEIRevert>(Env));
  addHostFunc("selfDestruct", std::make_unique<EEISelfDestruct>(Env));
  addHostFunc("storageLoad", std::make_unique<EEIStorageLoad>(Env));
  addHostFunc("storageStore", std::make_unique<EEIStorageStore>(Env));
  addHostFunc("useGas", std::make_unique<EEIUseGas>(Env));
}

} // namespace Host
} // namespace SSVM