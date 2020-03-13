// SPDX-License-Identifier: Apache-2.0
#include "evmc/evmc.h"
#include "evmc/utils.h"
#include "support/hexstr.h"
#include "vm/configure.h"
#include "vm/vm.h"

#include <iostream>

namespace {

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
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Ewasm);
  SSVM::VM::VM EVM(Conf);
  SSVM::VM::EVMEnvironment *Env = EVM.getEnvironment<SSVM::VM::EVMEnvironment>(
      SSVM::VM::Configure::VMType::Ewasm);
  Env->clear();
  Env->setEVMCContext(context);
  Env->setEVMCMessage(msg);
  Env->setEVMCCode(code, code_size);

  // Debug log
  std::cout << "code_size: " << code_size << std::endl;
  std::cout << "msg->gas: " << msg->gas << std::endl;
  std::cout << "msg->depth: " << msg->depth << std::endl;
  std::cout << "msg->input_size: " << msg->input_size << std::endl;
  std::cout << "Caller: " << Env->getCallerStr() << std::endl;
  std::cout << "CallValue: " << Env->getCallValueStr() << std::endl;

  // Set calldata
  if (msg->input_size > 0) {
    Env->getCallData() = std::vector<uint8_t>(
        msg->input_data, msg->input_data + msg->input_size);
  }

  // Set code & gas
  std::vector<uint8_t> Code(code, code + code_size);
  EVM.setCode(Code);
  EVM.setCostLimit(msg->gas);

  EVM.execute("main");

  // Get execution result
  uint64_t usedGas = EVM.getUsedCost();
  SSVM::VM::Result::State returnState = EVM.getResult().getState();
  // std::map<std::string, std::string> &storage = Env->getStorage();
  std::vector<unsigned char> &returnData = Env->getReturnData();

  // Debug log
  std::cout << "usedGas: " << usedGas << std::endl;
  std::cout << "return_size: " << returnData.size() << std::endl;
  std::cout << "return_data: ";
  for (auto it = returnData.begin(); it != returnData.end(); ++it) {
    printf("%02x", *it);
  }
  std::cout << std::endl;

  // Prepare EVMC result
  struct evmc_result result;
  result.gas_left = msg->gas - usedGas;
  result.output_size = 0;
  result.output_data = nullptr;
  result.release = ::release;
  result.create_address = {};

  if (returnState == SSVM::VM::Result::State::Commit) {
    result.status_code = EVMC_SUCCESS;
  } else if (returnState == SSVM::VM::Result::State::Revert) {
    result.status_code = EVMC_REVERT;
  } else {
    result.status_code = EVMC_FAILURE;
    result.gas_left = 0;
  }

  if (returnData.size() > 0) {
    uint8_t *outputData = new uint8_t[returnData.size()];
    copy(returnData.begin(), returnData.end(), outputData);
    result.output_size = returnData.size();
    result.output_data = outputData;
  }

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
