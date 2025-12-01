// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_plugin.h - WasmEdge C API -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions and data structures about plugins in
/// WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_PLUGIN_H
#define WASMEDGE_C_API_PLUGIN_H

#include "wasmedge/wasmedge_basic.h"

/// Type of option value.
typedef enum WasmEdge_ProgramOptionType {
  /// No option value.
  WasmEdge_ProgramOptionType_None,
  /// Boolean value.
  WasmEdge_ProgramOptionType_Toggle,
  WasmEdge_ProgramOptionType_Int8,
  WasmEdge_ProgramOptionType_Int16,
  WasmEdge_ProgramOptionType_Int32,
  WasmEdge_ProgramOptionType_Int64,
  WasmEdge_ProgramOptionType_UInt8,
  WasmEdge_ProgramOptionType_UInt16,
  WasmEdge_ProgramOptionType_UInt32,
  WasmEdge_ProgramOptionType_UInt64,
  WasmEdge_ProgramOptionType_Float,
  WasmEdge_ProgramOptionType_Double,
  /// WasmEdge_String.
  WasmEdge_ProgramOptionType_String,
} WasmEdge_ProgramOptionType;

/// Program option for plugins.
typedef struct WasmEdge_ProgramOption {
  const char *Name;
  const char *Description;
  WasmEdge_ProgramOptionType Type;
  void *Storage;
  const void *DefaultValue;
} WasmEdge_ProgramOption;

/// Module descriptor for plugins.
typedef struct WasmEdge_ModuleDescriptor {
  const char *Name;
  const char *Description;
  WasmEdge_ModuleInstanceContext *(*Create)(
      const struct WasmEdge_ModuleDescriptor *);
} WasmEdge_ModuleDescriptor;

/// Version data for plugins.
typedef struct WasmEdge_PluginVersionData {
  uint32_t Major;
  uint32_t Minor;
  uint32_t Patch;
  uint32_t Build;
} WasmEdge_PluginVersionData;

/// Plugin descriptor for plugins.
typedef struct WasmEdge_PluginDescriptor {
  const char *Name;
  const char *Description;
  uint32_t APIVersion;
  WasmEdge_PluginVersionData Version;
  uint32_t ModuleCount;
  uint32_t ProgramOptionCount;
  WasmEdge_ModuleDescriptor *ModuleDescriptions;
  WasmEdge_ProgramOption *ProgramOptions;
} WasmEdge_PluginDescriptor;

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge Plugin functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Load plugins with the default search paths.
///
/// The default paths are:
///   1. The environment variable "WASMEDGE_PLUGIN_PATH".
///   2. The "../plugin/" directory related to the WasmEdge installation path.
///   3. The "wasmedge/" directory under the library path if the WasmEdge is
///      installed under the "/usr".
WASMEDGE_CAPI_EXPORT extern void WasmEdge_PluginLoadWithDefaultPaths(void);

/// Load the plugin with the given file or directory.
///
/// For the given file path, this function will load the plug-in.
/// For the given directory path, this function will load the plug-ins under the
/// directory recursively.
///
/// \param Path the path to plug-in file or directory.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_PluginLoadFromPath(const char *Path);

/// Get the length of loaded plug-in list.
///
/// \returns length of loaded plug-in list.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_PluginListPluginsLength(void);

/// List the loaded plug-ins with their names.
///
/// The returned plug-in names filled into the `Names` array are owned by the
/// internal WasmEdge plug-in storage, and the caller should __NOT__ call the
/// `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the loaded
/// plug-in list size, the overflowed return values will be discarded.
///
/// \param [out] Names the output WasmEdge_String buffer of the function names.
/// \param Len the buffer length.
///
/// \returns actual loaded plug-in list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_PluginListPlugins(WasmEdge_String *Names, const uint32_t Len);

/// Find the loaded plug-in context by name.
///
/// After loading the plug-ins from default paths or the given path, developers
/// can use this API to retrieve the plug-in context by name. Then developers
/// can create the module instance from the plug-in contexts.
///
/// \param Name the plug-in name WasmEdge_String.
///
/// \returns pointer to the plug-in context. NULL if the plug-in not found.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_PluginContext *
WasmEdge_PluginFind(const WasmEdge_String Name);

/// Get the plug-in name of the plug-in context.
///
/// The returned string object is linked to the plug-in name of the plug-in
/// context, and the caller should __NOT__ call the `WasmEdge_StringDelete`.
///
/// \param Cxt the WasmEdge_PluginContext.
///
/// \returns string object. Length will be 0 and Buf will be NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_PluginGetPluginName(const WasmEdge_PluginContext *Cxt);

/// Get the length of module list in the plug-in context.
///
/// There may be several modules in a plug-in. Developers can use this function
/// to get the length of the module list in a plug-in.
///
/// \param Cxt the WasmEdge_PluginContext to get the length of the module list.
///
/// \returns length of module list.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_PluginListModuleLength(const WasmEdge_PluginContext *Cxt);

/// List the modules in the plug-in context with their names.
///
/// The returned module names filled into the `Names` array are owned by the
/// internal WasmEdge plug-in storage, and the caller should __NOT__ call the
/// `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the loaded
/// plug-in list size, the overflowed return values will be discarded.
///
/// \param Cxt the WasmEdge_PluginContext to list the modules.
/// \param [out] Names the output WasmEdge_String buffer of the function names.
/// \param Len the buffer length.
///
/// \returns actual module list size of the plug-in.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_PluginListModule(const WasmEdge_PluginContext *Cxt,
                          WasmEdge_String *Names, const uint32_t Len);

/// Create the module instance in the plug-in by the module name.
///
/// By giving the module name, developers can retrieve the module in the plug-in
/// and create the module instance.
/// The caller owns the object and should call `WasmEdge_ModuleInstanceDelete`
/// to destroy it.
///
/// \param Cxt the WasmEdge_PluginContext to retrieve and create module.
/// \param ModuleName the module name to retrieve.
///
/// \returns pointer to the module instance context, NULL if the module name not
/// found in the plug-in or the plug-in is not valid.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ModuleInstanceContext *
WasmEdge_PluginCreateModule(const WasmEdge_PluginContext *Cxt,
                            const WasmEdge_String ModuleName);

/// Initialize the wasi_nn plug-in.
///
/// This function will initialize the wasi_nn plug-in with the preloads string
/// list. Only available after loading the wasi_nn plug-in and before creating
/// the module instance from the plug-in.
///
/// \param NNPreloads the preload string list. NULL if the length is 0.
/// \param PreloadsLen the length of the preload list.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_PluginInitWASINN(const char *const *NNPreloads,
                          const uint32_t PreloadsLen);

/// Implement by plugins for returning the plugin descriptor.
///
/// \returns the plugin descriptor.
WASMEDGE_CAPI_PLUGIN_EXPORT extern const WasmEdge_PluginDescriptor *
WasmEdge_Plugin_GetDescriptor(void);

// <<<<<<<< WasmEdge Pluginfunctions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_PLUGIN_H
