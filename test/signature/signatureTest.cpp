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

namespace fs = std::filesystem;

namespace {
WasmEdge::Signature::Signature SignatureEngine;
TEST(SignatureTEST, KEYGEN) {
  std::vector<unsigned char> PublicKey;
  fs::path Base = "signatureTestData";
  fs::path WasmFile = Base / "hello.wasm";
  fs::path PrikeyFile = Base / "id_ed25519";
  fs::path PubkeyFile = Base / "id_ed25519.pub";
  testing::internal::CaptureStdout();

  // bool Res;
  if (fs ::exists(PrikeyFile))
    ASSERT_TRUE(fs::remove(PrikeyFile));

  if (fs ::exists(PubkeyFile))
    ASSERT_TRUE(fs::remove(PubkeyFile));

  EXPECT_TRUE(!fs::exists(PrikeyFile));
  EXPECT_TRUE(!fs::exists(PubkeyFile));
  RecordProperty("WasmPath: ", WasmFile);
  RecordProperty("CurrentPath: ", fs::current_path());
  ASSERT_TRUE(fs::exists(WasmFile));
  EXPECT_TRUE(SignatureEngine.signWasmFile(WasmFile));
  ASSERT_TRUE(fs::exists(PrikeyFile));
  ASSERT_TRUE(fs::exists(PubkeyFile));
}

TEST(SignatureTEST, VERIFY) {
  std::vector<unsigned char> PublicKey;
  fs::path Base = "signatureTestData";
  fs::path WasmFile = Base / "hello.wasm";
  fs::path PrikeyFile = Base / "id_ed25519";
  fs::path PubkeyFile = Base / "id_ed25519.pub";

  ASSERT_TRUE(fs::exists(WasmFile));
  ASSERT_TRUE(SignatureEngine.signWasmFile(WasmFile));

  // std::string Output = testing::internal::GetCapturedStdout();
  // RecordProperty("STDOUT: ", Output);
  // std::ifstream PubKeyFileStream(PubkeyFile, std::ios::binary);
  // ASSERT_TRUE(PubKeyFileStream.is_open());

  // auto Size = PubKeyFileStream.tellg();
  // std::string KeyStr(Size, '\0');

  // EXPECT_TRUE(PubKeyFileStream.seekg(0));
  // EXPECT_TRUE(PubKeyFileStream.read(&KeyStr[0], Size));
  // RecordProperty("Public Key: ", KeyStr);

  EXPECT_TRUE(SignatureEngine.verifyWasmFile(WasmFile, PubkeyFile));

  // RecordProperty("STDOUT: ", Output);
}
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::GTEST_FLAG(output) = "xml:./sign_gtest.xml";
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}