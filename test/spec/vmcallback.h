// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/spec/vmcallback.h - VM callbacks for spec tests -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the SpecTest callbacks that every runner on a VM shares.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "common/errcode.h"
#include "common/filesystem.h"
#include "loader/loader.h"
#include "spectest.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace SpecTestVM {

/// Register each shared module of the VM Src in the VM Dst under its alias
/// name. A thread then imports the module by the name that it expects.
inline void
shareModules(VM::VM &Dst, VM::VM &Src,
             const std::vector<std::pair<std::string, std::string>> &Shared) {
  for (const auto &[ParentName, AliasName] : Shared) {
    if (const auto *ModInst = Src.getStoreManager().findModule(ParentName)) {
      Dst.registerModule(AliasName, *ModInst);
    }
  }
}

/// Parse a wasm unit with the loader. For a binary file or a text file, Source
/// is the path of the file. Otherwise, Source holds the raw bytes of a binary
/// module or of a text module.
inline Expect<SpecTest::WasmUnit>
parse(std::string_view Source, Wast::ModuleType Type, const Configure &Conf) {
  Loader::Loader Ld(Conf);
  if (Type == Wast::ModuleType::BinaryFile ||
      Type == Wast::ModuleType::TextFile) {
    return Ld.parseWasmUnit(std::filesystem::path(std::string(Source)));
  }
  EXPECTED_TRY(
      auto ModPtr,
      Ld.parseModule(Span<const uint8_t>(
          reinterpret_cast<const uint8_t *>(Source.data()), Source.size())));
  return SpecTest::WasmUnit(std::move(ModPtr));
}

/// Instantiate a module. A named module is registered in the store manager,
/// and an anonymous module becomes the active module of the VM.
inline Expect<void> instantiate(VM::VM &VM, const std::string &ModName,
                                const AST::Module &Mod) {
  if (!ModName.empty()) {
    return VM.registerModule(ModName, Mod);
  }
  return VM.loadWasm(Mod)
      .and_then([&VM]() { return VM.validate(); })
      .and_then([&VM]() { return VM.instantiate(); });
}

/// Validate a module or a component with the configuration of the VM.
inline Expect<void> validate(VM::VM &VM, SpecTest::WasmUnit &Unit) {
  Validator::Validator Valid(VM.getExecutor().getConfigure());
  if (std::holds_alternative<std::unique_ptr<AST::Module>>(Unit)) {
    return Valid.validate(*std::get<std::unique_ptr<AST::Module>>(Unit));
  }
  return Valid.validate(
      *std::get<std::unique_ptr<AST::Component::Component>>(Unit));
}

/// Invoke a function. A named module is registered in the store manager, and
/// an anonymous module is the active module of the VM.
inline Expect<std::vector<std::pair<ValVariant, ValType>>>
invoke(VM::VM &VM, const std::string &ModName, const std::string &Field,
       const std::vector<ValVariant> &Params,
       const std::vector<ValType> &ParamTypes) {
  if (!ModName.empty()) {
    return VM.execute(ModName, Field, Params, ParamTypes);
  }
  return VM.execute(Field, Params, ParamTypes);
}

/// Get the value of an exported global.
inline Expect<std::pair<ValVariant, ValType>>
get(VM::VM &VM, const std::string &ModName, const std::string &Field) {
  const Runtime::Instance::ModuleInstance *ModInst =
      ModName.empty() ? VM.getActiveModule()
                      : VM.getStoreManager().findModule(ModName);
  if (ModInst == nullptr) {
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  Runtime::Instance::GlobalInstance *GlobInst =
      ModInst->findGlobalExports(Field);
  if (unlikely(GlobInst == nullptr)) {
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  return std::make_pair(GlobInst->getValue(),
                        GlobInst->getGlobalType().getValType());
}

} // namespace SpecTestVM
} // namespace WasmEdge
