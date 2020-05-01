// SPDX-License-Identifier: Apache-2.0
#include "host/ethereum/eeifunc.h"
#include "keccak/Keccak.h"
#include "support/hexstr.h"

#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Host {

Expect<uint32_t> EEICall::body(Runtime::Instance::MemoryInstance &MemInst,
                               uint64_t Gas, uint32_t AddressOffset,
                               uint32_t ValueOffset, uint32_t DataOffset,
                               uint32_t DataLength) {
  /// Prepare call message.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  evmc_uint256be Val;
  if (auto Res = loadUInt(MemInst, ValueOffset, 16)) {
    Val = std::move(*Res);
  } else {
    return Unexpect(Res);
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

  return callContract(MemInst, CallMsg, DataOffset, DataLength);
}

Expect<uint32_t> EEICallCode::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint64_t Gas, uint32_t AddressOffset,
                                   uint32_t ValueOffset, uint32_t DataOffset,
                                   uint32_t DataLength) {
  /// Prepare call message.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  evmc_uint256be Val;
  if (auto Res = loadUInt(MemInst, ValueOffset, 16)) {
    Val = std::move(*Res);
  } else {
    return Unexpect(Res);
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

  return callContract(MemInst, CallMsg, DataOffset, DataLength);
}

Expect<void> EEICallDataCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t ResultOffset, uint32_t DataOffset,
                                   uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Unexpect(Res);
  }
  return MemInst.setBytes(Env.getCallData(), ResultOffset, DataOffset, Length);
}

Expect<uint32_t>
EEICallDelegate::body(Runtime::Instance::MemoryInstance &MemInst, uint64_t Gas,
                      uint32_t AddressOffset, uint32_t DataOffset,
                      uint32_t DataLength) {
  /// Prepare call message.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
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

  return callContract(MemInst, CallMsg, DataOffset, DataLength);
}

Expect<uint32_t> EEICallStatic::body(Runtime::Instance::MemoryInstance &MemInst,
                                     uint64_t Gas, uint32_t AddressOffset,
                                     uint32_t DataOffset, uint32_t DataLength) {
  /// Load address and convert to uint256.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  boost::multiprecision::uint256_t AddrNum = 0;
  for (auto &I : Addr.bytes) {
    AddrNum <<= 8;
    AddrNum |= I;
  }

  if (AddrNum == 9) {
    /// Check data copy cost.
    if (auto Res = addCopyCost(DataLength); !Res) {
      return Unexpect(Res);
    }

    /// Prepare call data.
    std::vector<unsigned char> Data;
    if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
      Data = std::move(*Res);
    } else {
      return Unexpect(Res);
    }

    /// Run Keccak
    Keccak K(256);
    for (auto &I : Data) {
      K.addData(I);
    }
    Env.getReturnData() = K.digest();

    return UINT32_C(0);
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

    return callContract(MemInst, CallMsg, DataOffset, DataLength);
  }
}

Expect<void> EEICodeCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                               uint32_t ResultOffset, uint32_t CodeOffset,
                               uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Unexpect(Res);
  }
  return MemInst.setBytes(Env.getCode(), ResultOffset, CodeOffset, Length);
}

Expect<uint32_t> EEICreate::body(Runtime::Instance::MemoryInstance &MemInst,
                                 uint32_t ValueOffset, uint32_t DataOffset,
                                 uint32_t DataLength, uint32_t ResultOffset) {
  /// Prepare creation message.
  evmc_uint256be Val;
  if (auto Res = loadUInt(MemInst, ValueOffset, 16)) {
    Val = std::move(*Res);
  } else {
    return Unexpect(Res);
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
  return callContract(MemInst, CreateMsg, DataOffset, DataLength, ResultOffset);
}

Expect<void>
EEIExternalCodeCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t AddressOffset, uint32_t ResultOffset,
                          uint32_t CodeOffset, uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Unexpect(Res);
  }
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
  }

  /// Copy code to vector.
  std::vector<uint8_t> Buffer(Length, 0);
  size_t Copied =
      Cxt->host->copy_code(Cxt, &Addr, CodeOffset, Buffer.data(), Length);
  if (Length != Copied) {
    return Unexpect(ErrCode::AccessForbidMemory);
  }

  /// Store to memory instance.
  return MemInst.setBytes(Buffer, ResultOffset, 0, Length);
}

Expect<void> EEIFinish::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint32_t DataOffset, uint32_t DataLength) {
  Env.getReturnData().clear();
  if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
    Env.getReturnData() = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  return Unexpect(ErrCode::Terminated);
}

Expect<void> EEIGetAddress::body(Runtime::Instance::MemoryInstance &MemInst,
                                 uint32_t ResultOffset) {
  return MemInst.setBytes(Env.getAddress(), ResultOffset, 0, 20);
}

Expect<void>
EEIGetBlockCoinbase::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block coinbase and store bytes20.
  return storeAddress(MemInst, Cxt->host->get_tx_context(Cxt).block_coinbase,
                      ResultOffset);
}

Expect<void>
EEIGetBlockDifficulty::body(Runtime::Instance::MemoryInstance &MemInst,
                            uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block difficulty and store uint256 little-endian value.
  return storeUInt(MemInst, Cxt->host->get_tx_context(Cxt).block_difficulty,
                   ResultOffset);
}

Expect<uint64_t>
EEIGetBlockGasLimit::body(Runtime::Instance::MemoryInstance &MemInst) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: GasLimit(u64)
  return Cxt->host->get_tx_context(Cxt).block_gas_limit;
}

Expect<uint32_t>
EEIGetBlockHash::body(Runtime::Instance::MemoryInstance &MemInst,
                      uint64_t Number, uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get the block hash value.
  const evmc_bytes32 Hash = Cxt->host->get_block_hash(Cxt, Number);

  /// Check is zero.
  if (evmc::is_zero(Hash)) {
    /// Return: Result(u32)
    return UINT32_C(1);
  } else {
    /// Store bytes32.
    if (auto Res = storeBytes32(MemInst, Hash, ResultOffset); !Res) {
      return Unexpect(Res);
    }
    /// Return: Result(u32)
    return UINT32_C(0);
  }
}

Expect<uint64_t>
EEIGetBlockNumber::body(Runtime::Instance::MemoryInstance &MemInst) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: BlockNumber(u64)
  return Cxt->host->get_tx_context(Cxt).block_number;
}

Expect<uint64_t>
EEIGetBlockTimestamp::body(Runtime::Instance::MemoryInstance &MemInst) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Return: BlockNumber(u64)
  return Cxt->host->get_tx_context(Cxt).block_timestamp;
}

Expect<uint32_t>
EEIGetCallDataSize::body(Runtime::Instance::MemoryInstance &MemInst) {
  /// Return: Length(u32)
  return Env.getCallData().size();
}

Expect<void> EEIGetCaller::body(Runtime::Instance::MemoryInstance &MemInst,
                                uint32_t ResultOffset) {
  return MemInst.setBytes(Env.getCaller(), ResultOffset, 0, 20);
}

Expect<void> EEIGetCallValue::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t ResultOffset) {
  return MemInst.setBytes(Env.getCallValue(), ResultOffset, 0, 16);
}

Expect<uint32_t>
EEIGetCodeSize::body(Runtime::Instance::MemoryInstance &MemInst) {
  /// Return: CodeSize(u32)
  return Env.getCode().size();
}

Expect<void>
EEIGetExternalBalance::body(Runtime::Instance::MemoryInstance &MemInst,
                            uint32_t AddressOffset, uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
  }

  /// Get balance uint256 big-endian value.
  evmc_uint256be Balance = Cxt->host->get_balance(Cxt, &Addr);

  /// Store uint128 little-endian value.
  return storeUInt(MemInst, Balance, ResultOffset, 16);
}

Expect<uint32_t>
EEIGetExternalCodeSize::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint32_t AddressOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address from memory instance.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
  }

  /// Return: ExtCodeSize(u32)
  return Cxt->host->get_code_size(Cxt, &Addr);
}

Expect<uint64_t>
EEIGetGasLeft::body(Runtime::Instance::MemoryInstance &MemInst) {
  return Env.getGasLeft();
}

Expect<uint32_t>
EEIGetReturnDataSize::body(Runtime::Instance::MemoryInstance &MemInst) {
  /// Return: DataSize(u32)
  return Env.getReturnData().size();
}

Expect<void> EEIGetTxGasPrice::body(Runtime::Instance::MemoryInstance &MemInst,
                                    uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get tx gas price uint256 big-endian value.
  evmc_uint256be Price = Cxt->host->get_tx_context(Cxt).tx_gas_price;

  /// Store uint128 little-endian value.
  return storeUInt(MemInst, Price, ResultOffset, 16);
}

Expect<void> EEIGetTxOrigin::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t ResultOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get block coinbase and store bytes20.
  return storeAddress(MemInst, Cxt->host->get_tx_context(Cxt).tx_origin,
                      ResultOffset);
}

Expect<void> EEILog::body(Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t DataOffset, uint32_t DataLength,
                          uint32_t NumberOfTopics, uint32_t Topic1,
                          uint32_t Topic2, uint32_t Topic3, uint32_t Topic4) {
  /// Check number of topics
  if (NumberOfTopics > 4) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  /// Take additional gas of logs.
  uint64_t TakeGas = 375ULL * NumberOfTopics + 8ULL * DataLength;
  if (!Env.consumeGas(TakeGas)) {
    return Unexpect(ErrCode::CostLimitExceeded);
  }
  evmc_context *Cxt = Env.getEVMCContext();

  /// Copy topics to array.
  std::vector<evmc_bytes32> Topics(4, evmc_bytes32());
  if (NumberOfTopics >= 1) {
    if (auto Res = loadBytes32(MemInst, Topic1)) {
      Topics[0] = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
  }
  if (NumberOfTopics >= 2) {
    if (auto Res = loadBytes32(MemInst, Topic2)) {
      Topics[1] = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
  }
  if (NumberOfTopics >= 3) {
    if (auto Res = loadBytes32(MemInst, Topic3)) {
      Topics[2] = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
  }
  if (NumberOfTopics == 4) {
    if (auto Res = loadBytes32(MemInst, Topic4)) {
      Topics[3] = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
  }

  /// Load data.
  std::vector<uint8_t> Data;
  if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
    Data = std::move(*Res);
  } else {
    return Unexpect(Res);
  }

  /// Get address data.
  evmc_address Addr = Env.getAddressEVMC();

  /// Call emit_log.
  Cxt->host->emit_log(Cxt, &Addr, &Data[0], DataLength, &Topics[0],
                      NumberOfTopics);

  return {};
}

Expect<void> EEIReturnDataCopy::body(Runtime::Instance::MemoryInstance &MemInst,
                                     uint32_t ResultOffset, uint32_t DataOffset,
                                     uint32_t Length) {
  /// Take additional gas of copy.
  if (auto Res = addCopyCost(Length); !Res) {
    return Unexpect(Res);
  }
  return MemInst.setBytes(Env.getReturnData(), ResultOffset, DataOffset,
                          Length);
}

Expect<void> EEIRevert::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint32_t DataOffset, uint32_t DataLength) {
  Env.getReturnData().clear();
  if (auto Res = MemInst.getBytes(DataOffset, DataLength)) {
    Env.getReturnData() = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  return Unexpect(ErrCode::Revert);
}

Expect<void> EEISelfDestruct::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t AddressOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get address data.
  evmc_address Addr;
  if (auto Res = loadAddress(MemInst, AddressOffset)) {
    Addr = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  evmc_address Self = Env.getAddressEVMC();

  /// Take additional gas if call new account.
  if (!Cxt->host->account_exists(Cxt, &Addr)) {
    if (!Env.consumeGas(25000ULL)) {
      return Unexpect(ErrCode::CostLimitExceeded);
    }
  }

  /// Call selfdestruct.
  Cxt->host->selfdestruct(Cxt, &Self, &Addr);
  return Unexpect(ErrCode::Terminated);
}

Expect<void> EEIStorageLoad::body(Runtime::Instance::MemoryInstance &MemInst,
                                  uint32_t PathOffset, uint32_t ValueOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get destination, path, and value data.
  evmc_address Addr = Env.getAddressEVMC();
  evmc_bytes32 Path;
  if (auto Res = loadBytes32(MemInst, PathOffset)) {
    Path = std::move(*Res);
  } else {
    return Unexpect(Res);
  }

  /// Store bytes32 into memory instance.
  return storeBytes32(MemInst, Cxt->host->get_storage(Cxt, &Addr, &Path),
                      ValueOffset);
}

Expect<void> EEIStorageStore::body(Runtime::Instance::MemoryInstance &MemInst,
                                   uint32_t PathOffset, uint32_t ValueOffset) {
  evmc_context *Cxt = Env.getEVMCContext();

  /// Static mode cannot store storage
  if (Env.getFlag() & evmc_flags::EVMC_STATIC) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  /// Get destination, path, value data, and current storage value.
  evmc_address Addr = Env.getAddressEVMC();
  evmc_bytes32 Path, Value;
  if (auto Res = loadBytes32(MemInst, PathOffset)) {
    Path = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  if (auto Res = loadBytes32(MemInst, ValueOffset)) {
    Value = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  evmc_bytes32 CurrValue = Cxt->host->get_storage(Cxt, &Addr, &Path);

  /// Take additional gas if create case.
  if (evmc::is_zero(CurrValue) && !evmc::is_zero(Value)) {
    if (!Env.consumeGas(15000ULL)) {
      return Unexpect(ErrCode::CostLimitExceeded);
    }
  }

  /// Store value into storage.
  Cxt->host->set_storage(Cxt, &Addr, &Path, &Value);
  return {};
}

Expect<void> EEIUseGas::body(Runtime::Instance::MemoryInstance &MemInst,
                             uint64_t Amount) {
  /// Take gas.
  if (!Env.consumeGas(Amount)) {
    return Unexpect(ErrCode::CostLimitExceeded);
  }
  return {};
}

} // namespace Host
} // namespace SSVM
