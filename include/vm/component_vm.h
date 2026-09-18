// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/vm/component_vm.h - Component VM class definition --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component VM: the loader, validator, executors,
/// and stores a component runs on, and the stages of loading, validating,
/// instantiating, and executing it.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "common/async.h"
#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/span.h"
#include "common/statistics.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "runtime/component/storemgr.h"
#include "runtime/instance/component/component.h"
#include "runtime/storemgr.h"
#include "validator/validator.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace VM {

/// The component VM. It keeps every component it loaded alive while the
/// instances that read them exist, as a module outlives its instance.
class ComponentVM {
public:
  ComponentVM() = delete;
  ComponentVM(const Configure &Conf);
  ComponentVM(const Configure &Conf, Runtime::Component::StoreManager &S);
  ~ComponentVM();

  /// ======= Functions can be called before the instantiated stage. =======
  /// Register a component under a name.
  Expect<void> registerComponent(std::string_view Name,
                                 const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, Path);
  }

  Expect<void> registerComponent(std::string_view Name, Span<const Byte> Code) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, Code);
  }

  /// The caller keeps CompAST alive for as long as the instance.
  Expect<void> registerComponent(std::string_view Name,
                                 const AST::Component::Component &CompAST) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, CompAST);
  }

  /// Register an instantiated component under its own name.
  Expect<void>
  registerComponent(const Runtime::Instance::ComponentInstance &CompInst) {
    std::unique_lock Lock(Mutex);
    return ExecutorEngine.registerComponent(StoreRef, CompInst);
  }

  /// Load a component from a file or bytes.
  Expect<void> loadWasm(const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Path);
  }
  Expect<void> loadWasm(Span<const Byte> Code) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Code);
  }

  /// ======= Functions can be called after the loaded stage. =======
  Expect<void> validate() {
    std::unique_lock Lock(Mutex);
    return unsafeValidate();
  }

  /// ======= Functions can be called after the validated stage. =======
  Expect<void> instantiate() {
    std::unique_lock Lock(Mutex);
    return unsafeInstantiate();
  }

  /// ======= Functions can be called after the instantiated stage. =======
  /// Execute an exported function of the active component, by its name or
  /// by `interface#func`.
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  execute(std::string_view Func, Span<const ComponentValVariant> Params = {},
          Span<const ComponentValType> ParamTypes = {}) {
    std::shared_lock Lock(Mutex);
    return unsafeExecute(Func, Params, ParamTypes);
  }

  /// Execute an exported function of a registered component.
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  execute(std::string_view CompName, std::string_view Func,
          Span<const ComponentValVariant> Params = {},
          Span<const ComponentValType> ParamTypes = {}) {
    std::shared_lock Lock(Mutex);
    return unsafeExecute(CompName, Func, Params, ParamTypes);
  }

  /// Asynchronously execute an exported function.
  Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
  asyncExecute(std::string_view Func,
               Span<const ComponentValVariant> Params = {},
               Span<const ComponentValType> ParamTypes = {});

  Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
  asyncExecute(std::string_view CompName, std::string_view Func,
               Span<const ComponentValVariant> Params = {},
               Span<const ComponentValType> ParamTypes = {});

  /// Stop execution.
  void stop() noexcept { CoreExecutorEngine.stop(); }

  /// Drop every loaded component and instance.
  void cleanup() {
    std::unique_lock Lock(Mutex);
    unsafeCleanup();
  }

  /// Whether a component is loaded.
  bool holdsComponent() const {
    std::shared_lock Lock(Mutex);
    return Comp != nullptr;
  }

  /// The exported functions of the active component and their types.
  std::vector<std::pair<std::string, const AST::Component::FuncType &>>
  getFunctionList() const {
    std::shared_lock Lock(Mutex);
    return unsafeGetFunctionList();
  }

  /// The active component instance.
  const Runtime::Instance::ComponentInstance *getActiveComponent() const {
    std::shared_lock Lock(Mutex);
    return ActiveCompInst.get();
  }

  /// Getters for the engines and stores.
  Runtime::Component::StoreManager &getStoreManager() noexcept {
    return StoreRef;
  }
  const Runtime::Component::StoreManager &getStoreManager() const noexcept {
    return StoreRef;
  }
  Runtime::StoreManager &getCoreStoreManager() noexcept { return CoreStore; }
  Loader::Loader &getLoader() noexcept { return LoaderEngine; }
  Validator::Validator &getValidator() noexcept { return ValidatorEngine; }
  Executor::ComponentExecutor &getExecutor() noexcept { return ExecutorEngine; }
  Executor::Executor &getCoreExecutor() noexcept { return CoreExecutorEngine; }
  Statistics::Statistics &getStatistics() noexcept { return Stat; }

private:
  Expect<void> unsafeRegisterComponent(std::string_view Name,
                                       const std::filesystem::path &Path);
  Expect<void> unsafeRegisterComponent(std::string_view Name,
                                       Span<const Byte> Code);
  Expect<void>
  unsafeRegisterComponent(std::string_view Name,
                          const AST::Component::Component &CompAST);
  Expect<void>
  unsafeRegisterComponent(std::string_view Name,
                          std::unique_ptr<AST::Component::Component> CompAST);

  Expect<void> unsafeLoadWasm(const std::filesystem::path &Path);
  Expect<void> unsafeLoadWasm(Span<const Byte> Code);
  Expect<void> unsafeLoadWasm(std::unique_ptr<AST::Component::Component> C);

  Expect<void> unsafeValidate();
  Expect<void> unsafeInstantiate();

  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  unsafeExecute(std::string_view Func,
                Span<const ComponentValVariant> Params = {},
                Span<const ComponentValType> ParamTypes = {});
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  unsafeExecute(std::string_view CompName, std::string_view Func,
                Span<const ComponentValVariant> Params = {},
                Span<const ComponentValType> ParamTypes = {});
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  unsafeExecute(const Runtime::Instance::ComponentInstance *CompInst,
                std::string_view Func,
                Span<const ComponentValVariant> Params = {},
                Span<const ComponentValType> ParamTypes = {});

  void unsafeCleanup();

  std::vector<std::pair<std::string, const AST::Component::FuncType &>>
  unsafeGetFunctionList() const;

  void unsafeInitVM();
  void unsafeLoadPlugInHosts();
  void unsafeRegisterPlugInHosts();

  enum class VMStage : uint8_t { Inited, Loaded, Validated, Instantiated };

  /// \name VM environment.
  /// @{
  const Configure Conf;
  Statistics::Statistics Stat;
  VMStage Stage;
  mutable std::shared_mutex Mutex;
  /// @}

  /// \name VM components.
  /// @{
  Loader::Loader LoaderEngine;
  Validator::Validator ValidatorEngine;
  Executor::Executor CoreExecutorEngine;
  /// @}

  /// \name VM Storage. The instances go before the executor, so the executor
  /// aborts what still runs for them after they are gone.
  /// @{
  /// The core store of the module instances of the components.
  Runtime::StoreManager CoreStore;
  /// Self-owned component store (nullptr if an outside store is assigned in
  /// constructor).
  std::unique_ptr<Runtime::Component::StoreManager> Store;
  /// Reference to the component store.
  Runtime::Component::StoreManager &StoreRef;
  /// Every component loaded, kept alive for the instances reading it; the
  /// active one is Comp.
  std::vector<std::unique_ptr<AST::Component::Component>> Comps;
  const AST::Component::Component *Comp = nullptr;
  /// Active component instance.
  std::unique_ptr<Runtime::Instance::ComponentInstance> ActiveCompInst;
  /// Registered component instances by user.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      RegCompInsts;
  /// Loaded component instances from plug-ins.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      PlugInCompInsts;
  /// The component executor, after everything it may still run for.
  Executor::ComponentExecutor ExecutorEngine;
  /// @}
};

} // namespace VM
} // namespace WasmEdge
