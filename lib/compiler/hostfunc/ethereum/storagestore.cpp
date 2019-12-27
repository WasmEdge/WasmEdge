// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/storagestore.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Compiler {

void EEIStorageStore::run(uint32_t PathOffset, uint32_t ValueOffset) {
  using std::begin;
  using std::end;
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get destination, path, value data, and current storage value.
  evmc::bytes32 EPath, EValue;
  evmc::address Addr;
  {
    const auto &Path = Lib.getMemory<uint8_t>(PathOffset, 32);
    std::copy(begin(Path), end(Path), begin(EPath.bytes));
  }
  {
    const auto &Value = Lib.getMemory<uint8_t>(ValueOffset, 32);
    std::copy(begin(Value), end(Value), begin(EValue.bytes));
  }
  {
    const auto &Address = Env.getAddress();
    std::copy(begin(Address), end(Address), begin(Addr.bytes));
  }
  evmc::bytes32 CurrValue = Cxt->host->get_storage(Cxt, &Addr, &EPath);

  /// TODO:Take additional gas if create case.
}

} // namespace Compiler
} // namespace SSVM
