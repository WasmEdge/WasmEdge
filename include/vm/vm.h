// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/vm/vm.h - VM execution flow class definition -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of VM class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/async.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/types.h"

#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include "runtime/instance/module.h"
#include "runtime/storemgr.h"

#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace VM {

/// VM execution flow class
class VM {
public:
  VM() = delete;
  VM(const Configure &Conf);
  VM(const Configure &Conf, Runtime::StoreManager &S);
  ~VM() = default;

  /// ======= Functions can be called before instantiated stage. =======
  /// Register wasm modules and host modules.
  Expect<void> registerModule(std::string_view Name,
                              const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterModule(Name, Path);
  }
  Expect<void> registerModule(std::string_view Name, Span<const Byte> Code) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterModule(Name, Code);
  }
  Expect<void> registerModule(std::string_view Name,
                              const AST::Module &Module) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterModule(Name, Module);
  }
  Expect<void>
  registerModule(const Runtime::Instance::ModuleInstance &ModInst) {
    std::unique_lock Lock(Mutex);
    return unsafeRegisterModule(ModInst);
  }

  /// Rapidly load, validate, instantiate, and run wasm function.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  runWasmFile(const std::filesystem::path &Path, std::string_view Func,
              Span<const ValVariant> Params = {},
              Span<const ValType> ParamTypes = {}) {
    std::unique_lock Lock(Mutex);
    return unsafeRunWasmFile(Path, Func, Params, ParamTypes);
  }
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  runWasmFile(Span<const Byte> Code, std::string_view Func,
              Span<const ValVariant> Params = {},
              Span<const ValType> ParamTypes = {}) {
    std::unique_lock Lock(Mutex);
    return unsafeRunWasmFile(Code, Func, Params, ParamTypes);
  }
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  runWasmFile(const AST::Module &Module, std::string_view Func,
              Span<const ValVariant> Params = {},
              Span<const ValType> ParamTypes = {}) {
    std::unique_lock Lock(Mutex);
    return unsafeRunWasmFile(Module, Func, Params, ParamTypes);
  }

  Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
  asyncRunWasmFile(const std::filesystem::path &Path, std::string_view Func,
                   Span<const ValVariant> Params = {},
                   Span<const ValType> ParamTypes = {});
  Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
  asyncRunWasmFile(Span<const Byte> Code, std::string_view Func,
                   Span<const ValVariant> Params = {},
                   Span<const ValType> ParamTypes = {});
  Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
  asyncRunWasmFile(const AST::Module &Module, std::string_view Func,
                   Span<const ValVariant> Params = {},
                   Span<const ValType> ParamTypes = {});

  /// Load given wasm file, wasm bytecode, or wasm module.
  Expect<void> loadWasm(const std::filesystem::path &Path) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Path);
  }
  Expect<void> loadWasm(Span<const Byte> Code) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Code);
  }
  Expect<void> loadWasm(const AST::Module &Module) {
    std::unique_lock Lock(Mutex);
    return unsafeLoadWasm(Module);
  }

  /// ======= Functions can be called after loaded stage. =======
  /// Validate loaded wasm module.
  Expect<void> validate() {
    std::unique_lock Lock(Mutex);
    return unsafeValidate();
  }

  /// ======= Functions can be called after validated stage. =======
  /// Instantiate validated wasm module.
  Expect<void> instantiate() {
    std::unique_lock Lock(Mutex);
    return unsafeInstantiate();
  }

  /// ======= Functions can be called after instantiated stage. =======
  /// Execute wasm with given input.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  execute(std::string_view Func, Span<const ValVariant> Params = {},
          Span<const ValType> ParamTypes = {}) {
    std::shared_lock Lock(Mutex);
    return unsafeExecute(Func, Params, ParamTypes);
  }

  /// Execute function of registered module with given input.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  execute(std::string_view ModName, std::string_view Func,
          Span<const ValVariant> Params = {},
          Span<const ValType> ParamTypes = {}) {
    std::shared_lock Lock(Mutex);
    return unsafeExecute(ModName, Func, Params, ParamTypes);
  }

  /// Asynchronous execute wasm with given input.
  Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
  asyncExecute(std::string_view Func, Span<const ValVariant> Params = {},
               Span<const ValType> ParamTypes = {});

  /// Asynchronous execute function of registered module with given input.
  Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
  asyncExecute(std::string_view ModName, std::string_view Func,
               Span<const ValVariant> Params = {},
               Span<const ValType> ParamTypes = {});

  /// Stop execution
  void stop() noexcept { ExecutorEngine.stop(); }

  /// ======= Functions which are stageless. =======
  /// Clean up VM status
  void cleanup() {
    std::unique_lock Lock(Mutex);
    return unsafeCleanup();
  }

  /// Get list of callable functions and corresponding function types.
  std::vector<std::pair<std::string, const AST::FunctionType &>>
  getFunctionList() const {
    std::shared_lock Lock(Mutex);
    return unsafeGetFunctionList();
  }

  /// Get pre-registered module instance by configuration.
  Runtime::Instance::ModuleInstance *
  getImportModule(const HostRegistration Type) const {
    std::shared_lock Lock(Mutex);
    return unsafeGetImportModule(Type);
  }

  /// Get current instantiated module instance.
  const Runtime::Instance::ModuleInstance *getActiveModule() const {
    std::shared_lock Lock(Mutex);
    return unsafeGetActiveModule();
  }

  /// Getter of store set in VM.
  Runtime::StoreManager &getStoreManager() noexcept { return StoreRef; }
  const Runtime::StoreManager &getStoreManager() const noexcept {
    return StoreRef;
  }

  /// Getter of loader in VM.
  Loader::Loader &getLoader() noexcept { return LoaderEngine; }

  /// Getter of validator in VM.
  Validator::Validator &getValidator() noexcept { return ValidatorEngine; }

  /// Getter of executor in VM.
  Executor::Executor &getExecutor() noexcept { return ExecutorEngine; }

  /// Getter of statistics.
  Statistics::Statistics &getStatistics() noexcept { return Stat; }

private:
  Expect<void> unsafeRegisterModule(std::string_view Name,
                                    const std::filesystem::path &Path);
  Expect<void> unsafeRegisterModule(std::string_view Name,
                                    Span<const Byte> Code);
  Expect<void> unsafeRegisterModule(std::string_view Name,
                                    const AST::Module &Module);
  Expect<void>
  unsafeRegisterModule(const Runtime::Instance::ModuleInstance &ModInst);

  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeRunWasmFile(const std::filesystem::path &Path, std::string_view Func,
                    Span<const ValVariant> Params = {},
                    Span<const ValType> ParamTypes = {});
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeRunWasmFile(Span<const Byte> Code, std::string_view Func,
                    Span<const ValVariant> Params = {},
                    Span<const ValType> ParamTypes = {});
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeRunWasmFile(const AST::Module &Module, std::string_view Func,
                    Span<const ValVariant> Params = {},
                    Span<const ValType> ParamTypes = {});
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeRunWasmFile(const AST::Component::Component &Component,
                    std::string_view Func, Span<const ValVariant> Params = {},
                    Span<const ValType> ParamTypes = {});

  Expect<void> unsafeLoadWasm(const std::filesystem::path &Path);
  Expect<void> unsafeLoadWasm(Span<const Byte> Code);
  Expect<void> unsafeLoadWasm(const AST::Module &Module);

  Expect<void> unsafeValidate();

  Expect<void> unsafeInstantiate();

  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeExecute(std::string_view Func, Span<const ValVariant> Params = {},
                Span<const ValType> ParamTypes = {});

  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeExecute(std::string_view Mod, std::string_view Func,
                Span<const ValVariant> Params = {},
                Span<const ValType> ParamTypes = {});

  void unsafeCleanup();

  std::vector<std::pair<std::string, const AST::FunctionType &>>
  unsafeGetFunctionList() const;

  Runtime::Instance::ModuleInstance *
  unsafeGetImportModule(const HostRegistration Type) const;

  const Runtime::Instance::ModuleInstance *unsafeGetActiveModule() const;

  enum class VMStage : uint8_t { Inited, Loaded, Validated, Instantiated };

  void unsafeInitVM();
  void unsafeLoadBuiltInHosts();
  void unsafeLoadPlugInHosts();
  void unsafeRegisterBuiltInHosts();
  void unsafeRegisterPlugInHosts();

  /// Helper function for execution.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeExecute(const Runtime::Instance::ModuleInstance *ModInst,
                std::string_view Func, Span<const ValVariant> Params = {},
                Span<const ValType> ParamTypes = {});

  Expect<std::vector<std::pair<ValVariant, ValType>>>
  unsafeExecute(const Runtime::Instance::ComponentInstance *CompInst,
                std::string_view Func, Span<const ValVariant> Params = {},
                Span<const ValType> ParamTypes = {});

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
  Executor::Executor ExecutorEngine;
  /// @}

  /// \name VM Storage.
  /// @{
  /// Loaded AST module.
  std::unique_ptr<AST::Module> Mod;
  std::unique_ptr<AST::Component::Component> Comp;
  /// Active module instance.
  std::unique_ptr<Runtime::Instance::ModuleInstance> ActiveModInst;
  std::unique_ptr<Runtime::Instance::ComponentInstance> ActiveCompInst;
  /// Registered module instances by user.
  std::vector<std::unique_ptr<Runtime::Instance::ModuleInstance>> RegModInsts;
  /// Built-in module instances mapped to the configurations. For WASI.
  std::unordered_map<HostRegistration,
                     std::unique_ptr<Runtime::Instance::ModuleInstance>>
      BuiltInModInsts;
  /// Loaded module instances from plug-ins.
  std::vector<std::unique_ptr<Runtime::Instance::ModuleInstance>>
      PlugInModInsts;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
      PlugInCompInsts;
  /// Self-owned store (nullptr if an outside store is assigned in constructor).
  std::unique_ptr<Runtime::StoreManager> Store;
  /// Reference to the store.
  Runtime::StoreManager &StoreRef;
  /// @}
};

} // namespace VM
} // namespace WasmEdge
