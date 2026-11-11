// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasinn_mlx.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// A raw-bytes MLX load writes the model to a temporary safetensors file before
// conversion. Concurrent loads must not share that path, or one build can
// truncate the file another build is still reading. uniqueModelFileName is
// backend-agnostic string logic, so this guard runs in every build.
TEST(WasiNNTest, MLXTempPathIsUniquePerBuild) {
  constexpr std::size_t Count = 64;
  std::vector<std::string> Paths(Count);
  std::vector<std::thread> Workers;
  Workers.reserve(Count);
  for (std::size_t I = 0; I < Count; ++I) {
    Workers.emplace_back([&Paths, I]() {
      Paths[I] = WasmEdge::Host::WASINN::MLX::uniqueModelFileName(0);
    });
  }
  for (auto &Worker : Workers) {
    Worker.join();
  }
  const std::set<std::string> Distinct(Paths.begin(), Paths.end());
  EXPECT_EQ(Distinct.size(), Count);
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
