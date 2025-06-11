// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/type.h - Type class definitions ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Type node related classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/alias.h"
#include "ast/description.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// For easily understanding the type definitions in component model proposal,
// this header file defines the classes in bottom-up and simply separates the
// types into several parts.
//   1. DefValType: one of DefType.
//   2. FuncType: one of DefType.
//   3. Core::Type: all definitions of core::type.
//   4. InstanceType: one of DefType.
//   5. ComponentType: one of DefType.
//   6. ResourceType: one of DefType.
//   7. Type: union of DefType.
// However, this header should be refactored for preventing from recursively
// referring in the future.

// =============================================================================
// Part 1: DefValType and the child types definitions.
// =============================================================================

// primvaltype ::= 0x7f => bool
//               | 0x7e => s8
//               | 0x7d => u8
//               | 0x7c => s16
//               | 0x7b => u16
//               | 0x7a => s32
//               | 0x79 => u32
//               | 0x78 => s64
//               | 0x77 => u64
//               | 0x76 => f32
//               | 0x75 => f64
//               | 0x74 => char
//               | 0x73 => string
//               | 0x64 => error-context 📝

/// AST Component::PrimValType enum. (One type of DefValType)
enum class PrimValType : Byte {
  Bool = 0x7f,
  S8 = 0x7e,
  U8 = 0x7d,
  S16 = 0x7c,
  U16 = 0x7b,
  S32 = 0x7a,
  U32 = 0x79,
  S64 = 0x78,
  U64 = 0x77,
  F32 = 0x76,
  F64 = 0x75,
  Char = 0x74,
  String = 0x73,
  ErrorContext = 0x64,
};

// valtype ::= i:<typeidx>       => i
//           | pvt:<primvaltype> => pvt

/// AST Component::ValueType aliasing.
using ValueType = std::variant<uint32_t, PrimValType>;

// labelvaltype ::= l:<label'> t:<valtype> => l t
// label'       ::= len:<u32> l:<label>    => l    (if len = |l|)

/// AST Component::LabelValType node.
class LabelValType {
public:
  LabelValType() noexcept = default;
  LabelValType(const std::string &L, ValueType VT) noexcept
      : Label(L), ValTy(VT) {}

  std::string_view getLabel() const noexcept { return Label; }
  std::string &getLabel() noexcept { return Label; }
  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  std::string Label;
  ValueType ValTy;
};

// record ::= lt*:vec(<labelvaltype>) => (record (field lt)*) (if |lt*| > 0)

/// AST Component::RecordTy node. (One type of DefValType)
class RecordTy {
public:
  RecordTy() noexcept = default;
  RecordTy(std::initializer_list<LabelValType> I) noexcept : LabelTypes(I) {}

  Span<const LabelValType> getLabelTypes() const noexcept { return LabelTypes; }
  std::vector<LabelValType> &getLabelTypes() noexcept { return LabelTypes; }

private:
  std::vector<LabelValType> LabelTypes;
};

// case ::= l:<label'> t?:<valtype>? 0x00 => (case l t?)
// <T>? ::= 0x00                          => ϵ
//        | 0x01 t:<T>                    => t

/// AST Component::Case node.
class Case {
public:
  Case() noexcept = default;
  Case(const std::string &L) noexcept : Label(L), ValTy(std::nullopt) {}
  Case(const std::string &L, ValueType VT) noexcept : Label(L), ValTy(VT) {}

  std::string_view getLabel() const noexcept { return Label; }
  std::string &getLabel() noexcept { return Label; }
  const std::optional<ValueType> getValType() const noexcept { return ValTy; }
  std::optional<ValueType> &getValType() noexcept { return ValTy; }

private:
  std::string Label;
  std::optional<ValueType> ValTy;
};

// variant ::= case*:vec(<case>) => (variant case+) (if |case*| > 0)

/// AST Component::VariantTy node. (One type of DefValType)
class VariantTy {
public:
  VariantTy() noexcept = default;
  VariantTy(std::initializer_list<Case> Cs) noexcept : Cases(Cs) {}

  Span<const Case> getCases() const noexcept { return Cases; }
  std::vector<Case> &getCases() noexcept { return Cases; }

private:
  std::vector<Case> Cases;
};

// list ::= t:<valtype> => (list t)

// TODO: COMPONENT - lack of fixed length list type:
// list ::= t:<valtype> len:<u32> => (list t len) (if len > 0) 🔧

/// AST Component::ListTy node. (One type of DefValType)
class ListTy {
public:
  ListTy() noexcept = default;
  ListTy(const ValueType &T) : ValTy(T) {}

  const ValueType &getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  ValueType ValTy;
};

// tuple ::= t*:vec(<valtype>) => (tuple t+) (if |t*| > 0)

/// AST Component::TupleTy node. (One type of DefValType)
class TupleTy {
  // A tuple is the product of given non-empty type list
  // e.g. given [A, B, C], the tuple is a product A x B x C
public:
  Span<const ValueType> getTypes() const noexcept { return Types; }
  std::vector<ValueType> &getTypes() noexcept { return Types; }

private:
  std::vector<ValueType> Types;
};

// flags ::= l*:vec(<label'>) => (flags l+) (if 0 < |l*| <= 32)

/// AST Component::FlagsTy node. (One type of DefValType)
class FlagsTy {
public:
  Span<const std::string> getLabels() const noexcept { return Labels; }
  std::vector<std::string> &getLabels() noexcept { return Labels; }

private:
  std::vector<std::string> Labels;
};

// enum ::= l*:vec(<label'>) => (enum l+) (if |l*| > 0)

/// AST Component::EnumTy node. (One type of DefValType)
class EnumTy {
public:
  Span<const std::string> getLabels() const noexcept { return Labels; }
  std::vector<std::string> &getLabels() noexcept { return Labels; }

private:
  std::vector<std::string> Labels;
};

// option ::= t:<valtype> => (option t)

/// AST Component::OptionTy node. (One type of DefValType)
class OptionTy {
public:
  OptionTy() noexcept = default;
  OptionTy(const ValueType &T) noexcept : ValTy(T) {}

  const ValueType &getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  ValueType ValTy;
};

// result ::= t?:<valtype>? u?:<valtype>? => (result t? (error u)?)

/// AST Component::ResultTy node. (One type of DefValType)
class ResultTy {
public:
  const std::optional<ValueType> getValType() const noexcept { return ValTy; }
  std::optional<ValueType> &getValType() noexcept { return ValTy; }

  const std::optional<ValueType> getErrorType() const noexcept { return ErrTy; }
  std::optional<ValueType> &getErrorType() noexcept { return ErrTy; }

private:
  std::optional<ValueType> ValTy;
  std::optional<ValueType> ErrTy;
};

// own ::= i:<typeidx> => (own i)

/// AST Component::OwnTy node. (One type of DefValType)
class OwnTy {
public:
  uint32_t getIndex() const noexcept { return Idx; }
  uint32_t &getIndex() noexcept { return Idx; }

private:
  uint32_t Idx;
};

// borrow ::= i:<typeidx> => (borrow i)

/// AST Component::BorrowTy node. (One type of DefValType)
class BorrowTy {
public:
  uint32_t getIndex() const noexcept { return Idx; }
  uint32_t &getIndex() noexcept { return Idx; }

private:
  uint32_t Idx;
};

// TODO: COMPONENT - StreamTy and FutureTy
// TODO: COMPONENT - Refactor the DefValType into a class

// defvaltype ::= pvt:<primvaltype>          => pvt
//              | 0x72 lt*:vec(<labelvaltype>)
//                => (record (field lt)*) (if |lt*| > 0)
//              | 0x71 case*:vec(<case>) => (variant case+) (if |case*| > 0)/
//              | 0x70 t:<valtype>           => (list t)
//              | 0x67 t:<valtype> len:<u32> => (list t len) (if len > 0) 🔧
//              | 0x6f t*:vec(<valtype>)     => (tuple t+) (if |t*| > 0)
//              | 0x6e l*:vec(<label'>)      => (flags l+) (if 0 < |l*| <= 32)
//              | 0x6d l*:vec(<label'>)      => (enum l+) (if |l*| > 0)
//              | 0x6b t:<valtype>           => (option t)
//              | 0x6a t?:<valtype>? u?:<valtype>? => (result t? (error u)?)
//              | 0x69 i:<typeidx>           => (own i)
//              | 0x68 i:<typeidx>           => (borrow i)
//              | 0x66 t?:<valtype>?         => (stream t?) 🔀
//              | 0x65 t?:<valtype>?         => (future t?) 🔀

/// AST Component::DefValType aliasing.
using DefValType =
    std::variant<PrimValType, RecordTy, VariantTy, ListTy, TupleTy, FlagsTy,
                 EnumTy, OptionTy, ResultTy, OwnTy, BorrowTy>;

// =============================================================================
// Part 2: FuncType and the child types definitions.
// =============================================================================

// resultlist ::= 0x00 t:<valtype> => (result t)
//              | 0x01 0x00        => ϵ

/// FROM:
/// https://github.com/WebAssembly/component-model/blob/main/design/mvp/CanonicalABI.md#flattening
///
/// The number of flattened results is currently limited to 1 due to various
/// parts of the toolchain (notably the C ABI) not yet being able to express
/// multi-value returns. Hopefully this limitation is temporary and can be
/// lifted before the Component Model is fully standardized.
///
/// NOTE:
/// The original resultlist grammar:
///
/// resultlist ::= 0x00 t:<valtype>             => (result t)
///              | 0x01 lt*:vec(<labelvaltype>) => (result lt)*

// AST Component::ResultList aliasing.
using ResultList = std::variant<ValueType, std::vector<LabelValType>>;

// functype  ::= 0x40 ps:<paramlist> rs:<resultlist> => (func ps rs)
// paramlist ::= lt*:vec(<labelvaltype>)             => (param lt)*

/// AST Component::FuncType node.
class FuncType {
public:
  FuncType() noexcept = default;
  FuncType(const std::vector<LabelValType> &P, ResultList R) noexcept
      : ParamList(P), ResList(R) {}

  Span<const LabelValType> getParamList() const noexcept { return ParamList; }
  std::vector<LabelValType> &getParamList() noexcept { return ParamList; }
  ResultList getResultList() const noexcept { return ResList; }
  ResultList &getResultList() noexcept { return ResList; }

private:
  std::vector<LabelValType> ParamList;
  ResultList ResList;
};

/// AST Component::FunctionType node. (For interface types)
class FunctionType {
public:
  FunctionType() noexcept = default;
  FunctionType(Span<const InterfaceType> P,
               Span<const InterfaceType> R) noexcept
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()) {}

  friend bool operator==(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return LHS.ParamTypes == RHS.ParamTypes &&
           LHS.ReturnTypes == RHS.ReturnTypes;
  }
  friend bool operator!=(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return !(LHS == RHS);
  }

  const std::vector<InterfaceType> &getParamTypes() const noexcept {
    return ParamTypes;
  }
  std::vector<InterfaceType> &getParamTypes() noexcept { return ParamTypes; }

  const std::vector<InterfaceType> &getReturnTypes() const noexcept {
    return ReturnTypes;
  }
  std::vector<InterfaceType> &getReturnTypes() noexcept { return ReturnTypes; }

private:
  std::vector<InterfaceType> ParamTypes;
  std::vector<InterfaceType> ReturnTypes;
};

// =============================================================================
// Part 3: Core::Type and the child types definitions.
// =============================================================================

// core:exportdecl ::= n:<core:name> d:<core:importdesc> => (export n d)

/// AST Component::CoreExportDecl node.
class CoreExportDecl {
public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  ImportDesc getImportDesc() const noexcept { return Desc; }
  ImportDesc &getImportDesc() noexcept { return Desc; }

private:
  std::string Name;
  ImportDesc Desc;
};

// core:moduledecl ::= 0x00 i:<core:import>     => i
//                   | 0x01 t:<core:type>       => t
//                   | 0x02 a:<core:alias>      => a
//                   | 0x03 e:<core:exportdecl> => e

class CoreType;
/// AST Component::ModuleDecl aliasing.
using CoreModuleDecl =
    std::variant<ImportDesc, std::shared_ptr<CoreType>, Alias, CoreExportDecl>;

// core:moduletype ::= 0x50 md*:vec(<core:moduledecl>) => (module md*)

/// AST Component::CoreModuleType node.
class CoreModuleType {
public:
  Span<const CoreModuleDecl> getContent() const noexcept { return Decls; }
  std::vector<CoreModuleDecl> &getContent() noexcept { return Decls; }

private:
  std::vector<CoreModuleDecl> Decls;
};

/// FROM:
/// https://github.com/WebAssembly/component-model/blob/main/design/mvp/Binary.md#type-definitions
///
/// Unfortunately, the `core:deftype` rule results in an encoding ambiguity: the
/// `0x50` opcode is used by both `core:moduletype` and a non-final
/// `core:subtype`, which can be decoded as a top-level form of `core:rectype`.
///
/// To resolve this, prior to v1.0 of this specification, we require
/// `core:subtype` to be prefixed by `0x00` in this context (i.e., a non-final
/// sub as a component core type is `0x00 0x50`; elsewhere, `0x50`). By the v1.0
/// release of this specification, `core:moduletype` will receive a new,
/// non-overlapping opcode.

// core:deftype ::= rt:<core:rectype>
//                => rt (WebAssembly 3.0)
//                | 0x00 0x50 x*:vec(<core:typeidx>) ct:<core:comptype>
//                => sub x* ct (WebAssembly 3.0)
//                | mt:<core:moduletype>
//                => mt

// TODO: COMPONENT - Apply the GC proposal: use AST::SubType instead.
/// AST Component::CoreDefType aliasing.
using CoreDefType = std::variant<WasmEdge::AST::FunctionType, CoreModuleType>;

// core:type ::= dt:<core:deftype> => (type dt)

/// AST Component::CoreType node.
class CoreType {
public:
  CoreDefType getType() const noexcept { return T; }
  CoreDefType &getType() noexcept { return T; }

private:
  CoreDefType T;
};

// =============================================================================
// Part 4: InstanceType and the child types definitions.
// =============================================================================

// externdesc ::= 0x00 0x11 i:<core:typeidx> => (core module (type i))
//              | 0x01 i:<typeidx>           => (func (type i))
//              | 0x02 b:<valuebound>        => (value b) 🪙
//              | 0x03 b:<typebound>         => (type b)
//              | 0x04 i:<typeidx>           => (component (type i))
//              | 0x05 i:<typeidx>           => (instance (type i))
// valuebound ::= 0x00 i:<valueidx>          => (eq i) 🪙
//              | 0x01 t:<valtype>           => t 🪙
// typebound  ::= 0x00 i:<typeidx>           => (eq i)
//              | 0x01                       => (sub resource)

/// AST Component::IndexKind enum. (For DescTypeIndex using)
enum class IndexKind : Byte {
  CoreType = 0x00,
  FuncType = 0x01,
  ComponentType = 0x04,
  InstanceType = 0x05,
};

/// AST Component::DescTypeIndex node. (For ExternDesc using)
class DescTypeIndex {
public:
  uint32_t &getIndex() noexcept { return TyIdx; }
  uint32_t getIndex() const noexcept { return TyIdx; }
  IndexKind &getKind() noexcept { return Kind; }
  IndexKind getKind() const noexcept { return Kind; }

private:
  uint32_t TyIdx;
  IndexKind Kind;
};

/// FROM:
/// https://github.com/WebAssembly/component-model/blob/main/design/mvp/Explainer.md#type-checking
///
/// When we next consider type imports and exports, there are two distinct
/// subcases of typebound to consider: eq and sub.
///
/// The eq bound adds a type equality rule (extending the built-in set of
/// subtyping rules) saying that the imported type is structurally equivalent to
/// the type referenced in the bound.
///
/// In contrast, the sub bound introduces a new abstract type which the rest of
/// the component must conservatively assume can be any type that is a subtype
/// of the bound. What this means for type-checking is that each subtype-bound
/// type import/export introduces a fresh abstract type that is unequal to every
/// preceding type definition.
///
/// NOTE:
/// One just need to consider Java's `? extends T` in mind.
///
/// 1. optional `some i` as `(eq i)`
/// 2. optional `none` as `sub`, i.e. Subresource

/// AST Component::TypeBound aliasing.
using TypeBound = std::optional<uint32_t>;
/// AST Component::ExternDesc aliasing.
using ExternDesc = std::variant<DescTypeIndex, TypeBound, ValueType>;

// TODO: COMPONENT - Refactor the entire ExportDesc structure.

// exportdecl  ::= en:<exportname'> ed:<externdesc> => (export en ed)
// exportname' ::= 0x00 len:<u32> en:<exportname>   => en (if len = |en|)

/// AST Component::ExportDecl node.
class ExportDecl {
public:
  std::string_view getExportName() const noexcept { return ExportName; }
  std::string &getExportName() noexcept { return ExportName; }
  ExternDesc getExternDesc() const noexcept { return Desc; }
  ExternDesc &getExternDesc() noexcept { return Desc; }

private:
  std::string ExportName;
  ExternDesc Desc;
};

// instancedecl ::= 0x00 t:<core:type>   => t
//                | 0x01 t:<type>        => t
//                | 0x02 a:<alias>       => a
//                | 0x04 ed:<exportdecl> => ed

class Type;
/// AST Component::InstanceDecl aliasing.
using InstanceDecl =
    std::variant<CoreType, Alias, std::shared_ptr<Type>, ExportDecl>;

// instancetype ::= 0x42 id*:vec(<instancedecl>) => (instance id*)

/// AST Component::InstanceType node.
class InstanceType {
  std::vector<InstanceDecl> IdList;

public:
  Span<const InstanceDecl> getContent() const noexcept { return IdList; }
  std::vector<InstanceDecl> &getContent() noexcept { return IdList; }
};

// =============================================================================
// Part 5: ComponentType and the child types definitions.
// =============================================================================

// importdecl  ::= in:<importname'> ed:<externdesc> => (import in ed)
// importname' ::= 0x00 len:<u32> in:<importname>   => in (if len = |in|)

/// AST Component::ImportDecl node.
class ImportDecl {
public:
  std::string_view getImportName() const noexcept { return ImportName; }
  std::string &getImportName() noexcept { return ImportName; }
  ExternDesc getExternDesc() const noexcept { return Desc; }
  ExternDesc &getExternDesc() noexcept { return Desc; }

private:
  std::string ImportName;
  ExternDesc Desc;
};

// componentdecl ::= 0x03 id:<importdecl> => id
//                 | id:<instancedecl>    => id

/// AST Component::ComponentDecl aliasing.
using ComponentDecl = std::variant<ImportDecl, InstanceDecl>;

// componenttype ::= 0x41 cd*:vec(<componentdecl>) => (component cd*)

/// AST Component::ComponentType node.
class ComponentType {
public:
  Span<const ComponentDecl> getContent() const noexcept { return CdList; }
  std::vector<ComponentDecl> &getContent() noexcept { return CdList; }

private:
  std::vector<ComponentDecl> CdList;
};

// =============================================================================
// Part 6: ResourceType and the child types definitions.
// =============================================================================

// resourcetype ::= 0x3f 0x7f f?:<funcidx>?
//                => (resource (rep i32) (dtor f)?)
//                | 0x3e 0x7f f:<funcidx> cb?:<funcidx>?
//                => (resource (rep i32) (dtor async f (callback cb)?))

/// AST Component::ResourceType node.
class ResourceType {
public:
  ResourceType() noexcept : DtorSync(true) {}
  ResourceType(bool Sync) noexcept : DtorSync(Sync) {}

  std::optional<uint32_t> getDestructor() const noexcept { return Dtor; }
  std::optional<uint32_t> getCallback() const noexcept { return DtorCallback; }

  bool IsSync() noexcept { return DtorSync; }
  std::optional<uint32_t> &getDestructor() noexcept { return Dtor; }
  std::optional<uint32_t> &getCallback() noexcept { return DtorCallback; }

private:
  // Destructor is sync or not. True for sync, false for async.
  bool DtorSync;
  // Destructor function index.
  std::optional<uint32_t> Dtor;
  // Destructor callback function index.
  std::optional<uint32_t> DtorCallback;
};

// =============================================================================
// Part 7: Type definition.
// =============================================================================

// deftype ::= dvt:<defvaltype>   => dvt
//           | ft:<functype>      => ft
//           | ct:<componenttype> => ct
//           | it:<instancetype>  => it
//           | rt:<resourcetype>  => rt

/// AST Component::DefType aliasing.
using DefType = std::variant<DefValType, FuncType, ComponentType, InstanceType,
                             ResourceType>;

// type ::= dt:<deftype> => (type dt)

/// AST Component::Type node.
class Type {
public:
  Type(const DefType &V) : T(V) {}
  DefType getType() const noexcept { return T; }
  DefType &getType() noexcept { return T; }

private:
  DefType T;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge

template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

template <>
struct fmt::formatter<WasmEdge::AST::Component::PrimValType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::PrimValType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;
    switch (Type) {
    case PrimValType::Bool:
      return formatter<std::string_view>::format("bool"sv, Ctx);
    case PrimValType::S8:
      return formatter<std::string_view>::format("s8"sv, Ctx);
    case PrimValType::U8:
      return formatter<std::string_view>::format("u8"sv, Ctx);
    case PrimValType::S16:
      return formatter<std::string_view>::format("s16"sv, Ctx);
    case PrimValType::U16:
      return formatter<std::string_view>::format("u16"sv, Ctx);
    case PrimValType::S32:
      return formatter<std::string_view>::format("s32"sv, Ctx);
    case PrimValType::U32:
      return formatter<std::string_view>::format("u32"sv, Ctx);
    case PrimValType::S64:
      return formatter<std::string_view>::format("s64"sv, Ctx);
    case PrimValType::U64:
      return formatter<std::string_view>::format("u64"sv, Ctx);
    case PrimValType::F32:
      return formatter<std::string_view>::format("float32"sv, Ctx);
    case PrimValType::F64:
      return formatter<std::string_view>::format("float64"sv, Ctx);
    case PrimValType::Char:
      return formatter<std::string_view>::format("char"sv, Ctx);
    case PrimValType::String:
      return formatter<std::string_view>::format("string"sv, Ctx);
    default:
      return formatter<std::string_view>::format("unknown primvaltype"sv, Ctx);
    }
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::ValueType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::ValueType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    if (std::holds_alternative<PrimValType>(Type)) {
      return formatter<std::string_view>::format(
          fmt::format("{}", std::get<PrimValType>(Type)), Ctx);
    }
    // or it's an type index
    const auto Idx = std::get<uint32_t>(Type);
    return formatter<std::string_view>::format(fmt::format("!({})", Idx), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::RecordTy>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::RecordTy &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "record <"sv);
    for (const auto &LabelTyp : Type.getLabelTypes()) {
      fmt::format_to(std::back_inserter(Buffer), "| {} : {} "sv,
                     LabelTyp.getLabel(), LabelTyp.getValType());
    }
    fmt::format_to(std::back_inserter(Buffer), ">"sv);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::VariantTy>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::VariantTy &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "variant <"sv);
    for (const auto &Case : Type.getCases()) {
      if (Case.getValType().has_value()) {
        fmt::format_to(std::back_inserter(Buffer), "| {} : {} "sv,
                       Case.getLabel(), Case.getValType().value());
      } else {
        fmt::format_to(std::back_inserter(Buffer), "| {} "sv, Case.getLabel());
      }
    }
    fmt::format_to(std::back_inserter(Buffer), ">"sv);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::ListTy>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::ListTy &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "list<{}>"sv, Type.getValType());
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::TupleTy>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::TupleTy &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "tuple <"sv);
    for (const auto &Ty : Type.getTypes()) {
      fmt::format_to(std::back_inserter(Buffer), "| {} "sv, Ty);
    }
    fmt::format_to(std::back_inserter(Buffer), ">"sv);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::OptionTy>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::OptionTy &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "option<{}>"sv,
                   Type.getValType());
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::ResultTy>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::ResultTy &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    if (Type.getValType().has_value()) {
      fmt::format_to(std::back_inserter(Buffer), "result<{}"sv,
                     Type.getValType().value());
    } else {
      fmt::format_to(std::back_inserter(Buffer), "result<"sv);
    }
    if (Type.getErrorType().has_value()) {
      auto E = Type.getErrorType().value();
      fmt::format_to(std::back_inserter(Buffer), ", {}>"sv, E);
    } else {
      fmt::format_to(std::back_inserter(Buffer), ">"sv);
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::DefValType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::DefValType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;
    return formatter<std::string_view>::format(
        std::visit(
            Overloaded{
                [](const PrimValType &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "{}"sv, Arg);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const RecordTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "{}"sv, Arg);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const VariantTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "{}"sv, Arg);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const ListTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "{}"sv, Arg);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const TupleTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "{}"sv, Arg);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const FlagsTy &) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "flags"sv);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const EnumTy &) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "enum"sv);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const OptionTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "{}"sv, Arg);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const ResultTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "{}"sv, Arg);
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const OwnTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "own<!({})>"sv,
                                 Arg.getIndex());
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const BorrowTy &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "borrow<!({})>"sv,
                                 Arg.getIndex());
                  return std::string_view(Buffer.data(), Buffer.size());
                },
            },
            Type),
        Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::FuncType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::FuncType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "[ "sv);
    for (const auto &PL : Type.getParamList()) {
      fmt::format_to(std::back_inserter(Buffer), "({} : {}) "sv, PL.getLabel(),
                     PL.getValType());
    }
    fmt::format_to(std::back_inserter(Buffer), "] -> "sv);
    const auto &ResList = Type.getResultList();
    if (std::holds_alternative<ValueType>(ResList)) {
      fmt::format_to(std::back_inserter(Buffer), "{}"sv,
                     std::get<ValueType>(ResList));
    } else {
      fmt::format_to(std::back_inserter(Buffer), "[ "sv);
      for (const auto &RL : std::get<std::vector<LabelValType>>(ResList)) {
        fmt::format_to(std::back_inserter(Buffer), "({} : {}) "sv,
                       RL.getLabel(), RL.getValType());
      }
      fmt::format_to(std::back_inserter(Buffer), "]"sv);
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::ComponentType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::ComponentType &,
         fmt::format_context &Ctx) const noexcept {
    using namespace std::literals;
    return formatter<std::string_view>::format("component type"sv, Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::CoreType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::CoreType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    std::visit(Overloaded{
                   [&](const AST::FunctionType &) {
                     fmt::format_to(std::back_inserter(Buffer),
                                    "core:function type"sv);
                   },
                   [&](const AST::Component::CoreModuleType &) {
                     fmt::format_to(std::back_inserter(Buffer),
                                    "core:module type"sv);
                   },
               },
               Type.getType());
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::InstanceType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::InstanceType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "instance {{\n"sv);
    for (const auto &Ty : Type.getContent()) {
      fmt::format_to(std::back_inserter(Buffer), "  "sv);
      std::visit(
          Overloaded{
              [&](const CoreType &T) {
                fmt::format_to(std::back_inserter(Buffer), "{}"sv, T);
              },
              [&](const Alias &) {
                fmt::format_to(std::back_inserter(Buffer), "alias type"sv);
              },
              [&](const std::shared_ptr<WasmEdge::AST::Component::Type> &T) {
                fmt::format_to(std::back_inserter(Buffer), "{}"sv,
                               (*T.get()).getType());
              },
              [&](const ExportDecl &) {
                fmt::format_to(std::back_inserter(Buffer),
                               "export decl type"sv);
              }},
          Ty);
      fmt::format_to(std::back_inserter(Buffer), "\n"sv);
    }
    fmt::format_to(std::back_inserter(Buffer), "}}"sv);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::ResourceType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::ResourceType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "resource-type {{\n"sv);
    if (Type.getDestructor().has_value()) {
      fmt::format_to(std::back_inserter(Buffer), "  destructor-index = {}\n"sv,
                     Type.getDestructor().value());
    } else {
      fmt::format_to(std::back_inserter(Buffer),
                     "  destructor-index = none\n"sv);
    }
    if (Type.getCallback().has_value()) {
      fmt::format_to(std::back_inserter(Buffer), "  callback-index = {}\n"sv,
                     Type.getCallback().value());
    } else {
      fmt::format_to(std::back_inserter(Buffer), "  callback-index = none\n"sv);
    }
    fmt::format_to(std::back_inserter(Buffer), "}}"sv);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::DefType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::DefType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace WasmEdge::AST::Component;
    using namespace std::literals;

    return formatter<std::string_view>::format(
        std::visit(
            Overloaded{
                [](const DefValType &Arg) { return fmt::format("{}"sv, Arg); },
                [](const FuncType &Arg) { return fmt::format("{}"sv, Arg); },
                [](const ComponentType &Arg) {
                  return fmt::format("{}"sv, Arg);
                },
                [](const InstanceType &Arg) {
                  return fmt::format("{}"sv, Arg);
                },
                [](const ResourceType &Arg) {
                  return fmt::format("{}"sv, Arg);
                }},
            Type),
        Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::AST::Component::FunctionType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::FunctionType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "[ "sv);
    for (auto &P : Type.getParamTypes()) {
      fmt::format_to(std::back_inserter(Buffer), "{} "sv, P);
    }
    fmt::format_to(std::back_inserter(Buffer), "] -> [ "sv);
    for (auto &R : Type.getReturnTypes()) {
      fmt::format_to(std::back_inserter(Buffer), "{} "sv, R);
    }
    fmt::format_to(std::back_inserter(Buffer), "]"sv);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
