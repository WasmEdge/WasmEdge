#include "helper.h"

#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/symmetric/func.h"
#include "wasi_crypto/api.hpp"
using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST_F(WasiCryptoTest, Options) {
  // symmetric options
  {
    // open options
    EXPECT_EQ(testRun<Common::OptionsOpen>(
                  {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SYMMETRIC), 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SymmetricOptionsHandle = *MemInst.getPointer<__wasi_options_t *>(0);

    // set options
    writeString("context", 0);
    writeSpan("foo"_u8, 7);
    EXPECT_EQ(testRun<Common::OptionsSet>({SymmetricOptionsHandle, 0, 7, 7, 3}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("salt", 0);
    writeSpan("foo"_u8, 4);
    EXPECT_EQ(testRun<Common::OptionsSet>({SymmetricOptionsHandle, 0, 4, 4, 3}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("nonce", 0);
    writeSpan("foo"_u8, 5);
    EXPECT_EQ(testRun<Common::OptionsSet>({SymmetricOptionsHandle, 0, 5, 5, 3}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("memory_limit", 0);
    EXPECT_EQ(
        testRun<Common::OptionsSetU64>({SymmetricOptionsHandle, 0, 12, 1}),
        __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("ops_limit", 0);
    EXPECT_EQ(testRun<Common::OptionsSetU64>({SymmetricOptionsHandle, 0, 9, 1}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("parallelism", 0);
    EXPECT_EQ(
        testRun<Common::OptionsSetU64>({SymmetricOptionsHandle, 0, 11, 1}),
        __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("buffer", 0);
    writeSpan("foo"_u8, 6);
    EXPECT_EQ(testRun<Common::OptionsSetGuestBuffer>(
                  {SymmetricOptionsHandle, 0, 6, 6, 3}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    // unsupport options
    writeString("foofoo", 0);
    EXPECT_EQ(testRun<Common::OptionsSetGuestBuffer>(
                  {SymmetricOptionsHandle, 0, 3, 3, 3}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
    EXPECT_EQ(testRun<Common::OptionsSet>({SymmetricOptionsHandle, 0, 3, 3, 3}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
    EXPECT_EQ(testRun<Common::OptionsSetU64>({SymmetricOptionsHandle, 0, 3, 1}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    // open state
    writeString("HMAC/SHA-256", 0);
    writeOptOptions(SymmetricOptionsHandle, 12);
    EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 12, 12, 20}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SymmetricKeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(20);
    writeOptKey(SymmetricKeyHandle, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 20, 12, 28}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SymmetricStateHandle =
        *MemInst.getPointer<__wasi_symmetric_state_t *>(28);

    // check
    writeString("context", 0);
    EXPECT_EQ(testRun<Symmetric::StateOptionsGet>(
                  {SymmetricStateHandle, 0, 7, 7, 3, 10}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint8_t *>(10), 3);
    EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(7),
                                    MemInst.getPointer<uint8_t *>(7) + 3}),
              "foo"_u8);

    writeString("salt", 0);
    EXPECT_EQ(testRun<Symmetric::StateOptionsGet>(
                  {SymmetricStateHandle, 0, 4, 4, 3, 7}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint8_t *>(7), 3);
    EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(4),
                                    MemInst.getPointer<uint8_t *>(4) + 3}),
              "foo"_u8);
    writeString("salt", 0);

    writeString("nonce", 0);
    EXPECT_EQ(testRun<Symmetric::StateOptionsGet>(
                  {SymmetricStateHandle, 0, 5, 5, 3, 8}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint8_t *>(8), 3);
    EXPECT_EQ((std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(5),
                                    MemInst.getPointer<uint8_t *>(5) + 3}),
              "foo"_u8);

    writeString("memory_limit", 0);
    EXPECT_EQ(testRun<Symmetric::StateOptionsGetU64>(
                  {SymmetricStateHandle, 0, 12, 12}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint8_t *>(12), 1);

    writeString("ops_limit", 0);
    EXPECT_EQ(
        testRun<Symmetric::StateOptionsGetU64>({SymmetricStateHandle, 0, 9, 9}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint8_t *>(9), 1);

    writeString("parallelism", 0);
    EXPECT_EQ(testRun<Symmetric::StateOptionsGetU64>(
                  {SymmetricStateHandle, 0, 11, 11}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<uint8_t *>(11), 1);

    // close options
    EXPECT_EQ(testRun<Common::OptionsClose>({SymmetricOptionsHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }
  // sig options
  {
    // open options
    EXPECT_EQ(testRun<Common::OptionsOpen>(
                  {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SIGNATURES), 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto SigOptionsHandle = *MemInst.getPointer<__wasi_options_t *>(0);

    // unsupport options
    writeString("foofoo", 0);
    EXPECT_EQ(
        testRun<Common::OptionsSetGuestBuffer>({SigOptionsHandle, 0, 3, 3, 3}),
        __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
    EXPECT_EQ(testRun<Common::OptionsSet>({SigOptionsHandle, 0, 3, 3, 3}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
    EXPECT_EQ(testRun<Common::OptionsSetU64>({SigOptionsHandle, 0, 3, 1}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);

    // close options
    EXPECT_EQ(testRun<Common::OptionsClose>({SigOptionsHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // kx options
  {
    // open options
    EXPECT_EQ(
        testRun<Common::OptionsOpen>(
            {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE), 0}),
        __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KxOptionsHandle = *MemInst.getPointer<__wasi_options_t *>(0);

    // unsupport options
    writeString("foofoo", 0);
    EXPECT_EQ(
        testRun<Common::OptionsSetGuestBuffer>({KxOptionsHandle, 0, 3, 3, 3}),
        __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
    EXPECT_EQ(testRun<Common::OptionsSet>({KxOptionsHandle, 0, 3, 3, 3}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
    EXPECT_EQ(testRun<Common::OptionsSetU64>({KxOptionsHandle, 0, 3, 1}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION);
              
    // close options
    EXPECT_EQ(testRun<Common::OptionsClose>({KxOptionsHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }
}