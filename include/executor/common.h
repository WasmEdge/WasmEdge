//===-- ssvm/executor/common.h - Common definitions in Executor -*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the common-use definitions in Executor.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace Executor {

/// Executor error code enum class.
enum class ErrCode : unsigned int {
  Success = 0,
  WrongExecutorFlow,    /// Wrong executor's flow
  InstantiateFailed,    /// Fail when instantiating
  WrongInstanceAddress, /// Wrong access of instances
  ExecutionFailed,      /// Runtime error when executing
  TypeNotMatch, /// Value type between instructions and stack or store not match
  StackWrongEntry, /// Entry type not match when get or pop entry
  StackEmpty       /// Empry stack when get or pop entry
};

} // namespace Executor