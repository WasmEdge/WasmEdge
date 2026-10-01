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

#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Encodes an unsigned 32-bit integer as LEB128.
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

// Generates a module whose type k+1 subtypes type k, giving a subtype chain of
// depth NumTotalTypes - 1.
static std::vector<WasmEdge::Byte>
generateWasmWithSubtypeChain(int NumTotalTypes) {
  std::vector<WasmEdge::Byte> WasmBytes;
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

  // Type k+1: (sub k (func)).
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

  // 6. Custom "name" Section (ID 0x00), payload built first for its size
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

// Loads and validates a module with the GC proposal enabled.
ErrCode::Value validateModule(Span<const Byte> Wasm) {
  Configure Conf;
  Conf.addProposal(Proposal::GC);
  Loader::Loader LoadEngine(Conf);
  Validator::Validator ValidEngine(Conf);
  auto Res = LoadEngine.parseModule(Wasm).and_then(
      [&](auto Mod) { return ValidEngine.validate(*Mod); });
  return Res ? ErrCode::Value::Success : Res.error().getEnum();
}

// The subtype depth is limited to 63, so a chain of 64 types is the maximum.
TEST(ValidatorRegressionTest, SubtypeDepthLimit) {
  EXPECT_EQ(validateModule(generateWasmWithSubtypeChain(64)),
            ErrCode::Value::Success);
  EXPECT_EQ(validateModule(generateWasmWithSubtypeChain(65)),
            ErrCode::Value::InvalidSubType);
}

// A declared supertype must have a smaller type index than its sub type.
TEST(ValidatorRegressionTest, SupertypeIndexMustPrecedeSubtype) {
  // (module (type (func)) (type (sub 4294967295 (func))))
  std::array<WasmEdge::Byte, 24> OutOfRangeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x02, 0x60,
      0x00, 0x00, 0x50, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x60, 0x00, 0x00};
  EXPECT_EQ(validateModule(OutOfRangeWasm), ErrCode::Value::InvalidSubType);

  // (module
  //   (rec
  //     (type $mid (sub $top (func (result i32))))
  //     (type $top (sub (func (result i32))))))
  std::array<WasmEdge::Byte, 26> ForwardWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01,
      0x10, 0x01, 0x4e, 0x02, 0x50, 0x01, 0x01, 0x60, 0x00,
      0x01, 0x7f, 0x50, 0x00, 0x60, 0x00, 0x01, 0x7f};
  EXPECT_EQ(validateModule(ForwardWasm), ErrCode::Value::InvalidSubType);

  // $c names itself as supertype; it must be rejected before matching type 1
  // walks its supertypes forever.
  // (module
  //   (rec
  //     (type $a (sub (func (result (ref null $a)))))
  //     (type (sub $a (func (result (ref null $c)))))
  //     (type $c (sub $c (func)))))
  std::array<WasmEdge::Byte, 34> CyclicWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x18, 0x01, 0x4e,
      0x03, 0x50, 0x00, 0x60, 0x00, 0x01, 0x63, 0x00, 0x50, 0x01, 0x00, 0x60,
      0x00, 0x01, 0x63, 0x02, 0x50, 0x01, 0x02, 0x60, 0x00, 0x00};
  EXPECT_EQ(validateModule(CyclicWasm), ErrCode::Value::InvalidSubType);
}

// An active element segment type must match the table element type.
TEST(ValidatorRegressionTest, ActiveElemTypeMismatch) {
  // The segment type (ref $t0) is a supertype, not a subtype, of the table
  // element type (ref null $t1).
  // (module
  //   (type $t0 (sub (func)))
  //   (type $t1 (sub final $t0 (func)))
  //   (import "env" "f" (func (type $t0)))
  //   (table 1 (ref null $t1))
  //   (elem (table 0) (i32.const 0) (ref $t0) (ref.func 0)))
  std::array<WasmEdge::Byte, 54> ConcreteWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02,
      0x50, 0x00, 0x60, 0x00, 0x00, 0x4f, 0x01, 0x00, 0x60, 0x00, 0x00,
      0x02, 0x09, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x01, 0x66, 0x00, 0x00,
      0x04, 0x05, 0x01, 0x63, 0x01, 0x00, 0x01, 0x09, 0x0c, 0x01, 0x06,
      0x00, 0x41, 0x00, 0x0b, 0x64, 0x00, 0x01, 0xd2, 0x00, 0x0b};
  EXPECT_EQ(validateModule(ConcreteWasm), ErrCode::Value::TypeCheckFailed);

  // (module
  //   (type $t0 (func (param i32)))
  //   (type $t1 (func))
  //   (func $f0 (type $t1))
  //   (func $f1 (type $t0) (local.get 0) (drop))
  //   (func $f2 (type $t1)
  //     (call_ref $t1 (table.get 0 (i32.const 0))))
  //   (table 1 (ref null $t1))
  //   (elem (table 0) (i32.const 0) func $f1)   ;; implicit funcref
  //   (export "trigger" (func $f2)))
  std::array<WasmEdge::Byte, 74> FuncrefWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02,
      0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x03, 0x04, 0x03, 0x01,
      0x00, 0x01, 0x04, 0x05, 0x01, 0x63, 0x01, 0x00, 0x01, 0x07, 0x0b,
      0x01, 0x07, 0x74, 0x72, 0x69, 0x67, 0x67, 0x65, 0x72, 0x00, 0x02,
      0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x01, 0x0a, 0x13,
      0x03, 0x02, 0x00, 0x0b, 0x05, 0x00, 0x20, 0x00, 0x1a, 0x0b, 0x08,
      0x00, 0x41, 0x00, 0x25, 0x00, 0x14, 0x01, 0x0b};
  EXPECT_EQ(validateModule(FuncrefWasm), ErrCode::Value::TypeCheckFailed);
}

// call_indirect on a struct-typed table must fail validation instead of
// reinterpreting the struct reference as a function.
TEST(ValidatorRegressionTest, CallIndirectNonFuncTable) {
  // (module
  //   (type $f (func))
  //   (type $s (struct (field v128)))
  //   (table 1 (ref null $s))
  //   (func (export "_start")
  //     i32.const 0
  //     v128.const i32x4 0x41414141 0x41414141 0x41414149 0x41414141
  //     struct.new $s
  //     table.set 0
  //     i32.const 0
  //     call_indirect (type $f)))
  std::array<WasmEdge::Byte, 77> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02,
      0x60, 0x00, 0x00, 0x5f, 0x01, 0x7b, 0x00, 0x03, 0x02, 0x01, 0x00,
      0x04, 0x05, 0x01, 0x63, 0x01, 0x00, 0x01, 0x07, 0x0a, 0x01, 0x06,
      0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a, 0x22, 0x01,
      0x20, 0x00, 0x41, 0x00, 0xfd, 0x0c, 0x41, 0x41, 0x41, 0x41, 0x41,
      0x41, 0x41, 0x41, 0x49, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
      0xfb, 0x00, 0x01, 0x26, 0x00, 0x41, 0x00, 0x11, 0x00, 0x00, 0x0b};
  EXPECT_EQ(validateModule(Wasm), ErrCode::Value::TypeCheckFailed);
}

// ref.test and ref.cast must accept the bottom type left by unreachable.
TEST(ValidatorRegressionTest, RefTestAndCastAfterUnreachable) {
  // (module
  //   (type (func))
  //   (func (type 0) unreachable ref.test (ref func) drop)
  //   (func (type 0) unreachable ref.cast (ref null func) drop))
  std::array<WasmEdge::Byte, 38> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04,
      0x01, 0x60, 0x00, 0x00, 0x03, 0x03, 0x02, 0x00, 0x00, 0x0a,
      0x11, 0x02, 0x07, 0x00, 0x00, 0xfb, 0x14, 0x70, 0x1a, 0x0b,
      0x07, 0x00, 0x00, 0xfb, 0x17, 0x70, 0x1a, 0x0b};
  EXPECT_EQ(validateModule(Wasm), ErrCode::Value::Success);
}

// ref.func on an imported function needs a declarative element segment.
TEST(ValidatorRegressionTest, RefFuncToImportedFunction) {
  // (module
  //   (type (func))
  //   (import "env" "f" (func (type 0)))
  //   (func (type 0) ref.func 0 drop))
  std::array<WasmEdge::Byte, 38> UndeclaredWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04,
      0x01, 0x60, 0x00, 0x00, 0x02, 0x09, 0x01, 0x03, 0x65, 0x6e,
      0x76, 0x01, 0x66, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x0a,
      0x07, 0x01, 0x05, 0x00, 0xd2, 0x00, 0x1a, 0x0b};
  EXPECT_EQ(validateModule(UndeclaredWasm), ErrCode::Value::InvalidRefIdx);

  // (module
  //   (type (func))
  //   (import "env" "f" (func (type 0)))
  //   (elem declare func 0)
  //   (func (type 0) ref.func 0 drop))
  std::array<WasmEdge::Byte, 45> DeclaredWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x02, 0x09, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x01, 0x66, 0x00,
      0x00, 0x03, 0x02, 0x01, 0x00, 0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00,
      0x0a, 0x07, 0x01, 0x05, 0x00, 0xd2, 0x00, 0x1a, 0x0b};
  EXPECT_EQ(validateModule(DeclaredWasm), ErrCode::Value::Success);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
