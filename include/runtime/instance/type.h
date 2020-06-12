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

#include "common/types.h"
#include "support/span.h"

#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

/// Function type definition in this module.
struct FType {
  FType() = default;
  FType(Span<const ValType> P, Span<const ValType> R)
      : Params(P.begin(), P.end()), Returns(R.begin(), R.end()) {}
  std::vector<ValType> Params;
  std::vector<ValType> Returns;
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
