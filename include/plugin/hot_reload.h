// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugin/hot_reload.h - Plugin Hot-Reload Manager ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the HotReloadManager class for
/// dynamically loading, unloading, and reloading plugins at runtime without
/// restarting the WasmEdge host process.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/filesystem.h"
#include "plugin/plugin.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace Plugin {

using PluginEventCallback =
    std::function<void(const std::string &PluginName,
                       const std::filesystem::path &Path)>;

enum class PluginEvent {
  BeforeUnload,    ///< Called before a plugin is unloaded.
  AfterUnload,     ///< Called after a plugin is unloaded.
  BeforeLoad,      ///< Called before a plugin is loaded.
  AfterLoad,       ///< Called after a plugin is loaded.
  BeforeReload,    ///< Called before a plugin is reloaded.
  AfterReload,     ///< Called after a plugin is reloaded.
  LoadFailed,      ///< Called when plugin loading fails.
  UnloadFailed,    ///< Called when plugin unloading fails.
  WatchStarted,    ///< Called when file watching starts.
  WatchStopped,    ///< Called when file watching stops.
  FileChanged,     ///< Called when a watched file changes.
};

enum class PluginState {
  Unknown,    ///< Unknown state.
  Loaded,     ///< Plugin is loaded and ready.
  Unloaded,   ///< Plugin has been unloaded.
  Loading,    ///< Plugin is being loaded.
  Unloading,  ///< Plugin is being unloaded.
  Reloading,  ///< Plugin is being reloaded.
  Error,      ///< Plugin is in an error state.
};

struct WatchedPluginInfo {
  std::filesystem::path Path;
  std::filesystem::file_time_type LastModified;
  PluginState State;
  uint64_t LoadCount;        ///< Number of times the plugin has been loaded.
  uint64_t ReloadCount;      ///< Number of times the plugin has been reloaded.
  std::string LastError;     ///< Last error message if any.
  bool AutoReload;           ///< Whether to automatically reload on changes.
};

/// Configuration for hot-reload behavior.
struct HotReloadConfig {
  /// Enable file watching for automatic reload.
  bool EnableFileWatching = true;
  
  /// Interval for polling file changes (milliseconds).
  std::chrono::milliseconds WatchInterval{1000};
  
  /// Debounce delay to prevent multiple rapid reloads (milliseconds).
  std::chrono::milliseconds DebounceDelay{500};
  
  /// Maximum retry count for failed reloads.
  uint32_t MaxRetryCount = 3;
  
  /// Delay between retry attempts (milliseconds).
  std::chrono::milliseconds RetryDelay{1000};
  
  /// Whether to enable automatic reload on file changes.
  bool AutoReloadOnChange = true;
  
  /// Whether to backup the old plugin before reload.
  bool BackupBeforeReload = false;
  
  /// Directory for plugin backups (if BackupBeforeReload is true).
  std::filesystem::path BackupDirectory;
};

/// Hot-Reload Manager for WasmEdge plugins.
///
/// This class provides functionality to dynamically load, unload, and reload
/// plugins at runtime. It supports file watching for automatic reloading
/// when plugin files change on disk.
///
/// Example usage:
/// @code
/// auto& manager = HotReloadManager::getInstance();
/// 
/// // Register callbacks for lifecycle events
/// manager.registerCallback(PluginEvent::BeforeReload, 
///     [](const std::string& name, const auto& path) {
///         std::cout << "Reloading plugin: " << name << std::endl;
///     });
///
/// // Start watching a plugin directory
/// manager.startWatching("/path/to/plugins");
///
/// // Manually reload a specific plugin
/// if (manager.reloadPlugin("my_plugin")) {
///     std::cout << "Plugin reloaded successfully" << std::endl;
/// }
///
/// // Unload a plugin
/// manager.unloadPlugin("my_plugin");
///
/// // Stop watching
/// manager.stopWatching();
/// @endcode
class HotReloadManager {
public:
  HotReloadManager(const HotReloadManager &) = delete;
  HotReloadManager &operator=(const HotReloadManager &) = delete;
  HotReloadManager(HotReloadManager &&) = delete;
  HotReloadManager &operator=(HotReloadManager &&) = delete;

  /// Get the singleton instance of the HotReloadManager.
  WASMEDGE_EXPORT static HotReloadManager &getInstance() noexcept;

  /// Configure the hot-reload manager.
  /// \param Config the configuration to apply.
  WASMEDGE_EXPORT void configure(const HotReloadConfig &Config) noexcept;

  /// Get the current configuration.
  /// \returns the current configuration.
  WASMEDGE_EXPORT const HotReloadConfig &getConfig() const noexcept;

  /// Start watching plugin paths for changes.
  /// This will monitor files and directories for changes and automatically
  /// reload plugins when changes are detected.
  /// \param Paths the paths to watch (files or directories).
  /// \returns true if watching started successfully.
  WASMEDGE_EXPORT bool
  startWatching(const std::vector<std::filesystem::path> &Paths) noexcept;

  /// Start watching a single path for changes.
  /// \param Path the path to watch (file or directory).
  /// \returns true if watching started successfully.
  WASMEDGE_EXPORT bool startWatching(const std::filesystem::path &Path) noexcept;

  /// Stop watching all paths.
  WASMEDGE_EXPORT void stopWatching() noexcept;

  /// Check if currently watching any paths.
  /// \returns true if file watching is active.
  WASMEDGE_EXPORT bool isWatching() const noexcept;

  /// Get list of currently watched paths.
  /// \returns vector of watched paths.
  WASMEDGE_EXPORT std::vector<std::filesystem::path> getWatchedPaths() const noexcept;

  /// Load a plugin from the given path.
  /// \param Path the path to the plugin file.
  /// \returns true if the plugin was loaded successfully.
  WASMEDGE_EXPORT bool loadPlugin(const std::filesystem::path &Path) noexcept;

  /// Unload a plugin by name.
  /// This will unload the plugin and free its resources.
  /// \param Name the name of the plugin to unload.
  /// \returns true if the plugin was unloaded successfully.
  WASMEDGE_EXPORT bool unloadPlugin(std::string_view Name) noexcept;

  /// Unload a plugin by path.
  /// \param Path the path of the plugin to unload.
  /// \returns true if the plugin was unloaded successfully.
  WASMEDGE_EXPORT bool unloadPluginByPath(const std::filesystem::path &Path) noexcept;

  /// Reload a plugin by name.
  /// This will unload the current version and load the new version.
  /// \param Name the name of the plugin to reload.
  /// \returns true if the plugin was reloaded successfully.
  WASMEDGE_EXPORT bool reloadPlugin(std::string_view Name) noexcept;

  /// Reload a plugin by path.
  /// \param Path the path of the plugin to reload.
  /// \returns true if the plugin was reloaded successfully.
  WASMEDGE_EXPORT bool reloadPluginByPath(const std::filesystem::path &Path) noexcept;

  /// Reload all plugins that have changed on disk.
  /// \returns number of plugins reloaded.
  WASMEDGE_EXPORT uint32_t reloadChangedPlugins() noexcept;

  /// Unload all plugins.
  /// \returns number of plugins unloaded.
  WASMEDGE_EXPORT uint32_t unloadAllPlugins() noexcept;

  /// Get the state of a plugin by name.
  /// \param Name the name of the plugin.
  /// \returns the state of the plugin.
  WASMEDGE_EXPORT PluginState getPluginState(std::string_view Name) const noexcept;

  /// Get information about a watched plugin.
  /// \param Name the name of the plugin.
  /// \returns optional containing plugin info if found.
  WASMEDGE_EXPORT std::optional<WatchedPluginInfo>
  getPluginInfo(std::string_view Name) const noexcept;

  /// Get list of all plugin names currently managed.
  /// \returns vector of plugin names.
  WASMEDGE_EXPORT std::vector<std::string> getManagedPluginNames() const noexcept;

  /// Register a callback for plugin lifecycle events.
  /// \param Event the event type to listen for.
  /// \param Callback the callback function.
  /// \returns callback ID for later removal.
  WASMEDGE_EXPORT uint64_t registerCallback(PluginEvent Event,
                                             PluginEventCallback Callback) noexcept;

  /// Unregister a callback by ID.
  /// \param CallbackId the ID returned from registerCallback.
  /// \returns true if the callback was found and removed.
  WASMEDGE_EXPORT bool unregisterCallback(uint64_t CallbackId) noexcept;

  /// Clear all registered callbacks for an event type.
  /// \param Event the event type.
  WASMEDGE_EXPORT void clearCallbacks(PluginEvent Event) noexcept;

  /// Clear all registered callbacks.
  WASMEDGE_EXPORT void clearAllCallbacks() noexcept;

  /// Check if a plugin file has been modified since last load.
  /// \param Name the name of the plugin.
  /// \returns true if the plugin file has been modified.
  WASMEDGE_EXPORT bool hasPluginChanged(std::string_view Name) const noexcept;

  /// Get statistics about hot-reload operations.
  struct Statistics {
    uint64_t TotalLoads = 0;
    uint64_t TotalUnloads = 0;
    uint64_t TotalReloads = 0;
    uint64_t FailedLoads = 0;
    uint64_t FailedUnloads = 0;
    uint64_t FailedReloads = 0;
    uint64_t FileChangeEvents = 0;
  };

  /// Get current statistics.
  /// \returns current statistics.
  WASMEDGE_EXPORT Statistics getStatistics() const noexcept;

  /// Reset statistics counters.
  WASMEDGE_EXPORT void resetStatistics() noexcept;

private:
  HotReloadManager() noexcept;
  ~HotReloadManager() noexcept;

  /// Internal implementation details.
  struct Impl;
  std::unique_ptr<Impl> PImpl;
};

} // namespace Plugin
} // namespace WasmEdge
