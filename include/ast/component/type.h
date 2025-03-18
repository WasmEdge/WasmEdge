// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/component/type.h - type class definitions ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Type node class
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/alias.h"
#include "ast/description.h"
#include "ast/expression.h"
#include "ast/type.h"

#include <optional>
#include <vector>

namespace WasmEdge {

namespace Runtime::Instance {
class ComponentInstance;
}

namespace AST {

namespace Component {

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
  Float32 = 0x76,
  Float64 = 0x75,
  Char = 0x74,
  String = 0x73
};

using TypeIndex = uint32_t;
using ValueType = std::variant<TypeIndex, PrimValType>;

class LabelValType {
public:
  LabelValType() {}
  LabelValType(std::string L, ValueType VT) : Label{L}, ValTy{VT} {}

  std::string_view getLabel() const noexcept { return Label; }
  std::string &getLabel() noexcept { return Label; }
  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  std::string Label;
  ValueType ValTy;
};

class Case {
public:
  Case() : Label{}, ValTy{std::nullopt} {}
  Case(std::string &&L) : Label{L}, ValTy{std::nullopt} {}
  Case(std::string &&L, ValueType &&VT) : Label{L}, ValTy{VT} {}

  std::string_view getLabel() const noexcept { return Label; }
  std::string &getLabel() noexcept { return Label; }
  const std::optional<ValueType> getValType() const noexcept { return ValTy; }
  std::optional<ValueType> &getValType() noexcept { return ValTy; }

private:
  std::string Label;
  std::optional<ValueType> ValTy;
};

class RecordTy {
public:
  RecordTy() {}
  RecordTy(std::initializer_list<LabelValType> I) : LabelTypes{I} {}

  Span<const LabelValType> getLabelTypes() const noexcept { return LabelTypes; }
  std::vector<LabelValType> &getLabelTypes() noexcept { return LabelTypes; }

private:
  std::vector<LabelValType> LabelTypes;
};

class VariantTy {
public:
  VariantTy() : Cases{} {}
  VariantTy(std::initializer_list<Case> Cs) : Cases{Cs} {}

  Span<const Case> getCases() const noexcept { return Cases; }
  std::vector<Case> &getCases() noexcept { return Cases; }

private:
  std::vector<Case> Cases;
};

class ListTy {
public:
  ListTy() : ValTy{} {}
  ListTy(ValueType &&T) : ValTy{T} {}

  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  ValueType ValTy;
};

// A tuple is the product of given non-empty type list
// e.g. given [A, B, C], the tuple is a product A x B x C
class TupleTy {
public:
  Span<const ValueType> getTypes() const noexcept { return Types; }
  std::vector<ValueType> &getTypes() noexcept { return Types; }

private:
  std::vector<ValueType> Types;
};

class Flags {
public:
  Span<const std::string> getLabels() const noexcept { return Labels; }
  std::vector<std::string> &getLabels() noexcept { return Labels; }

private:
  std::vector<std::string> Labels;
};

class EnumTy {
public:
  Span<const std::string> getLabels() const noexcept { return Labels; }
  std::vector<std::string> &getLabels() noexcept { return Labels; }

private:
  std::vector<std::string> Labels;
};

class OptionTy {
public:
  OptionTy() {}
  OptionTy(ValueType T) : ValTy{T} {}

  const ValueType getValType() const noexcept { return ValTy; }
  ValueType &getValType() noexcept { return ValTy; }

private:
  ValueType ValTy;
};

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

class Own {
public:
  TypeIndex getIndex() const noexcept { return Idx; }
  TypeIndex &getIndex() noexcept { return Idx; }

private:
  TypeIndex Idx;
};

class Borrow {
public:
  TypeIndex getIndex() const noexcept { return Idx; }
  TypeIndex &getIndex() noexcept { return Idx; }

private:
  TypeIndex Idx;
};

using DefValType =
    std::variant<PrimValType, RecordTy, VariantTy, ListTy, TupleTy, Flags,
                 EnumTy, OptionTy, ResultTy, Own, Borrow>;
using ResultList = std::variant<ValueType, std::vector<LabelValType>>;
class FuncType {
public:
  FuncType() : ParamList{}, ResList{} {}
  FuncType(std::vector<LabelValType> P, ResultList R)
      : ParamList{P}, ResList{R} {}

  Span<const LabelValType> getParamList() const noexcept { return ParamList; }
  std::vector<LabelValType> &getParamList() noexcept { return ParamList; }
  ResultList getResultList() const noexcept { return ResList; }
  ResultList &getResultList() noexcept { return ResList; }

private:
  std::vector<LabelValType> ParamList;
  ResultList ResList;
};

/// AST FunctionType node.
class FunctionType {
public:
  /// Constructors.
  FunctionType() noexcept = default;
  FunctionType(Span<const InterfaceType> P,
               Span<const InterfaceType> R) noexcept
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()) {}

  /// `==` and `!=` operator overloadings.
  friend bool operator==(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return LHS.ParamTypes == RHS.ParamTypes &&
           LHS.ReturnTypes == RHS.ReturnTypes;
  }

  friend bool operator!=(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return !(LHS == RHS);
  }

  /// Getter of param types.
  const std::vector<InterfaceType> &getParamTypes() const noexcept {
    return ParamTypes;
  }
  std::vector<InterfaceType> &getParamTypes() noexcept { return ParamTypes; }

  /// Getter of return types.
  const std::vector<InterfaceType> &getReturnTypes() const noexcept {
    return ReturnTypes;
  }
  std::vector<InterfaceType> &getReturnTypes() noexcept { return ReturnTypes; }

private:
  /// \name Data of FunctionType.
  /// @{
  std::vector<InterfaceType> ParamTypes;
  std::vector<InterfaceType> ReturnTypes;
  /// @}
};

enum class IndexKind : Byte {
  CoreType = 0x00,
  FuncType = 0x02,
  ComponentType = 0x04,
  InstanceType = 0x05,
};
class DescTypeIndex {
  TypeIndex TyIdx;
  IndexKind Kind;

public:
  TypeIndex &getIndex() noexcept { return TyIdx; }
  TypeIndex getIndex() const noexcept { return TyIdx; }
  IndexKind &getKind() noexcept { return Kind; }
  IndexKind getKind() const noexcept { return Kind; }
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
using TypeBound = std::optional<TypeIndex>;

using ExternDesc = std::variant<DescTypeIndex, TypeBound, ValueType>;
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

class CoreExportDecl {
  std::string Name;
  ImportDesc Desc;

public:
  std::string_view getName() const noexcept { return Name; }
  std::string &getName() noexcept { return Name; }
  ImportDesc getImportDesc() const noexcept { return Desc; }
  ImportDesc &getImportDesc() noexcept { return Desc; }
};
class CoreType;
using ModuleDecl =
    std::variant<ImportDesc, std::shared_ptr<CoreType>, Alias, CoreExportDecl>;
class ModuleType {
  std::vector<ModuleDecl> Decls;

public:
  Span<const ModuleDecl> getContent() const noexcept { return Decls; }
  std::vector<ModuleDecl> &getContent() noexcept { return Decls; }
};

// TODO: wait GC proposal
// st:<core:structtype>     => st   (GC proposal)
// at:<core:arraytype>      => at   (GC proposal)
using CoreDefType = std::variant<WasmEdge::AST::FunctionType, ModuleType>;
class CoreType {
public:
  CoreDefType getType() const noexcept { return T; }
  CoreDefType &getType() noexcept { return T; }

private:
  CoreDefType T;
};

class Type;
using InstanceDecl =
    std::variant<CoreType, Alias, std::shared_ptr<Type>, ExportDecl>;
class InstanceType {
  std::vector<InstanceDecl> IdList;

public:
  Span<const InstanceDecl> getContent() const noexcept { return IdList; }
  std::vector<InstanceDecl> &getContent() noexcept { return IdList; }
};

// Pseudo Python code
//
// class ResourceType(Type):
//   impl: ComponentInstance
//   dtor: Optional[Callable] = None
//   dtor_sync: bool = True
//   dtor_callback: Optional[Callable] = None
using FuncIdx = uint32_t;
class ResourceType {
public:
  ResourceType() : DtorSync{true} {}
  ResourceType(bool Sync) : DtorSync{Sync} {}
  ResourceType(Runtime::Instance::ComponentInstance *I)
      : Impl{I}, DtorSync{true} {}
  ResourceType(Runtime::Instance::ComponentInstance *I, bool Sync)
      : Impl{I}, DtorSync{Sync} {}

  const Runtime::Instance::ComponentInstance *getImpl() const noexcept {
    return Impl;
  }

  std::optional<FuncIdx> getDestructor() const noexcept { return Dtor; }
  std::optional<FuncIdx> getCallback() const noexcept { return DtorCallback; }

  bool IsSync() noexcept { return DtorSync; }
  std::optional<FuncIdx> &getDestructor() noexcept { return Dtor; }
  std::optional<FuncIdx> &getCallback() noexcept { return DtorCallback; }

  // real implementation
  Runtime::Instance::ComponentInstance *Impl;

  // destructor is sync or not, true is sync, false is not sync
  bool DtorSync;
  // destructor
  std::optional<FuncIdx> Dtor;
  // destructor callback
  std::optional<FuncIdx> DtorCallback;
};

class ImportDecl {
  std::string ImportName;
  ExternDesc Desc;

public:
  std::string_view getImportName() const noexcept { return ImportName; }
  std::string &getImportName() noexcept { return ImportName; }
  ExternDesc getExternDesc() const noexcept { return Desc; }
  ExternDesc &getExternDesc() noexcept { return Desc; }
};
using ComponentDecl = std::variant<ImportDecl, InstanceDecl>;
class ComponentType {
  std::vector<ComponentDecl> CdList;

public:
  Span<const ComponentDecl> getContent() const noexcept { return CdList; }
  std::vector<ComponentDecl> &getContent() noexcept { return CdList; }
};

using DefType = std::variant<DefValType, FuncType, ComponentType, InstanceType,
                             ResourceType>;
class Type {
public:
  Type(DefType V) : T{V} {}
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
    case PrimValType::Float32:
      return formatter<std::string_view>::format("float32"sv, Ctx);
    case PrimValType::Float64:
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
    const auto Idx = std::get<TypeIndex>(Type);
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
                [](const Flags &) {
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
                [](const Own &Arg) {
                  fmt::memory_buffer Buffer;
                  fmt::format_to(std::back_inserter(Buffer), "own<!({})>"sv,
                                 Arg.getIndex());
                  return std::string_view(Buffer.data(), Buffer.size());
                },
                [](const Borrow &Arg) {
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
                   [&](const AST::Component::ModuleType &) {
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
