// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include <gtest/gtest.h>
#include "common/configure.h"
#include "loader/loader.h"
#include "executor/executor.h"
#include "runtime/storemgr.h"
#include <filesystem>

using namespace WasmEdge;

TEST(WasiExportedMemoryTest, SucceedsIfMemoryExported) {
  // Path to a valid WASI file with exported memory
  const std::string wasmPath = "../../10-01/wasm_file.wasm";
  ASSERT_TRUE(std::filesystem::exists(wasmPath));

  Loader::Loader loader;
  auto modRes = loader.parseModule(wasmPath);
  ASSERT_TRUE(modRes);
  auto &mod = *modRes;

  Runtime::StoreManager storeMgr;
  Configure conf;
  Executor::Executor executor(conf);

  auto instRes = executor.instantiateModule(storeMgr, mod);
  ASSERT_TRUE(instRes);
}
