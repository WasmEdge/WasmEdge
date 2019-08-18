#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker_util.h"

namespace SSVM {
namespace Executor {

template <typename T>
ErrCode Worker::runAddOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// FIXME: the following calculations do not apply `modulo 2^N`.
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Int1 + Int2);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runSubOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Int1 - Int2);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runMulOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// FIXME: the following calculations do not apply `modulo 2^N`.
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Int1 * Int2);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runDivUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// FIXME: the following calculations do not apply `modulo 2^N`.
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  if (Int2 == 0) {
    return ErrCode::DivideByZero;
  }
  std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Int1 / Int2);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runModUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// FIXME: the following calculations do not apply `modulo 2^N`.
  T Int1 = retrieveValue<T>(*Val1), Int2 = retrieveValue<T>(*Val2);
  if (Int2 == 0) {
    return ErrCode::DivideByZero;
  }
  std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Int1 % Int2);
  StackMgr.push(NewVal);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
