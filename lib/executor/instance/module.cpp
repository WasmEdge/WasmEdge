#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {
namespace Instance {

/// Adder of function types. See "include/executor/instance/module.h".
ErrCode ModuleInstance::addFuncType(std::vector<AST::ValType> &Params,
                                    std::vector<AST::ValType> &Returns) {
  auto NewFuncType = std::make_unique<FType>();
  NewFuncType->Params = std::move(Params);
  NewFuncType->Returns = std::move(Returns);
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

/// Getter of function instances number. See
/// "include/executor/instance/module.h".
ErrCode ModuleInstance::getFuncNum(unsigned int &Num) {
  Num = FuncAddrs.size();
  return ErrCode::Success;
}

/// Getter of table instances number. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getTableNum(unsigned int &Num) {
  Num = TableAddrs.size();
  return ErrCode::Success;
}

/// Getter of memory instances number. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getMemNum(unsigned int &Num) {
  Num = MemAddrs.size();
  return ErrCode::Success;
}

/// Getter of global instances number. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getGlobalNum(unsigned int &Num) {
  Num = GlobalAddrs.size();
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

/// Get start function address. See "include/executor/instance/module.h".
ErrCode ModuleInstance::getStartAddr(unsigned int &Addr) {
  if (!HasStartFunc)
    return ErrCode::WrongInstanceAddress;
  Addr = StartAddr;
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
