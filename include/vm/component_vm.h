// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/vm/component_vm.h - Component VM class definition --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the component model VM class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/async.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/types.h"

#include "executor/component/executor.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include "runtime/component/storemgr.h"
#include "runtime/instance/component/component.h"

#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace VM {
namespace Component {

/// Component model execution flow class. It owns the core executor it runs
/// on, so a component and a core module never share an instantiation state.
class VM {
public:
  VM() = delete;
  VM(const Configure &Conf);
  VM(const Configure &Conf, Runtime::Component::StoreManager &S);
  ~VM() = default;

  /// ======= Functions can be called before the instantiated stage. =======
  /// Register a component from a file path with the given name.
  Expect<void> registerComponent(std::string_view Name,
                                 const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, Path);
  }

  /// Register an already-loaded and validated component with the given name.
  Expect<void> registerComponent(std::string_view Name,
                                 const AST::Component::Component &CompAST) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterComponent(Name, CompAST);
  }

  /// Load the given component file or bytecode.
  Expect<void> loadWasm(const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Path);
  }
  Expect<void> loadWasm(Span<const Byte> Code) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Code);
  }

  /// ======= Functions can be called after the loaded stage. =======
  /// Validate the loaded component.
  Expect<void> validate() {
    std::unique_lock Lock(Mutex);
    return unsafeValidate();
  }

  /// ======= Functions can be called after the validated stage. =======
  /// Instantiate the validated component.
  Expect<void> instantiate() {
    std::unique_lock Lock(Mutex);
    return unsafeInstantiate();
  }

  /// ======= Functions can be called after the instantiated stage. =======
  /// Execute a component function with the given input.
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  execute(std::string_view Func, Span<const ComponentValVariant> Params = {},
          Span<const ComponentValType> ParamTypes = {}) {
    std::shared_lock Lock(Mutex);
    return unsafeExecute(Func, Params, ParamTypes);
  }

  /// Execute a function of a registered component with the given input.
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  execute(std::string_view CompName, std::string_view Func,
          Span<const ComponentValVariant> Params = {},
          Span<const ComponentValType> ParamTypes = {}) {
    std::shared_lock Lock(Mutex);
    return unsafeExecute(CompName, Func, Params, ParamTypes);
  }

  /// Asynchronously execute a component function with the given input.
  Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
  asyncExecute(std::string_view Func,
               Span<const ComponentValVariant> Params = {},
               Span<const ComponentValType> ParamTypes = {});

  /// Asynchronously execute a function of a registered component.
  Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
  asyncExecute(std::string_view CompName, std::string_view Func,
               Span<const ComponentValVariant> Params = {},
               Span<const ComponentValType> ParamTypes = {});

  /// Stop execution.
  void stop() noexcept { CoreExecEngine.stop(); }

  /// ======= Functions which are stageless. =======
  /// Clean up the VM status.
  void cleanup() {
    std::unique_lock Lock(Mutex);
    unsafeCleanup();
  }

  /// True when a component is loaded into this VM.
  bool holdsComponent() const {
    std::shared_lock Lock(Mutex);
    return Comp != nullptr;
  }

  /// Get the list of callable component functions and their types.
  std::vector<std::pair<std::string, const AST::Component::FuncType &>>
  getFunctionList() const {
    std::shared_lock Lock(Mutex);
    return unsafeGetFunctionList();
  }

  /// Getter for the component store of this VM.
  Runtime::Component::StoreManager &getStoreManager() noexcept {
    return StoreRef;
  }

  /// Getter for the loader in the VM.
  Loader::Loader &getLoader() noexcept { return LoaderEngine; }

  /// Getter for the validator in the VM.
  Validator::Validator &getValidator() noexcept { return ValidatorEngine; }

  /// Getter for the component executor in the VM.
  Executor::Component::Executor &getExecutor() noexcept { return ExecEngine; }

  /// Getter for the core executor the component executor runs on.
  Executor::Executor &getCoreExecutor() noexcept { return CoreExecEngine; }

  /// Getter for statistics.
  Statistics::Statistics &getStatistics() noexcept { return Stat; }

private:
  Expect<void> unsafeRegisterComponent(std::string_view Name,
                                       const std::filesystem::path &Path);
  Expect<void>
  unsafeRegisterComponent(std::string_view Name,
                          const AST::Component::Component &CompAST);

  Expect<void> unsafeLoadWasm(const std::filesystem::path &Path);
  Expect<void> unsafeLoadWasm(Span<const Byte> Code);

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

  /// Take the component alternative out of a parsed wasm unit.
  Expect<std::unique_ptr<AST::Component::Component>>
  unsafeTakeComponent(const std::filesystem::path &Path);

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
  /// The core executor below the component executor.
  Executor::Executor CoreExecEngine;
  Executor::Component::Executor ExecEngine{CoreExecEngine};
  /// @}

  /// \name VM storage.
  /// @{
  /// Loaded component AST.
  std::unique_ptr<AST::Component::Component> Comp;
  /// Active component instance.
  std::unique_ptr<Runtime::Instance::ComponentInstance> ActiveCompInst;
  /// Registered component instances and their owning ASTs.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      RegCompInsts;
  std::vector<std::unique_ptr<AST::Component::Component>> RegCompASTs;
  /// Component instances loaded from plug-ins.
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      PlugInCompInsts;
  /// Self-owned store (nullptr if an outside store is given to the ctor).
  std::unique_ptr<Runtime::Component::StoreManager> Store;
  /// Reference to the store.
  Runtime::Component::StoreManager &StoreRef;
  /// @}
};

} // namespace Component
} // namespace VM
} // namespace WasmEdge
