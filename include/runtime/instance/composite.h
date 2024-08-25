// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/composite.h - Composite base definition -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the base class definition of composite instances
/// (function, struct, and array).
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/types.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ModuleInstance;
class ComponentInstance;

class CompositeBase {
public:
  /// Constructor for only host function instance case.
  CompositeBase() noexcept : ModInst(nullptr), CompInst(nullptr), TypeIdx(0) {}
  /// Constructor for function, array, and struct instances.
  CompositeBase(const ModuleInstance *Mod, const uint32_t Idx) noexcept
      : ModInst(Mod), TypeIdx(Idx) {
    assuming(ModInst);
  }
  CompositeBase(const ComponentInstance *Comp, const uint32_t Idx) noexcept
      : CompInst(Comp), TypeIdx(Idx) {
    assuming(CompInst);
  }

  /// Getter of module instance of this instance.
  const ModuleInstance *getModule() const noexcept { return ModInst; }
  const ComponentInstance *getComponent() const noexcept { return CompInst; }

  /// Getter of closed type index of this instance in the module.
  uint32_t getTypeIndex() const noexcept { return TypeIdx; }

  /// Getter of value type in defined type form.
  ValType getDefType() const noexcept {
    if (ModInst || CompInst) {
      return ValType(TypeCode::Ref, TypeIdx);
    } else {
      // nullptr `ModInst` and `CompInst` case is only for host function
      // instance case.
      return ValType(TypeCode::Ref, TypeCode::FuncRef);
    }
  }

protected:
  friend class ModuleInstance;
  friend class ComponentInstance;
  void linkDefinedType(const ModuleInstance *Mod,
                       const uint32_t Index) noexcept {
    assuming(Mod);
    ModInst = Mod;
    TypeIdx = Index;
  }
  void linkDefinedType(const ComponentInstance *Comp,
                       const uint32_t Index) noexcept {
    assuming(Comp);
    CompInst = Comp;
    TypeIdx = Index;
  }

  /// \name Data of composite instances.
  /// @{
  const ModuleInstance *ModInst;
  const ComponentInstance *CompInst;
  uint32_t TypeIdx;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
