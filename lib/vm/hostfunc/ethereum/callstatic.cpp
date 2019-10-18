#include "vm/hostfunc/ethereum/callstatic.h"
#include "Keccak.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "support/hexstr.h"

#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Executor {

EEICallStatic::EEICallStatic(VM::EVMEnvironment &Env) : EEI(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendReturnDef(AST::ValType::I32);
}

ErrCode EEICallStatic::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                           std::vector<std::unique_ptr<ValueEntry>> &Res,
                           StoreManager &Store,
                           Instance::ModuleInstance *ModInst) {
  /// Arg: gas(u32), addressOffset(u32), dataOffset(u32), dataLength(u32)
  if (Args.size() != 4) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Gas = retrieveValue<uint32_t>(*Args[3].get());
  unsigned int AddressOffset = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int DataOffset = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int DataLength = retrieveValue<uint32_t>(*Args[0].get());

  std::vector<unsigned char> Address;
  std::vector<unsigned char> Data;
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = MemInst->getBytes(Address, AddressOffset, 20)) !=
      ErrCode::Success) {
    return Status;
  }
  if ((Status = MemInst->getBytes(Data, DataOffset, DataLength)) !=
      ErrCode::Success) {
    return Status;
  }

  boost::multiprecision::uint256_t AddressNum = 0;
  for (auto It = Address.crbegin(); It != Address.crend(); It++) {
    AddressNum <<= 8;
    AddressNum += *It;
  }

  unsigned int Result = 1U;
  if (AddressNum == 9) {
    /// Run Keccak
    Keccak K(256);
    for (auto It = Data.cbegin(); It != Data.cend(); It++) {
      K.addData(*It);
    }
    std::vector<unsigned char> &ReturnData = Env.getReturnData();
    ReturnData = K.digest();
    Result = 0U;
  }

  /// Return: result(u32)
  Res.push_back(std::make_unique<ValueEntry>(Result));
  return Status;
}

} // namespace Executor
} // namespace SSVM