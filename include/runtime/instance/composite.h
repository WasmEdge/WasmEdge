// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

#include <cstddef>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ModuleInstance;

class CompositeBase {
public:
  /// Constructor for only host function instance case.
  CompositeBase() noexcept : ModInst(nullptr), TypeIdx(0) {}
  /// Constructor for function, array, and struct instances.
  CompositeBase(const ModuleInstance *Mod, const uint32_t Idx) noexcept
      : ModInst(Mod), TypeIdx(Idx) {
    assuming(ModInst);
  }

  /// Getter for the module instance of this instance.
  const ModuleInstance *getModule() const noexcept {
    // RefVariant::getInnerPtr reads a function reference's payload (a
    // FunctionInstance, whose sole base is CompositeBase) as its leading
    // `const ModuleInstance *` without the concrete type; keep ModInst first.
    static_assert(offsetof(CompositeBase, ModInst) == 0,
                  "CompositeBase must begin with the ModInst pointer");
    return ModInst;
  }

  /// Getter for the closed type index of this instance in the module.
  uint32_t getTypeIndex() const noexcept { return TypeIdx; }

  /// Getter for the value type in defined type form.
  ValType getDefType() const noexcept {
    if (ModInst) {
      return ValType(TypeCode::Ref, TypeIdx);
    } else {
      // nullptr `ModInst` case is only for host function instance case.
      return ValType(TypeCode::Ref, TypeCode::FuncRef);
    }
  }

protected:
  friend class ModuleInstance;
  void linkDefinedType(const ModuleInstance *Mod,
                       const uint32_t Index) noexcept {
    assuming(Mod);
    ModInst = Mod;
    TypeIdx = Index;
  }

  /// \name Data of composite instances.
  /// @{
  const ModuleInstance *ModInst;
  uint32_t TypeIdx;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
