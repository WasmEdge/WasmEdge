// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/restore.h"

#include "common/filesystem.h"
#include "loader/filemgr.h"
#include "loader/loader.h"

#include <algorithm>
#include <array>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace WasmEdge {
namespace Coredump {
namespace Detail {
namespace {

using namespace Runtime::Instance;
constexpr auto Malformed = ErrCode::Value::CoredumpMalformed;
constexpr auto Mismatch = ErrCode::Value::CoredumpMismatch;
constexpr auto Unsupported = ErrCode::Value::CoredumpUnsupported;

class Reader {
public:
  explicit Reader(const AST::CustomSection &Section) {
    const auto Result = FM.setCode(Span<const Byte>(Section.getContent()));
    assuming(Result);
  }

  Expect<Byte> byte() { return FM.readByte().map_error(mapError); }
  Expect<uint32_t> u32() { return FM.readU32().map_error(mapError); }
  Expect<uint64_t> u64() { return FM.readU64().map_error(mapError); }
  Expect<void> name() {
    EXPECTED_TRY(FM.readName().map_error(mapError));
    return {};
  }
  Expect<void> end() const {
    if (FM.getRemainSize() != 0) {
      return Unexpect(Malformed);
    }
    return {};
  }
  Expect<void> version(uint32_t Expected) {
    EXPECTED_TRY(auto Actual, u32());
    if (Actual != Expected) {
      return Unexpect(Malformed);
    }
    return {};
  }
  Expect<uint32_t> count() {
    EXPECTED_TRY(auto Count, u32());
    if (Count > FM.getRemainSize()) {
      return Unexpect(Malformed);
    }
    return Count;
  }
  Expect<ValType> refType() {
    EXPECTED_TRY(auto Code, byte());
    auto RefCode = TypeCode::RefNull;
    int64_t Heap;
    if (Code == static_cast<Byte>(TypeCode::Ref) ||
        Code == static_cast<Byte>(TypeCode::RefNull)) {
      RefCode = static_cast<TypeCode>(Code);
      EXPECTED_TRY(Heap, FM.readS33().map_error(mapError));
    } else {
      if (Code < 0x40 || Code >= 0x80) {
        return Unexpect(Malformed);
      }
      Heap = static_cast<int64_t>(Code) - 128;
    }
    if (Heap >= 0) {
      return ValType(RefCode, static_cast<uint32_t>(Heap));
    }
    if (Heap < -64) {
      return Unexpect(Malformed);
    }
    const auto HeapCode = static_cast<TypeCode>(Heap & 0x7F);
    switch (HeapCode) {
    case TypeCode::FuncRef:
    case TypeCode::ExternRef:
    case TypeCode::AnyRef:
    case TypeCode::EqRef:
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
    case TypeCode::NullRef:
    case TypeCode::NullFuncRef:
    case TypeCode::NullExternRef:
    case TypeCode::ExnRef:
    case TypeCode::NullExnRef:
      return ValType(RefCode, HeapCode);
    default:
      return Unexpect(Malformed);
    }
  }
  Expect<AST::Limit> limit() {
    EXPECTED_TRY(auto Flags, byte());
    if (Flags > 7 || Flags == 2 || Flags == 6) {
      return Unexpect(Malformed);
    }
    EXPECTED_TRY(auto Min, (Flags & 4) ? u64() : u32().map([](uint32_t V) {
      return static_cast<uint64_t>(V);
    }));
    AST::Limit Limit(Min, bool(Flags & 4));
    Limit.setType(static_cast<AST::Limit::LimitType>(Flags));
    if (Flags & 1) {
      EXPECTED_TRY(auto Max, (Flags & 4) ? u64() : u32().map([](uint32_t V) {
        return static_cast<uint64_t>(V);
      }));
      if (Min > Max) {
        return Unexpect(Malformed);
      }
      Limit.setMax(Max);
    }
    return Limit;
  }

private:
  static ErrCode mapError(ErrCode) { return Malformed; }
  FileMgr FM;
};

template <typename T> class Mapping {
public:
  explicit Mapping(size_t Count) : Instances(Count), Owners(Count) {}

  Expect<void> read(Reader &R, Span<T *const> Live, ModuleInstance *Owner) {
    EXPECTED_TRY(auto Count, R.count());
    if (Count != Live.size()) {
      return Unexpect(Mismatch);
    }
    for (auto *Instance : Live) {
      EXPECTED_TRY(auto Idx, R.u32());
      if (Idx >= Instances.size()) {
        return Unexpect(Malformed);
      }
      if (Instance == nullptr ||
          (Instances[Idx] != nullptr && Instances[Idx] != Instance)) {
        return Unexpect(Mismatch);
      }
      auto [It, Inserted] = Indices.emplace(Instance, Idx);
      if (!Inserted && It->second != Idx) {
        return Unexpect(Mismatch);
      }
      if (Instances[Idx] == nullptr) {
        Instances[Idx] = Instance;
        Owners[Idx] = Owner;
      }
    }
    return {};
  }
  Expect<void> complete() const {
    if (std::find(Instances.begin(), Instances.end(), nullptr) !=
        Instances.end()) {
      return Unexpect(Malformed);
    }
    return {};
  }

  std::vector<T *> Instances;
  std::vector<ModuleInstance *> Owners;

private:
  std::unordered_map<T *, uint32_t> Indices;
};

Expect<TypeCode> bottomType(const ValType &Type, const ModuleInstance &Owner) {
  auto Heap = Type.getHeapTypeCode();
  if (Heap == TypeCode::TypeIndex) {
    const auto Def = Owner.getType(Type.getTypeIndex());
    if (!Def) {
      return Unexpect(Mismatch);
    }
    Heap = (*Def)->getCompositeType().isFunc() ? TypeCode::FuncRef
                                               : TypeCode::AnyRef;
  }
  switch (Heap) {
  case TypeCode::FuncRef:
  case TypeCode::NullFuncRef:
    return TypeCode::NullFuncRef;
  case TypeCode::ExternRef:
  case TypeCode::NullExternRef:
    return TypeCode::NullExternRef;
  case TypeCode::ExnRef:
  case TypeCode::NullExnRef:
    return TypeCode::NullExnRef;
  default:
    return TypeCode::NullRef;
  }
}

Expect<RefVariant> readRef(Reader &R, const ValType &Type,
                           const ModuleInstance &Owner,
                           Span<ModuleInstance *const> Modules) {
  EXPECTED_TRY(auto Tag, R.byte());
  switch (Tag) {
  case kRefNull: {
    if (!Type.isNullableRefType()) {
      return Unexpect(Mismatch);
    }
    EXPECTED_TRY(auto Bottom, bottomType(Type, Owner));
    return RefVariant(ValType(TypeCode::RefNull, Bottom));
  }
  case kRefFunction: {
    EXPECTED_TRY(auto Inst, R.u32());
    EXPECTED_TRY(auto FuncIdx, R.u32());
    if (Inst >= Modules.size()) {
      return Unexpect(Malformed);
    }
    const auto Functions = Modules[Inst]->getFunctionInstances();
    if (FuncIdx >= Functions.size() || Functions[FuncIdx] == nullptr) {
      return Unexpect(Malformed);
    }
    const auto *Func = Functions[FuncIdx];
    const auto *FuncOwner = Func->getModule();
    if (Type.getHeapTypeCode() == TypeCode::TypeIndex &&
        !Owner.getType(Type.getTypeIndex())) {
      return Unexpect(Mismatch);
    }
    if (FuncOwner == nullptr ||
        !AST::TypeMatcher::matchType(Owner.getTypeList(), Type,
                                     FuncOwner->getTypeList(),
                                     Func->getDefType())) {
      return Unexpect(Mismatch);
    }
    return RefVariant(Func->getDefType(), Func);
  }
  case kRefMissing:
    return Unexpect(Unsupported);
  default:
    return Unexpect(Malformed);
  }
}

template <typename T>
Expect<void> readDrops(Reader &R, Span<T *const> Live,
                       std::vector<T *> &Drops) {
  EXPECTED_TRY(auto Count, R.count());
  if (Count != Live.size()) {
    return Unexpect(Mismatch);
  }
  for (auto *Instance : Live) {
    EXPECTED_TRY(auto Dropped, R.byte());
    if (Dropped > 1) {
      return Unexpect(Malformed);
    }
    if (!Dropped && Instance->isDropped()) {
      return Unexpect(Unsupported);
    }
    if (Dropped && !Instance->isDropped()) {
      Drops.push_back(Instance);
    }
  }
  return {};
}

bool sameLimit(const AST::Limit &A, const AST::Limit &B) {
  return A.is64() == B.is64() && A.isShared() == B.isShared() &&
         A.hasMax() == B.hasMax() && (!A.hasMax() || A.getMax() == B.getMax());
}

template <typename T>
const ModuleInstance *typeOwner(T *Instance, const ModuleInstance *Fallback,
                                Span<ModuleInstance *const> Modules,
                                const ValType &Type) {
  if (Type.getHeapTypeCode() != TypeCode::TypeIndex) {
    return Fallback;
  }
  for (const auto *Module : Modules) {
    if (Module->owns(Instance)) {
      return Module;
    }
  }
  return nullptr;
}

bool validPlaceholder(const ValType &Type) {
  return Type == ValType(TypeCode::FuncRef) ||
         Type == ValType(TypeCode::ExternRef) ||
         Type == ValType(TypeCode::AnyRef) || Type == ValType(TypeCode::ExnRef);
}

Expect<RestorePlan> buildPlan(const RestoreRequest &Req,
                              const AST::Module &Dump) {
  const std::array<std::string_view, 5> Names = {
      "coremodules", "coreinstances", "wasmedge.tables", "wasmedge.instances",
      "wasmedge.globals"};
  std::array<const AST::CustomSection *, 5> Sections{};
  for (const auto &Section : Dump.getCustomSections()) {
    const auto It = std::find(Names.begin(), Names.end(), Section.getName());
    if (It != Names.end()) {
      auto &Slot = Sections[It - Names.begin()];
      if (Slot != nullptr) {
        return Unexpect(Malformed);
      }
      Slot = &Section;
    }
  }
  if (std::find(Sections.begin(), Sections.end(), nullptr) != Sections.end()) {
    return Unexpect(Malformed);
  }
  std::unordered_set<ModuleInstance *> Unique;
  for (auto *Module : Req.Modules) {
    if (Module == nullptr || !Unique.insert(Module).second) {
      return Unexpect(Mismatch);
    }
  }
  Reader Mods(*Sections[0]);
  EXPECTED_TRY(auto Count, Mods.count());
  if (Count != Req.Modules.size()) {
    return Unexpect(Mismatch);
  }
  for (uint32_t I = 0; I < Count; ++I) {
    EXPECTED_TRY(auto Tag, Mods.byte());
    if (Tag != 0) {
      return Unexpect(Malformed);
    }
    EXPECTED_TRY(Mods.name());
  }
  EXPECTED_TRY(Mods.end());

  const auto &MemTypes = Dump.getMemorySection().getContent();
  if (!Dump.getImportSection().getContent().empty() ||
      !Dump.getFunctionSection().getContent().empty() ||
      !Dump.getCodeSection().getContent().empty() ||
      !Dump.getElementSection().getContent().empty()) {
    return Unexpect(Malformed);
  }
  const auto &Globals = Dump.getGlobalSection().getContent();
  const auto &Tables = Dump.getTableSection().getContent();
  Mapping<MemoryInstance> MemMap(MemTypes.size());
  Mapping<GlobalInstance> GlobMap(Globals.size());
  Mapping<TableInstance> TabMap(Tables.size());
  Reader Core(*Sections[1]);
  EXPECTED_TRY(Count, Core.count());
  if (Count != Req.Modules.size()) {
    return Unexpect(Mismatch);
  }
  for (uint32_t I = 0; I < Count; ++I) {
    EXPECTED_TRY(auto Tag, Core.byte());
    EXPECTED_TRY(auto Idx, Core.u32());
    if (Tag != 0 || Idx != I) {
      return Unexpect(Malformed);
    }
    auto *Mod = Req.Modules[I];
    EXPECTED_TRY(MemMap.read(Core, Mod->getMemoryInstances(), Mod));
    EXPECTED_TRY(GlobMap.read(Core, Mod->getGlobalInstances(), Mod));
  }
  EXPECTED_TRY(Core.end());
  EXPECTED_TRY(MemMap.complete());
  EXPECTED_TRY(GlobMap.complete());

  RestorePlan Plan;
  Reader Instances(*Sections[3]);
  EXPECTED_TRY(Instances.version(kInstancesVersion));
  EXPECTED_TRY(Count, Instances.count());
  if (Count != Req.Modules.size()) {
    return Unexpect(Mismatch);
  }
  for (auto *Mod : Req.Modules) {
    EXPECTED_TRY(TabMap.read(Instances, Mod->getTableInstances(), Mod));
    EXPECTED_TRY(
        readDrops(Instances, Mod->getOwnedDataInstances(), Plan.DataDrops));
    EXPECTED_TRY(readDrops(Instances, Mod->getElemInstances(), Plan.ElemDrops));
  }
  EXPECTED_TRY(Instances.end());
  EXPECTED_TRY(TabMap.complete());

  for (size_t I = 0; I < MemTypes.size(); ++I) {
    auto *Mem = MemMap.Instances[I];
    const auto &Limit = MemTypes[I].getLimit();
    const auto &Live = Mem->getMemoryType().getLimit();
    if (Limit.isShared() || Live.isShared()) {
      return Unexpect(Unsupported);
    }
    if (Limit.hasMax() && Limit.getMin() > Limit.getMax()) {
      return Unexpect(Malformed);
    }
    const auto Pages = Limit.getMin();
    if (Limit.is64() != Live.is64() ||
        (Live.hasMax() && Pages > Live.getMax()) ||
        Pages > Mem->getPageLimit() ||
        Pages >
            std::numeric_limits<size_t>::max() / MemoryInstance::kPageSize ||
        (Live.is32() && Pages > MemoryInstance::kPageLimit32) ||
        Pages > MemoryInstance::kPageLimit64) {
      return Unexpect(Mismatch);
    }
    Plan.Memories.push_back({Mem, Pages, {}});
  }
  for (const auto &Segment : Dump.getDataSection().getContent()) {
    if (Segment.getMode() != AST::DataSegment::DataMode::Active ||
        Segment.getIdx() >= Plan.Memories.size()) {
      return Unexpect(Malformed);
    }
    auto &Memory = Plan.Memories[Segment.getIdx()];
    const auto &Expr = Segment.getExpr().getInstrs();
    const bool Is64 = Memory.Instance->getMemoryType().getLimit().is64();
    if (Expr.size() != 2 || Expr[1].getOpCode() != OpCode::End ||
        Expr[0].getOpCode() !=
            (Is64 ? OpCode::I64__const : OpCode::I32__const)) {
      return Unexpect(Malformed);
    }
    const uint64_t Offset = Is64 ? Expr[0].getNum().get<uint64_t>()
                                 : Expr[0].getNum().get<uint32_t>();
    const auto Size = Memory.Pages * MemoryInstance::kPageSize;
    if (Offset > Size || Segment.getData().size() > Size - Offset) {
      return Unexpect(Malformed);
    }
    Memory.Segments.emplace_back(
        Offset,
        std::vector<Byte>(Segment.getData().begin(), Segment.getData().end()));
  }
  for (auto &Memory : Plan.Memories) {
    std::stable_sort(
        Memory.Segments.begin(), Memory.Segments.end(),
        [](const auto &A, const auto &B) { return A.first < B.first; });
    uint64_t End = 0;
    for (const auto &[Offset, Bytes] : Memory.Segments) {
      if (Offset < End) {
        return Unexpect(Malformed);
      }
      End = Offset + Bytes.size();
    }
  }

  Reader TabReader(*Sections[2]);
  EXPECTED_TRY(TabReader.version(kTablesVersion));
  EXPECTED_TRY(Count, TabReader.count());
  if (Count != Tables.size()) {
    return Unexpect(Malformed);
  }
  for (uint32_t I = 0; I < Count; ++I) {
    EXPECTED_TRY(auto Type, TabReader.refType());
    EXPECTED_TRY(auto Limit, TabReader.limit());
    EXPECTED_TRY(auto Size, TabReader.u64());
    EXPECTED_TRY(auto RefCount, TabReader.count());
    if (RefCount != Size || Limit.getMin() != Size ||
        !sameLimit(Limit, Tables[I].getTableType().getLimit()) ||
        Tables[I].getTableType().getLimit().getMin() != Size) {
      return Unexpect(Malformed);
    }
    const auto &Live = TabMap.Instances[I]->getTableType();
    if (!validPlaceholder(Tables[I].getTableType().getRefType()) ||
        !Tables[I].getExpr().getInstrs().empty()) {
      return Unexpect(Malformed);
    }
    if (Type != Live.getRefType() || !sameLimit(Limit, Live.getLimit()) ||
        (Live.getLimit().hasMax() && Size > Live.getLimit().getMax()) ||
        Size > getMaxAddress(Live.getLimit().getAddrType())) {
      return Unexpect(Mismatch);
    }
    TablePlan Table;
    Table.Instance = TabMap.Instances[I];
    const auto *Owner =
        typeOwner(Table.Instance, TabMap.Owners[I], Req.Modules, Type);
    if (Owner == nullptr) {
      return Unexpect(Unsupported);
    }
    for (uint32_t J = 0; J < RefCount; ++J) {
      EXPECTED_TRY(auto Ref, readRef(TabReader, Type, *Owner, Req.Modules));
      Table.Refs.push_back(Ref);
    }
    Plan.Tables.push_back(std::move(Table));
  }
  EXPECTED_TRY(TabReader.end());

  std::vector<bool> RefSeen(Globals.size(), false);
  Reader GlobReader(*Sections[4]);
  EXPECTED_TRY(GlobReader.version(kGlobalsVersion));
  EXPECTED_TRY(Count, GlobReader.count());
  for (uint32_t I = 0; I < Count; ++I) {
    EXPECTED_TRY(auto Idx, GlobReader.u32());
    EXPECTED_TRY(auto Type, GlobReader.refType());
    EXPECTED_TRY(auto Mut, GlobReader.byte());
    if (Idx >= Globals.size() || RefSeen[Idx] || Mut > 1) {
      return Unexpect(Malformed);
    }
    auto *Global = GlobMap.Instances[Idx];
    const auto &Live = Global->getGlobalType();
    if (Type != Live.getValType() ||
        Mut != static_cast<Byte>(Live.getValMut())) {
      return Unexpect(Mismatch);
    }
    const auto *Owner =
        typeOwner(Global, GlobMap.Owners[Idx], Req.Modules, Type);
    if (Owner == nullptr) {
      return Unexpect(Unsupported);
    }
    EXPECTED_TRY(auto Ref, readRef(GlobReader, Type, *Owner, Req.Modules));
    RefSeen[Idx] = true;
    Plan.Globals.emplace_back(Global, Ref);
  }
  EXPECTED_TRY(GlobReader.end());
  for (size_t I = 0; I < Globals.size(); ++I) {
    const auto &Segment = Globals[I];
    const auto &Expr = Segment.getExpr().getInstrs();
    const auto &Type = GlobMap.Instances[I]->getGlobalType().getValType();
    if (Expr.size() != 2 || Expr[1].getOpCode() != OpCode::End) {
      return Unexpect(Malformed);
    }
    if (Type.isRefType()) {
      if (!RefSeen[I] ||
          !validPlaceholder(Segment.getGlobalType().getValType()) ||
          Expr[0].getOpCode() != OpCode::Ref__null ||
          Expr[0].getValType() != Segment.getGlobalType().getValType()) {
        return Unexpect(Malformed);
      }
      continue;
    }
    if (Type != Segment.getGlobalType().getValType()) {
      return Unexpect(Mismatch);
    }
    OpCode Expected;
    switch (Type.getCode()) {
    case TypeCode::I32:
      Expected = OpCode::I32__const;
      break;
    case TypeCode::I64:
      Expected = OpCode::I64__const;
      break;
    case TypeCode::F32:
      Expected = OpCode::F32__const;
      break;
    case TypeCode::F64:
      Expected = OpCode::F64__const;
      break;
    case TypeCode::V128:
      Expected = OpCode::V128__const;
      break;
    default:
      return Unexpect(Malformed);
    }
    if (Expr[0].getOpCode() != Expected) {
      return Unexpect(Malformed);
    }
    Plan.Globals.emplace_back(GlobMap.Instances[I], Expr[0].getNum());
  }
  return Plan;
}

} // namespace

Expect<RestorePlan> preflight(const RestoreRequest &Req) noexcept {
  Configure Conf;
  for (auto Proposal :
       {Proposal::MultiMemories, Proposal::Memory64, Proposal::GC,
        Proposal::FunctionReferences, Proposal::ExceptionHandling,
        Proposal::Threads, Proposal::SIMD, Proposal::ReferenceTypes}) {
    Conf.addProposal(Proposal);
  }
  Loader::Loader Loader(Conf);
  auto Dump = Loader.parseModule(std::filesystem::path(Req.Path));
  if (!Dump) {
    return Unexpect(Dump.error() == ErrCode::Value::IllegalPath
                        ? Dump.error()
                        : ErrCode(Malformed));
  }
  return buildPlan(Req, **Dump);
}

} // namespace Detail

Expect<void> restore(const RestoreRequest &Req) noexcept {
  EXPECTED_TRY(auto Plan, Detail::preflight(Req));
  for (auto &Memory : Plan.Memories) {
    auto &Instance = *Memory.Instance;
    const auto Old = Instance.getPageSize();
    const auto Resized = Memory.Pages < Old
                             ? Instance.shrinkPage(Old - Memory.Pages)
                             : Instance.growPage(Memory.Pages - Old);
    assuming(Resized);
    uint64_t End = 0;
    for (const auto &[Offset, Bytes] : Memory.Segments) {
      const auto Cleared = Instance.fillBytes(0, End, Offset - End);
      assuming(Cleared);
      const auto Written = Instance.setBytes(Bytes, Offset, 0, Bytes.size());
      assuming(Written);
      End = Offset + Bytes.size();
    }
    const auto Cleared = Instance.fillBytes(0, End, Instance.getSize() - End);
    assuming(Cleared);
  }
  for (auto &Table : Plan.Tables) {
    auto &Instance = *Table.Instance;
    const auto Old = Instance.getSize();
    const auto Size = Table.Refs.size();
    const auto Resized = Size < Old ? Instance.shrinkTable(Old - Size)
                                    : Instance.growTable(Size - Old);
    assuming(Resized);
    const auto Written = Instance.setRefs(Table.Refs, 0, 0, Size);
    assuming(Written);
  }
  for (auto &[Global, Value] : Plan.Globals) {
    Global->setValue(Value);
  }
  for (auto *Data : Plan.DataDrops) {
    Data->clear();
  }
  for (auto *Elem : Plan.ElemDrops) {
    Elem->clear();
  }
  return {};
}

} // namespace Coredump
} // namespace WasmEdge
