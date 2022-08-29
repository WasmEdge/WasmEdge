// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "aot/compiler.h"
#include "common/configure.h"
#include "common/errinfo.h"
#include "loader/loader.h"
#include "runtime/instance/module.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <gtest/gtest.h>
#include <iostream>
#include <vector>

namespace {

class HostPrintI32 : public WasmEdge::Runtime::HostFunction<HostPrintI32> {
public:
  WasmEdge::Expect<void> body(const WasmEdge::Runtime::CallingFrame &,
                              uint32_t Val) {
    std::cout << "-- Host Function: print I32 " << Val << std::endl;
    return {};
  }
};

class HostPrintF64 : public WasmEdge::Runtime::HostFunction<HostPrintF64> {
public:
  WasmEdge::Expect<void> body(const WasmEdge::Runtime::CallingFrame &,
                              double Val) {
    std::cout << "-- Host Function: print F64 " << Val << std::endl;
    return {};
  }
};

class HostModule : public WasmEdge::Runtime::Instance::ModuleInstance {
public:
  HostModule() : ModuleInstance("host") {
    addHostFunc("host_printI32", std::make_unique<HostPrintI32>());
    addHostFunc("host_printF64", std::make_unique<HostPrintF64>());
  }
  ~HostModule() noexcept override = default;
};

bool compileModule(const WasmEdge::Configure &Conf, std::string_view InPath,
                   std::string_view OutPath) {
  WasmEdge::Loader::Loader Load(Conf);
  WasmEdge::Validator::Validator Valid(Conf);
  WasmEdge::AOT::Compiler Compiler(Conf);

  auto Mod = Load.parseModule(InPath);
  auto Data = Load.loadFile(InPath);
  if (!Mod || !Data) {
    return false;
  }
  if (auto Res = Valid.validate(*(*Mod).get()); !Res) {
    return false;
  }
  if (auto Res = Compiler.compile(*Data, *(*Mod).get(), OutPath); !Res) {
    return false;
  }
  return true;
}

TEST(MixCallTest, Call__InterpCallAOT) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Expect<void> Res;
  HostModule HostMod;
  std::vector<WasmEdge::ValVariant> FuncArgs;
  std::vector<WasmEdge::ValType> FuncArgTypes;

  // Compile the `module2` into AOT mode.
  EXPECT_TRUE(compileModule(Conf, "mixcallTestData/module2.wasm",
                            "mixcallTestData/module2-uni.wasm"));

  // Register the `module2` and the host module.
  Res = VM.registerModule(HostMod);
  EXPECT_TRUE(Res);
  Res = VM.registerModule("module", "mixcallTestData/module2-uni.wasm");
  EXPECT_TRUE(Res);

  // Instantiate `module1`.
  Res = VM.loadWasm("mixcallTestData/module1.wasm");
  EXPECT_TRUE(Res);
  Res = VM.validate();
  EXPECT_TRUE(Res);
  Res = VM.instantiate();
  EXPECT_TRUE(Res);

  // Run `printAdd`
  FuncArgs = {uint32_t(1234), uint32_t(5678)};
  FuncArgTypes = {WasmEdge::ValType::I32, WasmEdge::ValType::I32};
  auto Ret = VM.execute("printAdd", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printDiv`
  FuncArgs = {double(9876.0), double(4321.0)};
  FuncArgTypes = {WasmEdge::ValType::F64, WasmEdge::ValType::F64};
  Ret = VM.execute("printDiv", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printI32`
  FuncArgs = {uint32_t(87654321)};
  FuncArgTypes = {WasmEdge::ValType::I32};
  Ret = VM.execute("printI32", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printF64`
  FuncArgs = {double(5566.1122)};
  FuncArgTypes = {WasmEdge::ValType::F64};
  Ret = VM.execute("printF64", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);
}

TEST(MixCallTest, Call__AOTCallInterp) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Expect<void> Res;
  HostModule HostMod;
  std::vector<WasmEdge::ValVariant> FuncArgs;
  std::vector<WasmEdge::ValType> FuncArgTypes;

  // Compile the `module1` into AOT mode.
  EXPECT_TRUE(compileModule(Conf, "mixcallTestData/module1.wasm",
                            "mixcallTestData/module1-uni.wasm"));

  // Register the `module2` and the host module.
  Res = VM.registerModule(HostMod);
  EXPECT_TRUE(Res);
  Res = VM.registerModule("module", "mixcallTestData/module2.wasm");
  EXPECT_TRUE(Res);

  // Instantiate `module1`.
  Res = VM.loadWasm("mixcallTestData/module1-uni.wasm");
  EXPECT_TRUE(Res);
  Res = VM.validate();
  EXPECT_TRUE(Res);
  Res = VM.instantiate();
  EXPECT_TRUE(Res);

  // Run `printAdd`
  FuncArgs = {uint32_t(1234), uint32_t(5678)};
  FuncArgTypes = {WasmEdge::ValType::I32, WasmEdge::ValType::I32};
  auto Ret = VM.execute("printAdd", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printDiv`
  FuncArgs = {double(9876.0), double(4321.0)};
  FuncArgTypes = {WasmEdge::ValType::F64, WasmEdge::ValType::F64};
  Ret = VM.execute("printDiv", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printI32`
  FuncArgs = {uint32_t(87654321)};
  FuncArgTypes = {WasmEdge::ValType::I32};
  Ret = VM.execute("printI32", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printF64`
  FuncArgs = {double(5566.1122)};
  FuncArgTypes = {WasmEdge::ValType::F64};
  Ret = VM.execute("printF64", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
