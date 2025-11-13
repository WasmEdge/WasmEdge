// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_value.h - WasmEdge C API ------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about WASM value and value type of WasmEdge
/// C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_VALUE_H
#define WASMEDGE_C_API_VALUE_H

#include "wasmedge/wasmedge_basic.h"

/// WasmEdge WASM value type struct.
typedef struct WasmEdge_ValType {
  // This struct contains the raw data which describes the value type in WASM.
  // Developers should use the corresponding `WasmEdge_ValueTypeGen` functions
  // to generate this struct.
  uint8_t Data[8];
} WasmEdge_ValType;

/// WasmEdge WASM value struct.
typedef struct WasmEdge_Value {
  uint128_t Value;
  // The value type `Type` is used in the parameters or returns of invoking
  // functions. Developers should use the corresponding `WasmEdge_ValueGen`
  // functions to generate this struct, and the `WasmEdge_ValueGet` functions to
  // retrieve the value from this struct.
  WasmEdge_ValType Type;
} WasmEdge_Value;

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge valtype functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Generate the I32 WASM value type.
///
/// \returns WasmEdge_ValType struct with the I32 value type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType WasmEdge_ValTypeGenI32(void);

/// Generate the I64 WASM value type.
///
/// \returns WasmEdge_ValType struct with the I64 value type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType WasmEdge_ValTypeGenI64(void);

/// Generate the F32 WASM value type.
///
/// \returns WasmEdge_ValType struct with the F32 value type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType WasmEdge_ValTypeGenF32(void);

/// Generate the F64 WASM value type.
///
/// \returns WasmEdge_ValType struct with the F64 value type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType WasmEdge_ValTypeGenF64(void);

/// Generate the V128 WASM value type.
///
/// \returns WasmEdge_ValType struct with the V128 value type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType WasmEdge_ValTypeGenV128(void);

/// Generate the FuncRef WASM value type.
///
/// \returns WasmEdge_ValType struct with the FuncRef value type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType WasmEdge_ValTypeGenFuncRef(void);

/// Generate the ExternRef WASM value type.
///
/// \returns WasmEdge_ValType struct with the ExternRef value type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType WasmEdge_ValTypeGenExternRef(void);

/// Compare the two WasmEdge_ValType objects.
///
/// \param ValType1 the first WasmEdge_ValType object to compare.
/// \param ValType2 the second WasmEdge_ValType object to compare.
///
/// \returns true if the content of two WasmEdge_ValType objects are the same,
/// false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsEqual(const WasmEdge_ValType ValType1,
                        const WasmEdge_ValType ValType2);

/// Specify the WASM value type is an I32 or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is an I32, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsI32(const WasmEdge_ValType ValType);

/// Specify the WASM value type is an I64 or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is an I64, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsI64(const WasmEdge_ValType ValType);

/// Specify the WASM value type is a F32 or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is a F32, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsF32(const WasmEdge_ValType ValType);

/// Specify the WASM value type is a F64 or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is a F64, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsF64(const WasmEdge_ValType ValType);

/// Specify the WASM value type is a V128 or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is a V128, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsV128(const WasmEdge_ValType ValType);

/// Specify the WASM value type is a FuncRef or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is a FuncRef, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsFuncRef(const WasmEdge_ValType ValType);

/// Specify the WASM value type is an ExternRef or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is an ExternRef, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsExternRef(const WasmEdge_ValType ValType);

/// Specify the WASM value type is a Ref (includes nullable and non-nullable) or
/// not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is a Ref, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsRef(const WasmEdge_ValType ValType);

/// Specify the WASM value type is a nullable Ref or not.
///
/// \param ValType the WasmEdge_ValType object to check.
///
/// \returns true if the value type is a nullable Ref, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValTypeIsRefNull(const WasmEdge_ValType ValType);

// <<<<<<<< WasmEdge valtype functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge value functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Generate the I32 WASM value.
///
/// \param Val the I32 value.
///
/// \returns WasmEdge_Value struct with the I32 value.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_ValueGenI32(const int32_t Val);

/// Generate the I64 WASM value.
///
/// \param Val the I64 value.
///
/// \returns WasmEdge_Value struct with the I64 value.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_ValueGenI64(const int64_t Val);

/// Generate the F32 WASM value.
///
/// \param Val the F32 value.
///
/// \returns WasmEdge_Value struct with the F32 value.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_ValueGenF32(const float Val);

/// Generate the F64 WASM value.
///
/// \param Val the F64 value.
///
/// \returns WasmEdge_Value struct with the F64 value.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_ValueGenF64(const double Val);

/// Generate the V128 WASM value.
///
/// \param Val the V128 value.
///
/// \returns WasmEdge_Value struct with the V128 value.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_ValueGenV128(const int128_t Val);

/// Generate the function reference WASM value.
///
/// The values generated by this function are only meaningful when the
/// `WasmEdge_Proposal_BulkMemoryOperations` or the
/// `WasmEdge_Proposal_ReferenceTypes` turns on in configuration.
///
/// \param Cxt the function instance context to convert to the reference.
///
/// \returns WasmEdge_Value struct with the function reference.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_ValueGenFuncRef(const WasmEdge_FunctionInstanceContext *Cxt);

/// Generate the function reference WASM value.
///
/// The values generated by this function are only meaningful when the
/// `WasmEdge_Proposal_ReferenceTypes` turns on in configuration.
///
/// \param Ref the reference to the external object.
///
/// \returns WasmEdge_Value struct with the external reference.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_ValueGenExternRef(void *Ref);

/// Retrieve the I32 value from the WASM value.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns I32 value in the input struct.
WASMEDGE_CAPI_EXPORT extern int32_t
WasmEdge_ValueGetI32(const WasmEdge_Value Val);

/// Retrieve the I64 value from the WASM value.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns I64 value in the input struct.
WASMEDGE_CAPI_EXPORT extern int64_t
WasmEdge_ValueGetI64(const WasmEdge_Value Val);

/// Retrieve the F32 value from the WASM value.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns F32 value in the input struct.
WASMEDGE_CAPI_EXPORT extern float
WasmEdge_ValueGetF32(const WasmEdge_Value Val);

/// Retrieve the F64 value from the WASM value.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns F64 value in the input struct.
WASMEDGE_CAPI_EXPORT extern double
WasmEdge_ValueGetF64(const WasmEdge_Value Val);

/// Retrieve the V128 value from the WASM value.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns V128 value in the input struct.
WASMEDGE_CAPI_EXPORT extern int128_t
WasmEdge_ValueGetV128(const WasmEdge_Value Val);

/// Specify the WASM value is a null reference or not.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns true if the value is a null reference, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ValueIsNullRef(const WasmEdge_Value Val);

/// Retrieve the function instance context from the WASM value.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns pointer to function instance context in the input struct.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_FunctionInstanceContext *
WasmEdge_ValueGetFuncRef(const WasmEdge_Value Val);

/// Retrieve the external reference from the WASM value.
///
/// \param Val the WasmEdge_Value struct.
///
/// \returns external reference in the input struct.
WASMEDGE_CAPI_EXPORT extern void *
WasmEdge_ValueGetExternRef(const WasmEdge_Value Val);

// <<<<<<<< WasmEdge value functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_VALUE_H
