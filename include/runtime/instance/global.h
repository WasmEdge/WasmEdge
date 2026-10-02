// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

class ModuleInstance;

class GlobalInstance {
public:
  GlobalInstance() = delete;
  GlobalInstance(const AST::GlobalType &GType,
                 ValVariant Val = uint128_t(0U)) noexcept
      : GlobType(GType), Value(Val) {
    assuming(GType.getValType().isNumType() ||
             GType.getValType().isNullableRefType() ||
             !Val.get<RefVariant>().isNull());
  }
  /// Constructor for a global instance defined by a module.
  GlobalInstance(const ModuleInstance *Mod, const AST::GlobalType &GType,
                 ValVariant Val) noexcept
      : GlobalInstance(GType, Val) {
    ModInst = Mod;
  }

  /// Getter for the defining module instance, if any.
  const ModuleInstance *getModule() const noexcept { return ModInst; }

  /// Getter for global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter for value.
  const ValVariant &getValue() const noexcept { return Value; }
  ValVariant &getValue() noexcept { return Value; }

  /// Setter for value.
  void setValue(const ValVariant &Val) noexcept { Value = Val; }

private:
  /// \name Data of global instance.
  /// @{
  const ModuleInstance *ModInst = nullptr;
  AST::GlobalType GlobType;
  alignas(16) ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
