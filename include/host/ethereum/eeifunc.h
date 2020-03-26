// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eeibase.h"

namespace SSVM {
namespace Host {

class EEICall : public EEI<EEICall> {
public:
  EEICall(EVMEnvironment &HostEnv) : EEI(HostEnv, 700) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint64_t Gas, uint32_t AddressOffset, uint32_t ValueOffset,
               uint32_t DataOffset, uint32_t DataLength);
};

class EEICallCode : public EEI<EEICallCode> {
public:
  EEICallCode(EVMEnvironment &HostEnv) : EEI(HostEnv, 700) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint64_t Gas, uint32_t AddressOffset, uint32_t ValueOffset,
               uint32_t DataOffset, uint32_t DataLength);
};

class EEICallDataCopy : public EEI<EEICallDataCopy> {
public:
  EEICallDataCopy(EVMEnvironment &HostEnv) : EEI(HostEnv, 3) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset, uint32_t DataOffset, uint32_t Length);
};

class EEICallDelegate : public EEI<EEICallDelegate> {
public:
  EEICallDelegate(EVMEnvironment &HostEnv) : EEI(HostEnv, 700) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint64_t Gas, uint32_t AddressOffset, uint32_t DataOffset,
               uint32_t DataLength);
};

class EEICallStatic : public EEI<EEICallStatic> {
public:
  EEICallStatic(EVMEnvironment &HostEnv) : EEI(HostEnv, 700) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint64_t Gas, uint32_t AddressOffset, uint32_t DataOffset,
               uint32_t DataLength);
};

class EEICodeCopy : public EEI<EEICodeCopy> {
public:
  EEICodeCopy(EVMEnvironment &HostEnv) : EEI(HostEnv, 3) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset, uint32_t CodeOffset, uint32_t Length);
};

class EEICreate : public EEI<EEICreate> {
public:
  EEICreate(EVMEnvironment &HostEnv) : EEI(HostEnv, 32000) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint32_t ValueOffset, uint32_t DataOffset, uint32_t DataLength,
               uint32_t ResultOffset);
};

class EEIExternalCodeCopy : public EEI<EEIExternalCodeCopy> {
public:
  EEIExternalCodeCopy(EVMEnvironment &HostEnv) : EEI(HostEnv, 700) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t AddressOffset, uint32_t ResultOffset,
               uint32_t CodeOffset, uint32_t Length);
};

class EEIFinish : public EEI<EEIFinish> {
public:
  EEIFinish(EVMEnvironment &HostEnv) : EEI(HostEnv, 0) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t DataOffset,
               uint32_t DataLength);
};

class EEIGetAddress : public EEI<EEIGetAddress> {
public:
  EEIGetAddress(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset);
};

class EEIGetBlockCoinbase : public EEI<EEIGetBlockCoinbase> {
public:
  EEIGetBlockCoinbase(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset);
};

class EEIGetBlockDifficulty : public EEI<EEIGetBlockDifficulty> {
public:
  EEIGetBlockDifficulty(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset);
};

class EEIGetBlockGasLimit : public EEI<EEIGetBlockGasLimit> {
public:
  EEIGetBlockGasLimit(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t &GasLimit);
};

class EEIGetBlockHash : public EEI<EEIGetBlockHash> {
public:
  EEIGetBlockHash(EVMEnvironment &HostEnv) : EEI(HostEnv, 800) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint64_t Number, uint32_t ResultOffset);
};

class EEIGetBlockNumber : public EEI<EEIGetBlockNumber> {
public:
  EEIGetBlockNumber(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint64_t &BlockNumber);
};

class EEIGetBlockTimestamp : public EEI<EEIGetBlockTimestamp> {
public:
  EEIGetBlockTimestamp(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint64_t &BlockTimestamp);
};

class EEIGetCallDataSize : public EEI<EEIGetCallDataSize> {
public:
  EEIGetCallDataSize(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret);
};

class EEIGetCaller : public EEI<EEIGetCaller> {
public:
  EEIGetCaller(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset);
};

class EEIGetCallValue : public EEI<EEIGetCallValue> {
public:
  EEIGetCallValue(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset);
};

class EEIGetCodeSize : public EEI<EEIGetCodeSize> {
public:
  EEIGetCodeSize(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret);
};

class EEIGetExternalBalance : public EEI<EEIGetExternalBalance> {
public:
  EEIGetExternalBalance(EVMEnvironment &HostEnv) : EEI(HostEnv, 400) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t AddressOffset, uint32_t ResultOffset);
};

class EEIGetExternalCodeSize : public EEI<EEIGetExternalCodeSize> {
public:
  EEIGetExternalCodeSize(EVMEnvironment &HostEnv) : EEI(HostEnv, 700) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
               uint32_t AddressOffset);
};

class EEIGetGasLeft : public EEI<EEIGetGasLeft> {
public:
  EEIGetGasLeft(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t &GasLeft);
};

class EEIGetReturnDataSize : public EEI<EEIGetReturnDataSize> {
public:
  EEIGetReturnDataSize(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &DataSize);
};

class EEIGetTxGasPrice : public EEI<EEIGetTxGasPrice> {
public:
  EEIGetTxGasPrice(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset);
};

class EEIGetTxOrigin : public EEI<EEIGetTxOrigin> {
public:
  EEIGetTxOrigin(EVMEnvironment &HostEnv) : EEI(HostEnv, 2) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset);
};

class EEILog : public EEI<EEILog> {
public:
  EEILog(EVMEnvironment &HostEnv) : EEI(HostEnv, 375) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t DataOffset,
               uint32_t DataLength, uint32_t NumberOfTopics, uint32_t Topic1,
               uint32_t Topic2, uint32_t Topic3, uint32_t Topic4);
};

class EEIReturnDataCopy : public EEI<EEIReturnDataCopy> {
public:
  EEIReturnDataCopy(EVMEnvironment &HostEnv) : EEI(HostEnv, 3) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t ResultOffset, uint32_t DataOffset, uint32_t Length);
};

class EEIRevert : public EEI<EEIRevert> {
public:
  EEIRevert(EVMEnvironment &HostEnv) : EEI(HostEnv, 0) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t DataOffset,
               uint32_t DataLength);
};

class EEISelfDestruct : public EEI<EEISelfDestruct> {
public:
  EEISelfDestruct(EVMEnvironment &HostEnv) : EEI(HostEnv, 5000) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst,
               uint32_t AddressOffset);
};

class EEIStorageLoad : public EEI<EEIStorageLoad> {
public:
  EEIStorageLoad(EVMEnvironment &HostEnv) : EEI(HostEnv, 200) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t PathOffset,
               uint32_t ValueOffset);
};

class EEIStorageStore : public EEI<EEIStorageStore> {
public:
  EEIStorageStore(EVMEnvironment &HostEnv) : EEI(HostEnv, 5000) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t PathOffset,
               uint32_t ValueOffset);
};

class EEIUseGas : public EEI<EEIUseGas> {
public:
  EEIUseGas(EVMEnvironment &HostEnv) : EEI(HostEnv, 0) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t Amount);
};

} // namespace Host
} // namespace SSVM
