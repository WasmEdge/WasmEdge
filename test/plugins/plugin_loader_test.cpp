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
  auto Source = getPluginPath(kPluginC);
  auto Dest = TempDir / kPluginC;

  std::error_code EC;
  std::filesystem::copy_file(
      Source, Dest, std::filesystem::copy_options::overwrite_existing, EC);
  ASSERT_FALSE(EC) << "Failed to copy plugin: " << EC.message();

  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(TempDir));
}

TEST_F(PluginLoaderTest, Discovery_HandleEmptyPath) {
  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(std::filesystem::path("")));
  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(
      std::filesystem::path("/non/existent/path/xyz")));
}

TEST_F(PluginLoaderTest, MultiplePluginLoading) {
  auto PathC = getPluginPath(kPluginC);
  auto PathCPP = getPluginPath(kPluginCPP);

  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(PathC));
  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(PathCPP));
}

TEST_F(PluginLoaderTest, Failure_InvalidBinary) {
  // Expected failure: plugin binary is invalid/corrupt
  auto InvalidFile = TempDir / ("invalid" WASMEDGE_LIB_EXTENSION);
  std::ofstream Out(InvalidFile);
  Out << "This is not a plugin binary";
  Out.close();

  EXPECT_FALSE(WasmEdge::Plugin::Plugin::load(InvalidFile));
}

TEST_F(PluginLoaderTest, Failure_MissingSymbols) {
  // Expected failure: plugin missing WasmEdge_Plugin_GetDescriptor symbol
  auto Path = getPluginPath(kPluginIncomplete);

  EXPECT_FALSE(WasmEdge::Plugin::Plugin::load(Path));
}

TEST_F(PluginLoaderTest, Failure_VersionMismatch) {
  // Expected failure: plugin with incompatible API version
  auto Path = getPluginPath(kPluginVersionMismatch);

  EXPECT_FALSE(WasmEdge::Plugin::Plugin::load(Path));
}

TEST_F(PluginLoaderTest, Lifecycle_RedundantLoad) {
  auto PathC = getPluginPath(kPluginC);

  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(PathC));
  EXPECT_NO_THROW(WasmEdge::Plugin::Plugin::load(PathC));
}

} // namespace
