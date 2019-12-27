// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/storageload.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Compiler {

void EEIStorageLoad::run(uint32_t PathOffset, uint32_t ValueOffset) {
  using std::begin;
  using std::end;
  evmc_context *Cxt = Env.getEVMCContext();

  /// Get destination, path, and value data.
  evmc::bytes32 EPath;
  {
    const auto &Path = Lib.getMemory<uint8_t>(PathOffset, 32);
    std::copy(begin(Path), end(Path), begin(EPath.bytes));
  }

  evmc::address Addr;
  {
    const auto &Address = Env.getAddress();
    std::copy(begin(Address), end(Address), begin(Addr.bytes));
  }

  evmc::bytes32 EValue = Cxt->host->get_storage(Cxt, &Addr, &EPath);
  {
    auto Value = Lib.getMemory<uint8_t>(ValueOffset, 32);
    std::copy(begin(EValue.bytes), end(EValue.bytes), begin(Value));
  }
}

} // namespace Compiler
} // namespace SSVM
