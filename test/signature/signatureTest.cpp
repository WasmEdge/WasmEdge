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

namespace {
WasmEdge::Signature::Signature SignatureEngine;
TEST(SignatureTEST, HELLO) {
  std::vector<unsigned char> PublicKey;
  std::filesystem::path Base = "signatureTestData";
  std::filesystem::path WasmFile = Base / "hello.wasm";
  std::filesystem::path PrikeyFile = Base / "id_ed25519";
  std::filesystem::path PubkeyFile = Base / "id_ed25519.pub";
  bool Res;
  ASSERT_TRUE(SignatureEngine.signWasmFile(WasmFile));
  EXPECT_TRUE(Res = *(SignatureEngine.verifyWasmFile(WasmFile, PrikeyFile)));
}
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
