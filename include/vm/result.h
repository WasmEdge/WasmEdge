// SPDX-License-Identifier: Apache-2.0
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
namespace VM {

class Result {
public:
  using ErrCode = unsigned int;
  enum class Stage : unsigned int {
    Init,
    Loader,
    Validator,
    Executor,
    Invalid
  };
  enum class StorageMutability : unsigned int { Pure, View, Modified };
  enum class State : unsigned int { Commit, Revert, Fail };

public:
  Result() = default;
  ~Result() = default;

  friend bool operator==(Result &LHS, Result &RHS) {
    if (&LHS == &RHS)
      return true;
    if (LHS.LastStage != RHS.LastStage)
      return false;
    if (LHS.Status != RHS.Status)
      return false;
    if (LHS.StorageMut != RHS.StorageMut)
      return false;
    if (LHS.ExecutionState != RHS.ExecutionState)
      return false;
    return true;
  }

  void setStage(Stage NewStage) { LastStage = NewStage; }
  void setStorageMut(StorageMutability NewStorageMut) {
    StorageMut = NewStorageMut;
  }
  void clear() {
    LastStage = Stage::Invalid;
    Status = 0;
    StorageMut = StorageMutability::Pure;
    ExecutionState = State::Fail;
  }
  void setState(State NewState) { ExecutionState = NewState; }
  void setErrCode(ErrCode Code) { Status = Code; }
  bool hasError() { return Status != 0; }
  State getState() { return ExecutionState; }
  Stage getStage() { return LastStage; }
  ErrCode getErrCode() { return Status; }

private:
  Stage LastStage = Stage::Invalid;
  ErrCode Status = 0;
  StorageMutability StorageMut = StorageMutability::Pure;
  State ExecutionState = State::Fail;
};

} // namespace VM
} // namespace SSVM
