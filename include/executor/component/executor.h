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
#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "executor/component/instantiator.h"
#include "runtime/component/importmgr.h"
#include "runtime/component/storemgr.h"
#include "runtime/component/taskmgr.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/waitable.h"
#include "runtime/instance/module.h"

#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

class Executor;

/// The component-model executor, holding the core executor below it. It
/// schedules the task threads and completes the copies of host-owned stream
/// and future ends.
class ComponentExecutor {
public:
  explicit ComponentExecutor(Executor &CoreExec) noexcept : Core(CoreExec) {}

  /// The core executor below this one.
  Executor &getCoreExecutor() noexcept { return Core; }

  /// Instantiate a component as an anonymous component instance. Comp is read
  /// during instantiation only: the instance owns what it keeps afterwards.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiateComponent(Runtime::Component::StoreManager &StoreMgr,
                       const AST::Component::Component &Comp);

  /// Instantiate and register a component as a named component instance;
  /// Comp is read during instantiation only.
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
  Runtime::Component::TaskManager &getTaskManager() noexcept { return TaskMgr; }

  /// \name The scheduler: which task thread runs next.
  /// @{
  /// Resume ready parked threads until Done(); traps on a deadlock. A
  /// non-async-typed task parked on Root restricts what may take the stack.
  Expect<void>
  pumpUntil(const std::function<bool()> &Done,
            Runtime::Component::TaskThread *Root = nullptr) noexcept;

  /// Whether T may block here; a sync task needs another runnable thread.
  bool mayBlock(const Runtime::Component::Task &T) const noexcept;

  /// Complete the rendezvous of every host end posted against a parked peer.
  Expect<void> drainPostedTransmits() noexcept;
  /// @}

  /// \name Guest-driving entries of the async task runtime.
  /// @{
  /// canon lift: build and start the task for a lifted component function.
  Expect<Runtime::Component::Task *>
  liftCall(const Runtime::Instance::ComponentFunctionInstance *FuncInst,
           Runtime::Component::Task::OnStartCallback OnStart,
           Runtime::Component::Task::OnResolveCallback OnResolve,
           Runtime::Component::Task *CallerTask) noexcept;

  /// Run a resource destructor as an implicit sync task of its instance.
  Expect<void>
  resourceDtorCall(const Runtime::Instance::ComponentInstance *Impl,
                   Runtime::Instance::FunctionInstance *Dtor,
                   uint64_t Rep) noexcept;

  /// The rendezvous of one stream or future end with its peer, for guest
  /// and host-owned ends alike; the buffer of E is prepared by the caller.
  Expect<void> startCopy(Runtime::Instance::Component::TransmitEnd &E) noexcept;
  /// @}

private:
  /// \name Functions for instantiation.
  /// @{
  /// Instantiation of a root component instance: a synchronous walk on the
  /// calling thread, as one embedder entry. Only a start function runs as a
  /// task.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(Runtime::Component::StoreManager &StoreMgr,
              const AST::Component::Component &Comp,
              std::optional<std::string_view> Name = std::nullopt);

  /// Instantiation of a nested component instance.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(Runtime::Component::ImportManager &ImportMgr,
              const AST::Component::Component &Comp,
              Runtime::Instance::ComponentInstance *Parent = nullptr);

  /// The section walk shared by both instantiation entries.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::Component &Comp);

  /// Instantiation of Core Module Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::CoreModuleSection &CoreModSec);

  /// Instantiation of Core Instance Section.
  Expect<void>
  instantiate(Component::Instantiator &Ctx,
              const AST::Component::CoreInstanceSection &CoreInstSec);

  /// Instantiation of Core Type Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::CoreTypeSection &CoreTypeSec);

  /// Instantiation of Component Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::ComponentSection &CompSec);

  /// Instantiation of Instance Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::InstanceSection &InstSec);

  /// Instantiation of Alias Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::AliasSection &AliasSec);

  /// Instantiation of Type Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::TypeSection &TypeSec);

  /// Instantiation of Canonical Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::CanonSection &CanonSec);

  /// Instantiation of Start Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::StartSection &StartSec);

  /// Instantiation of Value Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::ValueSection &ValSec);

  /// Instantiation of Import Section; picks the root or nested side.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Import Section from the embedder-provided store.
  Expect<void> instantiate(Runtime::Component::StoreManager &StoreMgr,
                           Component::Instantiator &Ctx,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Import Section from the instantiation arguments.
  Expect<void> instantiate(Runtime::Component::ImportManager &ImportMgr,
                           Component::Instantiator &Ctx,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Export Section.
  Expect<void> instantiate(Component::Instantiator &Ctx,
                           const AST::Component::ExportSection &ExportSec);
  /// @}

  /// The canon-lift task body (all four lift shapes).
  Expect<void> runTaskBody(Runtime::Component::Task &T) noexcept;

  /// The task body of a host component function.
  Expect<void> runHostTaskBody(Runtime::Component::Task &T) noexcept;

  /// Whether V may take the stack while Pin's non-async-typed call runs.
  bool
  canResumeUnder(const Runtime::Component::Task &Pin,
                 const Runtime::Component::TaskThread &Thread) const noexcept;

  /// Run Body as one embedder entry, an invoke or an instantiation. The
  /// outermost entry on the scheduler thread drains the tasks and threads
  /// afterwards, except suspended ones; a latched trap drains everything and
  /// is what the entry returns.
  Expect<void> runEntry(const std::function<Expect<void>()> &Body) noexcept;

  /// Run Body as an implicit synchronous task of Inst: nested in the current
  /// task thread, or from the scheduler thread on a task thread of its own,
  /// pumped until it finishes. The start function of a core module may
  /// suspend, so it cannot run on the scheduler thread.
  Expect<void>
  runImplicitTask(const Runtime::Instance::ComponentInstance *Inst,
                  const std::function<Expect<void>()> &Body) noexcept;

  /// The core executor below this one.
  Executor &Core;
  /// Component-model task manager.
  Runtime::Component::TaskManager TaskMgr;
  /// Depth of the embedder entries in progress.
  uint32_t EntryDepth = 0;
};

} // namespace Executor
} // namespace WasmEdge
