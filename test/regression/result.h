//===-- ssvm/test/regression/result.h - result class definition -----------===//
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

#include "executor/common.h"
#include "loader/common.h"
#include "loader/filemgr.h"
#include <variant>

class Result {
public:
  enum class Stage : unsigned int { Init, Loader, Executor, Invalid };
  enum class StorageMutability : unsigned int { Pure, View, Modified };
  enum class State : unsigned int { commit, revert };
  Result() = default;
  ~Result() = default;
  bool equal(Result &Other);

  bool setStage(Stage NewStage);
  bool setStorageMut(StorageMutability NewStorageMut);
  bool setState(State NewState);
  bool setErrCode(Loader::ErrCode Code);
  bool setErrCode(Executor::ErrCode Code);

private:
  Stage LastStage = Stage::Invalid;
  std::variant<Loader::ErrCode, Executor::ErrCode> Status = Loader::ErrCode::Success;
  StorageMutability StorageMut = StorageMutability::Pure;
  State ExecutionState = State::revert;
};