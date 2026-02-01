// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge_hot_reload.h"
#include "plugin/hot_reload.h"
#include "plugin/plugin.h"

#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using namespace WasmEdge;

inline std::string_view genStrView(const WasmEdge_String S) noexcept {
  return std::string_view(S.Buf, S.Length);
}

inline WasmEdge_PluginState
toPluginState(Plugin::PluginState State) noexcept {
  switch (State) {
  case Plugin::PluginState::Unknown:
    return WasmEdge_PluginState_Unknown;
  case Plugin::PluginState::Loaded:
    return WasmEdge_PluginState_Loaded;
  case Plugin::PluginState::Unloaded:
    return WasmEdge_PluginState_Unloaded;
  case Plugin::PluginState::Loading:
    return WasmEdge_PluginState_Loading;
  case Plugin::PluginState::Unloading:
    return WasmEdge_PluginState_Unloading;
  case Plugin::PluginState::Reloading:
    return WasmEdge_PluginState_Reloading;
  case Plugin::PluginState::Error:
    return WasmEdge_PluginState_Error;
  default:
    return WasmEdge_PluginState_Unknown;
  }
}

inline Plugin::PluginEvent toPluginEvent(WasmEdge_PluginEvent Event) noexcept {
  switch (Event) {
  case WasmEdge_PluginEvent_BeforeUnload:
    return Plugin::PluginEvent::BeforeUnload;
  case WasmEdge_PluginEvent_AfterUnload:
    return Plugin::PluginEvent::AfterUnload;
  case WasmEdge_PluginEvent_BeforeLoad:
    return Plugin::PluginEvent::BeforeLoad;
  case WasmEdge_PluginEvent_AfterLoad:
    return Plugin::PluginEvent::AfterLoad;
  case WasmEdge_PluginEvent_BeforeReload:
    return Plugin::PluginEvent::BeforeReload;
  case WasmEdge_PluginEvent_AfterReload:
    return Plugin::PluginEvent::AfterReload;
  case WasmEdge_PluginEvent_LoadFailed:
    return Plugin::PluginEvent::LoadFailed;
  case WasmEdge_PluginEvent_UnloadFailed:
    return Plugin::PluginEvent::UnloadFailed;
  case WasmEdge_PluginEvent_WatchStarted:
    return Plugin::PluginEvent::WatchStarted;
  case WasmEdge_PluginEvent_WatchStopped:
    return Plugin::PluginEvent::WatchStopped;
  case WasmEdge_PluginEvent_FileChanged:
    return Plugin::PluginEvent::FileChanged;
  default:
    return Plugin::PluginEvent::AfterLoad;
  }
}

struct CallbackWrapper {
  WasmEdge_PluginEventCallback Callback;
  void *UserData;
};

std::mutex CallbackWrapperMutex;
std::unordered_map<uint64_t, std::unique_ptr<CallbackWrapper>>
    CallbackWrappers;

}

#ifdef __cplusplus
extern "C" {
#endif

WASMEDGE_CAPI_EXPORT WasmEdge_HotReloadConfig
WasmEdge_HotReloadConfigCreate(void) {
  WasmEdge_HotReloadConfig Config;
  Config.EnableFileWatching = true;
  Config.WatchIntervalMs = 1000;
  Config.DebounceDelayMs = 500;
  Config.MaxRetryCount = 3;
  Config.RetryDelayMs = 1000;
  Config.AutoReloadOnChange = true;
  return Config;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_HotReloadConfigure(const WasmEdge_HotReloadConfig *Config) {
  if (!Config) {
    return;
  }

  WasmEdge::Plugin::HotReloadConfig CppConfig;
  CppConfig.EnableFileWatching = Config->EnableFileWatching;
  CppConfig.WatchInterval =
      std::chrono::milliseconds(Config->WatchIntervalMs);
  CppConfig.DebounceDelay =
      std::chrono::milliseconds(Config->DebounceDelayMs);
  CppConfig.MaxRetryCount = Config->MaxRetryCount;
  CppConfig.RetryDelay = std::chrono::milliseconds(Config->RetryDelayMs);
  CppConfig.AutoReloadOnChange = Config->AutoReloadOnChange;

  WasmEdge::Plugin::HotReloadManager::getInstance().configure(CppConfig);
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_HotReloadStartWatching(const char *const *Paths, uint32_t PathCount) {
  if (!Paths || PathCount == 0) {
    return false;
  }

  std::vector<std::filesystem::path> PathVec;
  PathVec.reserve(PathCount);
  for (uint32_t I = 0; I < PathCount; ++I) {
    if (Paths[I]) {
      PathVec.emplace_back(Paths[I]);
    }
  }

  return WasmEdge::Plugin::HotReloadManager::getInstance().startWatching(
      PathVec);
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_HotReloadStartWatchingPath(const char *Path) {
  if (!Path) {
    return false;
  }
  return WasmEdge::Plugin::HotReloadManager::getInstance().startWatching(
      std::filesystem::path(Path));
}

WASMEDGE_CAPI_EXPORT void WasmEdge_HotReloadStopWatching(void) {
  WasmEdge::Plugin::HotReloadManager::getInstance().stopWatching();
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_HotReloadIsWatching(void) {
  return WasmEdge::Plugin::HotReloadManager::getInstance().isWatching();
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_PluginUnload(const WasmEdge_String Name) {
  return WasmEdge::Plugin::Plugin::unload(genStrView(Name));
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_PluginUnloadByPath(const char *Path) {
  if (!Path) {
    return false;
  }
  return WasmEdge::Plugin::Plugin::unloadByPath(std::filesystem::path(Path));
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_PluginReload(const WasmEdge_String Name) {
  return WasmEdge::Plugin::Plugin::reload(genStrView(Name));
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_PluginReloadByPath(const char *Path) {
  if (!Path) {
    return false;
  }
  return WasmEdge::Plugin::Plugin::reloadByPath(std::filesystem::path(Path));
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_PluginReloadChanged(void) {
  return WasmEdge::Plugin::HotReloadManager::getInstance()
      .reloadChangedPlugins();
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_PluginUnloadAll(void) {
  return WasmEdge::Plugin::HotReloadManager::getInstance().unloadAllPlugins();
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_PluginIsLoaded(const WasmEdge_String Name) {
  return WasmEdge::Plugin::Plugin::isLoaded(genStrView(Name));
}

WASMEDGE_CAPI_EXPORT WasmEdge_PluginState
WasmEdge_PluginGetState(const WasmEdge_String Name) {
  return toPluginState(
      WasmEdge::Plugin::HotReloadManager::getInstance().getPluginState(
          genStrView(Name)));
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_PluginHasChanged(const WasmEdge_String Name) {
  const auto *Plugin = WasmEdge::Plugin::Plugin::find(genStrView(Name));
  if (Plugin) {
    return Plugin->hasChanged();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_PluginGetPath(const WasmEdge_String Name, char *PathBuf,
                       uint32_t BufLen) {
  const auto *Plugin = WasmEdge::Plugin::Plugin::find(genStrView(Name));
  if (!Plugin) {
    return 0;
  }

  std::string PathStr = Plugin->path().string();
  uint32_t PathLen = static_cast<uint32_t>(PathStr.size());

  if (PathBuf && BufLen > 0) {
    uint32_t CopyLen = std::min(PathLen, BufLen - 1);
    std::memcpy(PathBuf, PathStr.data(), CopyLen);
    PathBuf[CopyLen] = '\0';
  }

  return PathLen;
}

WASMEDGE_CAPI_EXPORT uint64_t WasmEdge_HotReloadRegisterCallback(
    WasmEdge_PluginEvent Event, WasmEdge_PluginEventCallback Callback,
    void *UserData) {
  if (!Callback) {
    return 0;
  }

  auto Wrapper = std::make_unique<CallbackWrapper>();
  Wrapper->Callback = Callback;
  Wrapper->UserData = UserData;
  CallbackWrapper *WrapperPtr = Wrapper.get();

  uint64_t Id = WasmEdge::Plugin::HotReloadManager::getInstance()
      .registerCallback(
          toPluginEvent(Event),
          [WrapperPtr](const std::string &PluginName,
                       const std::filesystem::path &Path) {
            WrapperPtr->Callback(PluginName.c_str(), Path.string().c_str(),
                                 WrapperPtr->UserData);
          });

  {
    std::lock_guard<std::mutex> Lock(CallbackWrapperMutex);
    CallbackWrappers[Id] = std::move(Wrapper);
  }

  return Id;
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_HotReloadUnregisterCallback(uint64_t CallbackId) {
  bool Result =
      WasmEdge::Plugin::HotReloadManager::getInstance().unregisterCallback(
          CallbackId);

  if (Result) {
    std::lock_guard<std::mutex> Lock(CallbackWrapperMutex);
    CallbackWrappers.erase(CallbackId);
  }

  return Result;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_HotReloadClearCallbacks(WasmEdge_PluginEvent Event) {
  WasmEdge::Plugin::HotReloadManager::getInstance().clearCallbacks(
      toPluginEvent(Event));
}

WASMEDGE_CAPI_EXPORT void WasmEdge_HotReloadClearAllCallbacks(void) {
  WasmEdge::Plugin::HotReloadManager::getInstance().clearAllCallbacks();

  std::lock_guard<std::mutex> Lock(CallbackWrapperMutex);
  CallbackWrappers.clear();
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_HotReloadGetStatistics(WasmEdge_HotReloadStatistics *Stats) {
  if (!Stats) {
    return;
  }

  auto CppStats =
      WasmEdge::Plugin::HotReloadManager::getInstance().getStatistics();
  Stats->TotalLoads = CppStats.TotalLoads;
  Stats->TotalUnloads = CppStats.TotalUnloads;
  Stats->TotalReloads = CppStats.TotalReloads;
  Stats->FailedLoads = CppStats.FailedLoads;
  Stats->FailedUnloads = CppStats.FailedUnloads;
  Stats->FailedReloads = CppStats.FailedReloads;
  Stats->FileChangeEvents = CppStats.FileChangeEvents;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_HotReloadResetStatistics(void) {
  WasmEdge::Plugin::HotReloadManager::getInstance().resetStatistics();
}

#ifdef __cplusplus
}
#endif
