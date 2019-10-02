#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICallDataCopy : public EEI {
public:
  EEICallDataCopy(std::vector<unsigned char> &CallData) : EEI(CallData) {}
  EEICallDataCopy() = delete;
  virtual ~EEICallDataCopy() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM