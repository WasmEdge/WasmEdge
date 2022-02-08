#include "gtest/gtest.h"

#include "helper.h"
#include "host/wasi_crypto/symmetric/func.h"
#include "wasi_crypto/api.hpp"

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST_F(WasiCryptoTest, Sha256) {
  // open state
  writeString("SHA-256", 0);
  writeOptKey(std::nullopt, 7);
  writeOptOptions(std::nullopt, 15);

  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 7, 7, 15, 23}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(23);

  // absorb "data"
  writeSpan("data"_u8, 0);
  EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 4}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 32}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  // hash "data"
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 32),
                            MemInst.getPointer<uint8_t *>(0, 32) + 32}),
      "3a6eb0790f39ac87c94f3856b2dd2c5d110e6811602261a9a923d3bb23adc8b7"_u8v);

  // absorb "more_data"
  writeSpan("more_data"_u8, 0);
  EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 9}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  // hash "datamore_data"
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 32}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 32),
                            MemInst.getPointer<uint8_t *>(0, 32) + 32}),
      "13c40eec22541a155e172010c7fd6ef654e4e138a0c20923f9a91062a27f57b6"_u8v);

  // smaller than the hash function output size. truncate output
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 31}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 31),
                            MemInst.getPointer<uint8_t *>(0, 31) + 31}),
      "13c40eec22541a155e172010c7fd6ef654e4e138a0c20923f9a91062a27f57b"_u8v);

  // requested size exceeds return invalid_length.
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 33}),
            __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  // some error case check

  writeString("SHA-256", 0);
  writeOptOptions(std::nullopt, 7);
  EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 7, 7, 15}),
            __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);

  writeString("SHA-256", 0);
  writeOptKey(InvaildHandle, 7);
  writeOptOptions(std::nullopt, 15);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 7, 7, 15, 23}),
            __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  writeString("foo", 0);
  writeSpan("00"_u8, 3);
  EXPECT_EQ(testRun<Symmetric::StateOptionsGet>({StateHandle, 0, 3, 3, 1, 4}),
            __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  EXPECT_EQ(testRun<Symmetric::StateOptionsGetU64>({StateHandle, 0, 3, 3}),
            __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);

  EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  writeString("SHA-256", 0);
  EXPECT_EQ(testRun<Symmetric::StateSqueezeKey>({StateHandle, 0, 7, 7}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateMaxTagLen>({StateHandle, 0}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateEncrypt>({StateHandle, 0, 1, 1, 1, 2}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(
      testRun<Symmetric::StateEncryptDetached>({StateHandle, 0, 1, 1, 1, 2}),
      __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateDecrypt>({StateHandle, 0, 1, 1, 1, 2}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateDecryptDetached>(
                {StateHandle, 0, 1, 1, 1, 2, 1, 3}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateRatchet>({StateHandle}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
  // close
  EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
}

TEST_F(WasiCryptoTest, Sha512) {
  // open state
  writeString("SHA-512", 0);
  writeOptKey(std::nullopt, 7);
  writeOptOptions(std::nullopt, 15);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 7, 7, 15, 23}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(23);

  // absortb "data"
  writeSpan("data"_u8, 0);
  EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 4}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  // hash "data"
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 64}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 64),
                            MemInst.getPointer<uint8_t *>(0, 64) + 64}),
      "77c7ce9a5d86bb386d443bb96390faa120633158699c8844c30b13ab0bf92760b7e4416aea397db91b4ac0e5dd56b8ef7e4b066162ab1fdc088319ce6defc876"_u8v);

  // absorb "more_data"
  writeSpan("more_data"_u8, 0);
  EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 9}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  // hash "datamore_data"
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 64}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 64),
                            MemInst.getPointer<uint8_t *>(0, 64) + 64}),
      "78d0b55eeb3a07754f0967a6e960b5b7488b09ec4d2a62d832a45d80f814aef88e5414e2115165012ac592ff050651e956089a5aacd4ea52cf247c3cc2f6add2"_u8v);

  // can be smaller than the hash function output size. truncate output
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 63}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 63),
                            MemInst.getPointer<uint8_t *>(0, 63) + 63}),
      "78d0b55eeb3a07754f0967a6e960b5b7488b09ec4d2a62d832a45d80f814aef88e5414e2115165012ac592ff050651e956089a5aacd4ea52cf247c3cc2f6add"_u8v);

  // requested size exceeds return invalid_length.
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 65}),
            __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  // some error case check

  writeString("SHA-512", 0);
  writeOptOptions(std::nullopt, 7);
  EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 7, 7, 15}),
            __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);

  writeString("SHA-512", 0);
  writeOptKey(InvaildHandle, 7);
  writeOptOptions(std::nullopt, 15);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 7, 7, 15, 23}),
            __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  writeString("foo", 0);
  writeSpan("00"_u8, 3);
  EXPECT_EQ(testRun<Symmetric::StateOptionsGet>({StateHandle, 0, 3, 3, 1, 4}),
            __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  EXPECT_EQ(testRun<Symmetric::StateOptionsGetU64>({StateHandle, 0, 3, 3}),
            __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);

  EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  writeString("SHA-512", 0);
  EXPECT_EQ(testRun<Symmetric::StateSqueezeKey>({StateHandle, 0, 7, 7}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateMaxTagLen>({StateHandle, 0}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateEncrypt>({StateHandle, 0, 1, 1, 1, 2}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(
      testRun<Symmetric::StateEncryptDetached>({StateHandle, 0, 1, 1, 1, 2}),
      __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateDecrypt>({StateHandle, 0, 1, 1, 1, 2}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateDecryptDetached>(
                {StateHandle, 0, 1, 1, 1, 2, 1, 3}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateRatchet>({StateHandle}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  // close
  EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
}

TEST_F(WasiCryptoTest, Sha512_256) {
  // open state
  writeString("SHA-512/256", 0);
  writeOptKey(std::nullopt, 11);
  writeOptOptions(std::nullopt, 19);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 11, 11, 19, 27}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(27);

  // absorb "data"
  writeSpan("data"_u8, 0);
  EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 4}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  // hash "data"
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 32}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 32),
                            MemInst.getPointer<uint8_t *>(0, 32) + 32}),
      "99902eaf90e92264667843cde66675ed94caa361634bad57874642aa364aa968"_u8v);

  // hash "more_data"
  writeSpan("more_data"_u8, 0);
  EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 9}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 32}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 32),
                            MemInst.getPointer<uint8_t *>(0, 32) + 32}),
      "d1def71920a44d8b6c83b2eaa99379a16047cc82cec8d80689fbf02fbd062481"_u8v);

  // can be smaller than the hash function output size. truncate output
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 31}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(
      (std::vector<uint8_t>{MemInst.getPointer<uint8_t *>(0, 31),
                            MemInst.getPointer<uint8_t *>(0, 31) + 31}),
      "d1def71920a44d8b6c83b2eaa99379a16047cc82cec8d80689fbf02fbd06248"_u8v);

  // requested size exceeds return invalid_length.
  EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 33}),
            __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  // some error case check

  writeString("SHA-512/256", 0);
  writeOptOptions(std::nullopt, 11);
  EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 11, 11, 19}),
            __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED);

  writeString("SHA-512/256", 0);
  writeOptKey(InvaildHandle, 11);
  writeOptOptions(std::nullopt, 19);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 11, 11, 19, 27}),
            __WASI_CRYPTO_ERRNO_INVALID_HANDLE);

  writeString("foo", 0);
  writeSpan("00"_u8, 3);
  EXPECT_EQ(testRun<Symmetric::StateOptionsGet>({StateHandle, 0, 3, 3, 1, 4}),
            __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  EXPECT_EQ(testRun<Symmetric::StateOptionsGetU64>({StateHandle, 0, 3, 3}),
            __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);

  EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  writeString("SHA-512/256", 0);
  EXPECT_EQ(testRun<Symmetric::StateSqueezeKey>({StateHandle, 0, 11, 11}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateMaxTagLen>({StateHandle, 0}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateEncrypt>({StateHandle, 0, 1, 1, 1, 2}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(
      testRun<Symmetric::StateEncryptDetached>({StateHandle, 0, 1, 1, 1, 2}),
      __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateDecrypt>({StateHandle, 0, 1, 1, 1, 2}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateDecryptDetached>(
                {StateHandle, 0, 1, 1, 1, 2, 1, 3}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

  EXPECT_EQ(testRun<Symmetric::StateRatchet>({StateHandle}),
            __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
  // close
  EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
}