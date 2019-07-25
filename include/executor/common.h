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

enum class ErrCode : unsigned int {
  Success = 0,
  InstantiateFailed,
  RuntimeDataMgrInitFailed,
  ExecutionEngineInitFailed,
  ExecutionFailed
};

} // namespace Executor