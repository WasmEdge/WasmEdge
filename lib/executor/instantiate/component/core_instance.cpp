// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <class> inline constexpr bool AlwaysFalseV = false;

Expect<void>
Executor::instantiateCore(Runtime::StoreManager &StoreMgr,
                          Runtime::Instance::ComponentInstance &CompInst,
                          const AST::Component &Comp,
                          const AST::CoreInstance::T &CoreInst) {
  CompInst.initCoreInstance();
  Span<const std::unique_ptr<AST::Module>> Mods =
      Comp.getModuleSection().getContent();
  return std::visit(
      [this, &StoreMgr, &Mods, &CompInst](auto &&Arg) -> Expect<void> {
        using T = std::decay_t<decltype(Arg)>;
        if constexpr (std::is_same_v<T, AST::CoreInstance::Instantiate>) {
          for (const AST::CoreInstantiateArg &InstantiateArg :
               Arg.getInstantiateArgs()) {
            // a scoped name only for instantiate the module
            auto Name = InstantiateArg.getName();
            // a module instance that already initialized
            auto Idx = InstantiateArg.getIndex();
            StoreMgr.NamedMod[Name] = CompInst.getCoreInstance(Idx);
          }

          // The module instantiate with meta information we just insert
          Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>> EModInst =
              instantiate(StoreMgr, std::move(*(Mods[Arg.getModuleIdx()])));
          if (!EModInst) {
            return Unexpect(EModInst);
          }
          CompInst.addCoreInstance(EModInst->get());
          return {};
        } else if constexpr (std::is_same_v<T, AST::CoreInstance::Export>) {
          for (auto &Export : Arg.getExports()) {
            auto *Mod = new Runtime::Instance::ModuleInstance{""};
            CompInst.addCoreInstance(Mod);

            std::string_view Name = Export.getName();
            AST::SortIndex Extern = Export.getExtern();
            auto I = Extern.getIndex();
            switch (Extern.getSort()) {
            case AST::Sort::CoreFunc:
              Mod->exportFunction(Name, I);
              break;
            case AST::Sort::Table:
              Mod->exportTable(Name, I);
              break;
            case AST::Sort::Memory:
              Mod->exportMemory(Name, I);
              break;
            case AST::Sort::Global:
              Mod->exportGlobal(Name, I);
              break;
            case AST::Sort::CoreType:
              // NOTE:
              // type is added to core:sort in anticipation of the type-imports
              // proposal. Until that proposal, core modules won't be able to
              // actually import or export types, however, the type sort is
              // allowed as part of outer aliases (below).
              spdlog::error(
                  ErrInfo::InfoAST(ASTNodeAttr::CompSec_CoreInstance));
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
              return Unexpect(ErrCode::Value::JunkSection);
            case AST::Sort::Module:
            case AST::Sort::CoreInstance:
              // NOTE:
              // module and instance are added to core:sort in anticipation of
              // the module-linking proposal, which would add these types to
              // Core WebAssembly. Until then, they are useful for aliases
              // (below).
              spdlog::error(
                  ErrInfo::InfoAST(ASTNodeAttr::CompSec_CoreInstance));
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
              return Unexpect(ErrCode::Value::JunkSection);
            default:
              spdlog::error(
                  ErrInfo::InfoAST(ASTNodeAttr::CompSec_CoreInstance));
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
              return Unexpect(ErrCode::Value::Unreachable);
            }
          }
          return {};
        } else {
          static_assert(AlwaysFalseV<T>, "non-exhaustive visitor!");
        }
      },
      CoreInst);
}

} // namespace Executor
} // namespace WasmEdge
