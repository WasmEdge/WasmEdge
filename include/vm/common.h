// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/vm/common.h - common definitions in vm -----------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the common-use definitions in VM.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace SSVM {
namespace VM {

/// VM error code enum class.
enum class ErrCode : unsigned int { Success = 0, Failed, Invalid };

} // namespace VM
} // namespace SSVM
