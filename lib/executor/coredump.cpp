// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "executor/coredump.h"
#include "ast/section.h"
#include "common/errcode.h"
#include "common/types.h"
#include "runtime/stackmgr.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <cstring>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Coredump {
void generateCoredump(const Runtime::StackManager &StackMgr,
                      bool ForWasmgdb) noexcept {
  spdlog::info("Generating coredump..."sv);
  if (ForWasmgdb) {
    spdlog::info("For wasmgdb"sv);
  }
  // Generate coredump.
  const auto *CurrentInstance = StackMgr.getModule();
  const Configure Config;
  Loader::Serializer Ser(Config);
  AST::Module Module{};
  std::vector<Byte> &Magic = Module.getMagic();
  std::string MagicStr("\0asm", 4);
  Magic.insert(Magic.begin(), MagicStr.begin(), MagicStr.end());
  std::vector<Byte> &Version = Module.getVersion();
  // Version must be 1 for support Wasmgdb
  Version.insert(Version.begin(), {0x01, 0x00, 0x00, 0x00});

  Module.getCustomSections().emplace_back(createCore());
  Module.getCustomSections().emplace_back(createCorestack(
      Ser, StackMgr.getFramesSpan(), StackMgr.getValueSpan(), ForWasmgdb));
  // TODO: reconstruct data instances
  // Module.getDataSection() =
  //     createData(CurrentInstance->getOwnedDataInstances());
  // TODO: pass all module instances
  Module.getCustomSections().emplace_back(
      createCoremodules(Ser, {CurrentInstance}));
  Module.getCustomSections().emplace_back(createCoreinstances({}));
  Module.getMemorySection() =
      createMemory(CurrentInstance->getMemoryInstances());
  Module.getGlobalSection() =
      createGlobals(CurrentInstance->getGlobalInstances());
  auto Res = Ser.serializeModule(Module);
  if (Res.has_value()) {
    spdlog::info("Coredump generated."sv);
    std::time_t Time = std::time(nullptr);
    std::string CoredumpPath = "coredump." + std::to_string(Time);
    std::ofstream File(CoredumpPath, std::ios::out | std::ios::binary);
    if (File.is_open()) {
      File.write(reinterpret_cast<const char *>(Res->data()),
                 static_cast<uint32_t>(Res->size()));
      File.close();
    } else {
      spdlog::error("Failed to generate coredump."sv);
      assumingUnreachable();
    }
  } else {
    spdlog::error("Failed to serialize coredump."sv);
    assumingUnreachable();
  }

  return;
}
AST::CustomSection createCore() {
  AST::CustomSection Core;
  Core.setName("core");
  auto &Content = Core.getContent();
  Content.insert(Content.begin(), {0x00, 0x00});
  return Core;
}

AST::CustomSection createCorestack(
    Loader::Serializer &Ser, Span<const Runtime::StackManager::Frame> Frames,
    Span<const Runtime::StackManager::Value> ValueStack, bool ForWasmgdb) {
  AST::CustomSection CoreStack;
  CoreStack.setName("corestack");
  auto &Content = CoreStack.getContent();
  // thread-info type 0x00 for wasmedbg
  Content.push_back(0x00);

  // Thread name size
  Content.push_back(0x04);

  std::string ThreadName = "main";
  Content.insert(Content.end(), ThreadName.begin(), ThreadName.end());
  auto FramesSize = Frames.size() - 1;
  Ser.serializeU32(static_cast<uint32_t>(FramesSize), Content);
  for (size_t Idx = FramesSize; Idx > 0; Idx--) {
    if (Frames[Idx].Module == nullptr) {
      continue;
    }
    // frame type 0x00 for wasmedbg
    Content.push_back(0x00);

    // Entry/start frames (module start, WASI _start) have no caller
    // instruction, so From is default-constructed. Output 0/0 to satisfy
    // format, but these are not meaningful function indices.
    uint32_t Funcidx = 0;
    uint32_t Codeoffset = 0;
    if (Frames[Idx].From != AST::InstrView::iterator()) {
      Funcidx = Frames[Idx].From->getTargetIndex();
      Codeoffset = Frames[Idx].From->getOffset();
    }

    uint32_t Lstart = Frames[Idx].VPos - Frames[Idx].Locals;
    uint32_t Lend = Frames[Idx].VPos;
    uint32_t Vstart = Frames[Idx].VPos;
    uint32_t Vend = (Idx != FramesSize)
                        ? Frames[Idx + 1].VPos - Frames[Idx + 1].Locals
                        : static_cast<uint32_t>(ValueStack.size());

    uint32_t Lsize = Lend - Lstart;
    uint32_t Vsize = Vend - Vstart;
    assuming(Lstart + Lsize <= ValueStack.size());
    auto Locals = Span<const Runtime::StackManager::Value>(
        ValueStack.begin() + Lstart, Lsize);
    assuming(Vstart + Vsize <= ValueStack.size());
    Span<const Runtime::StackManager::Value> Stacks;
    if (!ForWasmgdb) {
      Stacks = Span<const Runtime::StackManager::Value>(
          ValueStack.begin() + Vstart, Vsize);
    }
    Ser.serializeU32(Funcidx, Content);
    Ser.serializeU32(Codeoffset, Content);
    // locals size
    Ser.serializeU32(Frames[Idx].Locals, Content);
    // stack size
    Ser.serializeU32(Vsize, Content);
    for (auto &Iter : Locals) {
      // 0x7F implies i32, since it doesn't support i128 and wasmgdb not support
      // i64
      Content.push_back(0x7F);
      auto Value = Iter.unwrap();
      std::vector<Byte> ValueBytes(4);
      std::memcpy(ValueBytes.data(), &Value, sizeof(int64_t));
      Content.insert(Content.end(), ValueBytes.begin(), ValueBytes.end());
    }
    if (!ForWasmgdb) {
      for (auto &Iter : Stacks) {
        // 0x7F implies i32, since it doesn't support i128 and wasmgdb not
        // support i64
        Content.push_back(0x7F);
        auto Value = Iter.unwrap();
        std::vector<Byte> ValueBytes(4);
        std::memcpy(ValueBytes.data(), &Value, sizeof(int64_t));
        Content.insert(Content.end(), ValueBytes.begin(), ValueBytes.end());
      }
    }
  }
  return CoreStack;
}

// AST::DataSection
// createData(Span<const Runtime::Instance::DataInstance *const> DataInstances)
// {
//   AST::DataSection DataSec;
//   AST::DataSegment Seg;
//   auto &Content = Seg.getData();
//   for (auto &Data : DataInstances) {
//     Content.insert(Content.end(), Data->getData().begin(),
//                    Data->getData().end());
//   }
//   DataSec.getContent().push_back(Seg);
//   return DataSec;
// }
AST::GlobalSection createGlobals(
    Span<const Runtime::Instance::GlobalInstance *const> GlobalInstances) {
  AST::GlobalSection Globals;
  for (auto &Global : GlobalInstances) {
    AST::GlobalSegment Seg;
    Seg.getGlobalType() = Global->getGlobalType();
    Seg.getExpr().getInstrs() = {
        WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};
    Globals.getContent().push_back(Seg);
  }
  return Globals;
}
AST::MemorySection createMemory(
    Span<const Runtime::Instance::MemoryInstance *const> MemoryInstances) {
  AST::MemorySection Memory;
  auto &Content = Memory.getContent();
  if (MemoryInstances.size() == 0) {
    return Memory;
  }
  Content.push_back(MemoryInstances[0]->getMemoryType());
  return Memory;
}

AST::CustomSection createCoremodules(
    Loader::Serializer &Ser,
    Span<const Runtime::Instance::ModuleInstance *const> ModuleInstances) {
  AST::CustomSection CoreModules;
  CoreModules.setName("coremodules");
  auto &Content = CoreModules.getContent();
  Ser.serializeU32(static_cast<uint32_t>(ModuleInstances.size()), Content);
  for (auto &Module : ModuleInstances) {
    auto Name = Module->getModuleName();
    Content.push_back(0x00);
    Content.insert(Content.end(), Name.begin(), Name.end());
  }
  return CoreModules;
}

AST::CustomSection
createCoreinstances(Span<const Runtime::Instance::ModuleInstance *const>) {
  // TODO: Finish coreinstances
  AST::CustomSection CoreInstances;
  CoreInstances.setName("coreinstances");
  auto &Content = CoreInstances.getContent();
  Content.push_back(0x00);
  return CoreInstances;
}

} // namespace Coredump
} // namespace WasmEdge
