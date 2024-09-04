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
#include "ast/segment.h"
#include "common/configure.h"
#include "common/types.h"
#include "loader/serialize.h"
#include "runtime/stackmgr.h"
#include "runtime/storemgr.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>
namespace WasmEdge {
namespace Coredump {

class Coredump {
public:
  Coredump() {}
  ~Coredump() = default;

  void generateCoredump(Runtime::StackManager &StackMgr) {
    const Configure Conf;
    Loader::Serializer Ser(Conf);
    const auto *CurrentInstance = StackMgr.getModule();

    // CurrentInstance->unsafeGetStore();

    // process info collection
    auto Core = collectProcessInformation(CurrentInstance);
    auto CoreModules = collectCoreModules(StackMgr);
    auto DataSection = collectDataSection(StackMgr);
    auto MemSec = collectMemory(StackMgr);
    // auto CoreInstances = collectCoreInstances(StackMgr);

    //  final file generation
    AST::Module Mod{};
    std::vector<Byte> &Magic = Mod.getMagic();
    Magic.insert(Magic.begin(), {0x00, 0x61, 0x73, 0x6d});

    std::vector<Byte> &Version = Mod.getVersion();
    Version.insert(Version.begin(), {0x01, 0x00, 0x00, 0x00});

    Mod.getCustomSections().emplace_back(Core);
    Mod.getCustomSections().emplace_back(CoreModules);
    // Mod.getCustomSections().emplace_back(CoreInstances);
    Mod.getDataSection() = std::move(DataSection);
    Mod.getMemorySection() = std::move(MemSec);
    auto Res = Ser.serializeModule(Mod);
    std::ofstream File("coredump.wasm", std::ios::out | std::ios::binary);
    if (File.is_open()) {
      File.write(reinterpret_cast<const char *>(Res->data()), Res->size());
      File.close();
    } else {
      // Handle error opening file
      throw std::ios_base::failure("Failed");
    }
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
    if (Data.size() != 0) { // case where the module has an empty name field
      Content.insert(Content.end(), Data.begin(), Data.end());
    } else {
      Content.insert(Content.begin(), {0x00, 0x00});
    }

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
    std::set<const Runtime::Instance::ModuleInstance *> Names;

    // get address of all Modules associated to the Frames in the StackManager
    for (const auto &Item : Frames) {
      if (Item.Module == nullptr) {
        continue; // guard
      }
      Names.insert(Item.Module);
    }

    auto &Content = CoreModules.getContent();
    for (const auto &ModulePtr : Names) {
      const Byte *Bytes = reinterpret_cast<const Byte *>(&ModulePtr);
      Content.insert(Content.end(), Bytes, Bytes + sizeof(ModulePtr));
    }

    return CoreModules;
  }

  AST::MemorySection collectMemory(Runtime::StackManager &StackMgr) {
    spdlog::info("Collecting memory section");

    AST::MemorySection MemSec;

    const auto *CurrentInstance = StackMgr.getModule();
    auto &MemInstances = CurrentInstance->getOwnedMemoryInstances();
    auto &Content = MemSec.getContent();
    for (auto &Iter : MemInstances) {
      Content.push_back(Iter->getMemoryType());
    }

    return MemSec;
  }

  // Data section
  AST::DataSection collectDataSection(Runtime::StackManager &StackMgr) {
    spdlog::info("Collecting Memories");

    AST::DataSection Datasec;

    StackMgr.size();
    // const auto *CurrentInstance = StackMgr.getModule();
    // auto &DataInstances = CurrentInstance->getOwnedDataInstances();
    // // DataInstances := vector<std::unique_ptr<DataInstance>>
    // // DataInstance := Offset and vector<Byte>
    //
    // std::vector<Byte> D;
    // for (auto &Iter : DataInstances) {
    //   D.insert(D.begin(), Iter->getData().begin(), Iter->getData().end());
    // }
    // // D := vector<Bytes>
    // AST::DataSegment Ds;
    //
    // auto &DataSegs = Datasec.getContent();
    // // Content := vector<DataSegment>
    // // DataSegment := MemoryIdx(always 0) and vector<Byte> and a mode
    // auto &Content = Ds.getData();
    // Content.insert(Content.end(), D.begin(), D.end());
    //
    // DataSegs.insert(DataSegs.begin(), Ds);
    return Datasec;
  }

  // AST::CustomSection collectCoreStack(Runtime::StackManager &StackMgr) {
  //   spdlog::info("Constructing CoreStack");
  //
  //   AST::CustomSection CoreStack;
  //   CoreStack.setName("corestack");
  //
  //   auto Frames = StackMgr.getAllFrames();
  //   std::vector<Runtime::Instance::ModuleInstance> Modules;
  //
  //   for (auto Item : Frames) {
  //     Modules.emplace_back(Item.Module);
  //   }
  //
  //   return CoreStack;
  // }

  // // TODO coreinstances section
  // TODO This may not be compatible with current wasmgdb
  // coreinstances ::= customsec(vec(coreinstance))
  // coreinstance ::= 0x0 moduleidx:u32 memories:vec(u32) globals:vec(u32)
  // AST::CustomSection collectCoreInstances(Runtime::StackManager &StackMgr)
  // {
  //   spdlog::info("Constructing CoreInstances");
  //
  //   AST::CustomSection CoreInstances;
  //   CoreInstances.setName("coreinstances");
  //   auto Frames = StackMgr.getAllFrames();
  //
  //   // struct CoreInstance {
  //   //   const Runtime::Instance::ModuleInstance Moduleidx;
  //   //   std::vector<uint8_t> Memories;
  //   //   std::vector<uint8_t> Globals;
  //   // };
  //
  //   for (const auto &Item : Frames) {
  //     if (Item.Module == nullptr) {
  //       continue; // guard
  //     }
  //     std::cout << Item.Module << ":" << Item.Module
  //   }
  //
  //   return CoreInstances;
  // }
};

} // namespace Coredump
} // namespace WasmEdge
