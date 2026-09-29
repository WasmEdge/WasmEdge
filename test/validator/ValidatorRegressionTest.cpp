// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/validator/ValidatorRegressionTest.cpp - Wasm test suites
//===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains regression tests for WasmEdge Validator.
///
//===----------------------------------------------------------------------===//

#include "ast/module.h"
#include "common/spdlog.h"
#include "vm/vm.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;

class ValidatorRegressionTest : public testing::Test {
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

TEST_F(ValidatorRegressionTest, MaxSubtypeDepthExceeded) {
  auto Wasm = generateWasmWithSubtypeChain(65);
  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorRegressionTest, InvalidTypeIndex) {
  std::array<WasmEdge::Byte, 24> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x02, 0x60,
      0x00, 0x00, 0x50, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x60, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);
  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorRegressionTest, InvalidSupertypeIndex) {
  std::array<WasmEdge::Byte, 17> Wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00,
                                         0x00, 0x00, 0x01, 0x07, 0x01, 0x50,
                                         0x01, 0x00, 0x60, 0x00, 0x00};
  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);
  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorRegressionTest, ForwardSupertypeInRecGroup) {
  // Module: (module
  //   (rec
  //     (type $mid (sub $top (func (result i32))))
  //     (type $top (sub (func (result i32))))))
  //
  // $mid (type index 0) declares supertype $top (type index 1), a forward
  // reference. The GC spec requires a declared supertype to have a smaller
  // type index than the sub type itself, so validation must fail.
  std::array<WasmEdge::Byte, 26> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      // Type section: one rec group of 2 sub types
      0x01, 0x10, 0x01, 0x4e, 0x02,
      // type 0 ($mid): (sub 1 (func (result i32)))  -- forward supertype 1
      0x50, 0x01, 0x01, 0x60, 0x00, 0x01, 0x7f,
      // type 1 ($top): (sub (func (result i32)))
      0x50, 0x00, 0x60, 0x00, 0x01, 0x7f};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorRegressionTest, CyclicSupertypeInRecGroup) {
  // Module: (module
  //   (rec
  //     (type $a (sub (func (result (ref null $a)))))
  //     (type (sub $a (func (result (ref null $c)))))
  //     (type $c (sub $c (func)))))
  //
  // Matching type 1 walks the super types of $c, which names itself, so
  // validation must reject $c before matching instead of recursing forever.
  std::array<WasmEdge::Byte, 34> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      // Type section: one rec group of 3 sub types
      0x01, 0x18, 0x01, 0x4e, 0x03,
      // type 0 ($a): (sub (func (result (ref null 0))))
      0x50, 0x00, 0x60, 0x00, 0x01, 0x63, 0x00,
      // type 1: (sub 0 (func (result (ref null 2))))
      0x50, 0x01, 0x00, 0x60, 0x00, 0x01, 0x63, 0x02,
      // type 2 ($c): (sub 2 (func))  -- its own supertype
      0x50, 0x01, 0x02, 0x60, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorRegressionTest, ErrorPropagationRecursive) {
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

TEST_F(ValidatorRegressionTest, ErrorPropagationInTypeHierarchy) {
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

TEST_F(ValidatorRegressionTest, ExactMaxSubtypeDepth) {
  auto Wasm = generateWasmWithSubtypeChain(64);

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_TRUE(ValidationResult);
}

TEST_F(ValidatorRegressionTest, RejectMismatchedActiveElementRefType) {
  // Module: (module
  //   (type $t0 (sub (func)))
  //   (type $t1 (sub final $t0 (func)))
  //   (import "env" "f" (func (type $t0)))
  //   (table 1 (ref null $t1))
  //   (elem (table 0) (i32.const 0) (ref $t0) (ref.func 0)))
  //
  // The element segment's ref type (ref $t0) must be a subtype of the
  // table's element type (ref null $t1). Since $t0 is a supertype of $t1
  // (not a subtype), validation must fail with TypeCheckFailed.
  std::array<WasmEdge::Byte, 54> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      // Type section: 2 types
      //   type 0: (sub (func))             -- non-final, no super
      //   type 1: (sub final 0 (func))     -- final, supertype 0
      0x01, 0x0c, 0x02, 0x50, 0x00, 0x60, 0x00, 0x00, 0x4f, 0x01, 0x00, 0x60,
      0x00, 0x00,
      // Import section: "env"."f" func of type 0
      0x02, 0x09, 0x01, 0x03, 0x65, 0x6e, 0x76, // "env"
      0x01, 0x66,                               // "f"
      0x00, 0x00,                               // func kind, typeidx 0
      // Table section: 1 table of type (ref null 1), min 1
      0x04, 0x05, 0x01, 0x63, 0x01, // reftype: ref null, typeindex 1
      0x00, 0x01,                   // limits: min=1, no max
      // Element section: 1 active segment, table 0, offset i32.const 0,
      //   reftype (ref 0), init [ref.func 0]
      0x09, 0x0c, 0x01,
      0x06,             // form 6: active, tableidx, offset, reftype, vec(expr)
      0x00,             // tableidx 0
      0x41, 0x00, 0x0b, // offset: i32.const 0, end
      0x64, 0x00,       // reftype: (ref 0)
      0x01,             // 1 init expr
      0xd2, 0x00, 0x0b  // ref.func 0, end
  };

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(),
            WasmEdge::ErrCode::Value::TypeCheckFailed);
}

TEST_F(ValidatorRegressionTest, ActiveElemTypedFuncrefAssert) {
  // Module: (module
  //   (type $t0 (func (param i32)))
  //   (type $t1 (func))
  //   (func $f0 (type $t1))
  //   (func $f1 (type $t0) (local.get 0) (drop))
  //   (func $f2 (type $t1)
  //     (call_ref $t1 (table.get 0 (i32.const 0))))
  //   (table 1 (ref null $t1))
  //   (elem (table 0) (i32.const 0) func $f1)   ;; implicit funcref
  //   (export "trigger" (func $f2)))
  std::array<WasmEdge::Byte, 74> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02,
      0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x03, 0x04, 0x03, 0x01,
      0x00, 0x01, 0x04, 0x05, 0x01, 0x63, 0x01, 0x00, 0x01, 0x07, 0x0b,
      0x01, 0x07, 0x74, 0x72, 0x69, 0x67, 0x67, 0x65, 0x72, 0x00, 0x02,
      0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x01, 0x0a, 0x13,
      0x03, 0x02, 0x00, 0x0b, 0x05, 0x00, 0x20, 0x00, 0x1a, 0x0b, 0x08,
      0x00, 0x41, 0x00, 0x25, 0x00, 0x14, 0x01, 0x0b};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(),
            WasmEdge::ErrCode::Value::TypeCheckFailed);
}

TEST_F(ValidatorRegressionTest, ActiveElemTypedFuncrefTrap) {
  // Module: (module
  //   (type $t0 (func))
  //   (type $t1 (func (param (ref $t0))))
  //   (type $t2 (sub final $t1 (func (param funcref))))
  //   (type $t3 (func))
  //   (func $f0 (type $t1) (call_ref $t0 (local.get 0)))
  //   (func $f1 (type $t2) (local.get 0) (drop))
  //   (func $f2 (type $t0))
  //   (func $f3 (type $t3)
  //     (call_ref $t2 (ref.null func) (table.get 0 (i32.const 0))))
  //   (table 1 (ref null $t2))
  //   (elem (table 0) (i32.const 0) func $f0)   ;; implicit funcref
  //   (export "trigger" (func $f3)))
  std::array<WasmEdge::Byte, 95> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x13, 0x04, 0x60,
      0x00, 0x00, 0x60, 0x01, 0x64, 0x00, 0x00, 0x4f, 0x01, 0x01, 0x60, 0x01,
      0x70, 0x00, 0x60, 0x00, 0x00, 0x03, 0x05, 0x04, 0x01, 0x02, 0x00, 0x03,
      0x04, 0x05, 0x01, 0x63, 0x02, 0x00, 0x01, 0x07, 0x0b, 0x01, 0x07, 0x74,
      0x72, 0x69, 0x67, 0x67, 0x65, 0x72, 0x00, 0x03, 0x09, 0x07, 0x01, 0x00,
      0x41, 0x00, 0x0b, 0x01, 0x00, 0x0a, 0x1c, 0x04, 0x06, 0x00, 0x20, 0x00,
      0x14, 0x00, 0x0b, 0x05, 0x00, 0x20, 0x00, 0x1a, 0x0b, 0x02, 0x00, 0x0b,
      0x0a, 0x00, 0xd0, 0x70, 0x41, 0x00, 0x25, 0x00, 0x14, 0x02, 0x0b};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

// Test that ref.test after unreachable passes validation.
// The unreachable instruction produces bottom types on the stack, which should
// be accepted by ref.test per the Wasm spec.
TEST_F(ValidatorRegressionTest, RefTestAfterUnreachable) {
  // Module: (module
  //   (type (func))
  //   (func (type 0) unreachable  ref.test (ref func)  drop  end))
  std::array<WasmEdge::Byte, 29> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      // Type section: 1 type (func)
      0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
      // Function section: 1 function, type index 0
      0x03, 0x02, 0x01, 0x00,
      // Code section: 1 body
      // Body: unreachable (0x00), ref.test (0xfb 0x14), heaptype func (0x70),
      //       drop (0x1a), end (0x0b)
      0x0a, 0x09, 0x01, 0x07, 0x00, 0x00, 0xfb, 0x14, 0x70, 0x1a, 0x0b};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_TRUE(ValidationResult);
}

// Test that ref.cast after unreachable passes validation.
// The unreachable instruction produces bottom types on the stack, which should
// be accepted by ref.cast per the Wasm spec.
TEST_F(ValidatorRegressionTest, RefCastAfterUnreachable) {
  // Module: (module
  //   (type (func))
  //   (func (type 0) unreachable  ref.cast (ref null func)  drop  end))
  std::array<WasmEdge::Byte, 29> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      // Type section: 1 type (func)
      0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
      // Function section: 1 function, type index 0
      0x03, 0x02, 0x01, 0x00,
      // Code section: 1 body
      // Body: unreachable (0x00), ref.cast_null (0xfb 0x17),
      //       heaptype func (0x70), drop (0x1a), end (0x0b)
      0x0a, 0x09, 0x01, 0x07, 0x00, 0x00, 0xfb, 0x17, 0x70, 0x1a, 0x0b};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_TRUE(ValidationResult);
}

// Test that ref.func on an imported function is rejected when the function is
// not declared. Imported functions are not part of the module's declared
// function reference set, so they need an explicit declarative element segment
// like any other function.
TEST_F(ValidatorRegressionTest, UndeclaredRefFuncToImportedFunction) {
  // Module: (module
  //   (type (func))
  //   (import "env" "f" (func (type 0)))
  //   (func (type 0) ref.func 0 drop))
  std::array<WasmEdge::Byte, 38> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      // Type section: 1 type (func)
      0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
      // Import section: 1 import "env" "f" as function of type 0
      0x02, 0x09, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x01, 0x66, 0x00, 0x00,
      // Function section: 1 function, type index 0
      0x03, 0x02, 0x01, 0x00,
      // Code section: 1 body
      // Body: ref.func 0 (0xd2 0x00), drop (0x1a), end (0x0b)
      0x0a, 0x07, 0x01, 0x05, 0x00, 0xd2, 0x00, 0x1a, 0x0b};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidRefIdx);
}

// Test that ref.func on an imported function passes validation once the
// function is declared by a declarative element segment.
TEST_F(ValidatorRegressionTest, DeclaredRefFuncToImportedFunction) {
  // Module: (module
  //   (type (func))
  //   (import "env" "f" (func (type 0)))
  //   (elem declare func 0)
  //   (func (type 0) ref.func 0 drop))
  std::array<WasmEdge::Byte, 45> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
      // Type section: 1 type (func)
      0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
      // Import section: 1 import "env" "f" as function of type 0
      0x02, 0x09, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x01, 0x66, 0x00, 0x00,
      // Function section: 1 function, type index 0
      0x03, 0x02, 0x01, 0x00,
      // Element section: 1 declarative segment declaring function 0
      0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00,
      // Code section: 1 body
      // Body: ref.func 0 (0xd2 0x00), drop (0x1a), end (0x0b)
      0x0a, 0x07, 0x01, 0x05, 0x00, 0xd2, 0x00, 0x1a, 0x0b};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_TRUE(ValidationResult);
}

struct WideArithmeticEncoding {
  uint8_t Subopcode;
  bool IsAddOrSub;
  OpCode Opcode;
};

constexpr WideArithmeticEncoding WideArithmeticEncodings[] = {
    {0x13U, true, OpCode::I64__add128},
    {0x14U, true, OpCode::I64__sub128},
    {0x15U, false, OpCode::I64__mul_wide_s},
    {0x16U, false, OpCode::I64__mul_wide_u}};

static AST::Module makeWideArithmeticModule(
    OpCode Opcode, const std::vector<ValType> &Params,
    const std::vector<ValType> &Returns = {TypeCode::I64, TypeCode::I64}) {
  AST::Module Module;
  AST::FunctionType Type;
  Type.getParamTypes() = Params;
  Type.getReturnTypes() = Returns;
  Module.getTypeSection().getContent().emplace_back(Type);
  Module.getFunctionSection().getContent().push_back(0U);
  AST::CodeSegment Code;
  for (uint32_t I = 0; I < Params.size(); ++I) {
    AST::Instruction Get(OpCode::Local__get);
    Get.getTargetIndex() = I;
    Code.getExpr().getInstrs().push_back(Get);
  }
  Code.getExpr().getInstrs().emplace_back(Opcode);
  Code.getExpr().getInstrs().emplace_back(OpCode::End);
  Module.getCodeSection().getContent().push_back(Code);
  return Module;
}

TEST_F(ValidatorRegressionTest, ValidateWideArithmeticStackTypes) {
  Conf->addProposal(Proposal::WideArithmetic);
  Validator::Validator Validator(*Conf);
  Configure DisabledConf;
  DisabledConf.removeProposal(Proposal::WideArithmetic);
  Validator::Validator DisabledValidator(DisabledConf);
  for (const auto &Encoding : WideArithmeticEncodings) {
    SCOPED_TRACE(static_cast<unsigned>(Encoding.Subopcode));
    const size_t Arity = Encoding.IsAddOrSub ? 4U : 2U;
    const std::vector<ValType> Params(Arity, TypeCode::I64);
    auto Module = makeWideArithmeticModule(Encoding.Opcode, Params);
    EXPECT_TRUE(Validator.validate(Module));
    auto DisabledResult = DisabledValidator.validate(Module);
    ASSERT_FALSE(DisabledResult);
    EXPECT_EQ(DisabledResult.error(), ErrCode::Value::IllegalOpCode);

    for (size_t I = 0; I < Arity; ++I) {
      SCOPED_TRACE(I);
      for (const auto WrongType :
           {TypeCode::I32, TypeCode::F32, TypeCode::F64}) {
        SCOPED_TRACE(static_cast<unsigned>(WrongType));
        auto WrongParams = Params;
        WrongParams[I] = WrongType;
        auto Wrong = makeWideArithmeticModule(Encoding.Opcode, WrongParams);
        auto Result = Validator.validate(Wrong);
        ASSERT_FALSE(Result);
        EXPECT_EQ(Result.error(), ErrCode::Value::TypeCheckFailed);
      }
    }

    for (const size_t Count : {Arity - 1U, Arity + 1U}) {
      SCOPED_TRACE(Count);
      auto WrongArity = makeWideArithmeticModule(
          Encoding.Opcode, std::vector<ValType>(Count, TypeCode::I64));
      auto Result = Validator.validate(WrongArity);
      ASSERT_FALSE(Result);
      EXPECT_EQ(Result.error(), ErrCode::Value::TypeCheckFailed);
    }
  }
}

TEST_F(ValidatorRegressionTest, ValidateWideArithmeticResultTypes) {
  Conf->addProposal(Proposal::WideArithmetic);
  Validator::Validator Validator(*Conf);
  for (const auto &Encoding : WideArithmeticEncodings) {
    SCOPED_TRACE(static_cast<unsigned>(Encoding.Subopcode));
    const std::vector<ValType> Params(Encoding.IsAddOrSub ? 4U : 2U,
                                      TypeCode::I64);
    for (const size_t Count : {0U, 1U, 3U}) {
      SCOPED_TRACE(Count);
      auto Module = makeWideArithmeticModule(
          Encoding.Opcode, Params, std::vector<ValType>(Count, TypeCode::I64));
      auto Result = Validator.validate(Module);
      ASSERT_FALSE(Result);
      EXPECT_EQ(Result.error(), ErrCode::Value::TypeCheckFailed);
    }
    for (size_t I = 0; I < 2U; ++I) {
      SCOPED_TRACE(I);
      for (const auto WrongType :
           {TypeCode::I32, TypeCode::F32, TypeCode::F64}) {
        SCOPED_TRACE(static_cast<unsigned>(WrongType));
        std::vector<ValType> Returns(2U, TypeCode::I64);
        Returns[I] = WrongType;
        auto Module =
            makeWideArithmeticModule(Encoding.Opcode, Params, Returns);
        auto Result = Validator.validate(Module);
        ASSERT_FALSE(Result);
        EXPECT_EQ(Result.error(), ErrCode::Value::TypeCheckFailed);
      }
    }
  }
}

TEST_F(ValidatorRegressionTest, PreserveWideArithmeticStackPrefix) {
  Conf->addProposal(Proposal::WideArithmetic);
  Validator::Validator Validator(*Conf);
  for (const auto &Encoding : WideArithmeticEncodings) {
    SCOPED_TRACE(static_cast<unsigned>(Encoding.Subopcode));
    std::vector<ValType> Params(Encoding.IsAddOrSub ? 4U : 2U, TypeCode::I64);
    Params.insert(Params.begin(), TypeCode::I32);
    auto Module = makeWideArithmeticModule(
        Encoding.Opcode, Params, {TypeCode::I32, TypeCode::I64, TypeCode::I64});
    EXPECT_TRUE(Validator.validate(Module));
  }
}

TEST_F(ValidatorRegressionTest, ValidateWideArithmeticUnreachableStack) {
  Conf->addProposal(Proposal::WideArithmetic);
  Validator::Validator Validator(*Conf);
  for (const auto &Encoding : WideArithmeticEncodings) {
    SCOPED_TRACE(static_cast<unsigned>(Encoding.Subopcode));
    for (const bool WrongType : {false, true}) {
      SCOPED_TRACE(WrongType);
      auto Module = makeWideArithmeticModule(Encoding.Opcode, {});
      auto &Body =
          Module.getCodeSection().getContent()[0].getExpr().getInstrs();
      if (WrongType) {
        AST::Instruction Const(OpCode::I32__const);
        Const.setNum(uint32_t(0));
        Body.insert(Body.begin(), Const);
      }
      Body.insert(Body.begin(), AST::Instruction(OpCode::Unreachable));
      auto Result = Validator.validate(Module);
      if (WrongType) {
        ASSERT_FALSE(Result);
        EXPECT_EQ(Result.error(), ErrCode::Value::TypeCheckFailed);
      } else {
        EXPECT_TRUE(Result);
      }
    }
  }
}

TEST_F(ValidatorRegressionTest, RejectWideArithmeticASTWithProposalDisabled) {
  Conf->removeProposal(Proposal::WideArithmetic);
  Validator::Validator Validator(*Conf);
  for (const auto &Encoding : WideArithmeticEncodings) {
    SCOPED_TRACE(static_cast<unsigned>(Encoding.Subopcode));
    for (const bool Unreachable : {false, true}) {
      SCOPED_TRACE(Unreachable);
      auto Module = makeWideArithmeticModule(
          Encoding.Opcode,
          std::vector<ValType>(Encoding.IsAddOrSub ? 4U : 2U, TypeCode::I64));
      if (Unreachable) {
        auto &Body =
            Module.getCodeSection().getContent()[0].getExpr().getInstrs();
        Body.insert(Body.begin(), AST::Instruction(OpCode::Unreachable));
      }
      auto Result = Validator.validate(Module);
      ASSERT_FALSE(Result);
      EXPECT_EQ(Result.error(), ErrCode::Value::IllegalOpCode);
      EXPECT_FALSE(Module.getIsValidated());
    }
  }
}

static std::vector<Byte>
makeWideArithmeticFunction(const WideArithmeticEncoding &Encoding,
                           bool Unreachable = false) {
  // Add/sub module (use i64.sub128 for subtraction):
  // (module
  //   (type (func))
  //   (func (export "run") (type 0)
  //     i64.const 1 i64.const 1 i64.const 1 i64.const 1
  //     i64.add128 drop drop))
  // Multiplication module (use i64.mul_wide_u for unsigned multiplication):
  // (module
  //   (type (func))
  //   (func (export "run") (type 0)
  //     i64.const 1 i64.const 1
  //     i64.mul_wide_s drop drop))
  // The unreachable variant inserts unreachable before the first constant.
  std::vector<Byte> Body = {0x00}; // Local declarations: empty vector.
  if (Unreachable) {
    Body.push_back(0x00); // OpCode Unreachable.
  }
  const uint32_t OperandCount = Encoding.IsAddOrSub ? 4U : 2U;
  for (uint32_t I = 0; I < OperandCount; ++I) {
    Body.insert(Body.end(), {0x42, 0x01}); // OpCode I64__const, value 1.
  }
  // Wide opcode, drop both i64 results, expression end.
  Body.insert(Body.end(), {0xFC, Encoding.Subopcode, 0x1A, 0x1A, 0x0B});

  std::vector<Byte> Wasm = {
      // Preamble
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
      // Type section: 1 type (func)
      0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
      // Function section: 1 function, type index 0
      0x03, 0x02, 0x01, 0x00,
      // Export section: 1 function export named "run", function index 0
      0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6E, 0x00, 0x00,
      // Code section
      0x0A};
  // Content size includes the function count and the one-byte body size.
  Wasm.push_back(static_cast<Byte>(Body.size() + 2U));
  Wasm.push_back(0x01); // Function count.
  Wasm.push_back(static_cast<Byte>(Body.size()));
  Wasm.insert(Wasm.end(), Body.begin(), Body.end());
  return Wasm;
}

TEST_F(ValidatorRegressionTest, ValidateWideArithmeticFunctions) {
  Conf->addProposal(Proposal::WideArithmetic);
  Loader::Loader EnabledLoader(*Conf);
  Validator::Validator EnabledValidator(*Conf);
  for (const auto &Encoding : WideArithmeticEncodings) {
    SCOPED_TRACE(static_cast<unsigned>(Encoding.Subopcode));
    for (const bool Unreachable : {false, true}) {
      SCOPED_TRACE(Unreachable);
      auto Module = EnabledLoader.parseModule(
          makeWideArithmeticFunction(Encoding, Unreachable));
      ASSERT_TRUE(Module);
      EXPECT_FALSE((*Module)->getIsValidated());

      auto Result = EnabledValidator.validate(**Module);
      ASSERT_TRUE(Result);
      EXPECT_TRUE((*Module)->getIsValidated());
    }
  }
}

TEST_F(ValidatorRegressionTest, RejectWideArithmeticConstantExpressions) {
  // Add/sub module (use i64.sub128 for subtraction):
  // (module
  //   (global i64
  //     i64.const 1 i64.const 1 i64.const 1 i64.const 1
  //     i64.add128 i64.add))
  // Multiplication module (use i64.mul_wide_u for unsigned multiplication):
  // (module
  //   (global i64
  //     i64.const 1 i64.const 1
  //     i64.mul_wide_s i64.add))
  // The final i64.add reduces the two wide results to one initializer value.
  // Wide instructions are not permitted in constant expressions.
  Conf->addProposal(Proposal::WideArithmetic);
  Conf->addProposal(Proposal::ExtendedConst);
  Loader::Loader EnabledLoader(*Conf);
  Validator::Validator EnabledValidator(*Conf);
  for (const auto &Encoding : WideArithmeticEncodings) {
    SCOPED_TRACE(static_cast<unsigned>(Encoding.Subopcode));
    // One immutable i64 global.
    std::vector<Byte> Global = {0x01, 0x7E, 0x00};
    const uint32_t OperandCount = Encoding.IsAddOrSub ? 4U : 2U;
    for (uint32_t I = 0; I < OperandCount; ++I) {
      Global.insert(Global.end(), {0x42, 0x01}); // OpCode I64__const, value 1.
    }
    // Wide opcode, i64.add, expression end.
    Global.insert(Global.end(), {0xFC, Encoding.Subopcode, 0x7C, 0x0B});
    std::vector<Byte> Wasm = {// Preamble
                              0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
                              // Global section
                              0x06};
    // Content size: global count, type and initializer expression.
    Wasm.push_back(static_cast<Byte>(Global.size()));
    Wasm.insert(Wasm.end(), Global.begin(), Global.end());
    auto Module = EnabledLoader.parseModule(Wasm);
    ASSERT_TRUE(Module);

    auto Result = EnabledValidator.validate(**Module);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), ErrCode::Value::ConstExprRequired);
    EXPECT_FALSE((*Module)->getIsValidated());
  }
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
