// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/configure.h"
#include "common/errcode.h"
#include "loader/loader.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

namespace {

using namespace std::literals;

constexpr std::string_view MinimalWatSource = "(module)"sv;

std::vector<uint8_t> watBytes() {
  return std::vector<uint8_t>(MinimalWatSource.begin(), MinimalWatSource.end());
}

TEST(LoaderWatGate, RejectsWatWhenDisabled) {
  WasmEdge::Configure Conf;
  ASSERT_FALSE(Conf.isEnableWAT());

  WasmEdge::Loader::Loader Ld(Conf);
  auto Res = Ld.parseModule(WasmEdge::Span<const uint8_t>(watBytes()));
  ASSERT_FALSE(Res);
  // The bytes fall through to the binary-load path; the existing error path
  // produces a MalformedMagic since "(module)" has no WASM magic.
  EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::MalformedMagic);
}

TEST(LoaderWatGate, AcceptsWatWhenEnabled) {
  WasmEdge::Configure Conf;
  Conf.setEnableWAT(true);
  ASSERT_TRUE(Conf.isEnableWAT());

  WasmEdge::Loader::Loader Ld(Conf);
  auto Res = Ld.parseModule(WasmEdge::Span<const uint8_t>(watBytes()));
  ASSERT_TRUE(Res) << "WAT load failed with code "
                   << static_cast<uint32_t>(Res.error());
}

// RAII helper that removes a temp file on scope exit, even if the test fails.
// Uses the error_code overload so the destructor never throws.
struct TempFile {
  std::filesystem::path Path;
  explicit TempFile(std::filesystem::path P) : Path(std::move(P)) {}
  ~TempFile() {
    std::error_code EC;
    std::filesystem::remove(Path, EC);
  }
  // Non-copyable.
  TempFile(const TempFile &) = delete;
  TempFile &operator=(const TempFile &) = delete;
};

// Write MinimalWatSource to a temp file and return the RAII guard.
TempFile writeTempWatFile() {
  auto P = std::filesystem::temp_directory_path() /
           "wasmedge_loader_wat_gate_file.wat";
  std::ofstream Fout(P, std::ios::out | std::ios::trunc | std::ios::binary);
  // Surface temp-dir permission/quota failures here instead of letting the
  // Loader trip over a zero-byte or missing file downstream.
  if (!Fout.is_open()) {
    ADD_FAILURE() << "Unable to create temp WAT file at " << P;
    return TempFile(std::move(P));
  }
  Fout.write(MinimalWatSource.data(),
             static_cast<std::streamsize>(MinimalWatSource.size()));
  if (!Fout.good()) {
    ADD_FAILURE() << "Failed to write temp WAT file at " << P;
  }
  return TempFile(std::move(P));
}

TEST(LoaderWatGate, RejectsWatFileWhenDisabled) {
  auto Guard = writeTempWatFile();

  WasmEdge::Configure Conf;
  ASSERT_FALSE(Conf.isEnableWAT());

  WasmEdge::Loader::Loader Ld(Conf);
  auto Res = Ld.parseModule(Guard.Path);
  ASSERT_FALSE(Res);
  // The file bytes fall through to the binary-load path and produce
  // MalformedMagic since "(module)" has no WASM magic.
  EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::MalformedMagic);
}

TEST(LoaderWatGate, AcceptsWatFileWhenEnabled) {
  auto Guard = writeTempWatFile();

  WasmEdge::Configure Conf;
  Conf.setEnableWAT(true);
  ASSERT_TRUE(Conf.isEnableWAT());

  WasmEdge::Loader::Loader Ld(Conf);
  auto Res = Ld.parseModule(Guard.Path);
  ASSERT_TRUE(Res) << "WAT file load failed with code "
                   << static_cast<uint32_t>(Res.error());
}

} // namespace
