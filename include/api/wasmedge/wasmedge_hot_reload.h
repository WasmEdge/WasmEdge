// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/wasmedge_hot_reload.h - WasmEdge Hot-Reload C API --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the C API functions for plugin hot-reloading support
/// in WasmEdge. These functions allow dynamic loading, unloading, and
/// reloading of plugins at runtime without restarting the host process.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_HOT_RELOAD_H
#define WASMEDGE_C_API_HOT_RELOAD_H

#include "wasmedge/wasmedge_basic.h"

typedef enum WasmEdge_PluginState {
  WasmEdge_PluginState_Unknown = 0,
  WasmEdge_PluginState_Loaded = 1,
  WasmEdge_PluginState_Unloaded = 2,
  WasmEdge_PluginState_Loading = 3,
  WasmEdge_PluginState_Unloading = 4,
  WasmEdge_PluginState_Reloading = 5,
  WasmEdge_PluginState_Error = 6,
} WasmEdge_PluginState;

typedef enum WasmEdge_PluginEvent {
  WasmEdge_PluginEvent_BeforeUnload = 0,
  WasmEdge_PluginEvent_AfterUnload = 1,
  WasmEdge_PluginEvent_BeforeLoad = 2,
  WasmEdge_PluginEvent_AfterLoad = 3,
  WasmEdge_PluginEvent_BeforeReload = 4,
  WasmEdge_PluginEvent_AfterReload = 5,
  WasmEdge_PluginEvent_LoadFailed = 6,
  WasmEdge_PluginEvent_UnloadFailed = 7,
  WasmEdge_PluginEvent_WatchStarted = 8,
  WasmEdge_PluginEvent_WatchStopped = 9,
  WasmEdge_PluginEvent_FileChanged = 10,
} WasmEdge_PluginEvent;

typedef struct WasmEdge_HotReloadConfig {
  bool EnableFileWatching;
  uint32_t WatchIntervalMs;
  uint32_t DebounceDelayMs;
  uint32_t MaxRetryCount;
  uint32_t RetryDelayMs;
  bool AutoReloadOnChange;
} WasmEdge_HotReloadConfig;

typedef struct WasmEdge_HotReloadStatistics {
  uint64_t TotalLoads;
  uint64_t TotalUnloads;
  uint64_t TotalReloads;
  uint64_t FailedLoads;
  uint64_t FailedUnloads;
  uint64_t FailedReloads;
  uint64_t FileChangeEvents;
} WasmEdge_HotReloadStatistics;

/// Plugin event callback function type.
/// \param PluginName the name of the plugin (may be empty for some events).
/// \param Path the path to the plugin file.
/// \param UserData user-provided data pointer.
typedef void (*WasmEdge_PluginEventCallback)(const char *PluginName,
                                              const char *Path, void *UserData);

#ifdef __cplusplus
extern "C" {
#endif

/// Get the default hot-reload configuration.
///
/// \returns the default configuration with sensible defaults.
WASMEDGE_CAPI_EXPORT extern WasmEdge_HotReloadConfig
WasmEdge_HotReloadConfigCreate(void);

/// Configure the hot-reload manager.
///
/// \param Config the configuration to apply.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_HotReloadConfigure(const WasmEdge_HotReloadConfig *Config);

/// Start watching plugin paths for changes.
///
/// This will monitor the specified paths for file changes and automatically
/// reload plugins when changes are detected (if AutoReloadOnChange is enabled).
///
/// \param Paths array of paths to watch (files or directories).
/// \param PathCount number of paths in the array.
///
/// \returns true if watching started successfully.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_HotReloadStartWatching(const char *const *Paths, uint32_t PathCount);

/// Start watching a single path for changes.
///
/// \param Path the path to watch (file or directory).
///
/// \returns true if watching started successfully.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_HotReloadStartWatchingPath(const char *Path);

/// Stop watching all paths for changes.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_HotReloadStopWatching(void);

/// Check if file watching is currently active.
///
/// \returns true if watching is active.
WASMEDGE_CAPI_EXPORT extern bool WasmEdge_HotReloadIsWatching(void);

/// Unload a plugin by name.
///
/// This will unload the plugin and free its resources. After unloading,
/// the plugin can no longer be used until it is loaded again.
///
/// \param Name the name of the plugin to unload.
///
/// \returns true if the plugin was found and unloaded successfully.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_PluginUnload(const WasmEdge_String Name);

/// Unload a plugin by file path.
///
/// \param Path the path to the plugin file.
///
/// \returns true if the plugin was found and unloaded successfully.
WASMEDGE_CAPI_EXPORT extern bool WasmEdge_PluginUnloadByPath(const char *Path);

/// Reload a plugin by name.
///
/// This will unload the current version of the plugin and load the new
/// version from the same file path. Any existing references to module
/// instances from this plugin will become invalid.
///
/// \param Name the name of the plugin to reload.
///
/// \returns true if the plugin was reloaded successfully.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_PluginReload(const WasmEdge_String Name);

/// Reload a plugin by file path.
///
/// \param Path the path to the plugin file.
///
/// \returns true if the plugin was reloaded successfully.
WASMEDGE_CAPI_EXPORT extern bool WasmEdge_PluginReloadByPath(const char *Path);

/// Reload all plugins that have changed on disk.
///
/// This function checks all loaded plugins for file modifications and
/// reloads any that have changed.
///
/// \returns the number of plugins that were reloaded.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_PluginReloadChanged(void);

/// Unload all plugins.
///
/// This will unload all loaded plugins (except built-in plugins).
///
/// \returns the number of plugins that were unloaded.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_PluginUnloadAll(void);

/// Check if a plugin is currently loaded.
///
/// \param Name the name of the plugin to check.
///
/// \returns true if the plugin is loaded.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_PluginIsLoaded(const WasmEdge_String Name);

/// Get the state of a plugin.
///
/// \param Name the name of the plugin.
///
/// \returns the current state of the plugin.
WASMEDGE_CAPI_EXPORT extern WasmEdge_PluginState
WasmEdge_PluginGetState(const WasmEdge_String Name);

/// Check if a plugin file has been modified since it was loaded.
///
/// \param Name the name of the plugin to check.
///
/// \returns true if the plugin file has been modified.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_PluginHasChanged(const WasmEdge_String Name);

/// Get the path of a loaded plugin.
///
/// \param Name the name of the plugin.
/// \param PathBuf buffer to store the path.
/// \param BufLen length of the buffer.
///
/// \returns the length of the path, or 0 if the plugin is not found.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_PluginGetPath(const WasmEdge_String Name, char *PathBuf,
                       uint32_t BufLen);

/// Register a callback for plugin lifecycle events.
///
/// The callback will be invoked when the specified event occurs. The callback
/// receives the plugin name, file path, and user data.
///
/// \param Event the event type to listen for.
/// \param Callback the callback function.
/// \param UserData user-provided data to pass to the callback.
///
/// \returns a callback ID that can be used to unregister the callback.
WASMEDGE_CAPI_EXPORT extern uint64_t WasmEdge_HotReloadRegisterCallback(
    WasmEdge_PluginEvent Event, WasmEdge_PluginEventCallback Callback,
    void *UserData);

/// Unregister a callback by ID.
///
/// \param CallbackId the ID returned from WasmEdge_HotReloadRegisterCallback.
///
/// \returns true if the callback was found and removed.
WASMEDGE_CAPI_EXPORT extern bool
WasmEdge_HotReloadUnregisterCallback(uint64_t CallbackId);

/// Clear all registered callbacks for an event type.
///
/// \param Event the event type.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_HotReloadClearCallbacks(WasmEdge_PluginEvent Event);

/// Clear all registered callbacks.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_HotReloadClearAllCallbacks(void);

/// Get current hot-reload statistics.
///
/// \param Stats pointer to a statistics structure to fill.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_HotReloadGetStatistics(WasmEdge_HotReloadStatistics *Stats);

/// Reset hot-reload statistics counters.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_HotReloadResetStatistics(void);

#ifdef __cplusplus
}
#endif

#endif
