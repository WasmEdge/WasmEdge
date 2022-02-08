#include "helper.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/symmetric/func.h"
#include "wasi_crypto/api.hpp"

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST_F(WasiCryptoTest, Aesgcm) {
  writeString("AES-256-GCM"sv, 0);
  writeOptOptions(std::nullopt, 11);
  EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 11, 11, 19}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(19);

  EXPECT_EQ(testRun<Common::OptionsOpen>(
                {static_cast<uint32_t>(__WASI_ALGORITHM_TYPE_SYMMETRIC), 0}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto OptionsHandle = *MemInst.getPointer<__wasi_options_t *>(0);
  writeString("nonce"sv, 0);
  std::vector<uint8_t> InNonce(12, 42);
  writeSpan(InNonce, 5);
  EXPECT_EQ(testRun<Common::OptionsSet>({OptionsHandle, 0, 5, 5, 12}),
            __WASI_CRYPTO_ERRNO_SUCCESS);

  // ----- state 1, Test Nonce -----------

  writeString("AES-256-GCM"sv, 0);
  writeOptKey(KeyHandle, 11);
  writeOptOptions(OptionsHandle, 19);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 11, 11, 19, 27}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto State1Handle = *MemInst.getPointer<__wasi_symmetric_state_t *>(27);

  // std::array<uint8_t, 12> OutNonce;
  // EXPECT_EQ(testRun<Symmetric::StateOptionsGet>({State1Handle,
  // OutNonce.data(),
  //                                              OutNonce.size()}),
  //           __WASI_CRYPTO_ERRNO_SUCCESS);

  // EXPECT_EQ(InNonce, OutNonce);

  EXPECT_EQ(testRun<Symmetric::StateMaxTagLen>({State1Handle, 0}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto MaxTagLen = *MemInst.getPointer<uint32_t *>(0);
  EXPECT_EQ(MaxTagLen, 16);

  writeSpan("test"_u8, 20);
  EXPECT_EQ(testRun<Symmetric::StateEncrypt>({State1Handle, 0, 20, 20, 4, 24}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  std::vector<uint8_t> CiphertextWithTag(MemInst.getPointer<uint8_t *>(0, 20),
                                         MemInst.getPointer<uint8_t *>(0, 20) +
                                             20);
  EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(24), 20);
  EXPECT_EQ(testRun<Symmetric::StateClose>({State1Handle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);

  writeString("AES-256-GCM"sv, 0);
  writeOptKey(KeyHandle, 11);
  writeOptOptions(OptionsHandle, 19);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 11, 11, 19, 27}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto State2Handle = *MemInst.getPointer<__wasi_symmetric_state_t *>(27);

  writeSpan(CiphertextWithTag, 4);
  EXPECT_EQ(testRun<Symmetric::StateDecrypt>({State2Handle, 0, 4, 4, 20, 24}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(testRun<Symmetric::StateClose>({State2Handle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(24), 4);

  std::vector<uint8_t> Msg2(MemInst.getPointer<uint8_t *>(0, 4),
                            MemInst.getPointer<uint8_t *>(0) + 4);
  EXPECT_EQ("test"_u8, Msg2);

  writeString("AES-256-GCM"sv, 0);
  writeOptKey(KeyHandle, 11);
  writeOptOptions(OptionsHandle, 19);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 11, 11, 19, 27}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto State3Handle = *MemInst.getPointer<__wasi_symmetric_state_t *>(27);

  writeSpan("test"_u8, 4);
  EXPECT_EQ(
      testRun<Symmetric::StateEncryptDetached>({State3Handle, 0, 4, 4, 4, 8}),
      __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(testRun<Symmetric::StateClose>({State3Handle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  std::vector<uint8_t> Ciphertext(MemInst.getPointer<uint8_t *>(0),
                                  MemInst.getPointer<uint8_t *>(0) + 4);
  auto TagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(8);

  EXPECT_EQ(testRun<Symmetric::TagPull>({TagHandle, 0, 16, 16}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(16), 16);
  std::vector<uint8_t> Tag(MemInst.getPointer<uint8_t *>(0),
                           MemInst.getPointer<uint8_t *>(0) + 16);

  writeString("AES-256-GCM"sv, 0);
  writeOptKey(KeyHandle, 11);
  writeOptOptions(OptionsHandle, 19);
  EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 11, 11, 19, 27}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  auto State4Handle = *MemInst.getPointer<__wasi_symmetric_state_t *>(27);

  writeSpan(Ciphertext, 4);
  writeSpan(Tag, 8);
  EXPECT_EQ(testRun<Symmetric::StateDecryptDetached>(
                {State4Handle, 0, 4, 4, 4, 8, 16, 24}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(testRun<Symmetric::StateClose>({State4Handle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(24), 4);
  std::vector<uint8_t> Msg3(MemInst.getPointer<uint8_t *>(0),
                            MemInst.getPointer<uint8_t *>(0) + 4);
  EXPECT_EQ("test"_u8, Msg3);

  EXPECT_EQ(testRun<Common::OptionsClose>({OptionsHandle}),
            __WASI_CRYPTO_ERRNO_SUCCESS);
}