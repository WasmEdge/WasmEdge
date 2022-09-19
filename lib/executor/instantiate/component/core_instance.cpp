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
            std::string_view Name = Export.getName();
            AST::SortIndex Idx = Export.getExtern();
            Idx.getSort();
            Idx.getIndex();
            // TODO: what to do here?
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
