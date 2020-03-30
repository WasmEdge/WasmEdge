// SPDX-License-Identifier: Apache-2.0
#include "evmc/evmc.h"
#include "evmc/utils.h"
#include "support/hexstr.h"
#include "expvm/configure.h"
#include "expvm/vm.h"
#include "host/ethereum/eeimodule.h"

#include <iostream>

namespace {

static bool isWasmBinary(std::vector<uint8_t> &Code) {
  return Code.size() >= 8 && Code[0] == 0 && Code[1] == 'a' && Code[2] == 's' &&
         Code[3] == 'm';
}

static evmc_capabilities_flagset get_capabilities(struct evmc_instance *vm) {
  return EVMC_CAPABILITY_EWASM;
}

static void destroy(struct evmc_instance *vm) { return; }

static void release(const struct evmc_result *result) {
  if (result->output_data != nullptr) {
    delete[] result->output_data;
  }
  return;
}

static struct evmc_result execute(struct evmc_instance *vm,
                                  struct evmc_context *context,
                                  enum evmc_revision rev,
                                  const struct evmc_message *msg,
                                  uint8_t const *code, size_t code_size) {
  // Prepare EVMC result
  struct evmc_result result;
  result.status_code = EVMC_SUCCESS;
  result.gas_left = msg->gas;
  result.output_size = 0;
  result.output_data = nullptr;
  result.release = ::release;
  result.create_address = {};

  /// Create VM with ewasm configuration.
  SSVM::ExpVM::Configure Conf;
  Conf.addVMType(SSVM::ExpVM::Configure::VMType::Ewasm);
  SSVM::ExpVM::VM EVM(Conf);

  /// Set data from message.
  std::vector<uint8_t> Code(code, code + code_size);
  SSVM::Host::EEIModule &EEIObj = *dynamic_cast<SSVM::Host::EEIModule *>(
      EVM.getImportModule(SSVM::ExpVM::Configure::VMType::Ewasm));
  SSVM::Host::EVMEnvironment &EEIEnv = EEIObj.getEnv();
  EEIEnv.setEVMCContext(context);
  EEIEnv.setEVMCMessage(msg);
  EEIEnv.setEVMCCode(code, code_size);
  if (msg->input_size > 0) {
    EEIEnv.getCallData() = std::vector<uint8_t>(
        msg->input_data, msg->input_data + msg->input_size);
  }
  EVM.getMeasurement().getCostLimit() = msg->gas;

  /// Debug log.
  std::cout << "msg->gas: " << msg->gas << std::endl;
  std::cout << "msg->depth: " << msg->depth << std::endl;
  std::cout << "msg->input_size: " << msg->input_size << std::endl;
  std::cout << "Caller: " << EEIEnv.getCallerStr() << std::endl;
  std::cout << "CallValue: " << EEIEnv.getCallValueStr() << std::endl;

  /// Load, validate, and instantiate code.
  if (result.status_code == EVMC_SUCCESS && !EVM.loadWasm(Code)) {
    result.status_code = EVMC_FAILURE;
  }
  if (result.status_code == EVMC_SUCCESS && !EVM.validate()) {
    result.status_code = EVMC_FAILURE;
  }
  if (result.status_code == EVMC_SUCCESS && !EVM.instantiate()) {
    result.status_code = EVMC_FAILURE;
  }

  /// Checking for errors.
  auto &Store = EVM.getStoreManager();
  if (result.status_code == EVMC_SUCCESS && Store.getMemExports().size() == 0) {
    /// Memory exports not found
    result.status_code = EVMC_FAILURE;
  }
  if (result.status_code == EVMC_SUCCESS &&
      (*Store.getActiveModule())->getStartAddr()) {
    /// Module contains start function
    result.status_code = EVMC_FAILURE;
  }

  /// Execute.
  if (result.status_code == EVMC_SUCCESS) {
    auto Res = EVM.execute("main");
    if (!Res && Res.error() == SSVM::ErrCode::Revert) {
      result.status_code = EVMC_REVERT;
    } else if (!Res) {
      result.status_code = EVMC_FAILURE;
    }
  }

  /// Get execution results.
  uint64_t usedGas = EVM.getMeasurement().getCostSum();
  std::vector<uint8_t> &ReturnData = EEIEnv.getReturnData();

  /// Verify the deployed code.
  if (isWasmBinary(ReturnData) && msg->kind == EVMC_CREATE &&
      result.status_code != EVMC_REVERT) {
    SSVM::Loader::Loader WasmLoader;
    if (auto Res = WasmLoader.parseModule(ReturnData)) {
      if ((*Res)->getStartSection() != nullptr) {
        result.status_code = EVMC_FAILURE;
      }
    } else {
      result.status_code = EVMC_FAILURE;
    }
  }

  /// Copy return data and left gas.
  if (ReturnData.size() > 0) {
    uint8_t *outputData = new uint8_t[ReturnData.size()];
    std::copy(ReturnData.begin(), ReturnData.end(), outputData);
    result.output_size = ReturnData.size();
    result.output_data = outputData;
  }
  result.gas_left =
      (result.status_code == EVMC_FAILURE) ? 0 : msg->gas - usedGas;

  /// Debug log.
  std::cout << "gas_left: " << result.gas_left << std::endl;
  std::cout << "output_size: " << result.output_size << std::endl;

  return result;
}

} // namespace

extern "C" EVMC_EXPORT struct evmc_instance *evmc_create() EVMC_NOEXCEPT {
  static evmc_instance vm = {
      EVMC_ABI_VERSION,   "ssvm",  "0.4.0",
      ::destroy, // destroy
      ::execute, // execute
      ::get_capabilities, nullptr,
  };

  return &vm;
}
