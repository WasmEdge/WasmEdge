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

#include <type_traits>
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
  const ModuleInstance *getModule() const noexcept { return ModInst; }

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

// The AOT/JIT compiler reads ModInst (offset 0) and TypeIdx (offset 8) inline
// from a function instance, and FunctionInstance::CompiledCode right after the
// base. Standard layout keeps ModInst first with no tail-padding reuse, so
// these offsets and the base size are part of the codegen ABI.
static_assert(std::is_standard_layout_v<CompositeBase>,
              "compiled code relies on CompositeBase being standard layout");
static_assert(
    sizeof(CompositeBase) == 16,
    "compiled code reads FunctionInstance::CompiledCode at offset 16");

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
