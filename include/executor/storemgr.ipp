#include "storemgr.h"

namespace SSVM {
namespace Executor {

/// Helper function for inserting instance. See "include/executor/storemgr.h".
template <typename T>
std::enable_if_t<IsInstanceV<T>, ErrCode>
StoreManager::insertInstance(std::unique_ptr<T> &Inst,
                             std::vector<std::unique_ptr<T>> &InstsVec,
                             unsigned int &NewId) {
  Inst->Addr = InstsVec.size();
  NewId = InstsVec.size();
  InstsVec.push_back(std::move(Inst));
  return ErrCode::Success;
}

/// Helper function for getting instance. See "include/executor/storemgr.h".
template <typename T>
std::enable_if_t<IsInstanceV<T>, ErrCode>
StoreManager::getInstance(const unsigned int Addr,
                          std::vector<std::unique_ptr<T>> &InstsVec, T *&Inst) {
  if (InstsVec.size() <= Addr)
    return ErrCode::WrongInstanceAddress;
  Inst = InstsVec.at(Addr).get();
  return ErrCode::Success;
}

/// Helper function for finding instance. See "include/executor/storemgr.h".
template <typename T>
std::enable_if_t<IsEntityV<T>, ErrCode>
StoreManager::findEntity(const std::string &ModName,
                         const std::string &EntityName,
                         std::vector<std::unique_ptr<T>> &InstsVec, T *&Inst) {
  for (auto It = InstsVec.begin(); It != InstsVec.end(); It++) {
    if ((*It)->isName(ModName, EntityName)) {
      Inst = (*It).get();
      return ErrCode::Success;
    }
  }
  return ErrCode::WrongInstanceAddress;
}

} // namespace Executor
} // namespace SSVM
