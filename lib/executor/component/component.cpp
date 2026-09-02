// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>

namespace WasmEdge {
namespace Executor {

namespace {
// The AST node attribute of a component section type, for error reporting.
template <typename T> constexpr ASTNodeAttr sectionAttr() noexcept {
  if constexpr (std::is_same_v<T, AST::Component::CoreModuleSection>) {
    return ASTNodeAttr::Comp_Sec_CoreMod;
  } else if constexpr (std::is_same_v<T, AST::Component::CoreInstanceSection>) {
    return ASTNodeAttr::Comp_Sec_CoreInstance;
  } else if constexpr (std::is_same_v<T, AST::Component::CoreTypeSection>) {
    return ASTNodeAttr::Comp_Sec_CoreType;
  } else if constexpr (std::is_same_v<T, AST::Component::ComponentSection>) {
    return ASTNodeAttr::Comp_Sec_Component;
  } else if constexpr (std::is_same_v<T, AST::Component::InstanceSection>) {
    return ASTNodeAttr::Comp_Sec_Instance;
  } else if constexpr (std::is_same_v<T, AST::Component::AliasSection>) {
    return ASTNodeAttr::Comp_Sec_Alias;
  } else if constexpr (std::is_same_v<T, AST::Component::TypeSection>) {
    return ASTNodeAttr::Comp_Sec_Type;
  } else if constexpr (std::is_same_v<T, AST::Component::CanonSection>) {
    return ASTNodeAttr::Comp_Sec_Canon;
  } else if constexpr (std::is_same_v<T, AST::Component::StartSection>) {
    return ASTNodeAttr::Comp_Sec_Start;
  } else if constexpr (std::is_same_v<T, AST::Component::ImportSection>) {
    return ASTNodeAttr::Comp_Sec_Import;
  } else if constexpr (std::is_same_v<T, AST::Component::ExportSection>) {
    return ASTNodeAttr::Comp_Sec_Export;
  } else {
    static_assert(std::is_same_v<T, AST::Component::ValueSection>);
    return ASTNodeAttr::Comp_Sec_Value;
  }
}
} // namespace

// Walk the sections of one component. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::Component &Comp) {
  for (const auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        return {};
      } else {
        return instantiate(Ctx, Sec).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(sectionAttr<T>()));
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
          return E;
        });
      }
    };
    EXPECTED_TRY(std::visit(Func, Section));
  }
  return {};
}

// Instantiate a root component instance. See executor.h.
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiate(Runtime::Component::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp,
                               std::optional<std::string_view> Name) {
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>(Name.value_or(""));
  EXPECTED_TRY(runEntry([&]() -> Expect<void> {
    // The instance cannot be entered until instantiation completes.
    Runtime::Instance::ComponentInstance::EnteredGuard EnterGuard{*CompInst,
                                                                  true};

    Component::Instantiator Ctx{StoreMgr, *CompInst};
    EXPECTED_TRY(instantiate(Ctx, Comp));
    CompInst->releaseSource(Comp);

    if (Name.has_value()) {
      StoreMgr.registerInstance(CompInst.get());
    }
    return {};
  }));
  return CompInst;
}

// Run Body as an implicit task of Inst. See executor.h.
Expect<void> ComponentExecutor::runImplicitTask(
    const Runtime::Instance::ComponentInstance *Inst,
    const std::function<Expect<void>()> &Body) noexcept {
  if (TaskMgr.getCurrentThread() != nullptr) {
    // Nested in the current task thread.
    Runtime::Component::TaskManager::NestedTaskGuard TaskGuard{TaskMgr, Inst};
    return Body();
  }

  // From the scheduler thread, a root activation on its own task thread.
  Runtime::Component::Task *T = TaskMgr.newTask();
  T->Opts.Inst = Inst;
  T->Status = Runtime::Component::Task::State::Started;
  Expect<void> Result = Unexpect(ErrCode::Value::ComponentAsyncAborted);
  TaskMgr.newThread(T, [&Body, &Result](Runtime::Component::ResumeReason R) {
    if (R == Runtime::Component::ResumeReason::Abort) {
      return;
    }
    Result = Body();
  });
  TaskMgr.resumeThread(T->Thread, Runtime::Component::ResumeReason::Normal);
  auto PumpRes = pumpUntil([T]() { return T->Thread->Finished; }, T->Thread);
  if (!PumpRes) {
    TaskMgr.noteTrap(PumpRes.error(), Inst);
    return Unexpect(PumpRes.error());
  }
  return Result;
}

// Instantiate a nested component instance. See executor.h.
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiate(Runtime::Component::ImportManager &ImportMgr,
                               const AST::Component::Component &Comp,
                               Runtime::Instance::ComponentInstance *Parent) {
  // Outer aliases resolve through the lexical parent.
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>("", Parent);
  // The instance cannot be entered until instantiation completes.
  Runtime::Instance::ComponentInstance::EnteredGuard EnterGuard{*CompInst,
                                                                true};

  Component::Instantiator Ctx{ImportMgr, *CompInst};
  EXPECTED_TRY(instantiate(Ctx, Comp));

  return CompInst;
}

// Instantiate component section. See executor.h.
Expect<void> ComponentExecutor::instantiate(
    Component::Instantiator &Ctx,
    const AST::Component::ComponentSection &CompSec) {
  Ctx.getInstance().addComponent(CompSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
