// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_errcode.h - Error code enumerations ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of WasmEdge error code.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_ENUM_ERRCODE_H
#define WASMEDGE_C_API_ENUM_ERRCODE_H

/// Error category C enumeration.
enum WasmEdge_ErrCategory {
#define UseErrCategory
#define Line(NAME, VALUE) WasmEdge_ErrCategory_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseErrCategory
};

/// Error code C enumeration.
enum WasmEdge_ErrCode {
#define UseErrCode
#define Line(NAME, VALUE, STRING) WasmEdge_ErrCode_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseErrCode
};

#endif // WASMEDGE_C_API_ENUM_ERRCODE_H
