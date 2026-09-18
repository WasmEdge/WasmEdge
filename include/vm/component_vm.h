// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/vm/component_vm.h - Component VM class definition --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component VM definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "common/async.h"
#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/span.h"
#include "common/statistics.h"
#include "common/types.h"
#include "executor/component/executor.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "runtime/component/storemgr.h"
#include "runtime/instance/component/component.h"
#include "validator/validator.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace VM {

/// The component VM.
class ComponentVM {
public:
  ComponentVM() = delete;
  ComponentVM(const Configure &Conf);
  ComponentVM(const Configure &Conf, Runtime::Component::StoreManager &S);
  ~ComponentVM();

  /// ======= Functions can be called before the instantiated stage. =======
  /// Register components and host components.
  Expect<void> registerComponent(std::string_view Name,
                                 const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, Path);
  }

  Expect<void> registerComponent(std::string_view Name, Span<const Byte> Code) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, Code);
  }

  /// Register a loaded component.
  Expect<void> registerComponent(std::string_view Name,
                                 const AST::Component::Component &CompAST) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, CompAST);
  }

  /// Register an instantiated component under its own name.
  Expect<void>
  registerComponent(const Runtime::Instance::ComponentInstance &CompInst) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(CompInst);
  }

  /// Unregister a named component instance.
  Expect<void> unregisterComponent(std::string_view Name) {
    std::unique_lock Lock(Mutex);
    return unsafeUnregisterComponent(Name);
  }

  /// Load given component file or bytecode.
  Expect<void> loadWasm(const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Path);
  }
  Expect<void> loadWasm(Span<const Byte> Code) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Code);
  }

  /// ======= Functions can be called after the loaded stage. =======
  /// Validate loaded component.
  Expect<void> validate() {
    std::unique_lock Lock(Mutex);
    return unsafeValidate();
  }

  /// ======= Functions can be called after the validated stage. =======
  /// Instantiate validated component.
  Expect<void> instantiate() {
    std::unique_lock Lock(Mutex);
    return unsafeInstantiate();
  }

  /// ======= Functions can be called after the instantiated stage. =======
  /// Execute an exported function of the active component.
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

  /// Asynchronously execute an exported function of the active component.
  Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
  asyncExecute(std::string_view Func,
               Span<const ComponentValVariant> Params = {},
               Span<const ComponentValType> ParamTypes = {});

  /// Asynchronously execute an exported function of a registered component.
  Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
  asyncExecute(std::string_view CompName, std::string_view Func,
               Span<const ComponentValVariant> Params = {},
               Span<const ComponentValType> ParamTypes = {});

  /// Stop execution.
  void stop() noexcept { CoreExecutorEngine.stop(); }

  /// ======= Functions which are stageless. =======
  /// Clean up VM status.
  void cleanup() {
    std::unique_lock Lock(Mutex);
    unsafeCleanup();
  }

  /// Get list of exported functions of the active component and their types.
  std::vector<std::pair<std::string, const AST::Component::FuncType &>>
  getFunctionList() const {
    std::shared_lock Lock(Mutex);
    return unsafeGetFunctionList();
  }

  /// Get current instantiated component instance.
  const Runtime::Instance::ComponentInstance *getActiveComponent() const {
    std::shared_lock Lock(Mutex);
    return unsafeGetActiveComponent();
  }

  /// Getter for the store set in the VM.
  Runtime::Component::StoreManager &getStoreManager() noexcept {
    return StoreRef;
  }

  const Runtime::Component::StoreManager &getStoreManager() const noexcept {
    return StoreRef;
  }

  /// Getter for the loader in the VM.
  Loader::Loader &getLoader() noexcept { return LoaderEngine; }

  /// Getter for the validator in the VM.
  Validator::Validator &getValidator() noexcept { return ValidatorEngine; }

  /// Getter for the component executor in the VM.
  Executor::ComponentExecutor &getExecutor() noexcept { return ExecutorEngine; }

  /// Getter for statistics.
  Statistics::Statistics &getStatistics() noexcept { return Stat; }

  /// Get the state instance of a built-in host by configuration.
  Runtime::Instance::ComponentInstance *
  getImportComponent(const HostRegistration Type) const noexcept {
    std::shared_lock Lock(Mutex);
    return unsafeGetImportComponent(Type);
  }

private:
  /// Release and terminate the component instances of a container, then clear
  /// it.
  template <typename ContainerT>
  void cleanupCompInstContainer(ContainerT &Container) {
    for (auto &Item : Container) {
      Runtime::Instance::ComponentInstance *CompInst;
      if constexpr (std::is_same_v<typename ContainerT::value_type,
                                   std::unique_ptr<
                                       Runtime::Instance::ComponentInstance>>) {
        CompInst = Item.release();
      } else {
        CompInst = Item.second.release();
      }
      if (CompInst) {
        CompInst->terminate();
      }
    }
    Container.clear();
  }
  /// Terminate the instances, importers before the hosts they import from.
  void unsafeTerminateInstances();

  Expect<void> unsafeUnregisterComponent(std::string_view Name);
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
  Expect<void>
  unsafeRegisterComponent(const Runtime::Instance::ComponentInstance &CompInst);

  Expect<void> unsafeLoadWasm(const std::filesystem::path &Path);
  Expect<void> unsafeLoadWasm(Span<const Byte> Code);
  Expect<void>
  unsafeLoadWasm(std::unique_ptr<AST::Component::Component> CompAST);

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

  void unsafeCleanup();

  std::vector<std::pair<std::string, const AST::Component::FuncType &>>
  unsafeGetFunctionList() const;

  const Runtime::Instance::ComponentInstance *unsafeGetActiveComponent() const;

  Runtime::Instance::ComponentInstance *
  unsafeGetImportComponent(const HostRegistration Type) const noexcept;

  void unsafeInitVM();
  void unsafeLoadBuiltInHosts();
  void unsafeLoadPlugInHosts();
  void unsafeRegisterBuiltInHosts();
  void unsafeRegisterPlugInHosts();

  /// Helper function for execution.
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  unsafeExecute(const Runtime::Instance::ComponentInstance *CompInst,
                std::string_view Func,
                Span<const ComponentValVariant> Params = {},
                Span<const ComponentValType> ParamTypes = {});

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

  /// \name VM Storage.
  /// @{
  /// Self-owned component store; null if an outside store is assigned.
  std::unique_ptr<Runtime::Component::StoreManager> Store;
  /// Reference to the component store.
  Runtime::Component::StoreManager &StoreRef;
  /// Every component loaded.
  std::vector<std::unique_ptr<AST::Component::Component>> Comps;
  const AST::Component::Component *Comp = nullptr;
  /// Active component instance.
  std::unique_ptr<Runtime::Instance::ComponentInstance> ActiveCompInst;
  /// Registered component instances by user.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      RegCompInsts;
  /// State instances of the built-in hosts mapped to the configurations.
  std::unordered_map<HostRegistration,
                     std::unique_ptr<Runtime::Instance::ComponentInstance>>
      BuiltInHostInsts;
  /// Component instances of the built-in hosts.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      BuiltInCompInsts;
  /// Loaded component instances from plug-ins.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      PlugInCompInsts;
  /// Component executor; declared last so it is destroyed first.
  Executor::ComponentExecutor ExecutorEngine;
  /// @}
};

} // namespace VM
} // namespace WasmEdge
