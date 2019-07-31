//===-- ssvm/vm/result.h - result class definition ------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the result class interface.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace SSVM {

class Result {
public:
  enum class ErrCode : unsigned int { Success, Invalid };
  enum class Stage : unsigned int { Init, Loader, Executor, Invalid };
  enum class StorageMutability : unsigned int { Pure, View, Modified };
  enum class State : unsigned int { commit, revert };
  Result() = default;
  ~Result() = default;
  bool equal(Result &Other);

  bool setStage(Stage NewStage);
  bool setStorageMut(StorageMutability NewStorageMut);
  bool setState(State NewState);
  bool setErrCode(ErrCode Code);

private:
  Stage LastStage = Stage::Invalid;
  ErrCode Status = ErrCode::Success;
  StorageMutability StorageMut = StorageMutability::Pure;
  State ExecutionState = State::revert;
};

} // namespace SSVM
