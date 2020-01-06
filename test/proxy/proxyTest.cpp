// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/option/optionTest.cpp - Option parsing tests ------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of parsing options for SSVMRPC.
///
//===----------------------------------------------------------------------===//

#include "proxy/proxy.h"
#include "gtest/gtest.h"

#include "rapidjson/document.h"

#include <cstdlib>
#include <fstream>
#include <string>

namespace {

std::string WasmPath("WasmTestData/calc.wasm");
void readJSONFile(rapidjson::Document &Doc, std::ifstream &FS) {
  /// Get file size
  FS.unsetf(std::ios::skipws);
  FS.seekg(0, std::ios::end);
  std::streampos FileEndPos = FS.tellg();
  FS.seekg(0, std::ios::beg);
  uint32_t FileSize = FileEndPos - FS.tellg();

  /// Read output JSON
  std::vector<char> Data;
  Data.reserve(FileSize + 1);
  Data.insert(Data.begin(), std::istream_iterator<char>(FS),
              std::istream_iterator<char>());
  Data.push_back(0);
  Doc.Parse(&Data[0]);
}

TEST(ProxyTest, Calc__mplus) {
  /// Run input-mplus.json: mplus(255), original 9 in stored memory
  SSVM::Proxy::Proxy VMProxy;
  VMProxy.setInputJSONPath("inputJSONTestData/input-mplus.json");
  VMProxy.setOutputJSONPath("outputJSONTestData/output-mplus.json");
  VMProxy.setWasmPath(WasmPath);
  VMProxy.runRequest();

  /// Check output JSON file
  std::ifstream OutputFS("outputJSONTestData/output-mplus.json",
                         std::ios::binary);
  EXPECT_TRUE(OutputFS.is_open());
  rapidjson::Document Doc;
  readJSONFile(Doc, OutputFS);

  /// Check return value in JSON content
  EXPECT_NE(Doc.FindMember("result"), Doc.MemberEnd());
  EXPECT_NE(Doc["result"].FindMember("return_value"),
            Doc["result"].MemberEnd());
  std::string RetStr = Doc["result"]["return_value"].GetArray()[0].GetString();
  EXPECT_EQ(int64_t(std::strtoull(RetStr.c_str(), nullptr, 10)),
            int64_t(0xFF + 9));
}

TEST(ProxyTest, Calc__mminus) {
  /// Run input-mminus.json: mminus(160), original 9 in stored memory
  SSVM::Proxy::Proxy VMProxy;
  VMProxy.setInputJSONPath("inputJSONTestData/input-mminus.json");
  VMProxy.setOutputJSONPath("outputJSONTestData/output-mminus.json");
  VMProxy.setWasmPath(WasmPath);
  VMProxy.runRequest();

  /// Check output JSON file
  std::ifstream OutputFS("outputJSONTestData/output-mminus.json",
                         std::ios::binary);
  EXPECT_TRUE(OutputFS.is_open());
  rapidjson::Document Doc;
  readJSONFile(Doc, OutputFS);

  /// Check return value in JSON content
  EXPECT_NE(Doc.FindMember("result"), Doc.MemberEnd());
  EXPECT_NE(Doc["result"].FindMember("return_value"),
            Doc["result"].MemberEnd());
  std::string RetStr = Doc["result"]["return_value"].GetArray()[0].GetString();
  EXPECT_EQ(int64_t(std::strtoull(RetStr.c_str(), nullptr, 10)),
            int64_t(9 - 160));
}

TEST(ProxyTest, Calc__mrc) {
  /// Run input-mrc.json: mrc(), original 238 in stored memory
  SSVM::Proxy::Proxy VMProxy;
  VMProxy.setInputJSONPath("inputJSONTestData/input-mrc.json");
  VMProxy.setOutputJSONPath("outputJSONTestData/output-mrc.json");
  VMProxy.setWasmPath(WasmPath);
  VMProxy.runRequest();

  /// Check output JSON file
  std::ifstream OutputFS("outputJSONTestData/output-mrc.json",
                         std::ios::binary);
  EXPECT_TRUE(OutputFS.is_open());
  rapidjson::Document Doc;
  readJSONFile(Doc, OutputFS);

  /// Check return value in JSON content
  EXPECT_NE(Doc.FindMember("result"), Doc.MemberEnd());
  EXPECT_NE(Doc["result"].FindMember("return_value"),
            Doc["result"].MemberEnd());
  std::string RetStr = Doc["result"]["return_value"].GetArray()[0].GetString();
  EXPECT_EQ(int64_t(std::strtoull(RetStr.c_str(), nullptr, 10)), int64_t(238));
}
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
