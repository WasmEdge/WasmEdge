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

#include "ast/component/declarator.h"
#include "ast/component/valtype.h"
#include "ast/type.h"

#include <optional>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// For easily understanding the type definitions in component model proposal,
// this header file defines the classes in bottom-up and simply separates the
// types into several parts.
//   1. Core::Type.
//   2. DefValType: one of DefType.
//   2. FuncType: one of DefType.
//   4. InstanceType: one of DefType.
//   5. ComponentType: one of DefType.
//   6. ResourceType: one of DefType.
//   7. Type: union of DefType.

// =============================================================================
// Part 1: Core::Type definition.
// =============================================================================

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

// core:type       ::= dt:<core:deftype> => (type dt)
// core:deftype    ::= rt:<core:rectype>
//                   => rt (WebAssembly 3.0)
//                   | 0x00 0x50 x*:vec(<core:typeidx>) ct:<core:comptype>
//                   => sub x* ct (WebAssembly 3.0)
//                   | mt:<core:moduletype>
//                   => mt
// core:moduletype ::= 0x50 md*:vec(<core:moduledecl>) => (module md*)

/// AST Component::CoreDefType node.
class CoreDefType {
public:
  Span<const SubType> getSubTypes() const noexcept {
    return *std::get_if<std::vector<SubType>>(&Type);
  }
  void setSubTypes(std::vector<SubType> &&STypes) noexcept {
    Type.emplace<std::vector<SubType>>(std::move(STypes));
  }

  Span<const CoreModuleDecl> getModuleType() const noexcept {
    return *std::get_if<std::vector<CoreModuleDecl>>(&Type);
  }
  void setModuleType(std::vector<CoreModuleDecl> &&MDecls) noexcept {
    Type.emplace<std::vector<CoreModuleDecl>>(std::move(MDecls));
  }

  bool isRecType() const noexcept {
    return std::holds_alternative<std::vector<SubType>>(Type);
  }
  bool isModuleType() const noexcept {
    return std::holds_alternative<std::vector<CoreModuleDecl>>(Type);
  }

private:
  std::variant<std::vector<SubType>, std::vector<CoreModuleDecl>> Type;
};

// =============================================================================
// Part 2: DefValType definition.
// =============================================================================

// record ::= lt*:vec(<labelvaltype>) => (record (field lt)*) (if |lt*| > 0)

/// AST Component::RecordTy node. (One type of DefValType)
struct RecordTy {
  std::vector<LabelValType> LabelTypes;
};

// variant ::= case*:vec(<case>)             => (variant case+) (if |case*| > 0)
// case    ::= l:<label'> t?:<valtype>? 0x00 => (case l t?)
// label'  ::= len:<u32> l:<label>           => l (if len = |l|)

/// AST Component::VariantTy node. (One type of DefValType)
struct VariantTy {
  std::vector<std::pair<std::string, std::optional<ValueType>>> Cases;
};

// list ::= t:<valtype>           => (list t)
// list ::= t:<valtype> len:<u32> => (list t len) (if len > 0) 🔧

/// AST Component::ListTy node. (One type of DefValType)
struct ListTy {
  ValueType ValTy;
  std::optional<uint32_t> Len;
};

// tuple ::= t*:vec(<valtype>) => (tuple t+) (if |t*| > 0)
struct TupleTy {
  // A tuple is the product of given non-empty type list.
  // e.g. given [A, B, C], the tuple is a product A x B x C.
  std::vector<ValueType> Types;
};

// flags  ::= l*:vec(<label'>)    => (flags l+) (if 0 < |l*| <= 32)
// label' ::= len:<u32> l:<label> => l (if len = |l|)

/// AST Component::FlagsTy node. (One type of DefValType)
struct FlagsTy {
  std::vector<std::string> Labels;
};

// enum   ::= l*:vec(<label'>)    => (enum l+) (if |l*| > 0)
// label' ::= len:<u32> l:<label> => l (if len = |l|)

/// AST Component::EnumTy node. (One type of DefValType)
struct EnumTy {
  std::vector<std::string> Labels;
};

// option ::= t:<valtype> => (option t)

/// AST Component::OptionTy node. (One type of DefValType)
struct OptionTy {
  ValueType ValTy;
};

// result ::= t?:<valtype>? u?:<valtype>? => (result t? (error u)?)

/// AST Component::ResultTy node. (One type of DefValType)
struct ResultTy {
  std::optional<ValueType> ValTy, ErrTy;
};

// own ::= i:<typeidx> => (own i)

/// AST Component::OwnTy node. (One type of DefValType)
struct OwnTy {
  uint32_t Idx;
};

// borrow ::= i:<typeidx> => (borrow i)

/// AST Component::BorrowTy node. (One type of DefValType)
struct BorrowTy {
  uint32_t Idx;
};

// stream ::= t?:<valtype>? => (stream t?) 🔀

/// AST Component::StreamTy node. (One type of DefValType)
struct StreamTy {
  std::optional<ValueType> ValTy;
};

// future ::= t?:<valtype>? => (future t?) 🔀

/// AST Component::FutureTy node. (One type of DefValType)
struct FutureTy {
  std::optional<ValueType> ValTy;
};

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

class DefValType {
public:
  PrimValType getPrimValType() const noexcept {
    return *std::get_if<PrimValType>(&Type);
  }
  void setPrimValType(const PrimValType PVT) noexcept {
    Type.emplace<PrimValType>(PVT);
  }

  const RecordTy &getRecord() const noexcept {
    return *std::get_if<RecordTy>(&Type);
  }
  void setRecord(RecordTy &&Ty) noexcept {
    Type.emplace<RecordTy>(std::move(Ty));
  }

  const VariantTy &getVariant() const noexcept {
    return *std::get_if<VariantTy>(&Type);
  }
  void setVariant(VariantTy &&Ty) noexcept {
    Type.emplace<VariantTy>(std::move(Ty));
  }

  const ListTy &getList() const noexcept { return *std::get_if<ListTy>(&Type); }
  void setList(ListTy &&Ty) noexcept { Type.emplace<ListTy>(std::move(Ty)); }

  const TupleTy &getTuple() const noexcept {
    return *std::get_if<TupleTy>(&Type);
  }
  void setTuple(TupleTy &&Ty) noexcept { Type.emplace<TupleTy>(std::move(Ty)); }

  const FlagsTy &getFlags() const noexcept {
    return *std::get_if<FlagsTy>(&Type);
  }
  void setFlags(FlagsTy &&Ty) noexcept { Type.emplace<FlagsTy>(std::move(Ty)); }

  const EnumTy &getEnum() const noexcept { return *std::get_if<EnumTy>(&Type); }
  void setEnum(EnumTy &&Ty) noexcept { Type.emplace<EnumTy>(std::move(Ty)); }

  const OptionTy &getOption() const noexcept {
    return *std::get_if<OptionTy>(&Type);
  }
  void setOption(OptionTy &&Ty) noexcept {
    Type.emplace<OptionTy>(std::move(Ty));
  }

  const ResultTy &getResult() const noexcept {
    return *std::get_if<ResultTy>(&Type);
  }
  void setResult(ResultTy &&Ty) noexcept {
    Type.emplace<ResultTy>(std::move(Ty));
  }

  const OwnTy &getOwn() const noexcept { return *std::get_if<OwnTy>(&Type); }
  void setOwn(OwnTy &&Ty) noexcept { Type.emplace<OwnTy>(std::move(Ty)); }

  const BorrowTy &getBorrow() const noexcept {
    return *std::get_if<BorrowTy>(&Type);
  }
  void setBorrow(BorrowTy &&Ty) noexcept {
    Type.emplace<BorrowTy>(std::move(Ty));
  }

  const StreamTy &getStream() const noexcept {
    return *std::get_if<StreamTy>(&Type);
  }
  void setStream(StreamTy &&Ty) noexcept {
    Type.emplace<StreamTy>(std::move(Ty));
  }

  const FutureTy &getFuture() const noexcept {
    return *std::get_if<FutureTy>(&Type);
  }
  void setFuture(FutureTy &&Ty) noexcept {
    Type.emplace<FutureTy>(std::move(Ty));
  }

private:
  std::variant<PrimValType, RecordTy, VariantTy, ListTy, TupleTy, FlagsTy,
               EnumTy, OptionTy, ResultTy, OwnTy, BorrowTy, StreamTy, FutureTy>
      Type;
};

// =============================================================================
// Part 3: FuncType definition.
// =============================================================================

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

// functype   ::= 0x40 ps:<paramlist> rs:<resultlist> => (func ps rs)
// paramlist  ::= lt*:vec(<labelvaltype>)             => (param lt)*
// resultlist ::= 0x00 t:<valtype> => (result t)
//              | 0x01 0x00        => ϵ

/// AST Component::FuncType node.
class FuncType {
public:
  FuncType() noexcept = default;
  FuncType(const std::vector<LabelValType> &P, const ValueType &R) noexcept
      : ParamList(P), ResultList(R) {}
  FuncType(std::vector<LabelValType> &&P, const ValueType &R) noexcept
      : ParamList(std::move(P)), ResultList(R) {}
  FuncType(const std::vector<LabelValType> &P,
           const std::vector<LabelValType> &R) noexcept
      : ParamList(P), ResultList(R) {}
  FuncType(std::vector<LabelValType> &&P,
           std::vector<LabelValType> &&R) noexcept
      : ParamList(std::move(P)), ResultList(std::move(R)) {}

  Span<const LabelValType> getParamList() const noexcept { return ParamList; }
  void setParamList(std::vector<LabelValType> &&P) noexcept {
    ParamList = std::move(P);
  }

  const ValueType &getResultType() const noexcept {
    return *std::get_if<ValueType>(&ResultList);
  }
  void setResultType(const ValueType &VT) noexcept {
    ResultList.emplace<ValueType>(VT);
  }
  Span<const LabelValType> getResultList() const noexcept {
    return *std::get_if<std::vector<LabelValType>>(&ResultList);
  }
  void setResultList(std::vector<LabelValType> &&List) noexcept {
    ResultList.emplace<std::vector<LabelValType>>(std::move(List));
  }

  uint32_t getResultArity() const noexcept {
    if (std::holds_alternative<ValueType>(ResultList)) {
      return 1U;
    } else {
      return static_cast<uint32_t>(
          std::get<std::vector<LabelValType>>(ResultList).size());
    }
  }

private:
  std::vector<LabelValType> ParamList;
  std::variant<ValueType, std::vector<LabelValType>> ResultList;
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
// Part 4: InstanceType definition.
// =============================================================================

// instancetype ::= 0x42 id*:vec(<instancedecl>) => (instance id*)

/// AST Component::InstanceType node.
class InstanceType {
public:
  Span<const InstanceDecl> getDecl() const noexcept { return Decl; }
  void setDecl(std::vector<InstanceDecl> &&D) noexcept { Decl = std::move(D); }

private:
  std::vector<InstanceDecl> Decl;
};

// =============================================================================
// Part 5: ComponentType and the child types definitions.
// =============================================================================

// componenttype ::= 0x41 cd*:vec(<componentdecl>) => (component cd*)

/// AST Component::ComponentType node.
class ComponentType {
public:
  Span<const ComponentDecl> getDecl() const noexcept { return Decl; }
  void setDecl(std::vector<ComponentDecl> &&D) noexcept { Decl = std::move(D); }

private:
  std::vector<ComponentDecl> Decl;
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

// type    ::= dt:<deftype> => (type dt)
// deftype ::= dvt:<defvaltype>   => dvt
//           | ft:<functype>      => ft
//           | ct:<componenttype> => ct
//           | it:<instancetype>  => it
//           | rt:<resourcetype>  => rt

/// AST Component::DefType node.
class DefType {
public:
  const DefValType &getDefValType() const noexcept {
    return *std::get_if<DefValType>(&Type);
  }
  void setDefValType(DefValType &&FT) noexcept {
    Type.emplace<DefValType>(std::move(FT));
  }

  const FuncType &getFuncType() const noexcept {
    return *std::get_if<FuncType>(&Type);
  }
  void setFuncType(FuncType &&FT) noexcept {
    Type.emplace<FuncType>(std::move(FT));
  }

  const ComponentType &getComponentType() const noexcept {
    return *std::get_if<ComponentType>(&Type);
  }
  void setComponentType(ComponentType &&CT) noexcept {
    Type.emplace<ComponentType>(std::move(CT));
  }

  const InstanceType &getInstanceType() const noexcept {
    return *std::get_if<InstanceType>(&Type);
  }
  void setInstanceType(InstanceType &&IT) noexcept {
    Type.emplace<InstanceType>(std::move(IT));
  }

  const ResourceType &getResourceType() const noexcept {
    return *std::get_if<ResourceType>(&Type);
  }
  void setResourceType(ResourceType &&RT) noexcept {
    Type.emplace<ResourceType>(std::move(RT));
  }

  bool isDefValType() const noexcept {
    return std::holds_alternative<DefValType>(Type);
  }
  bool isFuncType() const noexcept {
    return std::holds_alternative<FuncType>(Type);
  }
  bool isComponentType() const noexcept {
    return std::holds_alternative<ComponentType>(Type);
  }
  bool isInstanceType() const noexcept {
    return std::holds_alternative<InstanceType>(Type);
  }
  bool isResourceType() const noexcept {
    return std::holds_alternative<ResourceType>(Type);
  }

private:
  std::variant<DefValType, FuncType, ComponentType, InstanceType, ResourceType>
      Type;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
