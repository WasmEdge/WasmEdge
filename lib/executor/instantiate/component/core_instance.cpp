// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <class> inline constexpr bool AlwaysFalseV = false;

Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   const AST::Component &Comp,
                                   const AST::CoreInstance::T &CoreInst) {
  auto Mods = Comp.getModuleSection().getContent();
  return std::visit(
      [this, &StoreMgr, &Mods](auto &&Arg) -> Expect<void> {
        using T = std::decay_t<decltype(Arg)>;
        if constexpr (std::is_same_v<T, AST::CoreInstance::Instantiate>) {
          Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>> EModInst =
              instantiate(StoreMgr, std::move(*(Mods[Arg.getModuleIdx()])));
          if (!EModInst) {
            return Unexpect(EModInst);
          }
          auto ModInst = std::move(*EModInst);
          for (auto &InstantiateArg : Arg.getInstantiateArgs()) {
            // (with name (instance idx))
            //
            // In this case, the idx should be a module that already
            // instantiated.
            auto Name = InstantiateArg.getName();
            auto Idx = InstantiateArg.getIndex();
            // FIXME:
            // Make a correct function that refers to another module
            (*ModInst).exportGlobal(Name, Idx);
          }
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
