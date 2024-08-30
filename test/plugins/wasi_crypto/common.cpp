// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/func.h"
#include "helper.h"

namespace {
template <typename T, typename M>
inline T *getHostFunc(const M &Mod, const char *Name) {
  if (Mod) {
    auto *FuncInst = Mod->findFuncExports(Name);
    if (FuncInst && FuncInst->isHostFunction()) {
      return dynamic_cast<T *>(&FuncInst->getHostFunc());
    }
  }
  return nullptr;
}
} // namespace

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Options) {
  // Symmetric options.
  {
    // Open options.
    WASI_CRYPTO_EXPECT_SUCCESS(SymmetricOptionsHandle,
                               optionsOpen(__WASI_ALGORITHM_TYPE_SYMMETRIC));

    // Set options.
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSet(SymmetricOptionsHandle, "context"sv, "foo"_u8));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSet(SymmetricOptionsHandle, "salt"sv, "foo"_u8));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSet(SymmetricOptionsHandle, "nonce"sv, "foo"_u8));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSetU64(SymmetricOptionsHandle, "memory_limit"sv, 0));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSetU64(SymmetricOptionsHandle, "ops_limit"sv, 0));
    WASI_CRYPTO_EXPECT_TRUE(
        optionsSetU64(SymmetricOptionsHandle, "parallelism"sv, 0));

    // Unsupported options.
    WASI_CRYPTO_EXPECT_FAILURE(
        optionsSet(SymmetricOptionsHandle, "foo"sv, "foo"_u8),
        __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
    WASI_CRYPTO_EXPECT_FAILURE(
        optionsSetU64(SymmetricOptionsHandle, "foo"sv, 0),
        __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    writeDummyMemoryContent();
    writeString("foo"sv, 0);
    uint32_t NameSize = 3;
    auto *Func = getHostFunc<Common::OptionsSetGuestBuffer>(
        WasiCryptoCommonMod, "options_set_guest_buffer");
    ASSERT_NE(Func, nullptr);
    EXPECT_TRUE(Func->run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              SymmetricOptionsHandle, 0, NameSize, 0, NameSize},
                          Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    WASI_CRYPTO_EXPECT_TRUE(optionsClose(SymmetricOptionsHandle));
  }

  // Signature options.
  {
    // Open options.
    WASI_CRYPTO_EXPECT_SUCCESS(SigOptionsHandle,
                               optionsOpen(__WASI_ALGORITHM_TYPE_SIGNATURES));

    // Unsupported options.
    WASI_CRYPTO_EXPECT_FAILURE(optionsSet(SigOptionsHandle, "foo"sv, "foo"_u8),
                               __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    WASI_CRYPTO_EXPECT_FAILURE(optionsSetU64(SigOptionsHandle, "foo"sv, 0),
                               __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    writeDummyMemoryContent();
    writeString("foo"sv, 0);
    uint32_t NameSize = 3;
    auto *Func = getHostFunc<Common::OptionsSetGuestBuffer>(
        WasiCryptoCommonMod, "options_set_guest_buffer");
    ASSERT_NE(Func, nullptr);
    EXPECT_TRUE(Func->run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              SigOptionsHandle, 0, NameSize, 0, NameSize},
                          Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    // Close options.
    WASI_CRYPTO_EXPECT_TRUE(optionsClose(SigOptionsHandle));
  }

  // Key exchange options.
  {
    // Open options.
    WASI_CRYPTO_EXPECT_SUCCESS(KxOptionsHandle,
                               optionsOpen(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE));
    // Unsupported options.
    WASI_CRYPTO_EXPECT_FAILURE(optionsSet(KxOptionsHandle, "foo"sv, "foo"_u8),
                               __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    WASI_CRYPTO_EXPECT_FAILURE(optionsSetU64(KxOptionsHandle, "foo"sv, 0),
                               __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    writeDummyMemoryContent();
    writeString("foo"sv, 0);
    uint32_t NameSize = 3;
    auto *Func = getHostFunc<Common::OptionsSetGuestBuffer>(
        WasiCryptoCommonMod, "options_set_guest_buffer");
    ASSERT_NE(Func, nullptr);
    EXPECT_TRUE(Func->run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              KxOptionsHandle, 0, NameSize, 0, NameSize},
                          Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    // Close options.
    WASI_CRYPTO_EXPECT_TRUE(optionsClose(KxOptionsHandle));
  }
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
