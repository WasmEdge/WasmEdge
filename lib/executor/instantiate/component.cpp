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

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreInstanceSection &Sec) {
  using namespace AST::Component;

  // TODO: maybe the instance index space should not in StoreMgr but
  // component instance?
  for (const CoreInstanceExpr &InstExpr : Sec.getContent()) {
    if (std::holds_alternative<CoreInstantiate>(InstExpr)) {
      auto Instantiate = std::get<CoreInstantiate>(InstExpr);

      for (auto Arg : Instantiate.getArgs()) {
        StoreMgr.addNamedModule(Arg.getName(),
                                CompInst.getModuleInstance(Arg.getIndex()));
      }
      const AST::Module &Mod = CompInst.getModule(Instantiate.getModuleIdx());
      auto Res = instantiate(StoreMgr, Mod);
      if (!Res) {
        return Unexpect(Res);
      }
      CompInst.addModuleInstance(std::move(*Res));
    } else {
      // TODO: create a module instance and insert the following function
      // instances into it, then put the new module instance into the component
      // instance.
      auto Sorts =
          std::get<AST::Component::CoreInlineExports>(InstExpr).getExports();
      for (auto S : Sorts) {
        auto SortIdx = S.getSortIdx();
        switch (SortIdx.getSort()) {
        case CoreSort::Func: {
          auto Idx = SortIdx.getSortIdx();
          CompInst.getFunctionInstance(Idx);
          break;
        }
        default:
          spdlog::warn("TODO: core inline export non-function");
          break;
        }
      }
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreTypeSection &CoreTypeSec) {
  for (auto Ty : CoreTypeSec.getContent()) {
    CompInst.addCoreType(Ty);
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::InstanceSection &Sec) {
  using namespace AST::Component;
  for (const auto &InstExpr : Sec.getContent()) {
    if (std::holds_alternative<AST::Component::Instantiate>(InstExpr)) {
      auto Instantiate = std::get<AST::Component::Instantiate>(InstExpr);

      for (auto Arg : Instantiate.getArgs()) {
        auto Idx = Arg.getIndex();
        auto S = Idx.getSort();
        if (std::holds_alternative<CoreSort>(S)) {
          // TODO: insert these into mapping
          switch (std::get<CoreSort>(S)) {
          case CoreSort::Func:
            spdlog::info("with {} core:func", Arg.getName());
            break;
          case CoreSort::Table:
            spdlog::info("with {} core:table", Arg.getName());
            break;
          case CoreSort::Memory:
            spdlog::info("with {} core:memory", Arg.getName());
            break;
          case CoreSort::Global:
            spdlog::info("with {} core:global", Arg.getName());
            break;
          case CoreSort::Type:
            spdlog::info("with {} core:type", Arg.getName());
            break;
          case CoreSort::Module:
            spdlog::info("with {} core:module", Arg.getName());
            break;
          case CoreSort::Instance:
            StoreMgr.addNamedModule(
                Arg.getName(), CompInst.getModuleInstance(Idx.getSortIdx()));
            break;
          }
        } else if (std::holds_alternative<SortCase>(S)) {
          switch (std::get<SortCase>(S)) {
          case SortCase::Func: // TODO:
            spdlog::info("with {} function", Arg.getName());
            break;
          case SortCase::Value: // TODO:
            spdlog::info("with {} value", Arg.getName());
            break;
          case SortCase::Type: // TODO:
            spdlog::info("with {} type", Arg.getName());
            break;
          case SortCase::Component:
            if (auto Res = StoreMgr.registerComponent(
                    Arg.getName(),
                    CompInst.getComponentInstance(Idx.getSortIdx()));
                !Res) {
              spdlog::error("register component instance failed");
              return Unexpect(Res);
            }
            break;
          case SortCase::Instance: // TODO:
            break;
          }
        }
      }
      auto C = CompInst.getComponent(Instantiate.getComponentIdx());
      auto Res = instantiate(StoreMgr, C);
      if (!Res) {
        return Unexpect(Res);
      }
      auto Inst = std::move(*Res);
      CompInst.addComponentInstance(std::move(Inst));
    } else {
      std::get<CompInlineExports>(InstExpr).getExports();
      // TODO: complete inline exports
      spdlog::warn("TODO: component inline exports");
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::AliasSection &AliasSec) {
  using namespace AST::Component;
  for (auto A : AliasSec.getContent()) {
    auto T = A.getTarget();
    auto S = A.getSort();
    if (std::holds_alternative<CoreSort>(S)) {
      switch (std::get<CoreSort>(S)) {
      case CoreSort::Func:
        if (std::holds_alternative<AliasTargetExport>(T)) {
          // This means instance exports a function
          auto Exp = std::get<AliasTargetExport>(T);
          const auto *ModInst =
              CompInst.getModuleInstance(Exp.getInstanceIdx());

          auto *FuncInst = ModInst->getFuncExports(
              [&](const std::map<std::string,
                                 Runtime::Instance::FunctionInstance *,
                                 std::less<>> &Map) {
                return ModInst->unsafeFindExports(Map, Exp.getName());
              });
          CompInst.addFunctionInstance(FuncInst);
        } else {
          auto Out = std::get<AliasTargetOuter>(T);
          Out.getComponent();
          Out.getIndex();
        }
        break;
      case CoreSort::Table: // TODO:
        break;
      case CoreSort::Memory: // TODO:
        break;
      case CoreSort::Global: // TODO:
        break;
      case CoreSort::Type: // TODO:
        break;
      case CoreSort::Module: // TODO:
        break;
      case CoreSort::Instance: // TODO:
        break;
      }
    } else if (std::holds_alternative<SortCase>(S)) {
      switch (std::get<SortCase>(S)) {
      case SortCase::Func: // TODO:
        if (std::holds_alternative<AliasTargetExport>(T)) {
          // This means instance exports a function
          auto Exp = std::get<AliasTargetExport>(T);

          // FIXME: this should, however, get a component instance, but we
          // haven't change anything at plugin part, and hence we do not have
          // anything create a component plugin.
          auto *ModInst = CompInst.getModuleInstance(Exp.getInstanceIdx());
          auto *FuncInst = ModInst->findFuncExports(Exp.getName());
          CompInst.addComponentFunctionInstance(FuncInst);
        } else {
          // TODO:
          spdlog::warn("TODO [alias function] outer export");
          auto Out = std::get<AliasTargetOuter>(T);
          auto NestedCompInst =
              CompInst.getComponentInstance(Out.getComponent());
          NestedCompInst->getFunctionInstance(Out.getIndex());
        }

        break;
      case SortCase::Value: // TODO:
        break;
      case SortCase::Type: // TODO:
        break;
      case SortCase::Component: // TODO:
        break;
      case SortCase::Instance: // TODO:
        break;
      }
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::TypeSection &TySec) {
  for (auto Ty : TySec.getContent()) {
    CompInst.addType(Ty);
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CanonSection &CanonSec) {
  using namespace AST::Component;
  for (auto &C : CanonSec.getContent()) {
    if (std::holds_alternative<Lift>(C)) {
      // lift wrap a core wasm function to a component function, with proper
      // modification about canonical ABI.

      auto L = std::get<Lift>(C);
      // TODO: apply options
      // L.getOptions();

      auto *FuncInst = CompInst.getFunctionInstance(L.getCoreFuncIndex());
      CompInst.addComponentFunctionInstance(FuncInst);
    } else if (std::holds_alternative<Lower>(C)) {
      // lower sends a component function to a core wasm function, with proper
      // modification about canonical ABI.

      auto L = std::get<Lower>(C);
      // TODO: apply options
      // L.getOptions();

      auto *FuncInst = CompInst.getComponentFunctionInstance(L.getFuncIndex());
      CompInst.addFunctionInstance(FuncInst);
    } else if (std::holds_alternative<ResourceNew>(C)) {
      // TODO:
    } else if (std::holds_alternative<ResourceDrop>(C)) {
      // TODO:
    } else if (std::holds_alternative<ResourceRep>(C)) {
      // TODO:
    }
  }
  return {};
}

Expect<void> Executor::instantiate(Runtime::StoreManager &,
                                   Runtime::Instance::ComponentInstance &,
                                   const AST::Component::StartSection &) {
  spdlog::warn("TODO: start section");
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ImportSection &Sec) {
  using namespace AST::Component;
  for (auto ImportStatement : Sec.getContent()) {
    auto Desc = ImportStatement.getDesc();
    if (std::holds_alternative<DescTypeIndex>(Desc)) {
      auto TypeIndex = std::get<DescTypeIndex>(Desc);
      // TODO: get index of type, then use this type to check the import
      // thing
      TypeIndex.getIndex();
      switch (TypeIndex.getKind()) {
      case IndexKind::CoreType:
        // TODO
        break;
      case IndexKind::FuncType:
        // TODO
        break;
      case IndexKind::ComponentType:
        // TODO
        break;
      case IndexKind::InstanceType:
        auto ModName = ImportStatement.getName();
        const auto *ImportedModInst = StoreMgr.findModule(ModName);
        if (unlikely(ImportedModInst == nullptr)) {
          spdlog::error(ErrCode::Value::UnknownImport);
          spdlog::error("module name: {}", ModName);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_CompImport));
          return Unexpect(ErrCode::Value::UnknownImport);
        }
        CompInst.addModuleInstance(ImportedModInst);
        break;
      }
    } else if (std::holds_alternative<TypeBound>(Desc)) {
      // TODO: import a type or resource
    } else if (std::holds_alternative<ValueType>(Desc)) {
      // TODO: import a value and check its type
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ExportSection &Sec) {
  using namespace WasmEdge::AST::Component;

  for (const auto &Export : Sec.getContent()) {
    auto SortIndex = Export.getSortIndex();

    auto Index = SortIndex.getSortIdx();
    auto S = SortIndex.getSort();
    if (std::holds_alternative<CoreSort>(S)) {
      spdlog::warn("TODO core sort {}",
                   static_cast<Byte>(std::get<CoreSort>(S)));
    } else {
      switch (std::get<SortCase>(S)) {
      case SortCase::Func:
        CompInst.addExport(Export.getName(),
                           CompInst.getComponentFunctionInstance(Index));
        break;

      default:
        spdlog::warn("TODO sort {}", static_cast<Byte>(std::get<SortCase>(S)));
        break;
      }
    }
  }

  return {};
}

} // namespace Executor
} // namespace WasmEdge
