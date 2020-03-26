// SPDX-License-Identifier: Apache-2.0
#include "host/ethereum/eeienv.h"

namespace SSVM {
namespace Host {

/// Consume gas. See "include/host/ethereum/eeienv.h".
bool EVMEnvironment::consumeGas(const uint64_t Gas) {
  if (GasLimit - GasUsed < Gas) {
    GasUsed = GasLimit;
    return false;
  }
  GasUsed += Gas;
  return true;
}

/// Return gas. See "include/host/ethereum/eeienv.h".
bool EVMEnvironment::returnGas(const uint64_t Gas) {
  if (GasUsed < Gas) {
    GasUsed = 0;
    return false;
  }
  GasUsed -= Gas;
  return true;
}

/// Initialize by EVMC message. See "include/host/ethereum/eeienv.h".
void EVMEnvironment::setEVMCMessage(const struct evmc_message *Msg) {
  /// Set depth.
  Depth = Msg->depth;

  /// Set call flag.
  Flag = Msg->flags;

  /// Set call kind.
  CallKind = Msg->kind;

  /// Set gas limit.
  GasLimit = Msg->gas;

  /// Set caller.
  Caller = Bytes(Msg->sender.bytes, Msg->sender.bytes + 20);

  /// Set call value. Convert big-endian to little-endian.
  CallValue = Bytes(Msg->value.bytes, Msg->value.bytes + 32);
  std::reverse(CallValue.begin(), CallValue.end());

  /// Set call data.
  CallData.clear();
  if (Msg->input_size > 0 && Msg->input_data != nullptr) {
    CallData.assign(Msg->input_data, Msg->input_data + Msg->input_size);
  }

  /// Set address.
  Address = Bytes(Msg->destination.bytes, Msg->destination.bytes + 20);
  ReturnData.clear();
}

} // namespace Host
} // namespace SSVM