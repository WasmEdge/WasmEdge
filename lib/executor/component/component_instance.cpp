// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <memory>
#include <utility>

namespace WasmEdge {
namespace Executor {

// Instantiate core instance section. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::CoreInstanceSection &CoreInstSec) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
    return E;
  };
  for (const auto &CoreInst : CoreInstSec.getContent()) {
    if (CoreInst.isInstantiateModule()) {
      EXPECTED_TRY(
          const auto *Mod,
          CompInst.getModule(CoreInst.getModuleIndex()).map_error(ReportError));
      EXPECTED_TRY(
          auto ModInst,
          instantiateModule(CompInst, *Mod, CoreInst.getInstantiateArgs())
              .map_error(ReportError));
      CompInst.addCoreModuleInstance(std::move(ModInst));
      continue;
    }
    // A core instance synthesized from the named core items.
    EXPECTED_TRY(auto ModInst,
                 instantiateInlineExports(CompInst, CoreInst.getInlineExports())
                     .map_error(ReportError));
    CompInst.addCoreModuleInstance(std::move(ModInst));
  }
  return {};
}

// Instantiate instance section. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::InstanceSection &InstSec) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
    return E;
  };
  for (const auto &Inst : InstSec.getContent()) {
    if (Inst.isInstantiateModule()) {
      // The imports resolve from the arguments, gathered as a provider.
      EXPECTED_TRY(const auto *Def,
                   CompInst.getComponentDefinition(Inst.getComponentIndex())
                       .map_error(ReportError));
      Runtime::Instance::ComponentInstance Provider("");
      for (const auto &Arg : Inst.getInstantiateArgs()) {
        EXPECTED_TRY(
            addExport(CompInst, Provider, Arg.getName(), Arg.getIndex())
                .map_error([](auto E) {
                  spdlog::error(
                      ErrInfo::InfoAST(ASTNodeAttr::Comp_InstanceArg));
                  return E;
                }));
      }
      EXPECTED_TRY(
          auto Nested,
          instantiate(Provider, *Def->Comp, Def->Env).map_error(ReportError));
      CompInst.addComponentInstance(std::move(Nested));
      continue;
    }
    // A component instance synthesized from the named items.
    auto Nested =
        std::make_unique<Runtime::Instance::ComponentInstance>("", &CompInst);
    for (const auto &Exp : Inst.getInlineExports()) {
      EXPECTED_TRY(addExport(CompInst, *Nested, Exp.getName(), Exp.getSortIdx())
                       .map_error([](auto E) {
                         spdlog::error(
                             ErrInfo::InfoAST(ASTNodeAttr::Comp_InlineExport));
                         return E;
                       })
                       .map_error(ReportError));
    }
    CompInst.addComponentInstance(std::move(Nested));
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
