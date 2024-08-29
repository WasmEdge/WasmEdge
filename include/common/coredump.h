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
    auto Core = collectProcessInformation(CurrentInstance, Ser);

    // TODO
    //  final file generation
    const AST::Module Mod{};
    // std::vector<AST::CustomSection> *CustomSections;
    // TODO Find out why this gets cast to a span and not the writable vector.
    // CustomSections = Mod.getCustomSections();
    auto Res = Ser.serializeModule(Mod);
    std::string Str(Res->begin(), Res->end());
    // std::cout << "Look at this:" << Str;
  }

  AST::CustomSection collectProcessInformation(
      const Runtime::Instance::ModuleInstance *CurrentInstance,
      Loader::Serializer &Ser) {
    spdlog::info("Process Info Collection");
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

    std::vector<uint8_t> OutVec;
    const auto &Sec = Core;

    Ser.serializeSection(Sec, OutVec);

    return Core;
  }
};

} // namespace Coredump
} // namespace WasmEdge
