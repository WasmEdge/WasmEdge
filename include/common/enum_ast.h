// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_ast.h - AST enumeration definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of Wasm VM used AST and instruction nodes
/// enumerations.
///
//===----------------------------------------------------------------------===//

// This header is not exported to the C API.

#ifndef WASMEDGE_C_API_ENUM_AST_H
#define WASMEDGE_C_API_ENUM_AST_H

#ifdef __cplusplus
#include "dense_enum_map.h"
#include "spare_enum_map.h"
#include <cstdint>
#include <string>
#include <tuple>
#endif

namespace WasmEdge {

/// AST node attributes enumeration class.
enum class ASTNodeAttr : uint8_t {
#define UseASTNodeAttr
#define Line(NAME, STRING) NAME,
#include "enum.inc"
#undef Line
#undef UseASTNodeAttr
};

/// AST node attributes enumeration string mapping.
static inline constexpr auto ASTNodeAttrStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ASTNodeAttr, std::string_view> Array[] = {
#define UseASTNodeAttr
#define Line(NAME, STRING) {ASTNodeAttr::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseASTNodeAttr
  };
  return DenseEnumMap(Array);
}
();

/// Instruction opcode enumeration class.
enum class OpCode : uint16_t {
#define UseOpCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseOpCode
};

/// Instruction opcode enumeration string mapping.
static inline constexpr const auto OpCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<OpCode, std::string_view> Array[] = {
#define UseOpCode
#define Line(NAME, VALUE, STRING) {OpCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseOpCode
  };
  return SpareEnumMap(Array);
}
();

/// Component Model Interface Types 
enum class InterfaceTypes{
   Unit,
    Bool,
    S8,
    U8,
    S16,
    U16,
    S32,
    U32,
    S64,
    U64,
    Float32,
    Float64,
    Char,
    String, 
    Record,
    Variant,
    List,
    Tuple,
    Flags,
    Enum, 
    Union, 
    Option, 
    Expected,
};


// Implementation of "record" type in interface types
struct typeRecord{
  RecordField *field = new RecordField;
};

struct RecordField {
  std::string name;
  InterfaceTypes ty;
};

// Implementation of "variant" type in interface types 
struct typeVariant {
  VariantCase *cases = new VariantCase;
};

struct VariantCase {
  std::string name;
  InterfaceTypes ty;
};

// Implementation of "tuple" type in interface types 
 struct typeTuple {
 InterfaceTypes *tyTup = new InterfaceTypes;
 };

// Implementation of "flags" type in interface types 
struct typeFlags {
std::string *names = new std::string;
};

// Implementation of "enum" type in interface types 
struct typeEnum {
 std::string *names = new std::string;
};

//Implentation of "union" type in interface types 
struct typeUnion {
  InterfaceTypes *tyUn = new InterfaceTypes;
}; 

//Implementation of "expected" type in interface types 
struct typeExpected {
  InterfaceTypes tyEx;
  InterfaceTypes err;
};

} // namespace WasmEdge

#endif // WASMEDGE_C_API_ENUM_AST_H
