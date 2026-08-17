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
#include "runtime/component/importmgr.h"
#include "runtime/component/storemgr.h"
#include "runtime/component/taskmgr.h"
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

/// The component-model executor, holding the core executor below it.
class ComponentExecutor {
public:
  explicit ComponentExecutor(Executor &CoreExec) noexcept : Core(CoreExec) {}

  /// The core executor below this one.
  Executor &core() noexcept { return Core; }

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
  invoke(const Runtime::Instance::ComponentFunctionInstance *FuncInst,
         Span<const ComponentValVariant> Params,
         Span<const ComponentValType> ParamTypes);

  /// The component-model task manager below this executor.
  Runtime::Component::TaskManager &taskManager() noexcept { return TaskMgr; }

  /// \name Guest-driving entries of the async task runtime.
  /// @{
  /// canon lift: build and start the task for a lifted component function.
  Expect<Runtime::Component::Task *>
  liftCall(const Runtime::Instance::ComponentFunctionInstance *FuncInst,
           Runtime::Component::Task::OnStartCallback OnStart,
           Runtime::Component::Task::OnResolveCallback OnResolve,
           Runtime::Component::Task *CallerTask) noexcept;

  /// The canon-lift task body (all four lift shapes).
  Expect<void> runTaskBody(Runtime::Component::Task &T) noexcept;

  /// Run a resource destructor as an implicit sync task of its instance.
  Expect<void>
  resourceDtorCall(const Runtime::Instance::ComponentInstance *Impl,
                   Runtime::Instance::FunctionInstance *Dtor,
                   uint64_t Rep) noexcept;
  /// @}

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
  instantiate(Runtime::Component::ImportManager &ImportMgr,
              const AST::Component::Component &Comp,
              const Runtime::Instance::ComponentInstance *Parent = nullptr);

  /// The section walk shared by both instantiation entries.
  Expect<void> instantiate(Component::Instantiator &Ctx,
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

  /// Instantiation of Import Section; picks the root or nested side.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Import Section from the embedder-provided store.
  Expect<void> instantiate(Runtime::Component::StoreManager &StoreMgr,
                           Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Import Section from the instantiation arguments.
  Expect<void> instantiate(Runtime::Component::ImportManager &ImportMgr,
                           Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Export Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ExportSection &ExportSec);
  /// @}

  /// The core executor below this one.
  Executor &Core;
  /// Component-model task manager.
  Runtime::Component::TaskManager TaskMgr;
};

} // namespace Executor
} // namespace WasmEdge
