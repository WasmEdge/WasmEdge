// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/instance/global.h - Global Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the global instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = delete;
  GlobalInstance(const AST::GlobalType &GType,
                 ValVariant Val = uint32_t(0)) noexcept
      : GlobType(GType), Value(Val) {}

  /// Getter of global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter of value.
  const ValVariant &getValue() const noexcept { return Value; }

  /// Getter of value.
  ValVariant &getValue() noexcept { return Value; }

private:
  /// \name Data of global instance.
  /// @{
  AST::GlobalType GlobType;
  ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
