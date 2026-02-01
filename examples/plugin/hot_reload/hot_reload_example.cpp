// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- hot_reload_example.cpp - Plugin Hot-Reload Example ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file demonstrates the plugin hot-reloading feature of WasmEdge.
/// It shows how to:
/// - Configure hot-reload settings
/// - Start/stop file watching for plugin changes
/// - Register callbacks for plugin lifecycle events
/// - Manually reload plugins
/// - Check plugin states and statistics
///
//===----------------------------------------------------------------------===//

#include <wasmedge/wasmedge.h>
#include <wasmedge/wasmedge_hot_reload.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

void OnPluginEvent(const char *PluginName, const char *Path, void *UserData) {
  const char *EventName = static_cast<const char *>(UserData);
  printf("[%s] Plugin: %s, Path: %s\n", EventName, 
         PluginName ? PluginName : "(none)",
         Path ? Path : "(none)");
}

void PrintUsage(const char *ProgramName) {
  printf("WasmEdge Plugin Hot-Reload Example\n");
  printf("===================================\n\n");
  printf("Usage: %s [OPTIONS] [PLUGIN_PATH]\n\n", ProgramName);
  printf("Options:\n");
  printf("  --watch           Start watching plugin directory for changes\n");
  printf("  --list            List all loaded plugins\n");
  printf("  --reload <name>   Reload a specific plugin by name\n");
  printf("  --unload <name>   Unload a specific plugin by name\n");
  printf("  --stats           Show hot-reload statistics\n");
  printf("  --help            Show this help message\n\n");
  printf("Example:\n");
  printf("  %s --watch /path/to/plugins\n", ProgramName);
  printf("  %s --reload wasi_nn\n", ProgramName);
}

void PrintStatistics() {
  WasmEdge_HotReloadStatistics Stats;
  WasmEdge_HotReloadGetStatistics(&Stats);
  
  printf("\nHot-Reload Statistics:\n");
  printf("  Total Loads:       %lu\n", Stats.TotalLoads);
  printf("  Total Unloads:     %lu\n", Stats.TotalUnloads);
  printf("  Total Reloads:     %lu\n", Stats.TotalReloads);
  printf("  Failed Loads:      %lu\n", Stats.FailedLoads);
  printf("  Failed Unloads:    %lu\n", Stats.FailedUnloads);
  printf("  Failed Reloads:    %lu\n", Stats.FailedReloads);
  printf("  File Change Events:%lu\n", Stats.FileChangeEvents);
}

void ListPlugins() {
  uint32_t PluginCount = WasmEdge_PluginListPluginsLength();
  printf("\nLoaded Plugins (%u):\n", PluginCount);
  
  if (PluginCount == 0) {
    printf("  (none)\n");
    return;
  }
  
  WasmEdge_String *Names = 
      static_cast<WasmEdge_String *>(malloc(PluginCount * sizeof(WasmEdge_String)));
  
  WasmEdge_PluginListPlugins(Names, PluginCount);
  
  for (uint32_t I = 0; I < PluginCount; ++I) {
    char PathBuf[1024] = {0};
    WasmEdge_PluginGetPath(Names[I], PathBuf, sizeof(PathBuf));
    
    WasmEdge_PluginState State = WasmEdge_PluginGetState(Names[I]);
    const char *StateStr = "Unknown";
    switch (State) {
    case WasmEdge_PluginState_Loaded:
      StateStr = "Loaded";
      break;
    case WasmEdge_PluginState_Unloaded:
      StateStr = "Unloaded";
      break;
    case WasmEdge_PluginState_Loading:
      StateStr = "Loading";
      break;
    case WasmEdge_PluginState_Unloading:
      StateStr = "Unloading";
      break;
    case WasmEdge_PluginState_Reloading:
      StateStr = "Reloading";
      break;
    case WasmEdge_PluginState_Error:
      StateStr = "Error";
      break;
    default:
      break;
    }
    
    bool Changed = WasmEdge_PluginHasChanged(Names[I]);
    
    printf("  %u. %.*s\n", I + 1, Names[I].Length, Names[I].Buf);
    printf("     Path: %s\n", PathBuf[0] ? PathBuf : "(built-in)");
    printf("     State: %s%s\n", StateStr, Changed ? " (changed)" : "");
  }
  
  free(Names);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    PrintUsage(argv[0]);
    return 0;
  }
  
  WasmEdge_LogSetDebugLevel();
  
  WasmEdge_PluginLoadWithDefaultPaths();
  
  WasmEdge_HotReloadConfig Config = WasmEdge_HotReloadConfigCreate();
  Config.EnableFileWatching = true;
  Config.WatchIntervalMs = 1000;
  Config.DebounceDelayMs = 500;
  Config.AutoReloadOnChange = true;
  WasmEdge_HotReloadConfigure(&Config);
  
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_BeforeLoad, OnPluginEvent, 
      const_cast<char *>("BeforeLoad"));
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_AfterLoad, OnPluginEvent, 
      const_cast<char *>("AfterLoad"));
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_BeforeUnload, OnPluginEvent, 
      const_cast<char *>("BeforeUnload"));
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_AfterUnload, OnPluginEvent, 
      const_cast<char *>("AfterUnload"));
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_BeforeReload, OnPluginEvent, 
      const_cast<char *>("BeforeReload"));
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_AfterReload, OnPluginEvent, 
      const_cast<char *>("AfterReload"));
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_FileChanged, OnPluginEvent, 
      const_cast<char *>("FileChanged"));
  WasmEdge_HotReloadRegisterCallback(
      WasmEdge_PluginEvent_LoadFailed, OnPluginEvent, 
      const_cast<char *>("LoadFailed"));
  
  for (int I = 1; I < argc; ++I) {
    if (strcmp(argv[I], "--help") == 0 || strcmp(argv[I], "-h") == 0) {
      PrintUsage(argv[0]);
      return 0;
    }
    
    if (strcmp(argv[I], "--list") == 0) {
      ListPlugins();
      continue;
    }
    
    if (strcmp(argv[I], "--stats") == 0) {
      PrintStatistics();
      continue;
    }
    
    if (strcmp(argv[I], "--reload") == 0 && I + 1 < argc) {
      ++I;
      WasmEdge_String Name = WasmEdge_StringCreateByCString(argv[I]);
      printf("Reloading plugin: %s\n", argv[I]);
      if (WasmEdge_PluginReload(Name)) {
        printf("Plugin reloaded successfully.\n");
      } else {
        printf("Failed to reload plugin.\n");
      }
      WasmEdge_StringDelete(Name);
      continue;
    }
    
    if (strcmp(argv[I], "--unload") == 0 && I + 1 < argc) {
      ++I;
      WasmEdge_String Name = WasmEdge_StringCreateByCString(argv[I]);
      printf("Unloading plugin: %s\n", argv[I]);
      if (WasmEdge_PluginUnload(Name)) {
        printf("Plugin unloaded successfully.\n");
      } else {
        printf("Failed to unload plugin.\n");
      }
      WasmEdge_StringDelete(Name);
      continue;
    }
    
    if (strcmp(argv[I], "--watch") == 0) {
      const char *WatchPath = ".";
      if (I + 1 < argc && argv[I + 1][0] != '-') {
        WatchPath = argv[++I];
      }
      
      printf("Starting file watcher for: %s\n", WatchPath);
      if (WasmEdge_HotReloadStartWatchingPath(WatchPath)) {
        printf("File watcher started. Press Ctrl+C to stop.\n");
        printf("Modify plugin files to trigger automatic reload.\n\n");
        
        while (WasmEdge_HotReloadIsWatching()) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
          
          uint32_t ReloadedCount = WasmEdge_PluginReloadChanged();
          if (ReloadedCount > 0) {
            printf("Reloaded %u plugins.\n", ReloadedCount);
            ListPlugins();
          }
        }
      } else {
        printf("Failed to start file watcher.\n");
        return 1;
      }
      continue;
    }
    
    printf("Loading plugin from: %s\n", argv[I]);
    WasmEdge_PluginLoadFromPath(argv[I]);
  }
  
  PrintStatistics();
  ListPlugins();
  
  WasmEdge_HotReloadStopWatching();
  WasmEdge_HotReloadClearAllCallbacks();
  
  return 0;
}
