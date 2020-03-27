// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/vm/vm.h - VM execution flow class definition -----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of VM class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/types.h"
#include "common/value.h"
#include "configure.h"
#include "costtable.h"
#include "interpreter/interpreter.h"
#include "loader/loader.h"
#include "runtime/importobj.h"
#include "runtime/storemgr.h"
#include "support/measure.h"
#include "validator/validator.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace ExpVM {

/// VM execution flow class
class VM {
public:
  VM() = delete;
  VM(Configure &InputConfig);
  VM(Configure &InputConfig, Runtime::StoreManager &S);
  ~VM() = default;

  /// ======= Functions can be called before instantiated stage. =======
  /// Register wasm modules and host modules.
  Expect<void> registerModule(const std::string &Name, const std::string &Path);
  Expect<void> registerModule(const std::string &Name, const Bytes &Code);
  Expect<void> registerModule(const Runtime::ImportObject &Obj);

  /// Rapidly load, validate, instantiate, and run wasm function.
  Expect<std::vector<ValVariant>>
  runWasmFile(const std::string &Path, const std::string &Func,
              const std::vector<ValVariant> &Params = {});
  Expect<std::vector<ValVariant>>
  runWasmFile(const Bytes &Code, const std::string &Func,
              const std::vector<ValVariant> &Params = {});

  /// Load given wasm file or wasm bytecode.
  Expect<void> loadWasm(const std::string &Path);
  Expect<void> loadWasm(const Bytes &Code);

  /// ======= Functions can be called after loaded stage. =======
  /// Validate loaded wasm module.
  Expect<void> validate();

  /// ======= Functions can be called after validated stage. =======
  /// Instantiate validated wasm module.
  Expect<void> instantiate();

  /// ======= Functions can be called after instantiated stage. =======
  /// Execute wasm with given input.
  Expect<std::vector<ValVariant>>
  execute(const std::string &Func, const std::vector<ValVariant> &Params = {});

  /// ======= Functions which are stageless. =======
  /// Clean up VM status
  void cleanup();

  /// Get list of callable functions and corresponding function types.
  std::vector<std::pair<std::string, Runtime::Instance::FType>>
  getFunctionList() const;

  /// Get import objects by configurations.
  Runtime::ImportObject *getImportModule(const Configure::VMType Type);

  /// Getter of store set in VM.
  Runtime::StoreManager &getStoreManager() { return StoreRef; }

  /// Getter of measurement.
  Support::Measurement &getMeasurement() { return Measure; }

  /// Getter of service name.
  std::string &getServiceName() { return ServiceName; }

  /// Getter of UUID.
  uint64_t &getUUID() { return UUID; }

private:
  enum class VMStage : uint8_t { Inited, Loaded, Validated, Instantiated };

  void initVM();
  Expect<void> registerModule(const std::string &Name,
                              const AST::Module &Module);
  Expect<std::vector<ValVariant>>
  runWasmFile(const AST::Module &Module, const std::string &Func,
              const std::vector<ValVariant> &Params);

  /// VM environment.
  Configure &Config;
  Support::Measurement Measure;
  VMStage Stage;

  /// VM runners.
  Loader::Loader LoaderEngine;
  Validator::Validator ValidatorEngine;
  Interpreter::Interpreter InterpreterEngine;
  /// TODO: Add AOT here.

  /// VM Storage.
  std::unique_ptr<AST::Module> Mod;
  std::unique_ptr<Runtime::StoreManager> Store;
  Runtime::StoreManager &StoreRef;
  std::map<Configure::VMType, std::unique_ptr<Runtime::ImportObject>> ImpObjs;
  CostTable CostTab;

  /// Identification
  std::string ServiceName;
  uint64_t UUID;
};

} // namespace ExpVM
} // namespace SSVM
