// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/coredump.h - Executor coredump definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the coredump class of runtime.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "ast/section.h"
#include "common/configure.h"
#include "common/types.h"
#include "loader/serialize.h"
#include "runtime/stackmgr.h"
#include <cstdint>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string_view>
namespace WasmEdge {
namespace Coredump {

class Coredump {
public:
  Coredump() {}
  ~Coredump() = default;

  void generateCoredump(Runtime::StackManager &StackMgr) {
    const Configure Conf;
    Loader::Serializer Ser(Conf);
    auto *CurrentInstance = StackMgr.getModule();

    // process info collection
    auto Core = collectProcessInformation(CurrentInstance);
    auto CoreModules = collectCoreModules(StackMgr);

    // TODO
    //  final file generation
    AST::Module Mod{};
    // TODO Find out why this gets cast to a span and not the writable vector.
    Mod.getCustomSections().emplace_back(Core);
    Mod.getCustomSections().emplace_back(CoreModules);
    auto Res = Ser.serializeModule(Mod);
    std::string Str(Res->begin(), Res->end());
    std::cout << "Look at this:" << Str;
  }

  AST::CustomSection collectProcessInformation(
      const Runtime::Instance::ModuleInstance *CurrentInstance) {
    spdlog::info("Constructing Core");
    // AST node
    AST::CustomSection Core;
    Core.setName("core");

    // Insert in data and set Name of section
    auto Name = CurrentInstance->getModuleName();
    std::vector<Byte> Data(reinterpret_cast<const Byte *>(Name.data()),
                           reinterpret_cast<const Byte *>(Name.data()) +
                               Name.size());

    auto &Content = Core.getContent();
    Content.insert(Content.end(), Data.begin(), Data.end());

    return Core;
  }

  // This custom section establishes an index space of modules
  // that can be referenced in the coreinstances custom section
  // coremodules := customsec(vec(coremodule))
  // coremodule := 0x0 module-name:name
  AST::CustomSection collectCoreModules(Runtime::StackManager &StackMgr) {
    spdlog::info("Constructing Coremodules");

    AST::CustomSection CoreModules;
    CoreModules.setName("coremodules");

    auto Frames = StackMgr.getAllFrames();
    std::set<std::string_view> Names;

    // get Names of all Modules present in the StackManager
    for (const auto &Item : Frames) {
      if (Item.Module == nullptr)
        continue; // guard
      Names.insert(Item.Module->getModuleName());
    }

    // TODO cast data from set Names to uint8_t data
    // Is there a better way to do this?

    // auto &Content = CoreModules.getContent();
    // Content.insert(Content.begin(), Data.begin(), Data.end());

    // std::vector<uint8_t> OutVec;
    // const auto &Sec = CoreModules;

    // Ser.serializeSection(Sec, OutVec);

    return CoreModules;
  }
};

} // namespace Coredump
} // namespace WasmEdge
