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
#include "ast/type.h"
#include "common/configure.h"
#include "common/types.h"
#include "loader/serialize.h"
#include "runtime/instance/memory.h"
#include "runtime/stackmgr.h"
#include <iostream>
#include <memory>
#include <ostream>
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
    if (CurrentInstance == nullptr) {
      return;
    }

    auto Core = collectProcessInformation(CurrentInstance);
    // auto DataSection = collectDataSection(StackMgr);
    auto MemSec = collectMemory(StackMgr);
    // auto Globals = collectGlobals(StackMgr);
    auto CoreStack = collectCoreStack(StackMgr);
    // auto CoreInstances = collectCoreInstances(StackMgr);
    // auto CoreModules = collectCoreModules(StackMgr);

    //  final file generation
    AST::Module Mod{};
    std::vector<Byte> &Magic = Mod.getMagic();
    Magic.insert(Magic.begin(), {0x00, 0x61, 0x73, 0x6d});

    std::vector<Byte> &Version = Mod.getVersion();
    Version.insert(Version.begin(), {0x01, 0x00, 0x00, 0x00});

    Mod.getCustomSections().emplace_back(Core);
    // Mod.getCustomSections().emplace_back(CoreModules);
    // Mod.getCustomSections().emplace_back(CoreInstances);
    Mod.getCustomSections().emplace_back(CoreStack);
    // Mod.getDataSection() = std::move(DataSection);
    Mod.getMemorySection() = std::move(MemSec);
    // TODO globals works with DWARF data, is this section needed?
    //  Mod.getGlobalSection() = std::move(Globals);
    const AST::Module Final = Mod;
    auto Res = Ser.serializeModule(Final);
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

  AST::MemorySection collectMemory(Runtime::StackManager &StackMgr) {
    spdlog::info("Collecting memory section");

    AST::MemorySection MemSec;
    const auto *CurrentInstance = StackMgr.getModule();
    auto MemInstances = CurrentInstance->getOwnedMemoryInstances();
    std::cout << MemInstances[0]->getPageSize();
    // In the current version of WebAssembly, at most one memory may be defined
    // or imported in a single module, and all constructs implicitly reference
    // this memory.
    // std::cout << MemInstances[0]->getMemoryType().getLimit().getMin();
    // auto &Content = MemSec.getContent();
    // TODO why is this setting max and min to the same value?
    // for (auto &Iter : MemInstances) {
    //   AST::MemoryType MemType;
    //   auto Entry = Iter->getMemoryType();
    //   if (Entry.getLimit().hasMax()) {
    //     spdlog::warn("Has max");
    //     auto Max = Entry.getLimit().getMax();
    //     MemType.getLimit().setMax(Max);
    //   }
    //   MemType.getLimit().setMin(0);
    //   Content.push_back(MemType);
    // }

    return MemSec;
  }

  // Data section
  // TODO malformed section
  AST::DataSection collectDataSection(Runtime::StackManager &StackMgr) {
    spdlog::info("Collecting Memories");

    AST::DataSection Datasec;

    const auto *CurrentInstance = StackMgr.getModule();
    auto &DataInstances = CurrentInstance->getOwnedDataInstances();

    AST::DataSegment Seg;
    auto &Content = Seg.getData();

    for (auto &Iter : DataInstances) {
      Content.insert(Content.end(), Iter->getData().begin(),
                     Iter->getData().end());
    }
    if (Content.size() == 0) {
      std::cout << "empty\n";
      std::cout << Content.max_size();
      Content.insert(Seg.getData().end(), 0x00);
    }
    std::cout << Content.size();
    Datasec.getContent().push_back(Seg);
    return Datasec;
  }

  // corestack   ::= customsec(thread-info vec(frame))
  // thread-info ::= 0x0 thread-name:name
  // frame       ::= 0x0 funcidx:u32 codeoffset:u32
  //                     locals:vec(value) stack:vec(value)
  AST::CustomSection collectCoreStack(Runtime::StackManager &StackMgr) {
    spdlog::info("Constructing CoreStack");

    AST::CustomSection CoreStack;
    CoreStack.setName("corestack");

    auto Frames = StackMgr.getAllFrames();

    // TODO get thread info
    auto &Content = CoreStack.getContent();
    for (auto &Item : Frames) {
      if (Item.Module == nullptr) // guard
        continue;

      auto Funcidx = Item.From->getSourceIndex(); // function index
      // TODO which to use?
      //  auto Funcidx = Item.From->getTargetIndex(); // function index
      auto Codeoffset = Item.From->getOffset();
      // auto Codeoffset = 0; // only if codeoffset is unknown?
      auto LocalsN = Item.Locals;

      auto Locals = StackMgr.getRangeSpan(Item.VPos, LocalsN + Item.VPos);
      auto Stack = StackMgr.getTopSpan(StackMgr.size() -
                                       LocalsN); // TODO is this correct?

      Content.push_back(Funcidx);
      Content.push_back(Codeoffset);
      //
      // // TODO get the number type before inserting in the bytes
      // Is that even required?
      for (auto &Iter : Locals) {
        Content.push_back(Iter.unwrap());
      }
      for (auto &Iter : Stack) {
        Content.push_back(Iter.unwrap());
      }
    }

    return CoreStack;
  }

  // AST::GlobalSection collectGlobals(Runtime::StackManager &StackMgr) {
  //   spdlog::info("Collecting Globals");
  //   AST::GlobalSection Globals;
  //   const auto *const CurrentInstance = StackMgr.getModule();
  //
  //   auto &GlobalInstances = CurrentInstance->getOwnedGlobalInstances();
  //   auto &Content = Globals.getContent();
  //
  //   for (auto &Iter : GlobalInstances) {
  //     AST::GlobalSegment Seg;
  //     auto &Type = Seg.getGlobalType();
  //     Type = Iter->getGlobalType();
  //     Content.push_back(Seg);
  //   }
  //
  //   std::cout << Content.size();
  //
  //   return Globals;
  // }
  // This custom section establishes an index space of modules
  // that can be referenced in the coreinstances custom section
  // NOTE This is for multi-module coredumps which are not supported by wasmgdb
  // as of now
  // coremodules := customsec(vec(coremodule)) coremodule := 0x0
  // module-name:name
  // AST::CustomSection collectCoreModules(Runtime::StackManager &StackMgr) {
  //   spdlog::info("Constructing Coremodules");
  //
  //   AST::CustomSection CoreModules;
  //   CoreModules.setName("coremodules");
  //
  //   auto Frames = StackMgr.getAllFrames();
  //   std::set<const Runtime::Instance::ModuleInstance *> Names;
  //
  //   // get address of all Modules associated to the Frames in the
  //   StackManager for (const auto &Item : Frames) {
  //     if (Item.Module == nullptr) {
  //       continue; // guard
  //     }
  //     Names.insert(Item.Module);
  //   }
  //
  //   auto &Content = CoreModules.getContent();
  //   for (const auto &ModulePtr : Names) {
  //     const Byte *Bytes = reinterpret_cast<const Byte *>(&ModulePtr);
  //     Content.insert(Content.end(), Bytes, Bytes + sizeof(ModulePtr));
  //   }
  //
  //   return CoreModules;
  // }
  //
  // coreinstances section
  // This may not be compatible with current wasmgdb
  // coreinstances ::= customsec(vec(coreinstance))
  // coreinstance ::= 0x0 moduleidx:u32 memories:vec(u32) globals:vec(u32)
  // AST::CustomSection collectCoreInstances(Runtime::StackManager &StackMgr) {
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
