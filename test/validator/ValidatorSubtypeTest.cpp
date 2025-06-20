// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/validator/ValidatorSubtypeTest.cpp - Wasm test suites
//===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for WasmEdge Validator's subtyping tests.
///
//===----------------------------------------------------------------------===//

#include "common/spdlog.h"
#include "vm/vm.h"

#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;

class ValidatorSubtypeTest : public testing::Test {
protected:
  void SetUp() override {
    Conf = std::make_unique<WasmEdge::Configure>();
    Conf->addProposal(WasmEdge::Proposal::GC);
    LoadEngine = std::make_unique<WasmEdge::Loader::Loader>(*Conf);
    ValidEngine = std::make_unique<WasmEdge::Validator::Validator>(*Conf);
  }

  std::unique_ptr<WasmEdge::Configure> Conf;
  std::unique_ptr<WasmEdge::Loader::Loader> LoadEngine;
  std::unique_ptr<WasmEdge::Validator::Validator> ValidEngine;
};

// Helper function to encode an unsigned 32-bit integer as LEB128
static std::vector<WasmEdge::Byte> encodeLEB128(uint32_t Value) {
  std::vector<WasmEdge::Byte> Leb;
  do {
    WasmEdge::Byte Byte = Value & 0x7FU;
    Value >>= 7;
    if (Value != 0) {
      Byte |= 0x80U;
    }
    Leb.push_back(Byte);
  } while (Value != 0);
  return Leb;
}

// Generates a Wasm module with a chain of subtypes.
// NumTotalTypes: The total number of types to define. Type k+1 subtypes type
// k. The maximum depth of the chain will be NumTotalTypes - 1.
static std::vector<WasmEdge::Byte>
generateWasmWithSubtypeChain(int NumTotalTypes) {
  std::vector<WasmEdge::Byte> WasmBytes;
  // Estimate capacity and Reserve a bit more to be safe. Max NumTotalTypes
  // expected is small (e.g. < 100).
  WasmBytes.reserve(static_cast<size_t>(NumTotalTypes) * 12 + 100);

  // 1. Preamble(magic number, wasm v1)
  WasmBytes.insert(WasmBytes.end(),
                   {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00});

  // 2. Type Section (ID 0x01)
  WasmBytes.push_back(0x01);
  uint32_t const TypeSectionPayloadSize =
      static_cast<uint32_t>(6 * NumTotalTypes);
  std::vector<WasmEdge::Byte> TypeSectionSizeLeb =
      encodeLEB128(TypeSectionPayloadSize);
  WasmBytes.insert(WasmBytes.end(), TypeSectionSizeLeb.begin(),
                   TypeSectionSizeLeb.end());

  WasmBytes.push_back(static_cast<WasmEdge::Byte>(NumTotalTypes));

  // First type entry (index 0): (sub (final) (func))
  WasmBytes.insert(WasmBytes.end(), {0x50, 0x00, 0x60, 0x00, 0x00});

  // Next (NumTotalTypes - 1) type entries.
  // Type with index `k+1` subtypes type with index `k`.
  for (int K = 0; K < NumTotalTypes - 1; ++K) {
    WasmBytes.push_back(0x50);
    WasmBytes.push_back(0x01);
    WasmBytes.push_back(static_cast<WasmEdge::Byte>(K));
    WasmBytes.push_back(0x60);
    WasmBytes.push_back(0x00);
    WasmBytes.push_back(0x00);
  }

  // 3. Function Section (ID 0x03) - Fixed content
  WasmBytes.insert(WasmBytes.end(), {0x03, 0x02, 0x01, 0x00});

  // 4. Export Section (ID 0x07) - Fixed content
  WasmBytes.insert(WasmBytes.end(),
                   {0x07, 0x08, 0x01, 0x04, 'm', 'a', 'i', 'n', 0x00, 0x00});

  // 5. Code Section (ID 0x0a) - Fixed content
  WasmBytes.insert(WasmBytes.end(), {0x0a, 0x05, 0x01, 0x03, 0x00, 0x01, 0x0b});

  // 6. Custom "name" Section (ID 0x00)
  // Build the entire payload of the "name" section to determine its size.
  std::vector<WasmEdge::Byte> NameSectionPayloadContent;

  // 6.1. Name of custom section: "name" (vec(byte))
  NameSectionPayloadContent.push_back(0x04);
  NameSectionPayloadContent.push_back('n');
  NameSectionPayloadContent.push_back('a');
  NameSectionPayloadContent.push_back('m');
  NameSectionPayloadContent.push_back('e');

  // 6.2. Function Names Subsection (ID 0x01)
  std::vector<WasmEdge::Byte> FuncNamesSubsectionContent;
  FuncNamesSubsectionContent.push_back(0x01);
  FuncNamesSubsectionContent.push_back(0x00);
  FuncNamesSubsectionContent.push_back(0x04);
  FuncNamesSubsectionContent.push_back('m');
  FuncNamesSubsectionContent.push_back('a');
  FuncNamesSubsectionContent.push_back('i');
  FuncNamesSubsectionContent.push_back('n');

  NameSectionPayloadContent.push_back(0x01);
  std::vector<WasmEdge::Byte> FuncNamesSubSizeLeb =
      encodeLEB128(static_cast<uint32_t>(FuncNamesSubsectionContent.size()));
  NameSectionPayloadContent.insert(NameSectionPayloadContent.end(),
                                   FuncNamesSubSizeLeb.begin(),
                                   FuncNamesSubSizeLeb.end());
  NameSectionPayloadContent.insert(NameSectionPayloadContent.end(),
                                   FuncNamesSubsectionContent.begin(),
                                   FuncNamesSubsectionContent.end());

  // 6.3. Type Names Subsection (ID 0x04)
  std::vector<WasmEdge::Byte> TypeNamesSubsectionContent;
  TypeNamesSubsectionContent.push_back(
      static_cast<WasmEdge::Byte>(NumTotalTypes));
  for (int I = 0; I < NumTotalTypes; ++I) {
    TypeNamesSubsectionContent.push_back(static_cast<WasmEdge::Byte>(I));
    std::string TypeNameStr = "t" + std::to_string(I);
    TypeNamesSubsectionContent.push_back(
        static_cast<WasmEdge::Byte>(TypeNameStr.length()));
    for (char C : TypeNameStr) {
      TypeNamesSubsectionContent.push_back(static_cast<WasmEdge::Byte>(C));
    }
  }

  NameSectionPayloadContent.push_back(0x04);
  std::vector<WasmEdge::Byte> TypeNamesSubSizeLeb =
      encodeLEB128(static_cast<uint32_t>(TypeNamesSubsectionContent.size()));
  NameSectionPayloadContent.insert(NameSectionPayloadContent.end(),
                                   TypeNamesSubSizeLeb.begin(),
                                   TypeNamesSubSizeLeb.end());
  NameSectionPayloadContent.insert(NameSectionPayloadContent.end(),
                                   TypeNamesSubsectionContent.begin(),
                                   TypeNamesSubsectionContent.end());

  // Add the custom "name" section to WasmBytes
  WasmBytes.push_back(0x00);
  std::vector<WasmEdge::Byte> NameSectionTotalSizeLeb =
      encodeLEB128(static_cast<uint32_t>(NameSectionPayloadContent.size()));
  WasmBytes.insert(WasmBytes.end(), NameSectionTotalSizeLeb.begin(),
                   NameSectionTotalSizeLeb.end());
  WasmBytes.insert(WasmBytes.end(), NameSectionPayloadContent.begin(),
                   NameSectionPayloadContent.end());

  return WasmBytes;
}

TEST_F(ValidatorSubtypeTest, MaxSubtypeDepthExceeded) {
  auto Wasm = generateWasmWithSubtypeChain(65);
  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, InvalidTypeIndex) {
  std::array<WasmEdge::Byte, 24> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x02, 0x60,
      0x00, 0x00, 0x50, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x60, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);
  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, InvalidSupertypeIndex) {
  std::array<WasmEdge::Byte, 17> Wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00,
                                         0x00, 0x00, 0x01, 0x07, 0x01, 0x50,
                                         0x01, 0x00, 0x60, 0x00, 0x00};
  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);
  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, ErrorPropagationRecursive) {
  std::array<WasmEdge::Byte, 255> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x27, 0x02, 0x4e,
      0x06, 0x5f, 0x01, 0x7f, 0x00, 0x5f, 0x01, 0x7d, 0x00, 0x5f, 0x01, 0x64,
      0x00, 0x00, 0x5f, 0x01, 0x64, 0x01, 0x00, 0x5f, 0x01, 0x64, 0x02, 0x00,
      0x50, 0x01, 0x04, 0x5f, 0x01, 0x64, 0x03, 0x00, 0x60, 0x01, 0x64, 0x05,
      0x00, 0x03, 0x02, 0x01, 0x06, 0x07, 0x0e, 0x01, 0x0a, 0x74, 0x65, 0x73,
      0x74, 0x5f, 0x75, 0x73, 0x61, 0x67, 0x65, 0x00, 0x00, 0x0a, 0x05, 0x01,
      0x03, 0x00, 0x01, 0x0b, 0x00, 0xb0, 0x01, 0x04, 0x6e, 0x61, 0x6d, 0x65,
      0x01, 0x0d, 0x01, 0x00, 0x0a, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x75, 0x73,
      0x61, 0x67, 0x65, 0x02, 0x06, 0x01, 0x00, 0x01, 0x00, 0x01, 0x70, 0x04,
      0x91, 0x01, 0x06, 0x00, 0x1c, 0x67, 0x72, 0x61, 0x6e, 0x64, 0x70, 0x61,
      0x72, 0x65, 0x6e, 0x74, 0x5f, 0x66, 0x69, 0x65, 0x6c, 0x64, 0x5f, 0x73,
      0x75, 0x70, 0x65, 0x72, 0x5f, 0x74, 0x79, 0x70, 0x65, 0x01, 0x1e, 0x67,
      0x72, 0x61, 0x6e, 0x64, 0x70, 0x61, 0x72, 0x65, 0x6e, 0x74, 0x5f, 0x66,
      0x69, 0x65, 0x6c, 0x64, 0x5f, 0x73, 0x75, 0x62, 0x5f, 0x74, 0x79, 0x70,
      0x65, 0x5f, 0x42, 0x41, 0x44, 0x02, 0x11, 0x70, 0x61, 0x72, 0x65, 0x6e,
      0x74, 0x5f, 0x73, 0x75, 0x70, 0x65, 0x72, 0x5f, 0x74, 0x79, 0x70, 0x65,
      0x03, 0x13, 0x70, 0x61, 0x72, 0x65, 0x6e, 0x74, 0x5f, 0x73, 0x75, 0x62,
      0x5f, 0x74, 0x79, 0x70, 0x65, 0x5f, 0x42, 0x41, 0x44, 0x04, 0x11, 0x61,
      0x63, 0x74, 0x75, 0x61, 0x6c, 0x5f, 0x73, 0x75, 0x70, 0x65, 0x72, 0x5f,
      0x74, 0x79, 0x70, 0x65, 0x05, 0x15, 0x61, 0x63, 0x74, 0x75, 0x61, 0x6c,
      0x5f, 0x73, 0x75, 0x62, 0x5f, 0x74, 0x79, 0x70, 0x65, 0x5f, 0x45, 0x52,
      0x52, 0x4f, 0x52};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);
  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, ErrorPropagationInTypeHierarchy) {
  std::array<WasmEdge::Byte, 30> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x14,
      0x04, 0x60, 0x00, 0x00, 0x5f, 0x00, 0x50, 0x01, 0x01, 0x5f,
      0x01, 0x7f, 0x00, 0x50, 0x01, 0x05, 0x5f, 0x01, 0x7f, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, ExactMaxSubtypeDepth) {
  auto Wasm = generateWasmWithSubtypeChain(64);

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_TRUE(ValidationResult);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
