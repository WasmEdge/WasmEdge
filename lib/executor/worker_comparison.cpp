#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker_util.h"

namespace SSVM {
namespace Executor {

template <typename T>
ErrCode Worker::runLeSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  std::unique_ptr<ValueEntry> NewVal =
      std::make_unique<ValueEntry>((Int1 <= Int2) ? 1 : 0);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runEqOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  std::unique_ptr<ValueEntry> NewVal =
      std::make_unique<ValueEntry>((Int1 == Int2) ? 1 : 0);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runNeOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  std::unique_ptr<ValueEntry> NewVal =
      std::make_unique<ValueEntry>((Int1 != Int2) ? 1 : 0);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runLtUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  std::unique_ptr<ValueEntry> NewVal =
      std::make_unique<ValueEntry>((Int1 < Int2) ? 1 : 0);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
