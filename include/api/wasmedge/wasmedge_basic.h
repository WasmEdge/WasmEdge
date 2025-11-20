// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_basic.h - WasmEdge C API ------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the basic definitions and data structures of WasmEdge C
/// API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_BASIC_H
#define WASMEDGE_C_API_BASIC_H

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#ifdef WASMEDGE_COMPILE_LIBRARY
#define WASMEDGE_CAPI_EXPORT __declspec(dllexport)
#else
#define WASMEDGE_CAPI_EXPORT __declspec(dllimport)
#endif // WASMEDGE_COMPILE_LIBRARY
#ifdef WASMEDGE_PLUGIN
#define WASMEDGE_CAPI_PLUGIN_EXPORT __declspec(dllexport)
#else
#define WASMEDGE_CAPI_PLUGIN_EXPORT __declspec(dllimport)
#endif // WASMEDGE_PLUGIN
#else
#define WASMEDGE_CAPI_EXPORT __attribute__((visibility("default")))
#define WASMEDGE_CAPI_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif // _WIN32

#if !defined(__cplusplus) &&                                                   \
    (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
#include <stdbool.h>
#endif
#include <stdint.h>
#include <time.h>

#include "wasmedge/wasmedge_context.h"

#include "wasmedge/enum_configure.h"
#include "wasmedge/enum_errcode.h"
#include "wasmedge/enum_types.h"
#include "wasmedge/int128.h"
#include "wasmedge/version.h"

/// WasmEdge string struct.
typedef struct WasmEdge_String {
  uint32_t Length;
  const char *Buf;
} WasmEdge_String;

/// WasmEdge bytes struct.
typedef struct WasmEdge_Bytes {
  uint32_t Length;
  const uint8_t *Buf;
} WasmEdge_Bytes;

/// WasmEdge result struct.
typedef struct WasmEdge_Result {
  uint32_t Code;
} WasmEdge_Result;

#ifdef __cplusplus
#define WasmEdge_Result_Success (WasmEdge_Result{/* Code */ 0x00})
#define WasmEdge_Result_Terminate (WasmEdge_Result{/* Code */ 0x01})
#define WasmEdge_Result_Fail (WasmEdge_Result{/* Code */ 0x02})
#else
#define WasmEdge_Result_Success ((WasmEdge_Result){.Code = 0x00})
#define WasmEdge_Result_Terminate ((WasmEdge_Result){.Code = 0x01})
#define WasmEdge_Result_Fail ((WasmEdge_Result){.Code = 0x02})
#endif

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge version functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Get the version string of the WasmEdge C API.
///
/// The returned string must __NOT__ be destroyed.
///
/// \returns NULL-terminated C string of version.
WASMEDGE_CAPI_EXPORT extern const char *WasmEdge_VersionGet(void);

/// Get the major version value of the WasmEdge C API.
///
/// \returns Value of the major version.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_VersionGetMajor(void);

/// Get the minor version value of the WasmEdge C API.
///
/// \returns Value of the minor version.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_VersionGetMinor(void);

/// Get the patch version value of the WasmEdge C API.
///
/// \returns Value of the patch version.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_VersionGetPatch(void);

// <<<<<<<< WasmEdge version functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge logging functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Set the logging system to filter to error level.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_LogSetErrorLevel(void);

/// Set the logging system to filter to debug level.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_LogSetDebugLevel(void);

/// Set the logging system off.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_LogOff(void);

typedef enum WasmEdge_LogLevel {
  WasmEdge_LogLevel_Trace,
  WasmEdge_LogLevel_Debug,
  WasmEdge_LogLevel_Info,
  WasmEdge_LogLevel_Warn,
  WasmEdge_LogLevel_Error,
  WasmEdge_LogLevel_Critical,
} WasmEdge_LogLevel;

WASMEDGE_CAPI_EXPORT extern void WasmEdge_LogSetLevel(WasmEdge_LogLevel Level);

typedef struct WasmEdge_LogMessage {
  WasmEdge_String Message;
  WasmEdge_String LoggerName;
  WasmEdge_LogLevel Level;
  time_t Time;
  uint64_t ThreadId;
} WasmEdge_LogMessage;

typedef void (*WasmEdge_LogCallback_t)(const WasmEdge_LogMessage *Message);

WASMEDGE_CAPI_EXPORT extern void
WasmEdge_LogSetCallback(WasmEdge_LogCallback_t Callback);

// <<<<<<<< WasmEdge logging functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge string functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_String with the C string.
///
/// The caller owns the object and should call `WasmEdge_StringDelete` to
/// destroy it. This function only supports the C string with NULL termination.
/// If the input string may have `\0` character, please use the
/// `WasmEdge_StringCreateByBuffer` instead.
///
/// \param Str the NULL-terminated C string to copy into the WasmEdge_String
/// object.
///
/// \returns string object. Length will be 0 and Buf will be NULL if failed or
/// the input string is a NULL.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_StringCreateByCString(const char *Str);

/// Creation of the WasmEdge_String with the buffer and its length.
///
/// The caller owns the object and should call `WasmEdge_StringDelete` to
/// destroy it.
///
/// \param Buf the buffer to wrap to the WasmEdge_String object.
/// \param Len the buffer length.
///
/// \returns string object. Length will be 0 and Buf will be NULL if failed or
/// the input buffer is a NULL.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_StringCreateByBuffer(const char *Buf, const uint32_t Len);

/// Create the WasmEdge_String wraps to the buffer.
///
/// This function creates a `WasmEdge_String` object which wraps to the input
/// buffer. The caller should guarantee the life cycle of the input buffer, and
/// should __NOT__ call the `WasmEdge_StringDelete`.
///
/// \param Buf the buffer to copy into the WasmEdge_String object.
/// \param Len the buffer length.
///
/// \returns string object refer to the input buffer with its length.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_StringWrap(const char *Buf, const uint32_t Len);

/// Compare the two WasmEdge_String objects.
///
/// \param Str1 the first WasmEdge_String object to compare.
/// \param Str2 the second WasmEdge_String object to compare.
///
/// \returns true if the content of two WasmEdge_String objects are the same,
/// false if not.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_StringIsEqual(const WasmEdge_String Str1, const WasmEdge_String Str2);

/// Copy the content of WasmEdge_String object to the buffer.
///
/// This function copy at most `Len` characters from the `WasmEdge_String`
/// object to the destination buffer. If the string length is less than `Len`
/// characters long, the remainder of the buffer is filled with `\0' characters.
/// Otherwise, the destination is not terminated.
///
/// \param Str the source WasmEdge_String object to copy.
/// \param Buf the buffer to fill the string content.
/// \param Len the buffer length.
///
/// \returns the copied length of string.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_StringCopy(const WasmEdge_String Str, char *Buf, const uint32_t Len);

/// Deletion of the WasmEdge_String.
///
/// After calling this function, the resources in the WasmEdge_String object
/// will be released and the object should __NOT__ be used.
///
/// \param Str the WasmEdge_String object to destroy.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_StringDelete(WasmEdge_String Str);

// <<<<<<<< WasmEdge string functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge bytes functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_Bytes with the buffer and its length.
///
/// The caller owns the object and should call `WasmEdge_BytesDelete` to destroy
/// it.
///
/// \param Buf the buffer to copy into the WasmEdge_Bytes object.
/// \param Len the buffer length.
///
/// \returns bytes object. Length will be 0 and Buf will be NULL if failed or
/// the input buffer is a NULL.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Bytes
WasmEdge_BytesCreate(const uint8_t *Buf, const uint32_t Len);

/// Create the WasmEdge_Bytes wraps to the buffer.
///
/// This function creates a `WasmEdge_Bytes` object which wraps to the input
/// buffer. The caller should guarantee the life cycle of the input buffer, and
/// should __NOT__ call the `WasmEdge_BytesDelete`.
///
/// \param Buf the buffer to wrap to the WasmEdge_Bytes object.
/// \param Len the buffer length.
///
/// \returns bytes object refer to the input buffer with its length.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Bytes
WasmEdge_BytesWrap(const uint8_t *Buf, const uint32_t Len);

/// Deletion of the WasmEdge_Bytes.
///
/// After calling this function, the resources in the WasmEdge_Bytes object
/// will be released and the object should __NOT__ be used.
///
/// \param Bytes the WasmEdge_Bytes object to destroy.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_BytesDelete(WasmEdge_Bytes Bytes);

// <<<<<<<< WasmEdge bytes functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge result functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Check the result is a success or not.
///
/// \param Res the WasmEdge_Result struct.
///
/// \returns true if the error code is WasmEdge_Result_Success or
/// WasmEdge_Result_Terminate, false for others.
WASMEDGE_CAPI_EXPORT extern bool WasmEdge_ResultOK(const WasmEdge_Result Res);

/// Generate the result with code.
///
/// \param Category the WasmEdge_ErrCategory to specify the error category.
/// \param Code the 24-bit length error code. The data exceeds 24 bits will be
/// stripped.
///
/// \returns WasmEdge_Result struct with the given data.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_ResultGen(const enum WasmEdge_ErrCategory Category,
                   const uint32_t Code);

/// Get the result code.
///
/// \param Res the WasmEdge_Result struct.
///
/// \returns result code (24-bit size data) in the WasmEdge_Result struct.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ResultGetCode(const WasmEdge_Result Res);

/// Get the error category.
///
/// \param Res the WasmEdge_Result struct.
///
/// \returns error category in the WasmEdge_Result struct.
WASMEDGE_CAPI_EXPORT extern enum WasmEdge_ErrCategory
WasmEdge_ResultGetCategory(const WasmEdge_Result Res);

/// Get the result message.
///
/// The returned string must __NOT__ be destroyed.
/// If the error category of the result is __NOT__ `WasmEdge_ErrCategory_WASM`,
/// the message will always be "user defined error code".
///
/// \param Res the WasmEdge_Result struct.
///
/// \returns NULL-terminated C string of the corresponding error message.
WASMEDGE_CAPI_EXPORT extern const char *
WasmEdge_ResultGetMessage(const WasmEdge_Result Res);

// <<<<<<<< WasmEdge result functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_BASIC_H
