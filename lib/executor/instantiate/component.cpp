#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

// Instantiate module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      const AST::Component::Component &Comp,
                      std::optional<std::string_view> Name) {
  using namespace AST::Component;

  std::unique_ptr<Runtime::Instance::ComponentInstance> CompInst;
  if (Name.has_value()) {
    CompInst =
        std::make_unique<Runtime::Instance::ComponentInstance>(Name.value());
  } else {
    CompInst = std::make_unique<Runtime::Instance::ComponentInstance>("");
  }

  for (auto &Sec : Comp.getSections()) {
    if (std::holds_alternative<AST::CustomSection>(Sec)) {
    } else if (std::holds_alternative<AST::CoreModuleSection>(Sec)) {
      CompInst->addModule(std::get<AST::CoreModuleSection>(Sec).getContent());
    } else if (std::holds_alternative<ComponentSection>(Sec)) {
      CompInst->addComponent(std::get<ComponentSection>(Sec).getContent());
    } else if (std::holds_alternative<CoreInstanceSection>(Sec)) {
      auto Res =
          instantiate(StoreMgr, *CompInst, std::get<CoreInstanceSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<InstanceSection>(Sec)) {
      auto Res =
          instantiate(StoreMgr, *CompInst, std::get<InstanceSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<ImportSection>(Sec)) {
      auto Res = instantiate(StoreMgr, *CompInst, std::get<ImportSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<CoreTypeSection>(Sec)) {
      auto Res =
          instantiate(StoreMgr, *CompInst, std::get<CoreTypeSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<TypeSection>(Sec)) {
      auto Res = instantiate(StoreMgr, *CompInst, std::get<TypeSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<StartSection>(Sec)) {
      auto Res = instantiate(StoreMgr, *CompInst, std::get<StartSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<CanonSection>(Sec)) {
      auto Res = instantiate(StoreMgr, *CompInst, std::get<CanonSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<AliasSection>(Sec)) {
      auto Res = instantiate(StoreMgr, *CompInst, std::get<AliasSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    } else if (std::holds_alternative<ExportSection>(Sec)) {
      auto Res = instantiate(StoreMgr, *CompInst, std::get<ExportSection>(Sec));
      if (!Res) {
        return Unexpect(Res);
      }
    }
  }

  StoreMgr.registerComponent(CompInst.get());

  return CompInst;
}

} // namespace Executor
} // namespace WasmEdge
