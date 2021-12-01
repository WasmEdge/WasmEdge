// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/signature/signatureTest.cpp - Signature unit tests --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of signing Wasm modules
///
//===----------------------------------------------------------------------===//

#include "signature/signature.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <ostream>

namespace {
WasmEdge::Signature::Signature SignatureEngine;
TEST(SignatureTEST, KEYGEN) {
  std::vector<unsigned char> PublicKey;
  std::filesystem::path Base = "signatureTestData";
  std::filesystem::path WasmFile = Base / "hello.wasm";
  std::filesystem::path PrikeyFile = Base / "id_ed25519";
  std::filesystem::path PubkeyFile = Base / "id_ed25519.pub";
  testing::internal::CaptureStdout();

  // bool Res;
  RecordProperty("WasmPath: ", WasmFile);
  RecordProperty("CurrentPath: ", std::filesystem::current_path());
  ASSERT_TRUE(std::filesystem::exists(WasmFile));
  EXPECT_TRUE(SignatureEngine.signWasmFile(WasmFile));
  ASSERT_TRUE(std::filesystem::exists(PrikeyFile));
  ASSERT_TRUE(std::filesystem::exists(PubkeyFile));

  // std::string Output = testing::internal::GetCapturedStdout();
  // RecordProperty("STDOUT: ", Output);
  // std::ifstream PubKeyFileStream(PubkeyFile, std::ios::binary);
  // ASSERT_TRUE(PubKeyFileStream.is_open());

  // auto Size = PubKeyFileStream.tellg();
  // std::string KeyStr(Size, '\0');

  // EXPECT_TRUE(PubKeyFileStream.seekg(0));
  // EXPECT_TRUE(PubKeyFileStream.read(&KeyStr[0], Size));
  // RecordProperty("Public Key: ", KeyStr);

  // EXPECT_TRUE(Res = *(SignatureEngine.verifyWasmFile(WasmFile, PrikeyFile)));
  // RecordProperty("Verify Result: ", Res);

  // RecordProperty("STDOUT: ", Output);
}

TEST(SignatureTEST, VERIFY) {}
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
