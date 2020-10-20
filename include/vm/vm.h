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
#include "common/measure.h"
#include "validator/validator.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace VM {

/// VM execution flow class
class VM {
public:
  VM() = delete;
  VM(Configure &InputConfig);
  VM(Configure &InputConfig, Runtime::StoreManager &S);
  ~VM() = default;

  /// ======= Functions can be called before instantiated stage. =======
  /// Register wasm modules and host modules.
  Expect<void> registerModule(std::string_view Name, std::string_view Path);
  Expect<void> registerModule(std::string_view Name, Span<const Byte> Code);
  Expect<void> registerModule(const Runtime::ImportObject &Obj);

  /// Rapidly load, validate, instantiate, and run wasm function.
  Expect<std::vector<ValVariant>>
  runWasmFile(std::string_view Path, std::string_view Func,
              Span<const ValVariant> Params = {});
  Expect<std::vector<ValVariant>>
  runWasmFile(Span<const Byte> Code, std::string_view Func,
              Span<const ValVariant> Params = {});

  /// Load given wasm file or wasm bytecode.
  Expect<void> loadWasm(std::string_view Path);
  Expect<void> loadWasm(Span<const Byte> Code);

  /// ======= Functions can be called after loaded stage. =======
  /// Validate loaded wasm module.
  Expect<void> validate();

  /// ======= Functions can be called after validated stage. =======
  /// Instantiate validated wasm module.
  Expect<void> instantiate();

  /// ======= Functions can be called after instantiated stage. =======
  /// Execute wasm with given input.
  Expect<std::vector<ValVariant>> execute(std::string_view Func,
                                          Span<const ValVariant> Params = {});

  /// Execute function of registered module with given input.
  Expect<std::vector<ValVariant>> execute(std::string_view Mod,
                                          std::string_view Func,
                                          Span<const ValVariant> Params = {});

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

  /// Getter of statistics.
  Statistics::Statistics &getStatistics() { return Stat; }

  /// Getter of service name.
  std::string &getServiceName() { return ServiceName; }

  /// Getter of UUID.
  uint64_t &getUUID() { return UUID; }

private:
  enum class VMStage : uint8_t { Inited, Loaded, Validated, Instantiated };

  void initVM();
  Expect<void> registerModule(std::string_view Name, const AST::Module &Module);
  Expect<std::vector<ValVariant>> runWasmFile(const AST::Module &Module,
                                              std::string_view Func,
                                              Span<const ValVariant> Params);

  /// VM environment.
  Configure &Config;
  Support::Measurement Measure;
  Statistics::Statistics Stat;
  VMStage Stage;

  /// VM runners.
  Loader::Loader LoaderEngine;
  Validator::Validator ValidatorEngine;
  Interpreter::Interpreter InterpreterEngine;

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

} // namespace VM
} // namespace SSVM
