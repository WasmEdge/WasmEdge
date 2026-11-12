// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>

namespace WasmEdge {
namespace LLVM {
namespace Test {

class ScopedTempDirectory {
public:
  explicit ScopedTempDirectory(const std::string &Prefix) {
    static std::atomic<uint64_t> Sequence{0};
    std::random_device Random;
    const auto Parent = std::filesystem::temp_directory_path();
    while (true) {
      Path = Parent / (Prefix + std::to_string(Random()) + "-" +
                       std::to_string(Sequence.fetch_add(1)));
      std::error_code Error;
      if (std::filesystem::create_directory(Path, Error))
        return;
      if (Error)
        throw std::filesystem::filesystem_error("create temp directory", Path,
                                                Error);
    }
  }

  ScopedTempDirectory(const ScopedTempDirectory &) = delete;
  ScopedTempDirectory &operator=(const ScopedTempDirectory &) = delete;

  ~ScopedTempDirectory() noexcept {
    if (Kept)
      return;
    std::error_code Error;
    std::filesystem::remove_all(Path, Error);
  }

  const std::filesystem::path &path() const noexcept { return Path; }

  void keep() noexcept { Kept = true; }

private:
  std::filesystem::path Path;
  bool Kept = false;
};

} // namespace Test
} // namespace LLVM
} // namespace WasmEdge
