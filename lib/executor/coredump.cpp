// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/coredump.h"

#include "ast/section.h"
#include "ast/segment.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "loader/serialize.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Coredump {

namespace {

using Runtime::StackManager;
using Runtime::Instance::FunctionInstance;
using Runtime::Instance::GlobalInstance;
using Runtime::Instance::MemoryInstance;
using Runtime::Instance::ModuleInstance;

/// Type tags of the `value` production of the coredump format.
static inline constexpr const Byte ValueMissing = 0x01;
static inline constexpr const Byte ValueI32 = 0x7F;
static inline constexpr const Byte ValueI64 = 0x7E;
static inline constexpr const Byte ValueF32 = 0x7D;
static inline constexpr const Byte ValueF64 = 0x7C;

/// Serialize a wasm `name`, which is a length prefixed byte vector.
void serializeName(const Loader::Serializer &Ser, std::string_view Name,
                   std::vector<Byte> &OutVec) noexcept {
  Ser.serializeU32(static_cast<uint32_t>(Name.size()), OutVec);
  OutVec.insert(OutVec.end(), Name.begin(), Name.end());
}

/// Serialize a `value`. The canonical encoder of the coredump format encodes
/// the integers as LEB128, but the parser of wasmgdb reads them as fixed width
/// little-endian values.
void serializeValue(const Loader::Serializer &Ser, const ValVariant &Val,
                    const ValType &Type, bool ForWasmgdb,
                    std::vector<Byte> &OutVec) noexcept {
  auto PushRaw = [&OutVec](const auto &Num) {
    Byte Bytes[sizeof(Num)];
    std::memcpy(Bytes, &Num, sizeof(Num));
    OutVec.insert(OutVec.end(), std::begin(Bytes), std::end(Bytes));
  };
  switch (Type.getCode()) {
  case TypeCode::I32:
    OutVec.push_back(ValueI32);
    if (ForWasmgdb) {
      PushRaw(Val.get<int32_t>());
    } else {
      Ser.serializeS32(Val.get<int32_t>(), OutVec);
    }
    return;
  case TypeCode::I64:
    OutVec.push_back(ValueI64);
    if (ForWasmgdb) {
      PushRaw(Val.get<int64_t>());
    } else {
      Ser.serializeS64(Val.get<int64_t>(), OutVec);
    }
    return;
  case TypeCode::F32:
    OutVec.push_back(ValueF32);
    Ser.serializeF32(Val.get<float>(), OutVec);
    return;
  case TypeCode::F64:
    OutVec.push_back(ValueF64);
    Ser.serializeF64(Val.get<double>(), OutVec);
    return;
  default:
    // The coredump format has no encoding for V128 and reference values.
    OutVec.push_back(ValueMissing);
    return;
  }
}

/// The index spaces of a coredump file. The runtime shares the entities between
/// the importing and the exporting module instances, therefore the entities are
/// deduplicated by their addresses and referred by the `coreinstances` section.
class IndexSpaces {
public:
  void addModule(const ModuleInstance *Mod) noexcept {
    if (Mod == nullptr || ModuleIdx.find(Mod) != ModuleIdx.end()) {
      return;
    }
    ModuleIdx.emplace(Mod, static_cast<uint32_t>(Modules.size()));
    Modules.push_back(Mod);
    auto &MemIdxVec = InstanceMemories.emplace_back();
    for (const auto *Mem : Mod->getMemoryInstances()) {
      MemIdxVec.push_back(add(Mem, Memories, MemoryIdx));
    }
    auto &GlobIdxVec = InstanceGlobals.emplace_back();
    for (const auto *Glob : Mod->getGlobalInstances()) {
      GlobIdxVec.push_back(add(Glob, Globals, GlobalIdx));
    }
  }

  uint32_t getModuleIdx(const ModuleInstance *Mod) const noexcept {
    const auto Iter = ModuleIdx.find(Mod);
    return Iter == ModuleIdx.end() ? 0U : Iter->second;
  }

  Span<const ModuleInstance *const> getModules() const noexcept {
    return Modules;
  }
  Span<const MemoryInstance *const> getMemories() const noexcept {
    return Memories;
  }
  Span<const GlobalInstance *const> getGlobals() const noexcept {
    return Globals;
  }
  const std::vector<std::vector<uint32_t>> &
  getInstanceMemories() const noexcept {
    return InstanceMemories;
  }
  const std::vector<std::vector<uint32_t>> &
  getInstanceGlobals() const noexcept {
    return InstanceGlobals;
  }

private:
  template <typename T>
  static uint32_t add(const T *Entity, std::vector<const T *> &Vec,
                      std::unordered_map<const T *, uint32_t> &Idx) noexcept {
    const auto Iter = Idx.find(Entity);
    if (Iter != Idx.end()) {
      return Iter->second;
    }
    const auto NewIdx = static_cast<uint32_t>(Vec.size());
    Idx.emplace(Entity, NewIdx);
    Vec.push_back(Entity);
    return NewIdx;
  }

  std::vector<const ModuleInstance *> Modules;
  std::vector<const MemoryInstance *> Memories;
  std::vector<const GlobalInstance *> Globals;
  std::unordered_map<const ModuleInstance *, uint32_t> ModuleIdx;
  std::unordered_map<const MemoryInstance *, uint32_t> MemoryIdx;
  std::unordered_map<const GlobalInstance *, uint32_t> GlobalIdx;
  std::vector<std::vector<uint32_t>> InstanceMemories;
  std::vector<std::vector<uint32_t>> InstanceGlobals;
};

/// Resolve an instruction pointer back to the function which contains it.
class FunctionResolver {
public:
  void addModule(const ModuleInstance *Mod) noexcept {
    if (Mod == nullptr || Ranges.find(Mod) != Ranges.end()) {
      return;
    }
    auto &Map = Ranges[Mod];
    const auto FuncInsts = Mod->getFunctionInstances();
    for (uint32_t I = 0; I < FuncInsts.size(); ++I) {
      const auto *Func = FuncInsts[I];
      if (Func == nullptr || !Func->isWasmFunction()) {
        continue;
      }
      const auto Instrs = Func->getInstrs();
      if (Instrs.begin() == Instrs.end()) {
        continue;
      }
      Map.emplace(Instrs.begin(), std::make_pair(I, Func));
      Map.emplace(Instrs.end(), std::make_pair(UINT32_MAX, nullptr));
    }
  }

  /// Return the index and the instance of the function containing `It`.
  std::pair<uint32_t, const FunctionInstance *>
  resolve(const ModuleInstance *Mod,
          AST::InstrView::iterator It) const noexcept {
    const auto ModIter = Ranges.find(Mod);
    if (ModIter == Ranges.end() || It == nullptr) {
      return {0U, nullptr};
    }
    const auto &Map = ModIter->second;
    auto Iter = Map.upper_bound(It);
    if (Iter == Map.begin()) {
      return {0U, nullptr};
    }
    --Iter;
    if (Iter->second.second == nullptr) {
      return {0U, nullptr};
    }
    return Iter->second;
  }

private:
  std::unordered_map<const ModuleInstance *,
                     std::map<AST::InstrView::iterator,
                              std::pair<uint32_t, const FunctionInstance *>>>
      Ranges;
};

/// The description of one wasm stack frame in the coredump.
struct FrameInfo {
  uint32_t InstanceIdx = 0;
  uint32_t FuncIdx = 0;
  uint32_t CodeOffset = 0;
  const FunctionInstance *Func = nullptr;
  Span<const StackManager::Value> Locals;
  Span<const StackManager::Value> Stack;
};

/// Collect the wasm frames from the youngest one to the oldest one.
std::vector<FrameInfo>
collectFrames(const StackManager &StackMgr, AST::InstrView::iterator PC,
              const IndexSpaces &Spaces,
              const FunctionResolver &Resolver) noexcept {
  const auto Frames = StackMgr.getFramesSpan();
  const auto ValueStack = StackMgr.getValueSpan();
  const auto FrameNum = Frames.size();
  std::vector<FrameInfo> Result;
  if (FrameNum == 0) {
    return Result;
  }
  Result.reserve(FrameNum);
  for (size_t Idx = FrameNum; Idx > 0; Idx--) {
    const auto &Frame = Frames[Idx - 1];
    // The bottom frame is a dummy frame pushed before entering a function.
    if (Frame.Module == nullptr) {
      continue;
    }
    FrameInfo Info;
    Info.InstanceIdx = Spaces.getModuleIdx(Frame.Module);
    // The `From` of the callee frame points to the call instruction in this
    // frame, and the innermost frame is executing the trapping instruction.
    const auto *const It = (Idx < FrameNum) ? Frames[Idx].From : PC;
    std::tie(Info.FuncIdx, Info.Func) = Resolver.resolve(Frame.Module, It);
    if (Info.Func != nullptr) {
      // The code offset is relative to the start of the function body.
      Info.CodeOffset = It->getOffset() - Info.Func->getInstrs()[0].getOffset();
    }

    // The locals of this frame are the values below its stack base, and its
    // operand stack ends where the locals of the callee frame begin.
    const auto ValueNum = static_cast<uint32_t>(ValueStack.size());
    const uint32_t LocalStart =
        Frame.VPos >= Frame.Locals ? Frame.VPos - Frame.Locals : 0U;
    const uint32_t StackEnd = (Idx < FrameNum)
                                  ? (Frames[Idx].VPos >= Frames[Idx].Locals
                                         ? Frames[Idx].VPos - Frames[Idx].Locals
                                         : 0U)
                                  : ValueNum;
    if (Frame.VPos <= ValueNum && LocalStart <= Frame.VPos) {
      Info.Locals = ValueStack.subspan(LocalStart, Frame.VPos - LocalStart);
    }
    if (StackEnd <= ValueNum && Frame.VPos <= StackEnd) {
      Info.Stack = ValueStack.subspan(Frame.VPos, StackEnd - Frame.VPos);
    }
    Result.push_back(Info);
  }
  return Result;
}

/// Collect the value types of the parameters and the locals of a function.
std::vector<ValType> collectLocalTypes(const FunctionInstance *Func) noexcept {
  std::vector<ValType> Types;
  if (Func == nullptr) {
    return Types;
  }
  const auto &Params = Func->getFuncType().getParamTypes();
  Types.insert(Types.end(), Params.begin(), Params.end());
  for (const auto &[Count, Type] : Func->getLocals()) {
    Types.insert(Types.end(), Count, Type);
  }
  return Types;
}

/// `core ::= customsec(process-info)`
/// `process-info ::= 0x0 executable-name:name`
AST::CustomSection createCore(const Loader::Serializer &Ser,
                              std::string_view ExecutableName) noexcept {
  AST::CustomSection Core;
  Core.setName("core");
  auto &Content = Core.getContent();
  Content.push_back(0x00);
  serializeName(Ser, ExecutableName, Content);
  return Core;
}

/// `coremodules ::= customsec(vec(coremodule))`
/// `coremodule ::= 0x0 module-name:name`
AST::CustomSection createCoreModules(const Loader::Serializer &Ser,
                                     const IndexSpaces &Spaces) noexcept {
  AST::CustomSection CoreModules;
  CoreModules.setName("coremodules");
  auto &Content = CoreModules.getContent();
  const auto Modules = Spaces.getModules();
  Ser.serializeU32(static_cast<uint32_t>(Modules.size()), Content);
  for (const auto *Mod : Modules) {
    Content.push_back(0x00);
    serializeName(Ser, Mod->getModuleName(), Content);
  }
  return CoreModules;
}

/// `coreinstances ::= customsec(vec(coreinstance))`
/// `coreinstance ::= 0x0 moduleidx:u32 memories:vec(u32) globals:vec(u32)`
AST::CustomSection createCoreInstances(const Loader::Serializer &Ser,
                                       const IndexSpaces &Spaces) noexcept {
  AST::CustomSection CoreInstances;
  CoreInstances.setName("coreinstances");
  auto &Content = CoreInstances.getContent();
  const auto &Memories = Spaces.getInstanceMemories();
  const auto &Globals = Spaces.getInstanceGlobals();
  Ser.serializeU32(static_cast<uint32_t>(Memories.size()), Content);
  for (uint32_t I = 0; I < Memories.size(); ++I) {
    Content.push_back(0x00);
    Ser.serializeU32(I, Content);
    Ser.serializeU32(static_cast<uint32_t>(Memories[I].size()), Content);
    for (const auto Idx : Memories[I]) {
      Ser.serializeU32(Idx, Content);
    }
    Ser.serializeU32(static_cast<uint32_t>(Globals[I].size()), Content);
    for (const auto Idx : Globals[I]) {
      Ser.serializeU32(Idx, Content);
    }
  }
  return CoreInstances;
}

/// `corestack ::= customsec(thread-info vec(frame))`
/// `thread-info ::= 0x0 thread-name:name`
/// `frame ::= 0x0 instanceidx:u32 funcidx:u32 codeoffset:u32 locals:vec(value)
///            stack:vec(value)`
///
/// The parser of wasmgdb reads the sizes of both vectors before the values and
/// never reads the content of the operand stack vector, therefore the wasmgdb
/// mode deliberately deviates from the grammar above and emits an empty
/// operand stack. Such a coredump is only readable by wasmgdb, which is why
/// this layout is selected by a dedicated option.
AST::CustomSection createCoreStack(const Loader::Serializer &Ser,
                                   Span<const FrameInfo> Frames,
                                   bool ForWasmgdb) noexcept {
  AST::CustomSection CoreStack;
  CoreStack.setName("corestack");
  auto &Content = CoreStack.getContent();
  Content.push_back(0x00);
  serializeName(Ser, "main"sv, Content);
  Ser.serializeU32(static_cast<uint32_t>(Frames.size()), Content);
  for (const auto &Frame : Frames) {
    Content.push_back(0x00);
    Ser.serializeU32(Frame.InstanceIdx, Content);
    Ser.serializeU32(Frame.FuncIdx, Content);
    Ser.serializeU32(Frame.CodeOffset, Content);

    const auto LocalTypes = collectLocalTypes(Frame.Func);
    const auto LocalNum = std::min(Frame.Locals.size(), LocalTypes.size());
    const auto StackNum = ForWasmgdb ? 0U : Frame.Stack.size();
    Ser.serializeU32(static_cast<uint32_t>(LocalNum), Content);
    if (ForWasmgdb) {
      Ser.serializeU32(static_cast<uint32_t>(StackNum), Content);
    }
    for (size_t I = 0; I < LocalNum; ++I) {
      serializeValue(Ser, Frame.Locals[I], LocalTypes[I], ForWasmgdb, Content);
    }
    if (!ForWasmgdb) {
      // The operand stack of the interpreter holds untyped values, the type of
      // an entry cannot be recovered without replaying the validation of the
      // function body. The depth is preserved and the entries are reported as
      // missing values, which the format reserves for this purpose.
      Ser.serializeU32(static_cast<uint32_t>(StackNum), Content);
      for (size_t I = 0; I < StackNum; ++I) {
        Content.push_back(ValueMissing);
      }
    }
  }
  return CoreStack;
}

AST::MemorySection createMemory(const IndexSpaces &Spaces) noexcept {
  AST::MemorySection Memory;
  auto &Content = Memory.getContent();
  for (const auto *Mem : Spaces.getMemories()) {
    Content.push_back(Mem->getMemoryType());
  }
  return Memory;
}

/// The content of every linear memory is dumped into the data section. The
/// parser of wasmgdb only reads the first data segment, therefore the whole
/// memory is dumped as a single segment in the wasmgdb mode. Otherwise the runs
/// of zeros are skipped to keep the coredump small.
AST::DataSection createData(const IndexSpaces &Spaces,
                            bool ForWasmgdb) noexcept {
  static constexpr const uint64_t ChunkSize = UINT64_C(4096);
  AST::DataSection Data;
  auto &Content = Data.getContent();
  const auto Memories = Spaces.getMemories();
  for (uint32_t I = 0; I < Memories.size(); ++I) {
    const auto *Mem = Memories[I];
    const auto Size = Mem->getSize();
    auto Bytes = Mem->getBytes(0, Size);
    if (!Bytes) {
      continue;
    }
    const bool Is64 = Mem->getMemoryType().getLimit().is64();
    auto AddSegment = [&](uint64_t Offset, Span<const Byte> Chunk) {
      AST::DataSegment Seg;
      Seg.setMode(AST::DataSegment::DataMode::Active);
      Seg.setIdx(I);
      AST::Instruction OffsetInstr(Is64 ? OpCode::I64__const
                                        : OpCode::I32__const);
      OffsetInstr.setNum(static_cast<uint128_t>(Offset));
      Seg.getExpr().getInstrs() = {OffsetInstr, AST::Instruction(OpCode::End)};
      Seg.getData().assign(Chunk.begin(), Chunk.end());
      Content.push_back(std::move(Seg));
    };
    if (ForWasmgdb) {
      AddSegment(0, *Bytes);
      continue;
    }
    for (uint64_t Begin = 0; Begin < Size; Begin += ChunkSize) {
      const auto End = std::min(Begin + ChunkSize, Size);
      const auto Chunk = Bytes->subspan(Begin, End - Begin);
      auto *const First =
          std::find_if(Chunk.begin(), Chunk.end(), [](Byte B) { return B; });
      if (First == Chunk.end()) {
        continue;
      }
      const auto Last =
          std::find_if(Chunk.rbegin(), Chunk.rend(), [](Byte B) { return B; });
      const auto Start = static_cast<uint64_t>(First - Chunk.begin());
      const auto Stop = static_cast<uint64_t>(Chunk.rend() - Last);
      AddSegment(Begin + Start, Chunk.subspan(Start, Stop - Start));
    }
  }
  return Data;
}

/// The globals are dumped as constant and non-mutable globals holding their
/// current values.
AST::GlobalSection createGlobals(const IndexSpaces &Spaces) noexcept {
  AST::GlobalSection Globals;
  auto &Content = Globals.getContent();
  for (const auto *Glob : Spaces.getGlobals()) {
    const auto &Type = Glob->getGlobalType().getValType();
    AST::GlobalSegment Seg;
    Seg.getGlobalType() = AST::GlobalType(Type, ValMut::Const);
    AST::Instruction Init(OpCode::Ref__null);
    switch (Type.getCode()) {
    case TypeCode::I32:
      Init = AST::Instruction(OpCode::I32__const);
      Init.setNum(Glob->getValue());
      break;
    case TypeCode::I64:
      Init = AST::Instruction(OpCode::I64__const);
      Init.setNum(Glob->getValue());
      break;
    case TypeCode::F32:
      Init = AST::Instruction(OpCode::F32__const);
      Init.setNum(Glob->getValue());
      break;
    case TypeCode::F64:
      Init = AST::Instruction(OpCode::F64__const);
      Init.setNum(Glob->getValue());
      break;
    case TypeCode::V128:
      Init = AST::Instruction(OpCode::V128__const);
      Init.setNum(Glob->getValue());
      break;
    default:
      // A reference cannot be represented by a constant expression of a
      // coredump, dump a null reference of the same type instead.
      Init.setValType(Type);
      break;
    }
    Seg.getExpr().getInstrs() = {Init, AST::Instruction(OpCode::End)};
    Content.push_back(std::move(Seg));
  }
  return Globals;
}

} // namespace

Expect<std::string> generateCoredump(const Runtime::StackManager &StackMgr,
                                     AST::InstrView::iterator PC,
                                     bool ForWasmgdb) noexcept {
  spdlog::info("Generating coredump..."sv);

  IndexSpaces Spaces;
  FunctionResolver Resolver;
  for (const auto &Frame : StackMgr.getFramesSpan()) {
    Spaces.addModule(Frame.Module);
    Resolver.addModule(Frame.Module);
  }
  Spaces.addModule(StackMgr.getModule());
  Resolver.addModule(StackMgr.getModule());
  if (Spaces.getModules().empty()) {
    spdlog::error("Failed to generate coredump: no module instance found."sv);
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  const Loader::Serializer Ser;
  AST::Module Module;
  auto &Magic = Module.getMagic();
  Magic.assign({0x00, 0x61, 0x73, 0x6D});
  // The version must be 1 to be supported by wasmgdb.
  auto &Version = Module.getVersion();
  Version.assign({0x01, 0x00, 0x00, 0x00});

  const auto Frames = collectFrames(StackMgr, PC, Spaces, Resolver);
  auto &CustomSections = Module.getCustomSections();
  CustomSections.push_back(
      createCore(Ser, Spaces.getModules().front()->getModuleName()));
  CustomSections.push_back(createCoreModules(Ser, Spaces));
  CustomSections.push_back(createCoreInstances(Ser, Spaces));
  CustomSections.push_back(createCoreStack(Ser, Frames, ForWasmgdb));
  Module.getMemorySection() = createMemory(Spaces);
  Module.getGlobalSection() = createGlobals(Spaces);
  Module.getDataSection() = createData(Spaces, ForWasmgdb);
  // The serializer emits the sections ordered by their start offsets, assign
  // the offsets of a well-formed module to keep the section order valid.
  Module.getMemorySection().setStartOffset(1);
  Module.getGlobalSection().setStartOffset(2);
  Module.getDataSection().setStartOffset(3);
  for (auto &Sec : CustomSections) {
    Sec.setStartOffset(4);
  }

  auto Res = Ser.serializeModule(Module);
  if (!Res) {
    spdlog::error("Failed to serialize coredump."sv);
    return Unexpect(Res);
  }

  const std::string Path =
      "coredump." + std::to_string(static_cast<uint64_t>(std::time(nullptr)));
  std::ofstream File(Path, std::ios::out | std::ios::binary);
  if (!File.is_open()) {
    spdlog::error("Failed to open the coredump file {}."sv, Path);
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  File.write(reinterpret_cast<const char *>(Res->data()),
             static_cast<std::streamsize>(Res->size()));
  File.close();
  if (File.fail()) {
    spdlog::error("Failed to write the coredump file {}."sv, Path);
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  spdlog::info("Coredump generated at {}."sv, Path);
  return Path;
}

} // namespace Coredump
} // namespace WasmEdge
