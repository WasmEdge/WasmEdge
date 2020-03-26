// SPDX-License-Identifier: Apache-2.0
#include "host/ethereum/eeifunc.h"
#include "keccak/Keccak.h"
#include "support/hexstr.h"

#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Host {

ErrCode EEICall::body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Ret,
                      uint64_t Gas, uint32_t AddressOffset,
                      uint32_t ValueOffset, uint32_t DataOffset,
                      uint32_t DataLength) {
  /// Prepare call message.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }
  evmc_uint256be Val;
  if (auto Res = loadUInt(MemInst, ValueOffset, 16)) {
    Val = *Res;
  } else {
    return Res.error();
  }
  evmc_message CallMsg = {
      .kind = evmc_call_kind::EVMC_CALL,
      .flags = Env.getFlag() & evmc_flags::EVMC_STATIC,
      .depth = static_cast<int32_t>(Env.getDepth() + 1),
      .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
      .destination = Addr,
      .sender = Env.getAddressEVMC(),
      .input_data = nullptr,
      .input_size = 0,
      .value = Val};

  /// Return: Result(i32)
  if (auto Res = callContract(MemInst, CallMsg, DataOffset, DataLength)) {
    Ret = *Res;
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEICallCode::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t &Ret, uint64_t Gas, uint32_t AddressOffset,
                          uint32_t ValueOffset, uint32_t DataOffset,
                          uint32_t DataLength) {
  /// Prepare call message.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }
  evmc_uint256be Val;
  if (auto Res = loadUInt(MemInst, ValueOffset, 16)) {
    Val = *Res;
  } else {
    return Res.error();
  }
  evmc_message CallMsg = {
      .kind = evmc_call_kind::EVMC_CALLCODE,
      .flags = Env.getFlag() & evmc_flags::EVMC_STATIC,
      .depth = static_cast<int32_t>(Env.getDepth() + 1),
      .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
      .destination = Addr,
      .sender = Env.getAddressEVMC(),
      .input_data = nullptr,
      .input_size = 0,
      .value = Val};

  /// Return: Result(i32)
  if (auto Res = callContract(MemInst, CallMsg, DataOffset, DataLength)) {
    Ret = *Res;
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEICallDataCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                              uint32_t ResultOffset, uint32_t DataOffset,
                              uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Res.error();
  }
  if (auto Res = MemInst.setBytes(Env.getCallData(), ResultOffset, DataOffset,
                                  Length)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEICallDelegate::body(Runtime::Instance::MemoryInstance &MemInst,
                              uint32_t &Ret, uint64_t Gas,
                              uint32_t AddressOffset, uint32_t DataOffset,
                              uint32_t DataLength) {
  /// Prepare call message.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }
  evmc_message CallMsg = {
      .kind = evmc_call_kind::EVMC_DELEGATECALL,
      .flags = Env.getFlag() & evmc_flags::EVMC_STATIC,
      .depth = static_cast<int32_t>(Env.getDepth() + 1),
      .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
      .destination = Addr,
      .sender = Env.getAddressEVMC(),
      .input_data = nullptr,
      .input_size = 0,
      .value = Env.getCallValueEVMC()};

  /// Return: Result(i32)
  if (auto Res = callContract(MemInst, CallMsg, DataOffset, DataLength)) {
    Ret = *Res;
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEICallStatic::body(Runtime::Instance::MemoryInstance &MemInst,
                            uint32_t &Ret, uint64_t Gas, uint32_t AddressOffset,
                            uint32_t DataOffset, uint32_t DataLength) {
  /// Load address and convert to uint256.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }
  boost::multiprecision::uint256_t AddrNum = 0;
  for (auto &I : Addr.bytes) {
    AddrNum <<= 8;
    AddrNum |= I;
  }

  if (AddrNum == 9) {
    /// Check data copy cost.
    if (auto Res = addCopyCost(DataLength); !Res) {
      return Res.error();
    }

    /// Prepare call data.
    std::vector<unsigned char> Data;
    if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
      Data = *Res;
    } else {
      return Res.error();
    }

    /// Run Keccak
    Keccak K(256);
    for (auto &I : Data) {
      K.addData(I);
    }
    Env.getReturnData() = K.digest();

    /// Return: Result(i32)
    Ret = 0U;
  } else {
    /// Prepare call message.
    evmc_message CallMsg = {
        .kind = evmc_call_kind::EVMC_CALL,
        .flags = evmc_flags::EVMC_STATIC,
        .depth = static_cast<int32_t>(Env.getDepth() + 1),
        .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
        .destination = Addr,
        .sender = Env.getAddressEVMC(),
        .input_data = nullptr,
        .input_size = 0,
        .value = {}};

    /// Return: Result(i32)
    if (auto Res = callContract(MemInst, CallMsg, DataOffset, DataLength)) {
      Ret = *Res;
      return ErrCode::Success;
    } else {
      return Res.error();
    }
  }
  return ErrCode::Success;
}

ErrCode EEICodeCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t ResultOffset, uint32_t CodeOffset,
                          uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Res.error();
  }
  if (auto Res =
          MemInst.setBytes(Env.getCode(), ResultOffset, CodeOffset, Length)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEICreate::body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t &Ret, uint32_t ValueOffset,
                        uint32_t DataOffset, uint32_t DataLength,
                        uint32_t ResultOffset) {
  /// Prepare creation message.
  evmc_uint256be Val;
  if (auto Res = loadUInt(MemInst, ValueOffset, 16)) {
    Val = *Res;
  } else {
    return Res.error();
  }
  evmc_message CreateMsg = {.kind = evmc_call_kind::EVMC_CREATE,
                            .flags = 0,
                            .depth = static_cast<int32_t>(Env.getDepth() + 1),
                            .gas = static_cast<int64_t>(getMaxCallGas()),
                            .destination = {},
                            .sender = Env.getAddressEVMC(),
                            .input_data = nullptr,
                            .input_size = 0,
                            .value = Val};

  /// Return: Result(i32)
  if (auto Res = callContract(MemInst, CreateMsg, DataOffset, DataLength,
                              ResultOffset)) {
    Ret = *Res;
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIExternalCodeCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t AddressOffset, uint32_t ResultOffset,
                                  uint32_t CodeOffset, uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Res.error();
  }
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }

  /// Copy code to vector.
  std::vector<uint8_t> Buffer(Length, 0);
  size_t Copied =
      Cxt->host->copy_code(Cxt, &Addr, CodeOffset, &Buffer[0], Length);
  if (Length != Copied) {
    return ErrCode::AccessForbidMemory;
  }

  /// Store to memory instance.
  if (auto Res = MemInst.setBytes(Buffer, ResultOffset, 0, Length)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIFinish::body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t DataOffset, uint32_t DataLength) {
  Env.getReturnData().clear();
  if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
    Env.getReturnData() = *Res;
  } else {
    return Res.error();
  }
  return ErrCode::Terminated;
}

ErrCode EEIGetAddress::body(Runtime::Instance::MemoryInstance &MemInst,
                            uint32_t ResultOffset) {
  if (auto Res = MemInst.setBytes(Env.getAddress(), ResultOffset, 0, 20)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIGetBlockCoinbase::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block coinbase and store bytes20.
  if (auto Res =
          storeAddress(MemInst, Cxt->host->get_tx_context(Cxt).block_coinbase,
                       ResultOffset)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIGetBlockDifficulty::body(Runtime::Instance::MemoryInstance &MemInst,
                                    uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block difficulty and store uint256 little-endian value.
  if (auto Res =
          storeUInt(MemInst, Cxt->host->get_tx_context(Cxt).block_difficulty,
                    ResultOffset)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIGetBlockGasLimit::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint64_t &GasLimit) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: GasLimit(u64)
  GasLimit =
      static_cast<uint64_t>(Cxt->host->get_tx_context(Cxt).block_gas_limit);
  return ErrCode::Success;
}

ErrCode EEIGetBlockHash::body(Runtime::Instance::MemoryInstance &MemInst,
                              uint32_t &Ret, uint64_t Number,
                              uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();
  ErrCode Status = ErrCode::Success;
  Ret = 0U;

  /// Get the block hash value.
  const evmc_bytes32 Hash = Cxt->host->get_block_hash(Cxt, Number);

  /// Check is zero.
  if (evmc::is_zero(Hash)) {
    Ret = 1U;
  } else {
    /// Store bytes32.
    if (auto Res = storeBytes32(MemInst, Hash, ResultOffset); !Res) {
      return Res.error();
    }
  }

  /// Return: Result(u32)
  return Status;
}

ErrCode EEIGetBlockNumber::body(Runtime::Instance::MemoryInstance &MemInst,
                                uint64_t &BlockNumber) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: BlockNumber(u64)
  BlockNumber =
      static_cast<uint64_t>(Cxt->host->get_tx_context(Cxt).block_number);
  return ErrCode::Success;
}

ErrCode EEIGetBlockTimestamp::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint64_t &BlockTimestamp) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: BlockNumber(u64)
  BlockTimestamp =
      static_cast<uint64_t>(Cxt->host->get_tx_context(Cxt).block_timestamp);
  return ErrCode::Success;
}

ErrCode EEIGetCallDataSize::body(Runtime::Instance::MemoryInstance &MemInst,
                                 uint32_t &Ret) {
  /// Return: Length(u32)
  Ret = Env.getCallData().size();
  return ErrCode::Success;
}

ErrCode EEIGetCaller::body(Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t ResultOffset) {
  if (auto Res = MemInst.setBytes(Env.getCaller(), ResultOffset, 0, 20)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIGetCallValue::body(Runtime::Instance::MemoryInstance &MemInst,
                              uint32_t ResultOffset) {
  if (auto Res = MemInst.setBytes(Env.getCallValue(), ResultOffset, 0, 16)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIGetCodeSize::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint32_t &Ret) {
  /// Return: CodeSize(u32)
  Ret = Env.getCode().size();
  return ErrCode::Success;
}

ErrCode EEIGetExternalBalance::body(Runtime::Instance::MemoryInstance &MemInst,
                                    uint32_t AddressOffset,
                                    uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }

  /// Get balance uint256 big-endian value.
  evmc_uint256be Balance = Cxt->host->get_balance(Cxt, &Addr);

  /// Store uint128 little-endian value.
  if (auto Res = storeUInt(MemInst, Balance, ResultOffset, 16)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIGetExternalCodeSize::body(Runtime::Instance::MemoryInstance &MemInst,
                                     uint32_t &Ret, uint32_t AddressOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }

  /// Return: ExtCodeSize(u32)
  Ret = Cxt->host->get_code_size(Cxt, &Addr);
  return ErrCode::Success;
}

ErrCode EEIGetGasLeft::body(Runtime::Instance::MemoryInstance &MemInst,
                            uint64_t &GasLeft) {
  GasLeft = Env.getGasLeft();
  return ErrCode::Success;
}

ErrCode EEIGetReturnDataSize::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t &DataSize) {
  /// Return: DataSize(u32)
  DataSize = Env.getReturnData().size();
  return ErrCode::Success;
}

ErrCode EEIGetTxGasPrice::body(Runtime::Instance::MemoryInstance &MemInst,
                               uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get tx gas price uint256 big-endian value.
  evmc_uint256be Price = Cxt->host->get_tx_context(Cxt).tx_gas_price;

  /// Store uint128 little-endian value.
  if (auto Res = storeUInt(MemInst, Price, ResultOffset, 16)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIGetTxOrigin::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block coinbase and store bytes20.
  if (auto Res = storeAddress(MemInst, Cxt->host->get_tx_context(Cxt).tx_origin,
                              ResultOffset)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEILog::body(Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t DataOffset, uint32_t DataLength,
                     uint32_t NumberOfTopics, uint32_t Topic1, uint32_t Topic2,
                     uint32_t Topic3, uint32_t Topic4) {
  /// Check number of topics
  if (NumberOfTopics > 4) {
    return ErrCode::ExecutionFailed;
  }

  /// Take additional gas of logs.
  uint64_t TakeGas = 375ULL * NumberOfTopics + 8ULL * DataLength;
  if (!Env.consumeGas(TakeGas)) {
    return ErrCode::CostLimitExceeded;
  }
  evmc_context *Cxt = Env.getEVMCContext();

  /// Copy topics to array.
  std::vector<evmc_bytes32> Topics(4, evmc_bytes32());
  if (NumberOfTopics >= 1) {
    if (auto Res = loadBytes32(MemInst, Topic1)) {
      Topics[0] = *Res;
    } else {
      return Res.error();
    }
  }
  if (NumberOfTopics >= 2) {
    if (auto Res = loadBytes32(MemInst, Topic2)) {
      Topics[1] = *Res;
    } else {
      return Res.error();
    }
  }
  if (NumberOfTopics >= 3) {
    if (auto Res = loadBytes32(MemInst, Topic3)) {
      Topics[2] = *Res;
    } else {
      return Res.error();
    }
  }
  if (NumberOfTopics == 4) {
    if (auto Res = loadBytes32(MemInst, Topic4)) {
      Topics[3] = *Res;
    } else {
      return Res.error();
    }
  }

  /// Load data.
  std::vector<uint8_t> Data;
  if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
    Data = *Res;
  } else {
    return Res.error();
  }

  /// Get address data.
  evmc_address Addr = Env.getAddressEVMC();

  /// Call emit_log.
  Cxt->host->emit_log(Cxt, &Addr, &Data[0], DataLength, &Topics[0],
                      NumberOfTopics);
  return ErrCode::Success;
}

ErrCode EEIReturnDataCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                                uint32_t ResultOffset, uint32_t DataOffset,
                                uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Res.error();
  }
  if (auto Res = MemInst.setBytes(Env.getReturnData(), ResultOffset, DataOffset,
                                  Length)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIRevert::body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t DataOffset, uint32_t DataLength) {
  Env.getReturnData().clear();
  if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
    Env.getReturnData() = *Res;
  } else {
    return Res.error();
  }
  return ErrCode::Revert;
}

ErrCode EEISelfDestruct::body(Runtime::Instance::MemoryInstance &MemInst,
                              uint32_t AddressOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address data.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = *Res;
  } else {
    return Res.error();
  }
  evmc_address Self = Env.getAddressEVMC();

  /// Take additional gas if call new account.
  if (!Cxt->host->account_exists(Cxt, &Addr)) {
    if (!Env.consumeGas(25000ULL)) {
      return ErrCode::CostLimitExceeded;
    }
  }

  /// Call selfdestruct.
  Cxt->host->selfdestruct(Cxt, &Self, &Addr);
  return ErrCode::Terminated;
}

ErrCode EEIStorageLoad::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint32_t PathOffset, uint32_t ValueOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get destination, path, and value data.
  evmc_address Addr = Env.getAddressEVMC();
  evmc_bytes32 Path;
  if (auto Res = loadBytes32(MemInst, PathOffset)) {
    Path = *Res;
  } else {
    return Res.error();
  }

  /// Store bytes32 into memory instance.
  if (auto Res = storeBytes32(
          MemInst, Cxt->host->get_storage(Cxt, &Addr, &Path), ValueOffset)) {
    return ErrCode::Success;
  } else {
    return Res.error();
  }
}

ErrCode EEIStorageStore::body(Runtime::Instance::MemoryInstance &MemInst,
                              uint32_t PathOffset, uint32_t ValueOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Static mode cannot store storage
  if (Env.getFlag() & evmc_flags::EVMC_STATIC) {
    return ErrCode::ExecutionFailed;
  }

  /// Get destination, path, value data, and current storage value.
  evmc_address Addr = Env.getAddressEVMC();
  evmc_bytes32 Path, Value;
  if (auto Res = loadBytes32(MemInst, PathOffset)) {
    Path = *Res;
  } else {
    return Res.error();
  }
  if (auto Res = loadBytes32(MemInst, ValueOffset)) {
    Value = *Res;
  } else {
    return Res.error();
  }
  evmc_bytes32 CurrValue = Cxt->host->get_storage(Cxt, &Addr, &Path);

  /// Take additional gas if create case.
  if (evmc::is_zero(CurrValue) && !evmc::is_zero(Value)) {
    if (!Env.consumeGas(15000ULL)) {
      return ErrCode::CostLimitExceeded;
    }
  }

  /// Store value into storage.
  Cxt->host->set_storage(Cxt, &Addr, &Path, &Value);
  return ErrCode::Success;
}

ErrCode EEIUseGas::body(Runtime::Instance::MemoryInstance &MemInst,
                        uint64_t Amount) {
  /// Take gas.
  if (!Env.consumeGas(Amount)) {
    return ErrCode::CostLimitExceeded;
  }
  return ErrCode::Success;
}

} // namespace Host
} // namespace SSVM