// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "plugin/hot_reload.h"
#include "common/defines.h"
#include "common/spdlog.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <optional>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <sys/stat.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

using namespace std::literals;

namespace WasmEdge {
namespace Plugin {

struct HotReloadManager::Impl {
  Impl() noexcept = default;
  ~Impl() noexcept { stopWatching(); }

  HotReloadConfig Config;

  mutable std::mutex Mutex;

  std::thread WatcherThread;
  std::atomic<bool> WatcherRunning{false};
  std::condition_variable WatcherCV;
  std::mutex WatcherMutex;

  std::vector<std::filesystem::path> WatchedPaths;
  std::unordered_map<std::string, WatchedPluginInfo> PluginInfoMap;
  std::unordered_map<std::string, std::filesystem::path> PathToNameMap;

  std::mutex CallbackMutex;
  uint64_t NextCallbackId = 1;
  std::map<uint64_t, std::pair<PluginEvent, PluginEventCallback>> Callbacks;

  std::atomic<uint64_t> TotalLoads{0};
  std::atomic<uint64_t> TotalUnloads{0};
  std::atomic<uint64_t> TotalReloads{0};
  std::atomic<uint64_t> FailedLoads{0};
  std::atomic<uint64_t> FailedUnloads{0};
  std::atomic<uint64_t> FailedReloads{0};
  std::atomic<uint64_t> FileChangeEvents{0};

  std::unordered_map<std::string, std::chrono::steady_clock::time_point>
      LastChangeTime;

  void fireCallbacks(PluginEvent Event, const std::string &PluginName,
                     const std::filesystem::path &Path) noexcept {
    std::lock_guard<std::mutex> Lock(CallbackMutex);
    for (const auto &[Id, CallbackPair] : Callbacks) {
      if (CallbackPair.first == Event) {
        try {
          CallbackPair.second(PluginName, Path);
        } catch (const std::exception &E) {
          spdlog::error("Plugin callback exception: {}"sv, E.what());
        } catch (...) {
          spdlog::error("Plugin callback unknown exception"sv);
        }
      }
    }
  }

  std::optional<std::filesystem::file_time_type>
  getFileModTime(const std::filesystem::path &Path) const noexcept {
    std::error_code EC;
    auto ModTime = std::filesystem::last_write_time(Path, EC);
    if (EC) {
      return std::nullopt;
    }
    return ModTime;
  }

  bool shouldProcessChange(const std::string &PathStr) noexcept {
    auto Now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> Lock(Mutex);
    
    auto It = LastChangeTime.find(PathStr);
    if (It != LastChangeTime.end()) {
      auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          Now - It->second);
      if (Elapsed < Config.DebounceDelay) {
        return false;
      }
    }
    LastChangeTime[PathStr] = Now;
    return true;
  }

  void watcherLoop() noexcept {
    spdlog::info("Plugin file watcher started"sv);
    fireCallbacks(PluginEvent::WatchStarted, "", "");

    while (WatcherRunning.load()) {
      {
        std::unique_lock<std::mutex> Lock(WatcherMutex);
        WatcherCV.wait_for(Lock, Config.WatchInterval,
                           [this] { return !WatcherRunning.load(); });
      }

      if (!WatcherRunning.load()) {
        break;
      }

      checkForChanges();
    }

    spdlog::info("Plugin file watcher stopped"sv);
    fireCallbacks(PluginEvent::WatchStopped, "", "");
  }

  void checkForChanges() noexcept {
    std::lock_guard<std::mutex> Lock(Mutex);

    for (const auto &WatchPath : WatchedPaths) {
      std::error_code EC;
      
      if (std::filesystem::is_directory(WatchPath, EC)) {
        for (const auto &Entry :
             std::filesystem::directory_iterator(WatchPath, EC)) {
          if (EC) continue;
          
          const auto &FilePath = Entry.path();
          if (FilePath.extension().u8string() == WASMEDGE_LIB_EXTENSION) {
            checkFileChange(FilePath);
          }
        }
      } else if (std::filesystem::is_regular_file(WatchPath, EC)) {
        checkFileChange(WatchPath);
      }
    }
  }

  void checkFileChange(const std::filesystem::path &FilePath) noexcept {
    auto ModTimeOpt = getFileModTime(FilePath);
    if (!ModTimeOpt) {
      return;
    }

    std::string PathStr = FilePath.string();
    
    for (auto &[Name, Info] : PluginInfoMap) {
      if (Info.Path == FilePath) {
        if (*ModTimeOpt != Info.LastModified) {
          FileChangeEvents.fetch_add(1);
          fireCallbacks(PluginEvent::FileChanged, Name, FilePath);
          
          if (Config.AutoReloadOnChange && Info.AutoReload) {
            if (shouldProcessChange(PathStr)) {
              spdlog::info("Plugin file changed, reloading: {}"sv, Name);
              Info.State = PluginState::Reloading;
            }
          }
          
          Info.LastModified = *ModTimeOpt;
        }
        break;
      }
    }
  }

  bool startWatching() noexcept {
    if (WatcherRunning.load()) {
      return true;
    }

    WatcherRunning.store(true);
    WatcherThread = std::thread(&Impl::watcherLoop, this);
    return true;
  }

  void stopWatching() noexcept {
    if (!WatcherRunning.load()) {
      return;
    }

    WatcherRunning.store(false);
    WatcherCV.notify_all();
    
    if (WatcherThread.joinable()) {
      WatcherThread.join();
    }
  }
};

HotReloadManager &HotReloadManager::getInstance() noexcept {
  static HotReloadManager Instance;
  return Instance;
}

HotReloadManager::HotReloadManager() noexcept
    : PImpl(std::make_unique<Impl>()) {}

HotReloadManager::~HotReloadManager() noexcept = default;

void HotReloadManager::configure(const HotReloadConfig &Config) noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  PImpl->Config = Config;
}

const HotReloadConfig &HotReloadManager::getConfig() const noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  return PImpl->Config;
}

bool HotReloadManager::startWatching(
    const std::vector<std::filesystem::path> &Paths) noexcept {
  {
    std::lock_guard<std::mutex> Lock(PImpl->Mutex);
    for (const auto &Path : Paths) {
      if (std::find(PImpl->WatchedPaths.begin(), PImpl->WatchedPaths.end(),
                    Path) == PImpl->WatchedPaths.end()) {
        PImpl->WatchedPaths.push_back(Path);
      }
    }
  }
  return PImpl->startWatching();
}

bool HotReloadManager::startWatching(
    const std::filesystem::path &Path) noexcept {
  return startWatching(std::vector<std::filesystem::path>{Path});
}

void HotReloadManager::stopWatching() noexcept { PImpl->stopWatching(); }

bool HotReloadManager::isWatching() const noexcept {
  return PImpl->WatcherRunning.load();
}

std::vector<std::filesystem::path>
HotReloadManager::getWatchedPaths() const noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  return PImpl->WatchedPaths;
}

bool HotReloadManager::loadPlugin(const std::filesystem::path &Path) noexcept {
  PImpl->fireCallbacks(PluginEvent::BeforeLoad, "", Path);

  bool Result = Plugin::load(Path);

  if (Result) {
    PImpl->TotalLoads.fetch_add(1);
    
    auto Plugins = Plugin::plugins();
    for (const auto &P : Plugins) {
      if (P.path() == Path) {
        std::lock_guard<std::mutex> Lock(PImpl->Mutex);
        WatchedPluginInfo Info;
        Info.Path = Path;
        Info.State = PluginState::Loaded;
        Info.LoadCount = 1;
        Info.ReloadCount = 0;
        Info.AutoReload = PImpl->Config.AutoReloadOnChange;
        
        auto ModTimeOpt = PImpl->getFileModTime(Path);
        if (ModTimeOpt) {
          Info.LastModified = *ModTimeOpt;
        }
        
        std::string Name = P.name();
        PImpl->PluginInfoMap[Name] = std::move(Info);
        PImpl->PathToNameMap[Path.string()] = Name;
        
        PImpl->fireCallbacks(PluginEvent::AfterLoad, Name, Path);
        break;
      }
    }
  } else {
    PImpl->FailedLoads.fetch_add(1);
    PImpl->fireCallbacks(PluginEvent::LoadFailed, "", Path);
  }

  return Result;
}

bool HotReloadManager::unloadPlugin(std::string_view Name) noexcept {
  std::filesystem::path PluginPath;
  
  {
    std::lock_guard<std::mutex> Lock(PImpl->Mutex);
    auto It = PImpl->PluginInfoMap.find(std::string(Name));
    if (It != PImpl->PluginInfoMap.end()) {
      PluginPath = It->second.Path;
      It->second.State = PluginState::Unloading;
    }
  }

  std::string NameStr(Name);
  PImpl->fireCallbacks(PluginEvent::BeforeUnload, NameStr, PluginPath);

  bool Result = Plugin::unload(Name);

  if (Result) {
    PImpl->TotalUnloads.fetch_add(1);
    
    {
      std::lock_guard<std::mutex> Lock(PImpl->Mutex);
      auto It = PImpl->PluginInfoMap.find(NameStr);
      if (It != PImpl->PluginInfoMap.end()) {
        It->second.State = PluginState::Unloaded;
      }
      PImpl->PathToNameMap.erase(PluginPath.string());
    }
    
    PImpl->fireCallbacks(PluginEvent::AfterUnload, NameStr, PluginPath);
  } else {
    PImpl->FailedUnloads.fetch_add(1);
    
    {
      std::lock_guard<std::mutex> Lock(PImpl->Mutex);
      auto It = PImpl->PluginInfoMap.find(NameStr);
      if (It != PImpl->PluginInfoMap.end()) {
        It->second.State = PluginState::Error;
        It->second.LastError = "Failed to unload plugin";
      }
    }
    
    PImpl->fireCallbacks(PluginEvent::UnloadFailed, NameStr, PluginPath);
  }

  return Result;
}

bool HotReloadManager::unloadPluginByPath(
    const std::filesystem::path &Path) noexcept {
  std::string Name;
  {
    std::lock_guard<std::mutex> Lock(PImpl->Mutex);
    auto It = PImpl->PathToNameMap.find(Path.string());
    if (It != PImpl->PathToNameMap.end()) {
      Name = It->second;
    }
  }
  
  if (!Name.empty()) {
    return unloadPlugin(Name);
  }
  
  return Plugin::unloadByPath(Path);
}

bool HotReloadManager::reloadPlugin(std::string_view Name) noexcept {
  std::filesystem::path PluginPath;
  
  {
    std::lock_guard<std::mutex> Lock(PImpl->Mutex);
    auto It = PImpl->PluginInfoMap.find(std::string(Name));
    if (It != PImpl->PluginInfoMap.end()) {
      PluginPath = It->second.Path;
      It->second.State = PluginState::Reloading;
    } else {
      const Plugin *P = Plugin::find(Name);
      if (P) {
        PluginPath = P->path();
      } else {
        spdlog::error("Plugin not found for reload: {}"sv, Name);
        return false;
      }
    }
  }

  std::string NameStr(Name);
  PImpl->fireCallbacks(PluginEvent::BeforeReload, NameStr, PluginPath);

  bool Result = Plugin::reload(Name);

  if (Result) {
    PImpl->TotalReloads.fetch_add(1);
    
    {
      std::lock_guard<std::mutex> Lock(PImpl->Mutex);
      auto It = PImpl->PluginInfoMap.find(NameStr);
      if (It != PImpl->PluginInfoMap.end()) {
        It->second.State = PluginState::Loaded;
        It->second.ReloadCount++;
        It->second.LoadCount++;
        
        auto ModTimeOpt = PImpl->getFileModTime(PluginPath);
        if (ModTimeOpt) {
          It->second.LastModified = *ModTimeOpt;
        }
      }
    }
    
    PImpl->fireCallbacks(PluginEvent::AfterReload, NameStr, PluginPath);
  } else {
    PImpl->FailedReloads.fetch_add(1);
    
    {
      std::lock_guard<std::mutex> Lock(PImpl->Mutex);
      auto It = PImpl->PluginInfoMap.find(NameStr);
      if (It != PImpl->PluginInfoMap.end()) {
        It->second.State = PluginState::Error;
        It->second.LastError = "Failed to reload plugin";
      }
    }
    
    spdlog::error("Failed to reload plugin: {}"sv, Name);
  }

  return Result;
}

bool HotReloadManager::reloadPluginByPath(
    const std::filesystem::path &Path) noexcept {
  std::string Name;
  {
    std::lock_guard<std::mutex> Lock(PImpl->Mutex);
    auto It = PImpl->PathToNameMap.find(Path.string());
    if (It != PImpl->PathToNameMap.end()) {
      Name = It->second;
    }
  }
  
  if (!Name.empty()) {
    return reloadPlugin(Name);
  }
  
  return Plugin::reloadByPath(Path);
}

uint32_t HotReloadManager::reloadChangedPlugins() noexcept {
  uint32_t ReloadCount = 0;
  std::vector<std::string> PluginsToReload;
  
  {
    std::lock_guard<std::mutex> Lock(PImpl->Mutex);
    for (const auto &[Name, Info] : PImpl->PluginInfoMap) {
      if (Info.State == PluginState::Loaded) {
        auto ModTimeOpt = PImpl->getFileModTime(Info.Path);
        if (ModTimeOpt && *ModTimeOpt != Info.LastModified) {
          PluginsToReload.push_back(Name);
        }
      }
    }
  }
  
  for (const auto &Name : PluginsToReload) {
    if (reloadPlugin(Name)) {
      ++ReloadCount;
    }
  }
  
  return ReloadCount;
}

uint32_t HotReloadManager::unloadAllPlugins() noexcept {
  uint32_t UnloadCount = 0;
  std::vector<std::string> PluginsToUnload;
  
  {
    std::lock_guard<std::mutex> Lock(PImpl->Mutex);
    for (const auto &[Name, Info] : PImpl->PluginInfoMap) {
      if (Info.State == PluginState::Loaded) {
        PluginsToUnload.push_back(Name);
      }
    }
  }
  
  for (const auto &Name : PluginsToUnload) {
    if (unloadPlugin(Name)) {
      ++UnloadCount;
    }
  }
  
  return UnloadCount;
}

PluginState
HotReloadManager::getPluginState(std::string_view Name) const noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  auto It = PImpl->PluginInfoMap.find(std::string(Name));
  if (It != PImpl->PluginInfoMap.end()) {
    return It->second.State;
  }
  return PluginState::Unknown;
}

std::optional<WatchedPluginInfo>
HotReloadManager::getPluginInfo(std::string_view Name) const noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  auto It = PImpl->PluginInfoMap.find(std::string(Name));
  if (It != PImpl->PluginInfoMap.end()) {
    return It->second;
  }
  return std::nullopt;
}

std::vector<std::string>
HotReloadManager::getManagedPluginNames() const noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  std::vector<std::string> Names;
  Names.reserve(PImpl->PluginInfoMap.size());
  for (const auto &[Name, Info] : PImpl->PluginInfoMap) {
    Names.push_back(Name);
  }
  return Names;
}

uint64_t HotReloadManager::registerCallback(
    PluginEvent Event, PluginEventCallback Callback) noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->CallbackMutex);
  uint64_t Id = PImpl->NextCallbackId++;
  PImpl->Callbacks[Id] = {Event, std::move(Callback)};
  return Id;
}

bool HotReloadManager::unregisterCallback(uint64_t CallbackId) noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->CallbackMutex);
  return PImpl->Callbacks.erase(CallbackId) > 0;
}

void HotReloadManager::clearCallbacks(PluginEvent Event) noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->CallbackMutex);
  for (auto It = PImpl->Callbacks.begin(); It != PImpl->Callbacks.end();) {
    if (It->second.first == Event) {
      It = PImpl->Callbacks.erase(It);
    } else {
      ++It;
    }
  }
}

void HotReloadManager::clearAllCallbacks() noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->CallbackMutex);
  PImpl->Callbacks.clear();
}

bool HotReloadManager::hasPluginChanged(std::string_view Name) const noexcept {
  std::lock_guard<std::mutex> Lock(PImpl->Mutex);
  auto It = PImpl->PluginInfoMap.find(std::string(Name));
  if (It != PImpl->PluginInfoMap.end()) {
    auto ModTimeOpt = PImpl->getFileModTime(It->second.Path);
    if (ModTimeOpt) {
      return *ModTimeOpt != It->second.LastModified;
    }
  }
  return false;
}

HotReloadManager::Statistics HotReloadManager::getStatistics() const noexcept {
  Statistics Stats;
  Stats.TotalLoads = PImpl->TotalLoads.load();
  Stats.TotalUnloads = PImpl->TotalUnloads.load();
  Stats.TotalReloads = PImpl->TotalReloads.load();
  Stats.FailedLoads = PImpl->FailedLoads.load();
  Stats.FailedUnloads = PImpl->FailedUnloads.load();
  Stats.FailedReloads = PImpl->FailedReloads.load();
  Stats.FileChangeEvents = PImpl->FileChangeEvents.load();
  return Stats;
}

void HotReloadManager::resetStatistics() noexcept {
  PImpl->TotalLoads.store(0);
  PImpl->TotalUnloads.store(0);
  PImpl->TotalReloads.store(0);
  PImpl->FailedLoads.store(0);
  PImpl->FailedUnloads.store(0);
  PImpl->FailedReloads.store(0);
  PImpl->FileChangeEvents.store(0);
}

} // namespace Plugin
} // namespace WasmEdge
