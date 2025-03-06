// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "gc/allocator.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

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

  ~GlobalInstance() noexcept {
    if (Allocator) {
      Allocator->removeGlobal(Value);
    }
  }

  void setAllocator(GC::Allocator &A) noexcept {
    assuming(Allocator == nullptr);
    Allocator = &A;
    Allocator->addGlobal(Value);
  }

  /// Getter of global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter of value.
  const ValVariant getValue() const noexcept { return Value; }

  /// Setter of value.
  void setValue(const ValVariant &Val) noexcept { Value = Val; }

  ValVariant *getAddress() noexcept { return &Value; }

private:
  /// \name Data of global instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  AST::GlobalType GlobType;
  ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
