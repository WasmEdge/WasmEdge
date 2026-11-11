// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "runtime/typetable.h"

#include "common/hash.h"

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
    : Cap(C), Blocks(std::make_unique<AST::SubType *[]>(
                  (static_cast<uint64_t>(C) + kBlockSize - 1) / kBlockSize)) {}

CanonicalTypeTable::~CanonicalTypeTable() noexcept {
  const uint64_t Count =
      (static_cast<uint64_t>(Cap) + kBlockSize - 1) / kBlockSize;
  for (uint64_t I = 0; I < Count; ++I) {
    delete[] Blocks[I];
  }
}

CanonicalTypeTable &CanonicalTypeTable::getGlobal() noexcept {
  static CanonicalTypeTable *Table = new CanonicalTypeTable();
  return *Table;
}

size_t CanonicalTypeTable::KeyHash::operator()(
    const std::vector<uint64_t> &Key) const noexcept {
  return static_cast<size_t>(Hash::Hash::rapidHash(
      cxx20::as_bytes(Span<const uint64_t>(Key.data(), Key.size()))));
}

Expect<uint32_t> CanonicalTypeTable::registerRecGroup(
    Span<const AST::SubType *const> Group, uint32_t Start,
    Span<const uint32_t> ModuleIds) noexcept try {
  const auto N = static_cast<uint32_t>(Group.size());
  assuming(N > 0);
  const uint32_t Known = size();
  auto Resolve = [&](uint32_t Idx) -> std::optional<uint64_t> {
    if (Idx >= Start && Idx - Start < N) {
      return kInGroup | (Idx - Start);
    }
    if (Idx < Start && Idx < ModuleIds.size() && ModuleIds[Idx] < Known) {
      return ModuleIds[Idx];
    }
    return std::nullopt;
  };

  std::vector<uint64_t> Key;
  Key.push_back(N);
  auto AppendValType = [&](const ValType &VT) -> bool {
    Key.push_back(encodeTypeCodes(VT));
    if (VT.isRefType() && !VT.isAbsHeapType()) {
      const auto Token = Resolve(VT.getTypeIndex());
      if (!Token.has_value()) {
        return false;
      }
      Key.push_back(*Token);
    }
    return true;
  };
  for (uint32_t I = 0; I < N; ++I) {
    const auto &Type = *Group[I];
    Key.push_back(Type.isFinal() ? 1U : 0U);
    const auto Supers = Type.getSuperTypeIndices();
    Key.push_back(Supers.size());
    for (const auto Super : Supers) {
      const auto Token = Resolve(Super);
      if (!Token.has_value() ||
          ((*Token & kInGroup) && (*Token & ~kInGroup) >= I)) {
        return Unexpect(ErrCode::Value::IncompatibleImportType);
      }
      Key.push_back(*Token);
    }
    const auto &CompType = Type.getCompositeType();
    Key.push_back(static_cast<uint64_t>(CompType.getContentTypeCode()));
    if (CompType.isFunc()) {
      const auto &FuncType = CompType.getFuncType();
      Key.push_back(FuncType.getParamTypes().size());
      for (const auto &VT : FuncType.getParamTypes()) {
        if (!AppendValType(VT)) {
          return Unexpect(ErrCode::Value::IncompatibleImportType);
        }
      }
      Key.push_back(FuncType.getReturnTypes().size());
      for (const auto &VT : FuncType.getReturnTypes()) {
        if (!AppendValType(VT)) {
          return Unexpect(ErrCode::Value::IncompatibleImportType);
        }
      }
    } else {
      Key.push_back(CompType.getFieldTypes().size());
      for (const auto &Field : CompType.getFieldTypes()) {
        Key.push_back(static_cast<uint64_t>(Field.getValMut()));
        if (!AppendValType(Field.getStorageType())) {
          return Unexpect(ErrCode::Value::IncompatibleImportType);
        }
      }
    }
  }

  std::unique_lock Lock(Mutex);
  if (auto It = Groups.find(Key); It != Groups.end()) {
    return It->second;
  }
  const uint32_t First = Size.load(std::memory_order_relaxed);
  if (N > Cap - First) {
    return Unexpect(ErrCode::Value::TypeTableFull);
  }

  auto ToId = [&](uint32_t Idx) -> uint32_t {
    const uint64_t Token = *Resolve(Idx);
    return (Token & kInGroup) ? First + static_cast<uint32_t>(Token & ~kInGroup)
                              : static_cast<uint32_t>(Token);
  };
  auto MapValType = [&](const ValType &VT) -> ValType {
    if (VT.isRefType() && !VT.isAbsHeapType()) {
      return ValType(VT.getCode(), ToId(VT.getTypeIndex()));
    }
    return VT;
  };
  for (uint32_t I = 0; I < N; ++I) {
    const auto &Type = *Group[I];
    AST::SubType Copy;
    Copy.setFinal(Type.isFinal());
    for (const auto Super : Type.getSuperTypeIndices()) {
      Copy.getSuperTypeIndices().push_back(ToId(Super));
    }
    const auto &CompType = Type.getCompositeType();
    switch (CompType.getContentTypeCode()) {
    case TypeCode::Func: {
      AST::FunctionType FuncType;
      for (const auto &VT : CompType.getFuncType().getParamTypes()) {
        FuncType.getParamTypes().push_back(MapValType(VT));
      }
      for (const auto &VT : CompType.getFuncType().getReturnTypes()) {
        FuncType.getReturnTypes().push_back(MapValType(VT));
      }
      Copy.getCompositeType().setFunctionType(std::move(FuncType));
      break;
    }
    case TypeCode::Struct: {
      std::vector<AST::FieldType> Fields;
      for (const auto &Field : CompType.getFieldTypes()) {
        Fields.emplace_back(MapValType(Field.getStorageType()),
                            Field.getValMut());
      }
      Copy.getCompositeType().setStructType(std::move(Fields));
      break;
    }
    case TypeCode::Array: {
      const auto &Field = CompType.getFieldTypes()[0];
      Copy.getCompositeType().setArrayType(AST::FieldType(
          MapValType(Field.getStorageType()), Field.getValMut()));
      break;
    }
    default:
      assumingUnreachable();
    }
    Copy.setRecursiveInfo(I, N);
    Copy.setTypeIndex(First + I);

    const uint32_t Id = First + I;
    auto &Slot = Blocks[Id / kBlockSize];
    if (Slot == nullptr) {
      Slot = new (std::nothrow) AST::SubType[kBlockSize];
      if (Slot == nullptr) {
        return Unexpect(ErrCode::Value::TypeTableFull);
      }
    }
    Slot[Id % kBlockSize] = std::move(Copy);
  }
  Groups.emplace(std::move(Key), First);
  Size.store(First + N, std::memory_order_release);
  return First;
} catch (const std::bad_alloc &) {
  return Unexpect(ErrCode::Value::TypeTableFull);
}

} // namespace Runtime
} // namespace WasmEdge
