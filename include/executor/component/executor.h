// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/executor.h ----------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The component model executor. It instantiates components and invokes their
/// functions, and it layers on the core executor.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "common/errcode.h"
#include "common/types.h"
#include "executor/component/instantiator.h"
#include "runtime/component/storemgr.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/module.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

class Executor;

namespace Component {

/// The component model executor. A component embeds core modules and a lifted
/// function calls a core function, so this class holds the core executor and
/// never the other way round.
class Executor {
public:
  explicit Executor(WasmEdge::Executor::Executor &CoreExec) noexcept
      : Core(CoreExec) {}

  /// The core executor below this one.
  WasmEdge::Executor::Executor &core() noexcept { return Core; }

  /// Instantiate a component as an anonymous component instance.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiateComponent(Runtime::Component::StoreManager &StoreMgr,
                       const AST::Component::Component &Comp);

  /// Instantiate and register a component as a named component instance.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  registerComponent(Runtime::Component::StoreManager &StoreMgr,
                    const AST::Component::Component &Comp,
                    std::string_view Name);

  /// Register an instantiated component under its own name.
  Expect<void>
  registerComponent(Runtime::Component::StoreManager &StoreMgr,
                    const Runtime::Instance::ComponentInstance &CompInst);

  /// Invoke a component function by function instance.
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  invoke(const Runtime::Instance::Component::FunctionInstance *FuncInst,
         Span<const ComponentValVariant> Params,
         Span<const ComponentValType> ParamTypes);

private:
  /// \name Functions for instantiation.
  /// @{
  /// Instantiation of a root component instance.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(Runtime::Component::StoreManager &StoreMgr,
              const AST::Component::Component &Comp,
              std::optional<std::string_view> Name = std::nullopt);

  /// Instantiation of a nested component instance.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(Runtime::Instance::Component::ImportManager &ImportMgr,
              const AST::Component::Component &Comp,
              const Runtime::Instance::ComponentInstance *Parent = nullptr);

  /// The section walk shared by both instantiation entries.
  Expect<void> instantiate(Instantiator &Ctx,
                           const AST::Component::Component &Comp);

  /// Instantiation of Core Module Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::CoreModuleSection &CoreModSec);

  /// Instantiation of Core Instance Section.
  Expect<void>
  instantiate(Runtime::Instance::ComponentInstance &CompInst,
              const AST::Component::CoreInstanceSection &CoreInstSec);

  /// Instantiation of Core Type Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::CoreTypeSection &CoreTypeSec);

  /// Instantiation of Component Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ComponentSection &CompSec);

  /// Instantiation of Instance Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::InstanceSection &InstSec);

  /// Instantiation of Alias Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::AliasSection &AliasSec);

  /// Instantiation of Type Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::TypeSection &TypeSec);

  /// Instantiation of Canonical Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::CanonSection &CanonSec);

  /// Instantiation of Start Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::StartSection &StartSec);

  /// Instantiation of Value Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ValueSection &ValSec);

  /// Instantiation of Import Section. It picks the resolution side, because
  /// the two sides differ in the sorts they provide and in their diagnostics.
  Expect<void> instantiate(Instantiator &Ctx,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Import Section from the embedder-provided store.
  Expect<void> instantiate(Runtime::Component::StoreManager &StoreMgr,
                           Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Import Section from the instantiation arguments.
  Expect<void>
  instantiate(Runtime::Instance::Component::ImportManager &ImportMgr,
              Runtime::Instance::ComponentInstance &CompInst,
              const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Export Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ExportSection &ExportSec);
  /// @}

  /// \name Helper Functions for canonical ABI
  /// @{
  Expect<std::vector<ValVariant>>
  convValsToCoreWASM(Span<const ComponentValVariant> Vals,
                     Span<const ComponentValType> ValTypes,
                     Runtime::Instance::FunctionInstance *RFuncInst,
                     Runtime::Instance::MemoryInstance *MemInst,
                     const Runtime::Instance::ComponentInstance *CompInst,
                     StringEncoding Enc = StringEncoding::UTF8);

  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  convValsToComponent(Span<const std::pair<ValVariant, ValType>> CoreVals,
                      Span<const ComponentValType> ValTypes,
                      Runtime::Instance::MemoryInstance *MemInst,
                      const Runtime::Instance::ComponentInstance *CompInst,
                      StringEncoding Enc = StringEncoding::UTF8);
  /// @}

  /// The core executor below this one.
  WasmEdge::Executor::Executor &Core;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
