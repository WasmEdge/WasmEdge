// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/type.h - Function type definition -----------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function type definition used in module instance.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

/// Function type definition in this module.
struct FType {
  FType() = default;
  FType(const std::vector<ValType> &P, const std::vector<ValType> &R)
      : Params(P), Returns(R) {}
  std::vector<ValType> Params;
  std::vector<ValType> Returns;
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM