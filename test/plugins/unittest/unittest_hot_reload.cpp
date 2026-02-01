// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- unittest_hot_reload.cpp - Hot-Reload Unit Tests -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains unit tests for the plugin hot-reloading feature.
///
//===----------------------------------------------------------------------===//

#include "plugin/hot_reload.h"
#include "plugin/plugin.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace WasmEdge::Plugin;

class HotReloadTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto &Manager = HotReloadManager::getInstance();
    Manager.stopWatching();
    Manager.clearAllCallbacks();
    Manager.resetStatistics();
  }

  void TearDown() override {
    auto &Manager = HotReloadManager::getInstance();
    Manager.stopWatching();
    Manager.clearAllCallbacks();
  }
};

TEST_F(HotReloadTest, GetInstance) {
  auto &Instance1 = HotReloadManager::getInstance();
  auto &Instance2 = HotReloadManager::getInstance();
  EXPECT_EQ(&Instance1, &Instance2);
}

TEST_F(HotReloadTest, Configuration) {
  auto &Manager = HotReloadManager::getInstance();

  HotReloadConfig Config;
  Config.EnableFileWatching = true;
  Config.WatchInterval = std::chrono::milliseconds(500);
  Config.DebounceDelay = std::chrono::milliseconds(250);
  Config.MaxRetryCount = 5;
  Config.RetryDelay = std::chrono::milliseconds(2000);
  Config.AutoReloadOnChange = false;

  Manager.configure(Config);

  const auto &CurrentConfig = Manager.getConfig();
  EXPECT_EQ(CurrentConfig.EnableFileWatching, true);
  EXPECT_EQ(CurrentConfig.WatchInterval, std::chrono::milliseconds(500));
  EXPECT_EQ(CurrentConfig.DebounceDelay, std::chrono::milliseconds(250));
  EXPECT_EQ(CurrentConfig.MaxRetryCount, 5u);
  EXPECT_EQ(CurrentConfig.RetryDelay, std::chrono::milliseconds(2000));
  EXPECT_EQ(CurrentConfig.AutoReloadOnChange, false);
}

TEST_F(HotReloadTest, FileWatchingStartStop) {
  auto &Manager = HotReloadManager::getInstance();

  EXPECT_FALSE(Manager.isWatching());

  EXPECT_TRUE(Manager.startWatching(std::filesystem::current_path()));
  EXPECT_TRUE(Manager.isWatching());

  auto Paths = Manager.getWatchedPaths();
  EXPECT_EQ(Paths.size(), 1u);

  Manager.stopWatching();
  EXPECT_FALSE(Manager.isWatching());
}

TEST_F(HotReloadTest, CallbackRegistration) {
  auto &Manager = HotReloadManager::getInstance();

  int CallbackCount = 0;

  uint64_t Id = Manager.registerCallback(
      PluginEvent::AfterLoad,
      [&CallbackCount](const std::string &, const std::filesystem::path &) {
        ++CallbackCount;
      });

  EXPECT_GT(Id, 0u);

  EXPECT_TRUE(Manager.unregisterCallback(Id));

  EXPECT_FALSE(Manager.unregisterCallback(Id));
}

TEST_F(HotReloadTest, MultipleCallbacks) {
  auto &Manager = HotReloadManager::getInstance();

  int Count1 = 0, Count2 = 0, Count3 = 0;

  auto Id1 = Manager.registerCallback(
      PluginEvent::BeforeLoad,
      [&Count1](const std::string &, const std::filesystem::path &) {
        ++Count1;
      });

  auto Id2 = Manager.registerCallback(
      PluginEvent::BeforeLoad,
      [&Count2](const std::string &, const std::filesystem::path &) {
        ++Count2;
      });

  auto Id3 = Manager.registerCallback(
      PluginEvent::AfterLoad,
      [&Count3](const std::string &, const std::filesystem::path &) {
        ++Count3;
      });

  EXPECT_NE(Id1, Id2);
  EXPECT_NE(Id2, Id3);

  Manager.clearCallbacks(PluginEvent::BeforeLoad);

  EXPECT_FALSE(Manager.unregisterCallback(Id1));
  EXPECT_FALSE(Manager.unregisterCallback(Id2));

  EXPECT_TRUE(Manager.unregisterCallback(Id3));
}

TEST_F(HotReloadTest, StatisticsInitialState) {
  auto &Manager = HotReloadManager::getInstance();

  auto Stats = Manager.getStatistics();

  EXPECT_EQ(Stats.TotalLoads, 0u);
  EXPECT_EQ(Stats.TotalUnloads, 0u);
  EXPECT_EQ(Stats.TotalReloads, 0u);
  EXPECT_EQ(Stats.FailedLoads, 0u);
  EXPECT_EQ(Stats.FailedUnloads, 0u);
  EXPECT_EQ(Stats.FailedReloads, 0u);
  EXPECT_EQ(Stats.FileChangeEvents, 0u);
}

TEST_F(HotReloadTest, PluginStateUnknown) {
  auto &Manager = HotReloadManager::getInstance();

  EXPECT_EQ(Manager.getPluginState("non_existent_plugin"),
            PluginState::Unknown);
}

TEST_F(HotReloadTest, PluginInfoNotFound) {
  auto &Manager = HotReloadManager::getInstance();

  auto Info = Manager.getPluginInfo("non_existent_plugin");
  EXPECT_FALSE(Info.has_value());
}

TEST_F(HotReloadTest, ManagedPluginNames) {
  auto &Manager = HotReloadManager::getInstance();

  auto Names = Manager.getManagedPluginNames();
  EXPECT_GE(Names.size(), 0u);
}

TEST_F(HotReloadTest, HasPluginChangedNonExistent) {
  auto &Manager = HotReloadManager::getInstance();

  EXPECT_FALSE(Manager.hasPluginChanged("non_existent_plugin"));
}

TEST_F(HotReloadTest, UnloadNonExistent) {
  auto &Manager = HotReloadManager::getInstance();

  EXPECT_FALSE(Manager.unloadPlugin("non_existent_plugin"));
}

TEST_F(HotReloadTest, ReloadNonExistent) {
  auto &Manager = HotReloadManager::getInstance();

  EXPECT_FALSE(Manager.reloadPlugin("non_existent_plugin"));
}

TEST_F(HotReloadTest, PluginIsLoaded) {
  EXPECT_FALSE(Plugin::isLoaded("non_existent_plugin"));
}

TEST_F(HotReloadTest, PluginCount) {
  size_t Count = Plugin::count();
  EXPECT_GE(Count, 0u);
}

TEST_F(HotReloadTest, PluginUnloadBuiltIn) {
  EXPECT_FALSE(Plugin::unload("wasi:logging/logging"));
}

TEST_F(HotReloadTest, PluginFindByPathNonExistent) {
  auto *P = Plugin::findByPath("/non/existent/path.so");
  EXPECT_EQ(P, nullptr);
}

TEST_F(HotReloadTest, PluginReloadByPathNonExistent) {
  EXPECT_FALSE(Plugin::reloadByPath("/non/existent/path.so"));
}

TEST_F(HotReloadTest, PluginUnloadByPathNonExistent) {
  EXPECT_FALSE(Plugin::unloadByPath("/non/existent/path.so"));
}

TEST_F(HotReloadTest, MultipleWatchPaths) {
  auto &Manager = HotReloadManager::getInstance();

  std::vector<std::filesystem::path> Paths = {
      std::filesystem::current_path(),
      std::filesystem::temp_directory_path()
  };

  EXPECT_TRUE(Manager.startWatching(Paths));
  EXPECT_TRUE(Manager.isWatching());

  auto WatchedPaths = Manager.getWatchedPaths();
  EXPECT_EQ(WatchedPaths.size(), 2u);

  Manager.stopWatching();
}

TEST_F(HotReloadTest, ClearAllCallbacks) {
  auto &Manager = HotReloadManager::getInstance();

  Manager.registerCallback(
      PluginEvent::BeforeLoad,
      [](const std::string &, const std::filesystem::path &) {});
  Manager.registerCallback(
      PluginEvent::AfterLoad,
      [](const std::string &, const std::filesystem::path &) {});
  Manager.registerCallback(
      PluginEvent::BeforeReload,
      [](const std::string &, const std::filesystem::path &) {});

  Manager.clearAllCallbacks();
}

} // namespace
