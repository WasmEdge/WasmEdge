// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ImportSection &ImportSec) {
  for (const auto &Import : ImportSec.getContent()) {
    const auto &Desc = Import.getDesc();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::CoreType:
    case AST::Component::ExternDesc::DescType::FuncType:
    case AST::Component::ExternDesc::DescType::ValueBound:
    case AST::Component::ExternDesc::DescType::TypeBound:
    case AST::Component::ExternDesc::DescType::ComponentType:
      // TODO: COMPONENT - complete the import instantiation.
      spdlog::error(ErrCode::Value::UnknownImport);
      spdlog::error("    incomplete import {} desc types"sv, Import.getName());
      return Unexpect(ErrCode::Value::UnknownImport);
    case AST::Component::ExternDesc::DescType::InstanceType: {
      // TODO: COMPONENT - type matching for the instance type.
      auto CompName = Import.getName();
      const auto *ImportedCompInst = StoreMgr.findComponent(CompName);
      if (unlikely(ImportedCompInst == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    component name: {}"sv, CompName);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addComponentInstance(ImportedCompInst);
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentImportManager &ImportMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ImportSection &ImportSec) {
  for (const auto &Import : ImportSec.getContent()) {
    const auto &Desc = Import.getDesc();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::FuncType: {
      auto Name = Import.getName();
      auto *Imported = ImportMgr.findFunction(Name);
      if (unlikely(Imported == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    component func name: {}"sv, Name);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addFunction(Imported);
      break;
    }
    case AST::Component::ExternDesc::DescType::ValueBound: {
      auto Name = Import.getName();
      auto Imported = ImportMgr.findValue(Name);
      if (unlikely(!Imported.has_value())) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    component value name: {}"sv, Name);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addValue(Imported.value());
      break;
    }
    case AST::Component::ExternDesc::DescType::ComponentType: {
      auto Name = Import.getName();
      auto *Imported = ImportMgr.findComponent(Name);
      if (unlikely(Imported == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    component name: {}"sv, Name);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addComponent(*Imported);
      break;
    }
    case AST::Component::ExternDesc::DescType::CoreType: {
      auto Name = Import.getName();
      auto *Imported = ImportMgr.findCoreType(Name);
      if (unlikely(Imported == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    core type name: {}"sv, Name);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addCoreType(*Imported);
      break;
    }
    case AST::Component::ExternDesc::DescType::TypeBound:
      // TODO: COMPONENT - complete the import instantiation.
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error("    incomplete import {} desc types"sv, Import.getName());
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    case AST::Component::ExternDesc::DescType::InstanceType: {
      // TODO: COMPONENT - type matching for the instance type.
      auto CompName = Import.getName();
      const auto *ImportedCompInst = ImportMgr.findComponentInstance(CompName);
      if (unlikely(ImportedCompInst == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    component name: {}"sv, CompName);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addComponentInstance(ImportedCompInst);
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
