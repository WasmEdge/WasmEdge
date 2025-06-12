// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/ast/component/descriptor.h - Descriptor class definitions ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Descriptor related class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/valtype.h"
#include "ast/type.h"

#include <variant>

namespace WasmEdge {
namespace AST {
namespace Component {

/// NOTE: The `ImportDesc` in AST implemented the fully import with name and
/// module name in a class. Therefore create a `CoreImportDesc` class with
/// only the import descriptions for component model.

/// AST Component::CoreImportDesc node.
class CoreImportDesc {
public:
  uint32_t getTypeIndex() const noexcept {
    return *std::get_if<uint32_t>(&Type);
  }
  void setTypeIndex(const uint32_t Idx) noexcept {
    Type.emplace<uint32_t>(Idx);
  }

  const TableType &getTableType() const noexcept {
    return *std::get_if<TableType>(&Type);
  }
  void setTableType(TableType &&TT) noexcept {
    Type.emplace<TableType>(std::move(TT));
  }

  const MemoryType &getMemoryType() const noexcept {
    return *std::get_if<MemoryType>(&Type);
  }
  void setMemoryType(MemoryType &&MT) noexcept {
    Type.emplace<MemoryType>(std::move(MT));
  }

  const GlobalType &getGlobalType() const noexcept {
    return *std::get_if<GlobalType>(&Type);
  }
  void setGlobalType(GlobalType &&GT) noexcept {
    Type.emplace<GlobalType>(std::move(GT));
  }

  const TagType &getTagType() const noexcept {
    return *std::get_if<TagType>(&Type);
  }
  void setTagType(TagType &&TT) noexcept {
    Type.emplace<TagType>(std::move(TT));
  }

private:
  std::variant<uint32_t, TableType, MemoryType, GlobalType, TagType> Type;
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

class ExternDesc {
public:
  enum class DescType : Byte {
    CoreType = 0x00,
    FuncType = 0x01,
    ValueBound = 0x02,
    TypeBound = 0x03,
    ComponentType = 0x04,
    InstanceType = 0x05,
  };

  DescType getDescType() const noexcept { return Type; }
  uint32_t getTypeIndex() const noexcept { return Idx; }
  bool isEqType() const noexcept { return Eq; }
  const ValueType &getValueType() const noexcept { return VType; }

  void setCoreTypeIdx(const uint32_t I) noexcept {
    Type = DescType::CoreType;
    Idx = I;
  }
  void setFuncTypeIdx(const uint32_t I) noexcept {
    Type = DescType::FuncType;
    Idx = I;
  }
  void setValueBound(const uint32_t I) noexcept {
    Type = DescType::ValueBound;
    Eq = true;
    Idx = I;
  }
  void setValueBound(const ValueType &T) noexcept {
    Type = DescType::ValueBound;
    Eq = false;
    VType = T;
  }
  void setTypeBound(const uint32_t I) noexcept {
    Type = DescType::TypeBound;
    Eq = true;
    Idx = I;
  }
  void setTypeBound() noexcept {
    Type = DescType::TypeBound;
    Eq = false;
  }
  void setComponentTypeIdx(const uint32_t I) noexcept {
    Type = DescType::ComponentType;
    Idx = I;
  }
  void setInstanceTypeIdx(const uint32_t I) noexcept {
    Type = DescType::InstanceType;
    Idx = I;
  }

private:
  DescType Type;
  bool Eq;
  uint32_t Idx;
  ValueType VType;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
