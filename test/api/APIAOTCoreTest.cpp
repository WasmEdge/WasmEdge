// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/api/APIAOTVMCoreTest.cpp - WasmEdge C API AOT core tests//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "wasmedge/wasmedge.h"

#include "../spec/spectest.h"
#include "helper.h"
#include "hostfunc_c.h"

#include "gtest/gtest.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#if defined(linux) || defined(__linux) || defined(__linux__) ||                \
    defined(__gnu_linux__)
#define EXTENSION ".so"sv
#elif defined(macintosh) || defined(Macintosh) ||                              \
    (defined(__APPLE__) && defined(__MACH__))
#define EXTENSION ".dylib"sv
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||              \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#define EXTENSION ".dll"sv
#endif

namespace {
using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

/// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge_ConfigureContext *ConfCxt = createConf(Conf);
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(ConfCxt, nullptr);
  WasmEdge_ConfigureCompilerSetOptimizationLevel(
      ConfCxt, WasmEdge_CompilerOptimizationLevel_O0);
  WasmEdge_ConfigureCompilerSetOutputFormat(
      ConfCxt, WasmEdge_CompilerOutputFormat_Native);
  WasmEdge_CompilerContext *CompilerCxt = WasmEdge_CompilerCreate(ConfCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_ImportObjectContext *TestModCxt = createSpecTestModule();
  WasmEdge_VMRegisterModuleFromImport(VM, TestModCxt);

  auto Compile = [&, Conf = std::cref(Conf)](
                     const std::string &Filename) -> Expect<std::string> {
    auto Path = std::filesystem::u8path(Filename);
    Path.replace_extension(std::filesystem::u8path(EXTENSION));
    const auto SOPath = Path.u8string();
    WasmEdge_Result Res =
        WasmEdge_CompilerCompile(CompilerCxt, Filename.c_str(), SOPath.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return SOPath;
  };
  T.onModule = [&VM, &Compile](const std::string &ModName,
                               const std::string &Filename) -> Expect<void> {
    return Compile(Filename).and_then(
        [&VM, &ModName](const std::string &SOFilename) -> Expect<void> {
          WasmEdge_Result Res;
          if (!ModName.empty()) {
            WasmEdge_String ModStr = WasmEdge_StringWrap(
                ModName.data(), static_cast<uint32_t>(ModName.length()));
            Res = WasmEdge_VMRegisterModuleFromFile(VM, ModStr,
                                                    SOFilename.c_str());
          } else {
            Res = WasmEdge_VMLoadWasmFromFile(VM, SOFilename.c_str());
            if (!WasmEdge_ResultOK(Res)) {
              return Unexpect(convResult(Res));
            }
            Res = WasmEdge_VMValidate(VM);
            if (!WasmEdge_ResultOK(Res)) {
              return Unexpect(convResult(Res));
            }
            Res = WasmEdge_VMInstantiate(VM);
          }
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          return {};
        });
  };
  T.onLoad = [&VM, &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename).and_then(
        [&](const std::string &SOFilename) -> Expect<void> {
          WasmEdge_Result Res =
              WasmEdge_VMLoadWasmFromFile(VM, SOFilename.c_str());
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          return {};
        });
  };
  T.onValidate = [&VM, &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename).and_then(
        [&](const std::string &SOFilename) -> Expect<void> {
          WasmEdge_Result Res =
              WasmEdge_VMLoadWasmFromFile(VM, SOFilename.c_str());
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          Res = WasmEdge_VMValidate(VM);
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          return {};
        });
  };
  T.onInstantiate = [&VM,
                     &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename).and_then(
        [&](const std::string &SOFilename) -> Expect<void> {
          WasmEdge_Result Res =
              WasmEdge_VMLoadWasmFromFile(VM, SOFilename.c_str());
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          Res = WasmEdge_VMValidate(VM);
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          Res = WasmEdge_VMInstantiate(VM);
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          return {};
        });
  };
  /// Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params,
                     const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    WasmEdge_Result Res;
    std::vector<WasmEdge_Value> CParams = convFromValVec(Params, ParamTypes);
    std::vector<WasmEdge_Value> CReturns;
    WasmEdge_String FieldStr = WasmEdge_StringWrap(
        Field.data(), static_cast<uint32_t>(Field.length()));
    if (!ModName.empty()) {
      /// Invoke function of named module. Named modules are registered in
      /// Store Manager.
      /// Get the function type to specify the return nums.
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_VMGetFunctionTypeRegistered(VM, ModStr, FieldStr);
      if (FuncType == nullptr) {
        return Unexpect(ErrCode::FuncNotFound);
      }
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      /// Execute.
      Res = WasmEdge_VMExecuteRegistered(
          VM, ModStr, FieldStr, &CParams[0],
          static_cast<uint32_t>(CParams.size()), &CReturns[0],
          static_cast<uint32_t>(CReturns.size()));
    } else {
      /// Invoke function of anonymous module. Anonymous modules are
      /// instantiated in VM.
      /// Get function type to specify the return nums.
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_VMGetFunctionType(VM, FieldStr);
      if (FuncType == nullptr) {
        return Unexpect(ErrCode::FuncNotFound);
      }
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      /// Execute.
      Res = WasmEdge_VMExecute(
          VM, FieldStr, &CParams[0], static_cast<uint32_t>(CParams.size()),
          &CReturns[0], static_cast<uint32_t>(CReturns.size()));
    }
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return convToValVec(CReturns);
  };
  /// Helper function to get values.
  T.onGet = [&VM](const std::string &ModName, const std::string &Field)
      -> Expect<std::pair<ValVariant, ValType>> {
    /// Get global instance.
    WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VM);
    WasmEdge_String ModStr = WasmEdge_StringWrap(
        ModName.data(), static_cast<uint32_t>(ModName.length()));
    WasmEdge_String FieldStr = WasmEdge_StringWrap(
        Field.data(), static_cast<uint32_t>(Field.length()));
    WasmEdge_GlobalInstanceContext *GlobCxt =
        WasmEdge_StoreFindGlobalRegistered(StoreCxt, ModStr, FieldStr);
    if (GlobCxt == nullptr) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    WasmEdge_Value Val = WasmEdge_GlobalInstanceGetValue(GlobCxt);
    return std::make_pair(ValVariant(Val.Value),
                          static_cast<ValType>(Val.Type));
  };

  T.run(Proposal, UnitName);

  WasmEdge_VMDelete(VM);
  WasmEdge_ImportObjectDelete(TestModCxt);
  WasmEdge_CompilerDelete(CompilerCxt);
}

/// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(TestUnit, CoreTest, testing::ValuesIn(T.enumerate()));

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
