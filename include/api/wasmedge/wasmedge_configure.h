// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_configure.h - WasmEdge C API --------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about configuration and statistics settings
/// in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_CONFIGURE_H
#define WASMEDGE_C_API_CONFIGURE_H

#include "wasmedge/wasmedge_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge configure functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_ConfigureContext.
///
/// The caller owns the object and should call `WasmEdge_ConfigureDelete` to
/// destroy it.
///
/// \returns pointer to the context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ConfigureContext *
WasmEdge_ConfigureCreate(void);

/// Add a proposal setting into the WasmEdge_ConfigureContext.
///
/// For turning on a specific WASM proposal in VM, loader, or compiler contexts,
/// etc., you can set the proposal value into the WasmEdge_ConfigureContext and
/// create the VM, loader, or compiler contexts, etc. with this context.
///
/// ```c
/// WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
/// WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_BulkMemoryOperations);
/// WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ReferenceTypes);
/// WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_SIMD);
/// WasmEdge_VMContext *VM = WasmEdge_VMCreate(Conf, NULL);
/// ```
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to add the proposal value.
/// \param Prop the proposal value.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureAddProposal(WasmEdge_ConfigureContext *Cxt,
                              const enum WasmEdge_Proposal Prop);

/// Remove a proposal setting in the WasmEdge_ConfigureContext.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to remove the proposal.
/// \param Prop the proposal value.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureRemoveProposal(WasmEdge_ConfigureContext *Cxt,
                                 const enum WasmEdge_Proposal Prop);

/// Check if a proposal setting exists in the WasmEdge_ConfigureContext or not.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to check the proposal value.
/// \param Prop the proposal value.
///
/// \returns true if the proposal setting exists, false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ConfigureHasProposal(const WasmEdge_ConfigureContext *Cxt,
                              const enum WasmEdge_Proposal Prop);

/// Set the WASM standard in the WasmEdge_ConfigureContext.
///
/// When setting the WASM standard, the proposal settings in this
/// WasmEdge_ConfigureContext will be overridden.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to check the proposal value.
/// \param Std the standard value.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureSetWASMStandard(WasmEdge_ConfigureContext *Cxt,
                                  const enum WasmEdge_Standard Std);

/// Add a built-in host registration setting into WasmEdge_ConfigureContext.
///
/// For turning on the Wasi support in `WasmEdge_VMContext`, you can set the
/// built-in host registration value into the `WasmEdge_ConfigureContext` and
/// create VM with this context.
///
/// ```c
/// WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
/// WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
/// WasmEdge_VMContext *VM = WasmEdge_VMCreate(Conf, NULL);
/// ```
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to add built-in host registration.
/// \param Host the built-in host registration value.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_ConfigureAddHostRegistration(
    WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_HostRegistration Host);

/// Remove a built-in host registration setting in the
/// WasmEdge_ConfigureContext.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to remove the host
/// pre-registration.
/// \param Host the built-in host registration value.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_ConfigureRemoveHostRegistration(
    WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_HostRegistration Host);

/// Check if a built-in host registration setting exists in the
/// WasmEdge_ConfigureContext or not.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to check the host pre-registration.
/// \param Host the built-in host registration value.
///
/// \returns true if the built-in host registration setting exists, false if
/// not.
WASMEDGE_CAPI_EXPORT extern bool WasmEdge_ConfigureHasHostRegistration(
    const WasmEdge_ConfigureContext *Cxt,
    const enum WasmEdge_HostRegistration Host);

/// Set the page limit of memory instances.
///
/// Limit the page count (64KiB per page) in memory instances.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the maximum page count.
/// \param Page the maximum page count.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureSetMaxMemoryPage(WasmEdge_ConfigureContext *Cxt,
                                   const uint32_t Page);

/// Get the setting of the page limit of memory instances.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the maximum page count
/// setting.
///
/// \returns the page count limitation value.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ConfigureGetMaxMemoryPage(const WasmEdge_ConfigureContext *Cxt);

/// Set the maximum call stack depth.
///
/// Limit the maximum call stack depth to prevent stack overflow from
/// infinite recursion. This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the maximum call depth.
/// \param Depth the maximum call stack depth (default: 10000).
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureSetMaxCallDepth(WasmEdge_ConfigureContext *Cxt,
                                  const uint32_t Depth);

/// Get the setting of the maximum call stack depth.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the maximum call depth
/// setting.
///
/// \returns the maximum call stack depth value.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ConfigureGetMaxCallDepth(const WasmEdge_ConfigureContext *Cxt);

/// Set the force interpreter mode execution option.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param IsForceInterpreter the boolean value to determine to forcibly run
/// WASM in interpreter mode or not.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureSetForceInterpreter(WasmEdge_ConfigureContext *Cxt,
                                      const bool IsForceInterpreter);

/// Get the force interpreter mode execution option.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to forcibly run WASM in interpreter
/// mode or not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ConfigureIsForceInterpreter(const WasmEdge_ConfigureContext *Cxt);

/// Set the option of enabling/disabling AF_UNIX support in the WASI socket.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param EnableAFUNIX the boolean value to determine to enable
/// the AF_UNIX support in the WASI socket or not.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureSetAllowAFUNIX(WasmEdge_ConfigureContext *Cxt,
                                 const bool EnableAFUNIX);

/// Get the AllowAFUNIX option.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to enable AF_UNIX support in the
/// WASI socket or not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ConfigureIsAllowAFUNIX(const WasmEdge_ConfigureContext *Cxt);

/// Set the optimization level of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the optimization level.
/// \param Level the AOT compiler optimization level.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_ConfigureCompilerSetOptimizationLevel(
    WasmEdge_ConfigureContext *Cxt,
    const enum WasmEdge_CompilerOptimizationLevel Level);

/// Get the optimization level of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the optimization level.
///
/// \returns the AOT compiler optimization level.
WASMEDGE_CAPI_EXPORT extern enum WasmEdge_CompilerOptimizationLevel
WasmEdge_ConfigureCompilerGetOptimizationLevel(
    const WasmEdge_ConfigureContext *Cxt);

/// Set the output binary format of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the output binary format.
/// \param Format the AOT compiler output binary format.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_ConfigureCompilerSetOutputFormat(
    WasmEdge_ConfigureContext *Cxt,
    const enum WasmEdge_CompilerOutputFormat Format);

/// Get the output binary format of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the output binary format.
///
/// \returns the AOT compiler output binary format.
WASMEDGE_CAPI_EXPORT extern enum WasmEdge_CompilerOutputFormat
WasmEdge_ConfigureCompilerGetOutputFormat(const WasmEdge_ConfigureContext *Cxt);

/// Set the dump IR option of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param IsDump the boolean value to determine to dump IR or not when
/// compilation in AOT compiler.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureCompilerSetDumpIR(WasmEdge_ConfigureContext *Cxt,
                                    const bool IsDump);

/// Get the dump IR option of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to dump IR or not when compilation
/// in AOT compiler.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ConfigureCompilerIsDumpIR(const WasmEdge_ConfigureContext *Cxt);

/// Set the generic binary option of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param IsGeneric the boolean value to determine to generate the generic
/// binary or not when compilation in AOT compiler.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureCompilerSetGenericBinary(WasmEdge_ConfigureContext *Cxt,
                                           const bool IsGeneric);

/// Get the generic binary option of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to generate the generic binary or
/// not when compilation in AOT compiler.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ConfigureCompilerIsGenericBinary(const WasmEdge_ConfigureContext *Cxt);

/// Set the interruptible option of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param IsInterruptible the boolean value to determine to generate
/// interruptible binary or not when compilation in AOT compiler.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureCompilerSetInterruptible(WasmEdge_ConfigureContext *Cxt,
                                           const bool IsInterruptible);

/// Get the interruptible option of the AOT compiler.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to generate interruptible binary or
/// not when compilation in AOT compiler.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ConfigureCompilerIsInterruptible(const WasmEdge_ConfigureContext *Cxt);

/// Set the instruction counting option for the statistics.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param IsCount the boolean value to determine to support instruction
/// counting when execution or not after compilation by the AOT compiler.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureStatisticsSetInstructionCounting(
    WasmEdge_ConfigureContext *Cxt, const bool IsCount);

/// Get the instruction counting option for the statistics.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to support instruction counting when
/// execution or not after compilation by the AOT compiler.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_ConfigureStatisticsIsInstructionCounting(
    const WasmEdge_ConfigureContext *Cxt);

/// Set the cost measuring option for the statistics.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param IsMeasure the boolean value to determine to support cost measuring
/// when execution or not after compilation by the AOT compiler.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureStatisticsSetCostMeasuring(WasmEdge_ConfigureContext *Cxt,
                                             const bool IsMeasure);

/// Get the cost measuring option for the statistics.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to support cost measuring when
/// execution or not after compilation by the AOT compiler.
WASMEDGE_CAPI_EXPORT extern bool WasmEdge_ConfigureStatisticsIsCostMeasuring(
    const WasmEdge_ConfigureContext *Cxt);

/// Set the time measuring option for the statistics.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to set the boolean value.
/// \param IsMeasure the boolean value to determine to support time when
/// execution or not after compilation by the AOT compiler.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureStatisticsSetTimeMeasuring(WasmEdge_ConfigureContext *Cxt,
                                             const bool IsMeasure);

/// Get the time measuring option for the statistics.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to get the boolean value.
///
/// \returns the boolean value to determine to support time measuring when
/// execution or not after compilation by the AOT compiler.
WASMEDGE_CAPI_EXPORT extern bool WasmEdge_ConfigureStatisticsIsTimeMeasuring(
    const WasmEdge_ConfigureContext *Cxt);

/// Deletion of the WasmEdge_ConfigureContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ConfigureContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ConfigureDelete(WasmEdge_ConfigureContext *Cxt);

// <<<<<<<< WasmEdge configure functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge statistics functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_StatisticsContext.
///
/// The caller owns the object and should call `WasmEdge_StatisticsDelete` to
/// destroy it.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_StatisticsContext *
WasmEdge_StatisticsCreate(void);

/// Get the instruction count in execution.
///
/// \param Cxt the WasmEdge_StatisticsContext to get data.
///
/// \returns the instruction count in total execution.
WASMEDGE_CAPI_EXPORT extern uint64_t
WasmEdge_StatisticsGetInstrCount(const WasmEdge_StatisticsContext *Cxt);

/// Get the instruction count per second in execution.
///
/// \param Cxt the WasmEdge_StatisticsContext to get data.
///
/// \returns the instruction count per second.
WASMEDGE_CAPI_EXPORT extern double
WasmEdge_StatisticsGetInstrPerSecond(const WasmEdge_StatisticsContext *Cxt);

/// Get the total cost in execution.
///
/// \param Cxt the WasmEdge_StatisticsContext to get data.
///
/// \returns the total cost.
WASMEDGE_CAPI_EXPORT extern uint64_t
WasmEdge_StatisticsGetTotalCost(const WasmEdge_StatisticsContext *Cxt);

/// Set the costs of instructions.
///
/// \param Cxt the WasmEdge_StatisticsContext to set the cost table.
/// \param CostArr the cost table array.
/// \param Len the length of the cost table array.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_StatisticsSetCostTable(WasmEdge_StatisticsContext *Cxt,
                                uint64_t *CostArr, const uint32_t Len);

/// Set the cost limit in execution.
///
/// The WASM execution will be aborted if the instruction costs exceeded the
/// limit and the ErrCode::Value::CostLimitExceeded will be returned.
///
/// \param Cxt the WasmEdge_StatisticsContext to set the cost table.
/// \param Limit the cost limit.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_StatisticsSetCostLimit(WasmEdge_StatisticsContext *Cxt,
                                const uint64_t Limit);

/// Clear all data in the WasmEdge_StatisticsContext.
///
/// \param Cxt the WasmEdge_StatisticsContext to clear.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_StatisticsClear(WasmEdge_StatisticsContext *Cxt);

/// Deletion of the WasmEdge_StatisticsContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_StatisticsContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_StatisticsDelete(WasmEdge_StatisticsContext *Cxt);

// <<<<<<<< WasmEdge statistics functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_CONFIGURE_H
