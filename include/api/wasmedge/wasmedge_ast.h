// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_ast.h - WasmEdge C API --------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about WASM AST (limit, module, function
/// type, table type, memory type, tag type, global type, import type, and
/// export type) in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_AST_H
#define WASMEDGE_C_API_AST_H

#include "wasmedge/wasmedge_basic.h"
#include "wasmedge/wasmedge_value.h"

/// Struct of WASM limit.
typedef struct WasmEdge_Limit {
  /// Boolean to describe has max value or not.
  bool HasMax;
  /// Boolean to describe is shared memory or not.
  bool Shared;
  /// Minimum value.
  uint64_t Min;
  /// Maximum value. Will be ignored if the `HasMax` is false.
  uint64_t Max;
} WasmEdge_Limit;

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge limit functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Compare the two WasmEdge_Limit objects.
///
/// \param Lim1 the first WasmEdge_Limit object to compare.
/// \param Lim2 the second WasmEdge_Limit object to compare.
///
/// \returns true if the content of two WasmEdge_Limit objects are the same,
/// false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_LimitIsEqual(const WasmEdge_Limit Lim1, const WasmEdge_Limit Lim2);

// <<<<<<<< WasmEdge limit functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge AST module functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Get the length of imports list of the AST module.
///
/// \param Cxt the WasmEdge_ASTModuleContext.
///
/// \returns length of the imports list.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ASTModuleListImportsLength(const WasmEdge_ASTModuleContext *Cxt);

/// List the imports of the AST module.
///
/// If the `Imports` buffer length is smaller than the result of the imports
/// list size, the overflowed return values will be discarded.
///
/// \param Cxt the WasmEdge_ASTModuleContext.
/// \param [out] Imports the import type contexts buffer. Can be NULL if import
/// types are not needed.
/// \param Len the buffer length.
///
/// \returns actual exported function list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ASTModuleListImports(const WasmEdge_ASTModuleContext *Cxt,
                              const WasmEdge_ImportTypeContext **Imports,
                              const uint32_t Len);

/// Get the length of exports list of the AST module.
///
/// \param Cxt the WasmEdge_ASTModuleContext.
///
/// \returns length of the exports list.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ASTModuleListExportsLength(const WasmEdge_ASTModuleContext *Cxt);

/// List the exports of the AST module.
///
/// If the `Exports` buffer length is smaller than the result of the exports
/// list size, the overflowed return values will be discarded.
///
/// \param Cxt the WasmEdge_ASTModuleContext.
/// \param [out] Exports the export type contexts buffer. Can be NULL if export
/// types are not needed.
/// \param Len the buffer length.
///
/// \returns actual exported function list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ASTModuleListExports(const WasmEdge_ASTModuleContext *Cxt,
                              const WasmEdge_ExportTypeContext **Exports,
                              const uint32_t Len);

/// Deletion of the WasmEdge_ASTModuleContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_ASTModuleContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ASTModuleDelete(WasmEdge_ASTModuleContext *Cxt);

// <<<<<<<< WasmEdge AST module functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge function type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_FunctionTypeContext.
///
/// The caller owns the object and should call `WasmEdge_FunctionTypeDelete` to
/// destroy it.
///
/// \param ParamList the value types list of parameters. NULL if the length is
/// 0.
/// \param ParamLen the ParamList buffer length.
/// \param ReturnList the value types list of returns. NULL if the length is 0.
/// \param ReturnLen the ReturnList buffer length.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_FunctionTypeContext *
WasmEdge_FunctionTypeCreate(const WasmEdge_ValType *ParamList,
                            const uint32_t ParamLen,
                            const WasmEdge_ValType *ReturnList,
                            const uint32_t ReturnLen);

/// Get the parameter types list length from the WasmEdge_FunctionTypeContext.
///
/// \param Cxt the WasmEdge_FunctionTypeContext.
///
/// \returns the parameter types list length.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_FunctionTypeGetParametersLength(
    const WasmEdge_FunctionTypeContext *Cxt);

/// Get the parameter types list from the WasmEdge_FunctionTypeContext.
///
/// If the `List` buffer length is smaller than the length of the parameter type
/// list, the overflowed values will be discarded.
///
/// \param Cxt the WasmEdge_FunctionTypeContext.
/// \param [out] List the WasmEdge_ValType buffer to fill the parameter value
/// types.
/// \param Len the value type buffer length.
///
/// \returns the actual parameter types list length.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_FunctionTypeGetParameters(const WasmEdge_FunctionTypeContext *Cxt,
                                   WasmEdge_ValType *List, const uint32_t Len);

/// Get the return types list length from the WasmEdge_FunctionTypeContext.
///
/// \param Cxt the WasmEdge_FunctionTypeContext.
///
/// \returns the return types list length.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_FunctionTypeGetReturnsLength(const WasmEdge_FunctionTypeContext *Cxt);

/// Get the return types list from the WasmEdge_FunctionTypeContext.
///
/// If the `List` buffer length is smaller than the length of the return type
/// list, the overflowed values will be discarded.
///
/// \param Cxt the WasmEdge_FunctionTypeContext.
/// \param [out] List the WasmEdge_ValType buffer to fill the return value
/// types.
/// \param Len the value type buffer length.
///
/// \returns the actual return types list length.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_FunctionTypeGetReturns(const WasmEdge_FunctionTypeContext *Cxt,
                                WasmEdge_ValType *List, const uint32_t Len);

/// Deletion of the WasmEdge_FunctionTypeContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_FunctionTypeContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_FunctionTypeDelete(WasmEdge_FunctionTypeContext *Cxt);

// <<<<<<<< WasmEdge function type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge table type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_TableTypeContext.
///
/// The caller owns the object and should call `WasmEdge_TableTypeDelete` to
/// destroy it.
///
/// \param RefType the value type of the table type. This value type should be a
/// reference type, or this function will fail.
/// \param Limit the limit struct of the table type.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_TableTypeContext *
WasmEdge_TableTypeCreate(const WasmEdge_ValType RefType,
                         const WasmEdge_Limit Limit);

/// Get the reference type from a table type.
///
/// \param Cxt the WasmEdge_TableTypeContext.
///
/// \returns the value type of the table type. This value type will must be a
/// reference type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType
WasmEdge_TableTypeGetRefType(const WasmEdge_TableTypeContext *Cxt);

/// Get the limit from a table type.
///
/// \param Cxt the WasmEdge_TableTypeContext.
///
/// \returns the limit struct of the table type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Limit
WasmEdge_TableTypeGetLimit(const WasmEdge_TableTypeContext *Cxt);

/// Deletion of the WasmEdge_TableTypeContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_TableTypeContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_TableTypeDelete(WasmEdge_TableTypeContext *Cxt);

// <<<<<<<< WasmEdge table type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge memory type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_MemoryTypeContext.
///
/// The caller owns the object and should call `WasmEdge_MemoryTypeDelete` to
/// destroy it.
///
/// \param Limit the limit struct of the memory type.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_MemoryTypeContext *
WasmEdge_MemoryTypeCreate(const WasmEdge_Limit Limit);

/// Get the limit from a memory type.
///
/// \param Cxt the WasmEdge_MemoryTypeContext.
///
/// \returns the limit struct of the memory type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Limit
WasmEdge_MemoryTypeGetLimit(const WasmEdge_MemoryTypeContext *Cxt);

/// Deletion of the WasmEdge_MemoryTypeContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_MemoryTypeContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_MemoryTypeDelete(WasmEdge_MemoryTypeContext *Cxt);

// <<<<<<<< WasmEdge memory type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge tag type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Get the function type from a tag type.
///
/// \param Cxt the WasmEdge_TagTypeContext.
///
/// \returns pointer to function type context of the tag type, NULL if failed.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_FunctionTypeContext *
WasmEdge_TagTypeGetFunctionType(const WasmEdge_TagTypeContext *Cxt);

// <<<<<<<< WasmEdge tag type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge global type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_GlobalTypeContext.
///
/// The caller owns the object and should call `WasmEdge_GlobalTypeDelete` to
/// destroy it.
///
/// \param ValType the value type of the global type.
/// \param Mut the mutation of the global type.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_GlobalTypeContext *
WasmEdge_GlobalTypeCreate(const WasmEdge_ValType ValType,
                          const enum WasmEdge_Mutability Mut);

/// Get the value type from a global type.
///
/// \param Cxt the WasmEdge_GlobalTypeContext.
///
/// \returns the value type of the global type.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValType
WasmEdge_GlobalTypeGetValType(const WasmEdge_GlobalTypeContext *Cxt);

/// Get the mutability from a global type.
///
/// \param Cxt the WasmEdge_GlobalTypeContext.
///
/// \returns the mutability of the global type.
WASMEDGE_CAPI_EXPORT extern enum WasmEdge_Mutability
WasmEdge_GlobalTypeGetMutability(const WasmEdge_GlobalTypeContext *Cxt);

/// Deletion of the WasmEdge_GlobalTypeContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_GlobalTypeContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_GlobalTypeDelete(WasmEdge_GlobalTypeContext *Cxt);

// <<<<<<<< WasmEdge global type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge import type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Get the external type from an import type.
///
/// \param Cxt the WasmEdge_ImportTypeContext.
///
/// \returns the external type of the import type.
WASMEDGE_CAPI_EXPORT extern enum WasmEdge_ExternalType
WasmEdge_ImportTypeGetExternalType(const WasmEdge_ImportTypeContext *Cxt);

/// Get the module name from an import type.
///
/// The returned string object is linked to the module name of the import type,
/// and the caller should __NOT__ call the `WasmEdge_StringDelete`.
///
/// \param Cxt the WasmEdge_ImportTypeContext.
///
/// \returns string object. Length will be 0 and Buf will be NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_ImportTypeGetModuleName(const WasmEdge_ImportTypeContext *Cxt);

/// Get the external name from an import type.
///
/// The returned string object is linked to the external name of the import
/// type, and the caller should __NOT__ call the `WasmEdge_StringDelete`.
///
/// \param Cxt the WasmEdge_ImportTypeContext.
///
/// \returns string object. Length will be 0 and Buf will be NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_ImportTypeGetExternalName(const WasmEdge_ImportTypeContext *Cxt);

/// Get the external value (which is function type) from an import type.
///
/// The import type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The function type context links to the function type in the import type
/// context and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_FunctionTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ImportTypeContext which queried from the `ASTCxt`.
///
/// \returns the function type. NULL if failed or the external type of the
/// import type is not `WasmEdge_ExternalType_Function`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_FunctionTypeContext *
WasmEdge_ImportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt,
                                   const WasmEdge_ImportTypeContext *Cxt);

/// Get the external value (which is table type) from an import type.
///
/// The import type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The table type context links to the table type in the import type context
/// and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_TableTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ImportTypeContext which queried from the `ASTCxt`.
///
/// \returns the table type. NULL if failed or the external type of the import
/// type is not `WasmEdge_ExternalType_Table`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_TableTypeContext *
WasmEdge_ImportTypeGetTableType(const WasmEdge_ASTModuleContext *ASTCxt,
                                const WasmEdge_ImportTypeContext *Cxt);

/// Get the external value (which is memory type) from an import type.
///
/// The import type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The memory type context links to the memory type in the import type context
/// and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_MemoryTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ImportTypeContext which queried from the `ASTCxt`.
///
/// \returns the memory type. NULL if failed or the external type of the import
/// type is not `WasmEdge_ExternalType_Memory`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_MemoryTypeContext *
WasmEdge_ImportTypeGetMemoryType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ImportTypeContext *Cxt);

/// Get the external value (which is tag type) from an import type.
///
/// The import type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The tag type context links to the tag type in the import type context
/// and the AST module context.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ImportTypeContext which queried from the `ASTCxt`.
///
/// \returns the tag type. NULL if failed or the external type of the import
/// type is not `WasmEdge_ExternalType_TagType`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_TagTypeContext *
WasmEdge_ImportTypeGetTagType(const WasmEdge_ASTModuleContext *ASTCxt,
                              const WasmEdge_ImportTypeContext *Cxt);

/// Get the external value (which is global type) from an import type.
///
/// The import type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The global type context links to the global type in the import type context
/// and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_GlobalTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ImportTypeContext which queried from the `ASTCxt`.
///
/// \returns the global type. NULL if failed or the external type of the import
/// type is not `WasmEdge_ExternalType_Global`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_GlobalTypeContext *
WasmEdge_ImportTypeGetGlobalType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ImportTypeContext *Cxt);

// <<<<<<<< WasmEdge import type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge export type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Get the external type from an export type.
///
/// \param Cxt the WasmEdge_ExportTypeContext.
///
/// \returns the external type of the export type.
WASMEDGE_CAPI_EXPORT extern enum WasmEdge_ExternalType
WasmEdge_ExportTypeGetExternalType(const WasmEdge_ExportTypeContext *Cxt);

/// Get the external name from an export type.
///
/// The returned string object is linked to the external name of the export
/// type, and the caller should __NOT__ call the `WasmEdge_StringDelete`.
///
/// \param Cxt the WasmEdge_ExportTypeContext.
///
/// \returns string object. Length will be 0 and Buf will be NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_ExportTypeGetExternalName(const WasmEdge_ExportTypeContext *Cxt);

/// Get the external value (which is function type) from an export type.
///
/// The export type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The function type context links to the function type in the export type
/// context and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_FunctionTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ExportTypeContext which queried from the `ASTCxt`.
///
/// \returns the function type. NULL if failed or the external type of the
/// export type is not `WasmEdge_ExternalType_Function`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_FunctionTypeContext *
WasmEdge_ExportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt,
                                   const WasmEdge_ExportTypeContext *Cxt);

/// Get the external value (which is table type) from an export type.
///
/// The export type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The table type context links to the table type in the export type context
/// and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_TableTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ExportTypeContext which queried from the `ASTCxt`.
///
/// \returns the table type. NULL if failed or the external type of the export
/// type is not `WasmEdge_ExternalType_Table`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_TableTypeContext *
WasmEdge_ExportTypeGetTableType(const WasmEdge_ASTModuleContext *ASTCxt,
                                const WasmEdge_ExportTypeContext *Cxt);

/// Get the external value (which is memory type) from an export type.
///
/// The export type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The memory type context links to the memory type in the export type context
/// and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_MemoryTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ExportTypeContext which queried from the `ASTCxt`.
///
/// \returns the memory type. NULL if failed or the external type of the export
/// type is not `WasmEdge_ExternalType_Memory`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_MemoryTypeContext *
WasmEdge_ExportTypeGetMemoryType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ExportTypeContext *Cxt);

/// Get the external value (which is tag type) from an export type.
///
/// The export type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The tag type context links to the tag type in the export type context
/// and the AST module context.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ExportTypeContext which queried from the `ASTCxt`.
///
/// \returns the tag type. NULL if failed or the external type of the export
/// type is not `WasmEdge_ExternalType_Tag`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_TagTypeContext *
WasmEdge_ExportTypeGetTagType(const WasmEdge_ASTModuleContext *ASTCxt,
                              const WasmEdge_ExportTypeContext *Cxt);

/// Get the external value (which is global type) from an export type.
///
/// The export type context should be the one queried from the AST module
/// context, or this function will cause unexpected error.
/// The global type context links to the global type in the export type context
/// and the AST module context. The caller should __NOT__ call the
/// `WasmEdge_GlobalTypeDelete`.
///
/// \param ASTCxt the WasmEdge_ASTModuleContext.
/// \param Cxt the WasmEdge_ExportTypeContext which queried from the `ASTCxt`.
///
/// \returns the global type. NULL if failed or the external type of the export
/// type is not `WasmEdge_ExternalType_Global`.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_GlobalTypeContext *
WasmEdge_ExportTypeGetGlobalType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ExportTypeContext *Cxt);

// <<<<<<<< WasmEdge export type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_AST_H
