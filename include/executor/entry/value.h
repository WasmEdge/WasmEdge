// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/entry/value.h - Value Entry class definition --------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Value Entry class in stack manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/common.h"

namespace SSVM {
namespace Executor {

using Value = AST::ValVariant;

} // namespace Executor
} // namespace SSVM
