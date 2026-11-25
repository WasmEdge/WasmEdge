// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "runtime/typetable.h"

#include "common/hash.h"

#include <algorithm>
#include <new>
#include <optional>

namespace WasmEdge {
namespace Runtime {

namespace {

constexpr uint64_t kInGroup = UINT64_C(1) << 32;

uint64_t encodeTypeCodes(const ValType &VT) noexcept {
  return (static_cast<uint64_t>(VT.getCode()) << 8) |
         static_cast<uint64_t>(VT.getHeapTypeCode());
}

} // namespace

CanonicalTypeTable::CanonicalTypeTable(uint32_t C) noexcept
    : Cap(std::min(C, kDefaultCap)) {}

CanonicalTypeTable::~CanonicalTypeTable() noexcept {
  for (uint32_t I = 0; I < kMaxBlocks; ++I) {
    delete[] Blocks[I];
    delete[] Layouts[I];
  }
}

uint32_t CanonicalTypeTable::getFieldSize(const ValType &StorageType) noexcept {
  if (StorageType.isRefType()) {
    return static_cast<uint32_t>(sizeof(RefVariant));
  }
  return StorageType.getBitWidth() / 8;
}

namespace {
std::atomic<CanonicalTypeTable *> GlobalTable = nullptr;
std::mutex GlobalTableMutex;
struct alignas(CanonicalTypeTable) TableStorage {
  unsigned char Bytes[sizeof(CanonicalTypeTable)];
};
TableStorage DefaultTable;
} // namespace

CanonicalTypeTable &CanonicalTypeTable::getGlobal() noexcept {
  if (auto *Table = GlobalTable.load(std::memory_order_acquire)) {
    return *Table;
  }
  std::unique_lock Lock(GlobalTableMutex);
  auto *Table = GlobalTable.load(std::memory_order_relaxed);
  if (Table == nullptr) {
    Table = new (DefaultTable.Bytes) CanonicalTypeTable();
    GlobalTable.store(Table, std::memory_order_release);
  }
  return *Table;
}

bool CanonicalTypeTable::attachGlobal(CanonicalTypeTable *Table) noexcept {
  if (Table == nullptr) {
    return false;
  }
  std::unique_lock Lock(GlobalTableMutex);
  auto *Current = GlobalTable.load(std::memory_order_relaxed);
  if (Current == nullptr) {
    GlobalTable.store(Table, std::memory_order_release);
    return true;
  }
  return Current == Table;
}

size_t CanonicalTypeTable::KeyHash::operator()(
    const std::vector<uint64_t> &Key) const noexcept {
  return static_cast<size_t>(Hash::Hash::rapidHash(
      cxx20::as_bytes(Span<const uint64_t>(Key.data(), Key.size()))));
}

namespace {

/// Module type indices of a rec group as key tokens. A token is the
/// canonical id of an earlier type, or `kInGroup` with a position in the
/// group.
struct GroupIndex {
  uint32_t Start;
  uint32_t N;
  Span<const uint32_t> ModuleIds;
  uint32_t Known;

  std::optional<uint64_t> resolve(uint32_t Idx) const noexcept {
    if (Idx >= Start && Idx - Start < N) {
      return kInGroup | (Idx - Start);
    }
    if (Idx < Start && Idx < ModuleIds.size() && ModuleIds[Idx] < Known) {
      return ModuleIds[Idx];
    }
    return std::nullopt;
  }

  /// Canonical id of the type `Idx` if the group starts at the id `First`.
  uint32_t toId(uint32_t Idx, uint32_t First) const noexcept {
    const uint64_t Token = *resolve(Idx);
    return (Token & kInGroup) ? First + static_cast<uint32_t>(Token & ~kInGroup)
                              : static_cast<uint32_t>(Token);
  }

  ValType toCanonical(const ValType &VT, uint32_t First) const noexcept {
    if (VT.isRefType() && !VT.isAbsHeapType()) {
      return ValType(VT.getCode(), toId(VT.getTypeIndex(), First));
    }
    return VT;
  }
};

/// Append the tokens of `VT` to `Key`. Fails if `VT` refers to a type that
/// is not known.
bool appendValType(std::vector<uint64_t> &Key, const GroupIndex &Index,
                   const ValType &VT) {
  Key.push_back(encodeTypeCodes(VT));
  if (VT.isRefType() && !VT.isAbsHeapType()) {
    const auto Token = Index.resolve(VT.getTypeIndex());
    if (!Token.has_value()) {
      return false;
    }
    Key.push_back(*Token);
  }
  return true;
}

/// Append the count and the tokens of the parameter or result `Types`.
bool appendValTypes(std::vector<uint64_t> &Key, const GroupIndex &Index,
                    Span<const ValType> Types) {
  Key.push_back(Types.size());
  return std::all_of(Types.begin(), Types.end(), [&](const ValType &VT) {
    return appendValType(Key, Index, VT);
  });
}

/// Key of the rec group. Two groups get the same key if and only if their
/// types are equal after their references become canonical ids.
Expect<std::vector<uint64_t>>
makeGroupKey(Span<const AST::SubType *const> Group, const GroupIndex &Index) {
  std::vector<uint64_t> Key;
  Key.push_back(Index.N);
  for (uint32_t I = 0; I < Index.N; ++I) {
    const auto &Type = *Group[I];
    Key.push_back(Type.isFinal() ? 1U : 0U);
    const auto Supers = Type.getSuperTypeIndices();
    Key.push_back(Supers.size());
    for (const auto Super : Supers) {
      const auto Token = Index.resolve(Super);
      if (!Token.has_value() ||
          ((*Token & kInGroup) && (*Token & ~kInGroup) >= I)) {
        return Unexpect(ErrCode::Value::IncompatibleImportType);
      }
      Key.push_back(*Token);
    }
    const auto &CompType = Type.getCompositeType();
    Key.push_back(static_cast<uint64_t>(CompType.getContentTypeCode()));
    bool Known = true;
    if (CompType.isFunc()) {
      const auto &FuncType = CompType.getFuncType();
      Known = appendValTypes(Key, Index, FuncType.getParamTypes()) &&
              appendValTypes(Key, Index, FuncType.getReturnTypes());
    } else {
      const auto &Fields = CompType.getFieldTypes();
      Key.push_back(Fields.size());
      Known = std::all_of(
          Fields.begin(), Fields.end(), [&](const AST::FieldType &Field) {
            Key.push_back(static_cast<uint64_t>(Field.getValMut()));
            return appendValType(Key, Index, Field.getStorageType());
          });
    }
    if (!Known) {
      return Unexpect(ErrCode::Value::IncompatibleImportType);
    }
  }
  return Key;
}

/// Copy of the type at `I` in a group that starts at the id `First`, with
/// canonical ids in place of module type indices.
AST::SubType makeCanonicalType(const AST::SubType &Type,
                               const GroupIndex &Index, uint32_t First,
                               uint32_t I) {
  AST::SubType Copy;
  Copy.setFinal(Type.isFinal());
  Copy.getSuperTypeIndices().reserve(Type.getSuperTypeIndices().size());
  for (const auto Super : Type.getSuperTypeIndices()) {
    Copy.getSuperTypeIndices().push_back(Index.toId(Super, First));
  }
  auto ToField = [&](const AST::FieldType &Field) {
    return AST::FieldType(Index.toCanonical(Field.getStorageType(), First),
                          Field.getValMut());
  };
  const auto &CompType = Type.getCompositeType();
  switch (CompType.getContentTypeCode()) {
  case TypeCode::Func: {
    AST::FunctionType FuncType;
    const auto &Params = CompType.getFuncType().getParamTypes();
    const auto &Results = CompType.getFuncType().getReturnTypes();
    FuncType.getParamTypes().reserve(Params.size());
    for (const auto &VT : Params) {
      FuncType.getParamTypes().push_back(Index.toCanonical(VT, First));
    }
    FuncType.getReturnTypes().reserve(Results.size());
    for (const auto &VT : Results) {
      FuncType.getReturnTypes().push_back(Index.toCanonical(VT, First));
    }
    Copy.getCompositeType().setFunctionType(std::move(FuncType));
    break;
  }
  case TypeCode::Struct: {
    std::vector<AST::FieldType> Fields;
    Fields.reserve(CompType.getFieldTypes().size());
    for (const auto &Field : CompType.getFieldTypes()) {
      Fields.push_back(ToField(Field));
    }
    Copy.getCompositeType().setStructType(std::move(Fields));
    break;
  }
  case TypeCode::Array:
    Copy.getCompositeType().setArrayType(ToField(CompType.getFieldTypes()[0]));
    break;
  default:
    assumingUnreachable();
  }
  Copy.setRecursiveInfo(I, Index.N);
  Copy.setTypeIndex(First + I);
  return Copy;
}

/// Field positions of a struct type, then its total field size. Empty for
/// other types. Fails if the fields do not fit in `kMaxFieldBytes`.
Expect<std::vector<uint32_t>>
makeFieldLayout(const AST::CompositeType &CompType) {
  std::vector<uint32_t> Positions;
  if (CompType.getContentTypeCode() != TypeCode::Struct) {
    return Positions;
  }
  Positions.reserve(CompType.getFieldTypes().size() + 1);
  uint64_t Pos = 0;
  for (const auto &Field : CompType.getFieldTypes()) {
    const uint64_t FieldSize =
        CanonicalTypeTable::getFieldSize(Field.getStorageType());
    Pos = (Pos + FieldSize - 1) & ~(FieldSize - 1);
    if (Pos + FieldSize > CanonicalTypeTable::kMaxFieldBytes) {
      return Unexpect(ErrCode::Value::TypeTableFull);
    }
    Positions.push_back(static_cast<uint32_t>(Pos));
    Pos += FieldSize;
  }
  Positions.push_back(static_cast<uint32_t>(Pos));
  return Positions;
}

} // namespace

Expect<uint32_t> CanonicalTypeTable::registerRecGroup(
    Span<const AST::SubType *const> Group, uint32_t Start,
    Span<const uint32_t> ModuleIds) noexcept try {
  const auto N = static_cast<uint32_t>(Group.size());
  assuming(N > 0);
  const GroupIndex Index{Start, N, ModuleIds, size()};
  EXPECTED_TRY(auto Key, makeGroupKey(Group, Index));

  std::unique_lock Lock(Mutex);
  if (!Groups) {
    Groups.emplace();
  }
  if (auto It = Groups->find(Key); It != Groups->end()) {
    return It->second;
  }
  const uint32_t First = Size.load(std::memory_order_relaxed);
  if (N > Cap - First) {
    return Unexpect(ErrCode::Value::TypeTableFull);
  }

  for (uint32_t I = 0; I < N; ++I) {
    EXPECTED_TRY(auto Positions, makeFieldLayout(Group[I]->getCompositeType()));
    const uint32_t Id = First + I;
    auto &Slot = Blocks[Id / kBlockSize];
    auto &LayoutSlot = Layouts[Id / kBlockSize];
    if (Slot == nullptr) {
      Slot = new (std::nothrow) AST::SubType[kBlockSize];
      LayoutSlot = new (std::nothrow) std::vector<uint32_t>[kBlockSize];
      if (Slot == nullptr || LayoutSlot == nullptr) {
        delete[] Slot;
        delete[] LayoutSlot;
        Slot = nullptr;
        LayoutSlot = nullptr;
        return Unexpect(ErrCode::Value::TypeTableFull);
      }
    }
    Slot[Id % kBlockSize] = makeCanonicalType(*Group[I], Index, First, I);
    LayoutSlot[Id % kBlockSize] = std::move(Positions);
  }
  Groups->emplace(std::move(Key), First);
  Size.store(First + N, std::memory_order_release);
  return First;
} catch (const std::bad_alloc &) {
  return Unexpect(ErrCode::Value::TypeTableFull);
}

} // namespace Runtime
} // namespace WasmEdge
