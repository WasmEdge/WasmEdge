// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

/// WASM Type code C enumeration.
enum WasmEdge_TypeCode {
#define UseTypeCode
#define Line(NAME, VALUE, STRING) WasmEdge_TypeCode_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseTypeCode
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
