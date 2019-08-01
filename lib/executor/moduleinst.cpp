#include "executor/moduleinst.h"

/// Adder of function types. See "include/executor/moduleinst.h".
Executor::ErrCode
ModuleInstance::addFuncType(std::vector<AST::ValType> &Params,
                            std::vector<AST::ValType> &Returns) {
  auto NewFuncType = std::make_unique<FType>();
  NewFuncType->Params = std::move(Params);
  NewFuncType->Returns = std::move(Returns);
  FuncTypes.push_back(std::move(NewFuncType));
  return Executor::ErrCode::Success;
}

/// Adder of function address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::addFuncAddr(unsigned int StoreFuncAddr) {
  FuncAddrs.push_back(StoreFuncAddr);
  return Executor::ErrCode::Success;
}

/// Adder of table address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::addTableAddr(unsigned int StoreTableAddr) {
  TableAddrs.push_back(StoreTableAddr);
  return Executor::ErrCode::Success;
}

/// Adder of memory address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::addMemAddr(unsigned int StoreMemAddr) {
  MemAddrs.push_back(StoreMemAddr);
  return Executor::ErrCode::Success;
}

/// Adder of global address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::addGlobalAddr(unsigned int StoreGlobAddr) {
  GlobalAddrs.push_back(StoreGlobAddr);
  return Executor::ErrCode::Success;
}

/// Getter of function address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::getFuncAddr(unsigned int Idx,
                                              unsigned int &Addr) {
  if (FuncAddrs.size() <= Idx)
    return Executor::ErrCode::WrongInstanceAddress;
  Addr = FuncAddrs[Addr];
  return Executor::ErrCode::Success;
}

/// Getter of table address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::getTableAddr(unsigned int Idx,
                                               unsigned int &Addr) {
  if (TableAddrs.size() <= Idx)
    return Executor::ErrCode::WrongInstanceAddress;
  Addr = TableAddrs[Addr];
  return Executor::ErrCode::Success;
}

/// Getter of memory address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::getMemAddr(unsigned int Idx,
                                             unsigned int &Addr) {
  if (MemAddrs.size() <= Idx)
    return Executor::ErrCode::WrongInstanceAddress;
  Addr = MemAddrs[Addr];
  return Executor::ErrCode::Success;
}

/// Getter of global address. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::getGlobalAddr(unsigned int Idx,
                                                unsigned int &Addr) {
  if (GlobalAddrs.size() <= Idx)
    return Executor::ErrCode::WrongInstanceAddress;
  Addr = GlobalAddrs[Addr];
  return Executor::ErrCode::Success;
}

/// Set start function address in Store. See "include/executor/moduleinst.h".
Executor::ErrCode ModuleInstance::setStartIdx(unsigned int Idx) {
  if (FuncAddrs.size() <= Idx)
    return Executor::ErrCode::WrongInstanceAddress;
  StartAddr = FuncAddrs[Idx];
  return Executor::ErrCode::Success;
}