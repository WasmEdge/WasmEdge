#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {
namespace Instance {

/// Adder of function types. See "include/executor/instance/module.h".
ErrCode ModuleInstance::addFuncType(const std::vector<AST::ValType> &Params,
                                    const std::vector<AST::ValType> &Returns) {
  auto NewFuncType = std::make_unique<FType>();
  NewFuncType->Params = Params;
  NewFuncType->Returns = Returns;
  FuncTypes.push_back(std::move(NewFuncType));
  return ErrCode::Success;
}

/// Adder of function address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::addFuncAddr(unsigned int StoreFuncAddr) {
  FuncAddrs.push_back(StoreFuncAddr);
  return ErrCode::Success;
}

/// Adder of table address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::addTableAddr(unsigned int StoreTableAddr) {
  TableAddrs.push_back(StoreTableAddr);
  return ErrCode::Success;
}

/// Adder of memory address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::addMemAddr(unsigned int StoreMemAddr) {
  MemAddrs.push_back(StoreMemAddr);
  return ErrCode::Success;
}

/// Adder of global address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::addGlobalAddr(unsigned int StoreGlobAddr) {
  GlobalAddrs.push_back(StoreGlobAddr);
  return ErrCode::Success;
}

/// Getter of function address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getFuncAddr(unsigned int Idx, unsigned int &Addr) {
  if (FuncAddrs.size() <= Idx)
    return ErrCode::WrongInstanceAddress;
  Addr = FuncAddrs[Addr];
  return ErrCode::Success;
}

/// Getter of table address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getTableAddr(unsigned int Idx, unsigned int &Addr) {
  if (TableAddrs.size() <= Idx)
    return ErrCode::WrongInstanceAddress;
  Addr = TableAddrs[Addr];
  return ErrCode::Success;
}

/// Getter of memory address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getMemAddr(unsigned int Idx, unsigned int &Addr) {
  if (MemAddrs.size() <= Idx)
    return ErrCode::WrongInstanceAddress;
  Addr = MemAddrs[Addr];
  return ErrCode::Success;
}

/// Getter of global address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getGlobalAddr(unsigned int Idx, unsigned int &Addr) {
  if (GlobalAddrs.size() <= Idx)
    return ErrCode::WrongInstanceAddress;
  Addr = GlobalAddrs[Addr];
  return ErrCode::Success;
}

/// Set start function address in Store. See
/// "include/executor/instance/module.h".
ErrCode ModuleInstance::setStartIdx(unsigned int Idx) {
  if (FuncAddrs.size() <= Idx)
    return ErrCode::WrongInstanceAddress;
  StartAddr = FuncAddrs[Idx];
  HasStartFunc = true;
  return ErrCode::Success;
}

/// Get function type by index. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getFuncType(unsigned int Idx, FType *Type) {
  if (FuncTypes.size() <= Idx)
    return ErrCode::WrongInstanceAddress;
  Type = FuncTypes[Idx].get();
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
