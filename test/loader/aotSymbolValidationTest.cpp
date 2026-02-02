// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errcode.h"
#include "common/executable.h"
#include "common/types.h"
#include "loader/loader.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace {

using namespace WasmEdge;

/// Mock Executable that returns a null code symbol to simulate JIT/AOT failure.
class MockExecutableWithNullCodeSymbol : public Executable {
public:
  Symbol<const Executable::IntrinsicsTable *> getIntrinsics() noexcept override {
    return createSymbol<const Executable::IntrinsicsTable *>(&DummyIntrinsics);
  }

  std::vector<Symbol<Executable::Wrapper>> getTypes(size_t Size) noexcept override {
    std::vector<Symbol<Executable::Wrapper>> Result;
    for (size_t I = 0; I < Size; ++I) {
      Result.push_back(createSymbol<Executable::Wrapper>(&DummyWrapper));
    }
    return Result;
  }

  std::vector<Symbol<void>> getCodes(size_t, size_t Size) noexcept override {
    std::vector<Symbol<void>> Result;
    for (size_t I = 0; I < Size; ++I) {
      Result.push_back(createSymbol<void>(nullptr)); // Null symbol
    }
    return Result;
  }

private:
  static void DummyWrapper(void *, void *, const ValVariant *, ValVariant *) {}
  static const Executable::IntrinsicsTable *DummyIntrinsics;
};

const Executable::IntrinsicsTable *MockExecutableWithNullCodeSymbol::DummyIntrinsics = nullptr;

/// Loader fails early when a JIT/AOT code symbol is null.
TEST(AOTSymbolValidation, FailOnNullCodeSymbol) {
  // Minimal WASM: one function returning i32
  std::vector<uint8_t> Wasm = {
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, // magic + version
      0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7F,       // type section
      0x03, 0x02, 0x01, 0x00,                         // function section
      0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x2A, 0x0B  // code section
  };

  Configure Conf;
  Loader::Loader Ldr{Conf};

  auto Mod = Ldr.parseModule(Wasm);
  ASSERT_TRUE(Mod);

  auto MockExec = std::make_shared<MockExecutableWithNullCodeSymbol>();
  auto Result = Ldr.loadExecutable(*Mod, MockExec);

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::IllegalGrammar);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
