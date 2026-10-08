// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/type_translator.h"
#include "debuginfo/dwarf_reader.h"

#include <llvm/ADT/APInt.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/Support/Path.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;

namespace {

/// The element count of a subrange from Lower to Upper. It is -1 if Upper is
/// below Lower - 1, and nothing if the count does not fit in int64_t.
std::optional<int64_t> subrangeCount(int64_t Lower,
                                     const IntegerConstant &Upper) noexcept {
  const llvm::APInt Low(128, static_cast<uint64_t>(Lower), true);
  const llvm::APInt High =
      Upper.Signed
          ? llvm::APInt(128, static_cast<uint64_t>(Upper.SignedValue), true)
          : llvm::APInt(128, Upper.UnsignedValue);
  const auto Count = High - Low + 1;
  if (Count.isNegative()) {
    return -1;
  }
  if (Count.sgt(INT64_MAX)) {
    return std::nullopt;
  }
  return Count.getSExtValue();
}

/// The distance in bits between the elements that D gives, 0 if D gives
/// none, or std::nullopt if the distance is not a positive constant.
std::optional<uint64_t> strideBits(DWARFDie D) noexcept {
  for (const auto [Attr, Scale] :
       {std::pair{dwarf::DW_AT_byte_stride, UINT64_C(8)},
        std::pair{dwarf::DW_AT_bit_stride, UINT64_C(1)}}) {
    if (auto V = D.find(Attr)) {
      const auto C = integerConstant(*V, false);
      const auto Value = C ? C->toInt64() : std::nullopt;
      if (!Value || *Value <= 0 ||
          static_cast<uint64_t>(*Value) > UINT64_MAX / Scale) {
        return std::nullopt;
      }
      return static_cast<uint64_t>(*Value) * Scale;
    }
  }
  return 0;
}

/// The offset in bits of a DWARF 2 bit field from the start of its parent.
/// The field starts FromHigh bits below the high end of its storage unit,
/// which starts StorageStart bits into the parent. A packed field that
/// crosses its storage unit has a negative FromHigh. The result must not be
/// negative.
std::optional<uint64_t> bitFieldStart(uint64_t StorageStart,
                                      uint64_t StorageBits,
                                      std::optional<int64_t> FromHigh,
                                      uint64_t BitSize) noexcept {
  if (!FromHigh) {
    return std::nullopt;
  }
  const llvm::APInt Start =
      llvm::APInt(128, StorageStart) + llvm::APInt(128, StorageBits) -
      llvm::APInt(128, static_cast<uint64_t>(*FromHigh), true) -
      llvm::APInt(128, BitSize);
  if (Start.isNegative() || Start.ugt(UINT64_MAX)) {
    return std::nullopt;
  }
  return Start.getZExtValue();
}

/// True for a language whose arrays are column-major by default.
bool isColumnMajorLanguage(uint64_t Language) noexcept {
  switch (Language) {
  case dwarf::DW_LANG_Fortran77:
  case dwarf::DW_LANG_Fortran90:
  case dwarf::DW_LANG_Fortran95:
  case dwarf::DW_LANG_Fortran03:
  case dwarf::DW_LANG_Fortran08:
#if LLVM_VERSION_MAJOR < 16
#else
  case dwarf::DW_LANG_Fortran18:
#endif
    return true;
  default:
    return false;
  }
}

uint64_t storageBits(const llvm::DIType *T) noexcept {
  while (const auto *DT = llvm::dyn_cast_or_null<llvm::DIDerivedType>(T)) {
    switch (DT->getTag()) {
    case dwarf::DW_TAG_typedef:
    case dwarf::DW_TAG_const_type:
    case dwarf::DW_TAG_volatile_type:
    case dwarf::DW_TAG_restrict_type:
    case dwarf::DW_TAG_atomic_type:
      T = DT->getBaseType();
      continue;
    default:
      return DT->getSizeInBits();
    }
  }
  return T ? T->getSizeInBits() : 0;
}

std::string typeName(DWARFDie D, uint32_t Depth) {
  if (!D) {
    return "void";
  }
  if (Depth >= 16) {
    return "...";
  }
  switch (D.getTag()) {
  case dwarf::DW_TAG_const_type:
    return typeName(ref(D), Depth + 1) + " const";
  case dwarf::DW_TAG_volatile_type:
    return typeName(ref(D), Depth + 1) + " volatile";
  case dwarf::DW_TAG_restrict_type:
    return typeName(ref(D), Depth + 1) + " restrict";
  case dwarf::DW_TAG_atomic_type:
    return "_Atomic " + typeName(ref(D), Depth + 1);
  case dwarf::DW_TAG_pointer_type:
    return typeName(ref(D), Depth + 1) + " *";
  case dwarf::DW_TAG_reference_type:
    return typeName(ref(D), Depth + 1) + " &";
  case dwarf::DW_TAG_rvalue_reference_type:
    return typeName(ref(D), Depth + 1) + " &&";
  case dwarf::DW_TAG_array_type:
    return typeName(ref(D), Depth + 1) + "[]";
  case dwarf::DW_TAG_subroutine_type:
    return "fn";
  default:
    break;
  }
  std::string Name = name(D).str();
  if (Name.empty()) {
    Name = "(anonymous)";
  }
  for (auto P = D.getParent(); P; P = P.getParent()) {
    const auto Tag = P.getTag();
    if (Tag != dwarf::DW_TAG_namespace && Tag != dwarf::DW_TAG_structure_type &&
        Tag != dwarf::DW_TAG_class_type && Tag != dwarf::DW_TAG_union_type) {
      break;
    }
    const auto Outer = name(P);
    Name = (Outer.empty() ? "(anonymous)"s : Outer.str()) + "::" + Name;
  }
  return Name;
}

} // namespace

llvm::DIFile *fileFor(UnitState &S, llvm::StringRef Path,
                      llvm::StringRef Dir) noexcept {
  if (llvm::sys::path::is_absolute(Path)) {
    return S.DB->createFile(llvm::sys::path::filename(Path),
                            llvm::sys::path::parent_path(Path));
  }
  return S.DB->createFile(Path, Dir);
}

llvm::DIFile *file(UnitState &S, DWARFDie D) noexcept {
  auto Path = declFile(D);
  if (Path.empty()) {
    return S.CU->getFile();
  }
  return fileFor(S, Path, "");
}

llvm::DIType *TypeTranslator::uchar(UnitState &S) noexcept {
  return S.DB->createBasicType("unsigned char", 8, dwarf::DW_ATE_unsigned_char);
}

llvm::DIType *TypeTranslator::uint32(UnitState &S) noexcept {
  return S.DB->createBasicType("uint32_t", 32, dwarf::DW_ATE_unsigned);
}

llvm::DIType *TypeTranslator::type(UnitState &S, DWARFDie D) noexcept {
  if (!D) {
    return nullptr;
  }
  if (auto It = Types.find(dieKey(D)); It != Types.end()) {
    return It->second;
  }
  auto &DB = *S.DB;
  llvm::DIType *T = nullptr;
  switch (D.getTag()) {
  case dwarf::DW_TAG_base_type:
    T = DB.createBasicType(
        name(D), byteSize(D) * 8,
        static_cast<unsigned>(dwarf::toUnsigned(D.find(dwarf::DW_AT_encoding),
                                                dwarf::DW_ATE_unsigned)));
    break;
  case dwarf::DW_TAG_unspecified_type:
    T = DB.createUnspecifiedType(name(D));
    break;
  case dwarf::DW_TAG_typedef:
    T = DB.createTypedef(type(S, ref(D)), name(D), file(S, D), line(D),
                         typeScope(S, D));
    break;
  case dwarf::DW_TAG_const_type:
  case dwarf::DW_TAG_volatile_type:
  case dwarf::DW_TAG_restrict_type:
  case dwarf::DW_TAG_atomic_type:
    T = DB.createQualifiedType(D.getTag(), type(S, ref(D)));
    break;
  case dwarf::DW_TAG_pointer_type:
  case dwarf::DW_TAG_reference_type:
  case dwarf::DW_TAG_rvalue_reference_type:
    T = wasmPointer(S, D);
    break;
  case dwarf::DW_TAG_structure_type:
  case dwarf::DW_TAG_class_type:
  case dwarf::DW_TAG_union_type:
    T = composite(S, D);
    break;
  case dwarf::DW_TAG_enumeration_type:
    T = enumeration(S, D);
    break;
  case dwarf::DW_TAG_array_type:
    T = array(S, D);
    break;
  case dwarf::DW_TAG_subroutine_type:
    T = subroutine(S, D);
    break;
  default:
    T = opaque(S, byteSize(D));
    break;
  }
  Types[dieKey(D)] = T;
  return T;
}

llvm::DIType *TypeTranslator::opaque(UnitState &S, uint64_t Size) noexcept {
  auto &DB = *S.DB;
  return DB.createArrayType(Size * 8, 8, uchar(S),
                            DB.getOrCreateArray({DB.getOrCreateSubrange(
                                0, static_cast<int64_t>(Size))}));
}

llvm::DIType *TypeTranslator::funcRef(UnitState &S) noexcept {
  auto &Cached = PointerTypes[{reinterpret_cast<uintptr_t>(S.CU), FuncRefKey}];
  if (Cached) {
    return Cached;
  }
  auto &DB = *S.DB;
  auto *T = DB.createReplaceableCompositeType(
      dwarf::DW_TAG_structure_type, "__wasm_funcref", S.CU, nullptr, 0, 0, 32,
      32, llvm::DINode::FlagZero);
  auto *Index = DB.createMemberType(T, "__index", nullptr, 0, 32, 32, 0,
                                    llvm::DINode::FlagZero, uint32(S));
  DB.replaceArrays(T, DB.getOrCreateArray({Index}));
  Cached = llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(T));
  return Cached;
}

llvm::DIType *TypeTranslator::wasmPointer(UnitState &S, DWARFDie D) noexcept {
  auto Pointee = ref(D);
  if (auto Target = stripAliases(Pointee);
      Target && Target.getTag() == dwarf::DW_TAG_subroutine_type) {
    return funcRef(S);
  }
  auto &Cached = PointerTypes[{reinterpret_cast<uintptr_t>(S.CU),
                                Pointee ? dieKey(Pointee) : VoidPointeeKey}];
  if (Cached) {
    return Cached;
  }
  auto &DB = *S.DB;
  auto *T = DB.createReplaceableCompositeType(
      dwarf::DW_TAG_structure_type, pointerName(Pointee), S.CU, nullptr, 0, 0,
      32, 32, llvm::DINode::FlagZero);
  Types[dieKey(D)] = T;
  Cached = T;
  llvm::DIType *Elem = Pointee ? type(S, Pointee) : nullptr;
  if (!Elem) {
    Elem = uchar(S);
  }
  auto *Addr = DB.createMemberType(T, "__addr", nullptr, 0, 32, 32, 0,
                                   llvm::DINode::FlagZero, uint32(S));
  auto *Arr = DB.createArrayType(
      0, 0, Elem,
      DB.getOrCreateArray({DB.getOrCreateSubrange(int64_t(0), int64_t(0))}));
  auto *Pt = DB.createMemberType(T, "__pointee", nullptr, 0, uint64_t(0), 0,
                                 uint64_t(0), llvm::DINode::FlagZero, Arr);
  llvm::Metadata *Fields[] = {Addr, Pt};
  DB.replaceArrays(T, DB.getOrCreateArray(Fields));
  return llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(T));
}

std::string TypeTranslator::pointerName(DWARFDie Pointee) {
  std::string Name = "__wasm_ptr<" + typeName(Pointee, 0) + ">";
  const uint64_t Key = Pointee ? dieKey(Pointee) : UINT64_MAX;
  auto [It, Inserted] = PointerNames.emplace(Name, Key);
  if (Inserted || It->second == Key) {
    return Name;
  }
  Name += "#" + std::to_string(Key);
  PointerNames.emplace(Name, Key);
  return Name;
}

llvm::DIScope *TypeTranslator::typeScope(UnitState &S, DWARFDie D) noexcept {
  auto P = D.getParent();
  if (!P) {
    return S.CU;
  }
  switch (P.getTag()) {
  case dwarf::DW_TAG_structure_type:
  case dwarf::DW_TAG_class_type:
  case dwarf::DW_TAG_union_type:
    if (auto *T = llvm::dyn_cast_or_null<llvm::DICompositeType>(type(S, P))) {
      return T;
    }
    return S.CU;
  default:
    return S.CU;
  }
}

llvm::DIDerivedType *TypeTranslator::member(UnitState &S, llvm::DIScope *Scope,
                                            DWARFDie C) noexcept {
  auto &DB = *S.DB;
  auto *MT = type(S, ref(C));
  const auto BitOffset = memberBitOffset(C);
  if (!MT || !BitOffset) {
    return nullptr;
  }
  const uint64_t Off = *BitOffset;
  if (auto BitSize = dwarf::toUnsigned(C.find(dwarf::DW_AT_bit_size))) {
    uint64_t BitOff = Off;
    if (auto Data = dwarf::toUnsigned(C.find(dwarf::DW_AT_data_bit_offset))) {
      BitOff = *Data;
    } else if (auto Attr = C.find(dwarf::DW_AT_bit_offset)) {
      const auto Value = integerConstant(*Attr, false);
      const auto FromHigh = Value ? Value->toInt64() : std::nullopt;
      const uint64_t Storage = dwarf::toUnsigned(C.find(dwarf::DW_AT_byte_size),
                                                 storageBits(MT) / 8) *
                               8;
      const auto Start = bitFieldStart(Off, Storage, FromHigh, *BitSize);
      if (!Start) {
        return nullptr;
      }
      BitOff = *Start;
    }
    return DB.createBitFieldMemberType(Scope, name(C), file(S, C), line(C),
                                       *BitSize, BitOff, 0,
                                       llvm::DINode::FlagZero, MT);
  }
  return DB.createMemberType(Scope, name(C), file(S, C), line(C),
                             storageBits(MT), 0, Off, llvm::DINode::FlagZero,
                             MT);
}

llvm::Metadata *TypeTranslator::variantPart(UnitState &S, llvm::DIScope *Scope,
                                            DWARFDie VP) noexcept {
  auto &DB = *S.DB;
  llvm::DIDerivedType *Discr = nullptr;
  bool DiscrSigned = false;
  if (auto DiscrDie = VP.getAttributeValueAsReferencedDie(dwarf::DW_AT_discr)) {
    Discr = member(S, Scope, DiscrDie);
    DiscrSigned = isSignedType(ref(DiscrDie));
  }
  llvm::SmallVector<llvm::Metadata *, 8> Variants;
  for (auto V : VP.children()) {
    if (V.getTag() != dwarf::DW_TAG_variant) {
      continue;
    }
    if (V.find(dwarf::DW_AT_discr_list)) {
      return nullptr;
    }
    llvm::Constant *Value = nullptr;
    if (auto DV = V.find(dwarf::DW_AT_discr_value)) {
      auto C = integerConstant(*DV, DiscrSigned);
      if (!C || !Discr) {
        return nullptr;
      }
      Value = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Ctx), C->bits(),
                                     C->Signed);
    }
    llvm::SmallVector<DWARFDie, 2> Fields;
    for (auto C : V.children()) {
      if (C.getTag() == dwarf::DW_TAG_member) {
        Fields.push_back(C);
      }
    }
    if (Fields.size() == 1 && !Fields.front().find(dwarf::DW_AT_bit_size)) {
      auto C = Fields.front();
      auto *MT = type(S, ref(C));
      const auto Off = memberBitOffset(C);
      if (!MT || !Off) {
        return nullptr;
      }
      Variants.push_back(DB.createVariantMemberType(
          Scope, name(C), file(S, C), line(C), storageBits(MT), 0, *Off, Value,
          llvm::DINode::FlagZero, MT));
      continue;
    }
    const uint64_t Size = byteSize(VP.getParent()) * 8;
    llvm::TempDICompositeType Group(DB.createReplaceableCompositeType(
        dwarf::DW_TAG_structure_type, "", Scope, nullptr, 0, 0, Size, 0,
        llvm::DINode::FlagArtificial));
    llvm::SmallVector<llvm::Metadata *, 4> Members;
    for (auto C : Fields) {
      auto *E = member(S, Group.get(), C);
      if (!E) {
        return nullptr;
      }
      Members.push_back(E);
    }
    auto *Fill = Group.get();
    DB.replaceArrays(Fill, DB.getOrCreateArray(Members));
    auto *Final = llvm::MDNode::replaceWithDistinct(std::move(Group));
    Variants.push_back(
        DB.createVariantMemberType(Scope, "", nullptr, 0, Size, 0, 0, Value,
                                   llvm::DINode::FlagArtificial, Final));
  }
  return DB.createVariantPart(Scope, "", nullptr, 0, 0, 0,
                              llvm::DINode::FlagZero, Discr,
                              DB.getOrCreateArray(Variants));
}

llvm::DIType *TypeTranslator::composite(UnitState &S, DWARFDie D) noexcept {
  auto &DB = *S.DB;
  const bool Decl = dwarf::toUnsigned(D.find(dwarf::DW_AT_declaration), 0);
  auto *T = DB.createReplaceableCompositeType(
      D.getTag(), name(D), typeScope(S, D), file(S, D), line(D), 0,
      byteSize(D) * 8, 0,
      Decl ? llvm::DINode::FlagFwdDecl : llvm::DINode::FlagZero);
  Types[dieKey(D)] = T;
  llvm::SmallVector<llvm::Metadata *, 16> Members;
  for (auto C : D.children()) {
    llvm::Metadata *E = nullptr;
    switch (C.getTag()) {
    case dwarf::DW_TAG_member:
      E = member(S, T, C);
      break;
    case dwarf::DW_TAG_inheritance:
      if (dwarf::toUnsigned(C.find(dwarf::DW_AT_virtuality),
                            dwarf::DW_VIRTUALITY_none) !=
          dwarf::DW_VIRTUALITY_none) {
        break;
      }
      if (const auto Off = memberOffset(C)) {
        if (auto *Base = type(S, ref(C))) {
          E = DB.createInheritance(T, Base, *Off * 8, 0,
                                   llvm::DINode::FlagZero);
        }
      }
      break;
    case dwarf::DW_TAG_variant_part:
      E = variantPart(S, T, C);
      break;
    default:
      break;
    }
    if (E) {
      Members.push_back(E);
    }
  }
  DB.replaceArrays(T, DB.getOrCreateArray(Members));
  auto *Final = llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(T));
  Types[dieKey(D)] = Final;
  return Final;
}

llvm::DIType *TypeTranslator::enumeration(UnitState &S, DWARFDie D) noexcept {
  auto &DB = *S.DB;
  const bool Signed = isSignedType(ref(D));
  llvm::SmallVector<llvm::Metadata *, 16> Items;
  for (auto C : D.children()) {
    if (C.getTag() != dwarf::DW_TAG_enumerator) {
      continue;
    }
    auto V = C.find(dwarf::DW_AT_const_value);
    if (!V) {
      continue;
    }
    if (auto Value = integerConstant(*V, Signed)) {
      Items.push_back(
          DB.createEnumerator(name(C), Value->bits(), !Value->Signed));
    }
  }
  const bool Scoped = dwarf::toUnsigned(D.find(dwarf::DW_AT_enum_class), 0);
  return DB.createEnumerationType(typeScope(S, D), name(D), file(S, D), line(D),
                                  byteSize(D) * 8, 0,
                                  DB.getOrCreateArray(Items), type(S, ref(D)),
#if LLVM_VERSION_MAJOR < 18
#else
                                  0,
#endif
                                  "", Scoped);
}

llvm::DIType *TypeTranslator::array(UnitState &S, DWARFDie D) noexcept {
  auto &DB = *S.DB;
  auto *Elem = type(S, ref(D));
  if (!Elem) {
    Elem = uchar(S);
  }
  const auto Language = dwarf::toUnsigned(
      D.getDwarfUnit()->getUnitDIE().find(dwarf::DW_AT_language), 0);
  const bool DefaultColumnMajor = isColumnMajorLanguage(Language);
  const auto Ordering = dwarf::toUnsigned(D.find(dwarf::DW_AT_ordering));
  if (Ordering && *Ordering != dwarf::DW_ORD_row_major &&
      *Ordering != dwarf::DW_ORD_col_major) {
    return opaque(S, byteSize(D));
  }
  const bool ColumnMajor =
      Ordering ? *Ordering == dwarf::DW_ORD_col_major : DefaultColumnMajor;
  const uint64_t ElemBits = storageBits(Elem);
  if (const auto Stride = strideBits(D);
      !Stride || (*Stride != 0 && *Stride != ElemBits)) {
    return opaque(S, byteSize(D));
  }
  llvm::SmallVector<llvm::Metadata *, 4> Subs;
  llvm::SmallVector<std::pair<int64_t, uint64_t>, 4> Dimensions;
  for (auto C : D.children()) {
    if (C.getTag() != dwarf::DW_TAG_subrange_type) {
      continue;
    }
    const auto Stride = strideBits(C);
    if (!Stride) {
      return opaque(S, byteSize(D));
    }
    const bool Signed = isSignedType(ref(C));
    int64_t Lower = defaultLowerBound(Language);
    if (auto LowerForm = C.find(dwarf::DW_AT_lower_bound)) {
      const auto L = integerConstant(*LowerForm, Signed);
      const auto Value = L ? L->toInt64() : std::nullopt;
      if (!Value) {
        return opaque(S, byteSize(D));
      }
      Lower = *Value;
    }
    std::optional<int64_t> Count = -1;
    if (auto CountForm = C.find(dwarf::DW_AT_count)) {
      if (auto N = integerConstant(*CountForm, false)) {
        Count = N->toInt64();
        if (Count && *Count < 0) {
          Count.reset();
        }
      }
    } else if (auto UpperForm = C.find(dwarf::DW_AT_upper_bound)) {
      if (auto U = integerConstant(*UpperForm, Signed)) {
        Count = subrangeCount(Lower, *U);
      }
    }
    if (!Count) {
      return opaque(S, byteSize(D));
    }
    Subs.push_back(DB.getOrCreateSubrange(Lower, *Count));
    Dimensions.emplace_back(*Count, *Stride);
  }
  if (ColumnMajor != DefaultColumnMajor && Dimensions.size() > 1) {
    return opaque(S, byteSize(D));
  }
  if (!ColumnMajor) {
    std::reverse(Dimensions.begin(), Dimensions.end());
  }
  std::optional<uint64_t> Contiguous = ElemBits;
  for (const auto &[Count, Stride] : Dimensions) {
    if (Stride != 0 && Stride != Contiguous) {
      return opaque(S, byteSize(D));
    }
    if (!Contiguous || Count < 0 ||
        (Count > 0 &&
         *Contiguous > UINT64_MAX / static_cast<uint64_t>(Count))) {
      Contiguous.reset();
    } else {
      *Contiguous *= static_cast<uint64_t>(Count);
    }
  }
  uint64_t SizeBits = byteSize(D) * 8;
  if (SizeBits == 0 && ElemBits != 0 && Contiguous) {
    SizeBits = *Contiguous;
  }
  return DB.createArrayType(SizeBits, 0, Elem, DB.getOrCreateArray(Subs));
}

llvm::DISubroutineType *TypeTranslator::subroutine(UnitState &S,
                                                   DWARFDie D) noexcept {
  llvm::SmallVector<llvm::Metadata *, 8> Sig;
  Sig.push_back(type(S, ref(D)));
  bool Variadic = false;
  for (auto C : D.children()) {
    if (C.getTag() == dwarf::DW_TAG_formal_parameter) {
      auto *T = type(S, ref(C));
      if (T && isObjectPointer(D, C)) {
#if LLVM_VERSION_MAJOR < 20
        T = llvm::DIBuilder::createObjectPointerType(T);
#else
        T = llvm::DIBuilder::createObjectPointerType(T, true);
#endif
      } else if (T && dwarf::toUnsigned(
                          C.findRecursively(dwarf::DW_AT_artificial), 0)) {
        T = S.DB->createArtificialType(T);
      }
      Sig.push_back(T);
    } else if (C.getTag() == dwarf::DW_TAG_unspecified_parameters) {
      Variadic = true;
    }
  }
  if (Variadic) {
    Sig.push_back(nullptr);
  }
  return S.DB->createSubroutineType(S.DB->getOrCreateTypeArray(Sig));
}

} // namespace WasmEdge::LLVM::DebugInfo
