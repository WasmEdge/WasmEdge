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
#include "spectest.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace SpecTestVM {

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
