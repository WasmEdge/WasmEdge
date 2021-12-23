// SPDX-License-Identifier: Apache-2.0
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

#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/types.h"

#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include "runtime/importobj.h"
#include "runtime/storemgr.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace WasmEdge {
namespace VM {

template <typename T> class Async;
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
                              const std::filesystem::path &Path);
  Expect<void> registerModule(std::string_view Name, Span<const Byte> Code);
  Expect<void> registerModule(std::string_view Name, const AST::Module &Module);
  Expect<void> registerModule(const Runtime::ImportObject &Obj);

  /// Rapidly load, validate, instantiate, and run wasm function.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  runWasmFile(const std::filesystem::path &Path, std::string_view Func,
              Span<const ValVariant> Params = {},
              Span<const ValType> ParamTypes = {});
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  runWasmFile(Span<const Byte> Code, std::string_view Func,
              Span<const ValVariant> Params = {},
              Span<const ValType> ParamTypes = {});
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  runWasmFile(const AST::Module &Module, std::string_view Func,
              Span<const ValVariant> Params = {},
              Span<const ValType> ParamTypes = {});

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
  Expect<void> loadWasm(const std::filesystem::path &Path);
  Expect<void> loadWasm(Span<const Byte> Code);
  Expect<void> loadWasm(const AST::Module &Module);

  /// ======= Functions can be called after loaded stage. =======
  /// Validate loaded wasm module.
  Expect<void> validate();

  /// ======= Functions can be called after validated stage. =======
  /// Instantiate validated wasm module.
  Expect<void> instantiate();

  /// ======= Functions can be called after instantiated stage. =======
  /// Execute wasm with given input.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  execute(std::string_view Func, Span<const ValVariant> Params = {},
          Span<const ValType> ParamTypes = {});

  /// Execute function of registered module with given input.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  execute(std::string_view ModName, std::string_view Func,
          Span<const ValVariant> Params = {},
          Span<const ValType> ParamTypes = {});

  /// Asynchronous execute wasm with given input.
  Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
  asyncExecute(std::string_view Func, Span<const ValVariant> Params = {},
               Span<const ValType> ParamTypes = {});

  /// Asynchronous execute function of registered module with given input.
  Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
  asyncExecute(std::string_view ModName, std::string_view Func,
               Span<const ValVariant> Params = {},
               Span<const ValType> ParamTypes = {});

  /// Register new thread
  void newThread() noexcept { ExecutorEngine.newThread(); }
  /// Stop execution
  void stop() noexcept { ExecutorEngine.stop(); }

  /// ======= Functions which are stageless. =======
  /// Clean up VM status
  void cleanup();

  /// Get list of callable functions and corresponding function types.
  std::vector<std::pair<std::string, const AST::FunctionType &>>
  getFunctionList() const;

  /// Get import objects by configurations.
  Runtime::ImportObject *getImportModule(const HostRegistration Type);

  /// Getter of store set in VM.
  Runtime::StoreManager &getStoreManager() { return StoreRef; }

  /// Getter of statistics.
  Statistics::Statistics &getStatistics() { return Stat; }

private:
  enum class VMStage : uint8_t { Inited, Loaded, Validated, Instantiated };

  void initVM();

  /// Helper function for execution.
  Expect<std::vector<std::pair<ValVariant, ValType>>>
  execute(Runtime::Instance::ModuleInstance *ModInst, std::string_view Func,
          Span<const ValVariant> Params = {},
          Span<const ValType> ParamTypes = {});

  /// VM environment.
  const Configure Conf;
  Statistics::Statistics Stat;
  VMStage Stage;

  /// VM runners.
  Loader::Loader LoaderEngine;
  Validator::Validator ValidatorEngine;
  Executor::Executor ExecutorEngine;

  /// VM Storage.
  std::unique_ptr<AST::Module> Mod;
  std::unique_ptr<Runtime::StoreManager> Store;
  Runtime::StoreManager &StoreRef;
  std::map<HostRegistration, std::unique_ptr<Runtime::ImportObject>> ImpObjs;
};

} // namespace VM
} // namespace WasmEdge

#include "async.h"
