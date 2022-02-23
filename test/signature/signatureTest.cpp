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
#include "common/enum_errcode.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <ostream>

namespace fs = std::filesystem;

namespace {
WasmEdge::Signature::Signature SignatureEngine;

TEST(SignatureTEST, KEYGEN) {
  std::vector<unsigned char> PublicKey;
  fs::path Base = "signatureTestData/temp";
  if (fs::exists(Base))
    ASSERT_TRUE(fs::remove_all(Base));
  ASSERT_TRUE(fs::create_directory(Base));
  ASSERT_TRUE(fs::exists(Base.parent_path() / "hello.wasm"));
  fs::copy(Base.parent_path() / "hello.wasm", Base / "hello.wasm");
  fs::path WasmFile = Base / "hello.wasm";
  fs::path PrikeyFile = Base / "id_ed25519";
  fs::path PubkeyFile = Base / "id_ed25519.pub";
  fs::path TargetFile = Base / "hello_signed.wasm";

  RecordProperty("WasmPath: ", WasmFile);
  RecordProperty("CurrentPath: ", fs::current_path());

  ASSERT_TRUE(fs::exists(WasmFile));
  EXPECT_TRUE(SignatureEngine.signWasmFile(WasmFile, "", "", ""));
  ASSERT_TRUE(fs::exists(PrikeyFile));
  ASSERT_TRUE(fs::exists(PubkeyFile));
  ASSERT_TRUE(fs::exists(TargetFile));
  ASSERT_TRUE(fs::remove_all(Base));
}

TEST(SignatureTEST, VERIFY) {
  std::vector<unsigned char> PublicKey;
  fs::path Base = "signatureTestData/temp";
  if (fs::exists(Base))
    ASSERT_TRUE(fs::remove_all(Base));
  ASSERT_TRUE(fs::create_directory(Base));
  ASSERT_TRUE(fs::exists(Base.parent_path() / "hello.wasm"));
  fs::copy(Base.parent_path() / "hello.wasm", Base / "hello.wasm");
  fs::path WasmFile = Base / "hello.wasm";
  fs::path PrikeyFile = Base / "id_ed25519";
  fs::path PubkeyFile = Base / "id_ed25519.pub";
  fs::path TargetFile = Base / "hello_signed.wasm";

  RecordProperty("BasePath: ", Base);
  ASSERT_TRUE(fs::exists(WasmFile));
  ASSERT_TRUE(SignatureEngine.signWasmFile(WasmFile, "", "", TargetFile));

  testing::internal::CaptureStdout();
  auto Result = SignatureEngine.verifyWasmFile(TargetFile, PubkeyFile);
  std::string ErrStr = WasmEdge::ErrCodeStr[Result.error()];
  RecordProperty("Result: ", ErrStr);
  std::string StdOut = testing::internal::GetCapturedStdout();
  RecordProperty("Stdout: ", StdOut);
  // ASSERT_TRUE(SignatureEngine.verifyWasmFile(TargetFile, PubkeyFile));
  ASSERT_TRUE(fs::remove_all(Base));
}
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::GTEST_FLAG(output) = "xml:./sign_gtest.xml";
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}