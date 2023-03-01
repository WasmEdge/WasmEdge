// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_types.h - WASM types related enumerations ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of WASM types related enumerations.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_ENUM_TYPES_H
#define WASMEDGE_C_API_ENUM_TYPES_H

/// WASM Value type C enumeration.
enum WasmEdge_ValType {
#define UseValType
#define Line(NAME, VALUE, STRING) WasmEdge_ValType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseValType
};

enum WasmEdge_ValTypeCode {
#define UseNumType
#define Line(NAME, VALUE) WasmEdge_ValTypeCode_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseNumType

#define UseRefType
#define Line(NAME, VALUE) WasmEdge_ValTypeCode_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefType
};

enum WasmEdge_HeapTypeCode {
#define UseHeapTypeCode
#define Line(NAME, VALUE) WasmEdge_HeapTypeCode_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseHeapTypeCode
};

typedef struct WasmEdge_HeapType {
  enum WasmEdge_HeapTypeCode HeapTypeCode;
  uint32_t DefinedTypeIdx;
} WasmEdge_HeapType;

union WasmEdge_ValTypeExt {
  WasmEdge_HeapType HeapType;
};

typedef struct WasmEdge_FullValType {
  enum WasmEdge_ValTypeCode TypeCode;
  union WasmEdge_ValTypeExt Ext;
} WasmEdge_FullValType;

/// WASM Number type C enumeration.
enum WasmEdge_NumType {
#define UseNumType
#define Line(NAME, VALUE) WasmEdge_NumType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseNumType
};

/// WASM Reference type C enumeration.
enum WasmEdge_RefType {
#define UseRefType
#define Line(NAME, VALUE) WasmEdge_RefType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefType
};

enum WasmEdge_RefTypeCode {
#define UseRefTypeCode
#define Line(NAME, VALUE) WasmEdge_RefTypeCode_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefTypeCode
};

/// WASM Mutability C enumeration.
enum WasmEdge_Mutability {
#define UseValMut
#define Line(NAME, VALUE, STRING) WasmEdge_Mutability_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseValMut
};

/// WASM External type C enumeration.
enum WasmEdge_ExternalType {
#define UseExternalType
#define Line(NAME, VALUE, STRING) WasmEdge_ExternalType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseExternalType
};

#endif // WASMEDGE_C_API_ENUM_TYPES_H
