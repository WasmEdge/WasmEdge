// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "plugin/plugin.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <system_error>

#if defined(_WIN32)
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace {

// Use existing CI-built plugins
static const std::string kPluginC =
    "wasmedgePluginTestModuleC" WASMEDGE_LIB_EXTENSION;
static const std::string kPluginCPP =
    "wasmedgePluginTestModuleCPP" WASMEDGE_LIB_EXTENSION;
static const std::string kPluginNameC = "wasmedge_plugintest_c";
static const std::string kPluginNameCPP = "wasmedge_plugintest_cpp";

// Fixtures created in test/plugins/fixtures/
static const std::string kPluginIncomplete =
    "wasmedgePluginTestIncomplete" WASMEDGE_LIB_EXTENSION;
static const std::string kPluginVersionMismatch =
    "wasmedgePluginTestVersionMismatch" WASMEDGE_LIB_EXTENSION;

class PluginLoaderTest : public ::testing::Test {
protected:
  std::filesystem::path TempDir;

  void SetUp() override {
    // Generates a collision-safe temporary directory for CI environments
    auto SystemTemp = std::filesystem::temp_directory_path();
    // Deterministic and collision-safe in parallel CI (process isolation)
    TempDir =
        SystemTemp / ("wasmedge_plugin_test_" + std::to_string(::getpid()));
    std::filesystem::create_directories(TempDir);
  }

  void TearDown() override { std::filesystem::remove_all(TempDir); }

  std::filesystem::path getPluginPath(const std::string &Filename) {
    // Plugins are expected to be in the current working directory (build dir)
    return std::filesystem::current_path() / Filename;
  }
};

// Plugin registry is global and persistent across tests.
// No unload/reset API exists.
// Tests are written to avoid assumptions about registry state.

TEST_F(PluginLoaderTest, Discovery_LoadFromDirectory) {
  // Arrange: Copy a valid plugin to the temp directory
  auto Source = getPluginPath(kPluginC);
  auto Dest = TempDir / kPluginC;

  std::error_code EC;
  std::filesystem::copy_file(
      Source, Dest, std::filesystem::copy_options::overwrite_existing, EC);
  ASSERT_FALSE(EC) << "Failed to copy plugin: " << EC.message();

  // Act: Load from the directory
  bool Result = WasmEdge::Plugin::Plugin::load(TempDir);

  // Assert: Load should succeed and plugin should be found
  EXPECT_TRUE(Result);
  auto *Plugin = WasmEdge::Plugin::Plugin::find(kPluginNameC);
  EXPECT_NE(Plugin, nullptr);
  if (Plugin) {
    EXPECT_STREQ(Plugin->name(), kPluginNameC.c_str());
  }
}

TEST_F(PluginLoaderTest, Discovery_HandleEmptyPath) {
  // Graceful handling of invalid paths is expected (no throw)
  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(std::filesystem::path("")));
  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(
      std::filesystem::path("/non/existent/path/xyz")));
}

TEST_F(PluginLoaderTest, MultiplePluginLoading) {
  // Arrange: Paths to two different plugins
  auto PathC = getPluginPath(kPluginC);
  auto PathCPP = getPluginPath(kPluginCPP);

  // Act: Load both
  bool Res1 = WasmEdge::Plugin::Plugin::load(PathC);
  bool Res2 = WasmEdge::Plugin::Plugin::load(PathCPP);

  // Assert: Both loads succeed and both plugins are registered
  EXPECT_TRUE(Res1);
  EXPECT_TRUE(Res2);

  EXPECT_NE(WasmEdge::Plugin::Plugin::find(kPluginNameC), nullptr);
  EXPECT_NE(WasmEdge::Plugin::Plugin::find(kPluginNameCPP), nullptr);
}

TEST_F(PluginLoaderTest, Failure_InvalidBinary) {
  // Expected failure: plugin binary is invalid/corrupt
  auto InvalidFile = TempDir / ("invalid" WASMEDGE_LIB_EXTENSION);
  std::ofstream Out(InvalidFile);
  Out << "This is not a plugin binary";
  Out.close();

  // Act: Try to load
  bool Result = WasmEdge::Plugin::Plugin::load(InvalidFile);

  // Assert: Should fail
  EXPECT_FALSE(Result);
}

TEST_F(PluginLoaderTest, Failure_MissingSymbols) {
  // Expected failure: plugin missing WasmEdge_Plugin_GetDescriptor symbol
  auto Path = getPluginPath(kPluginIncomplete);

  // Act
  bool Result = WasmEdge::Plugin::Plugin::load(Path);

  // Assert
  EXPECT_FALSE(Result);
}

TEST_F(PluginLoaderTest, Failure_VersionMismatch) {
  // Expected failure: plugin with incompatible API version
  auto Path = getPluginPath(kPluginVersionMismatch);

  // Act
  bool Result = WasmEdge::Plugin::Plugin::load(Path);

  // Assert
  EXPECT_FALSE(Result);
}

TEST_F(PluginLoaderTest, Lifecycle_RedundantLoad) {
  // Arrange
  auto PathC = getPluginPath(kPluginC);

  // Lifecycle stability check: Redundant loads should not crash.
  // Return value semantics are not asserted.
  // Primary expectation is no crash / no invalid state.

  // Act: Load the same plugin twice
  bool Res1 = WasmEdge::Plugin::Plugin::load(PathC);
  WasmEdge::Plugin::Plugin::load(PathC); // Return value ignored

  // Assert: Should not crash.
  EXPECT_TRUE(Res1);

  auto *Plugin = WasmEdge::Plugin::Plugin::find(kPluginNameC);
  EXPECT_NE(Plugin, nullptr);
}

} // namespace
