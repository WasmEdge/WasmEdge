#include "helper.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/symmetric/func.h"

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST_F(WasiCryptoTest, HmacSha256) {
  // generate key hmac
  {
    writeString("HMAC/SHA-256", 0);
    writeOptOptions(std::nullopt, 12);
    EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 12, 12, 20}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(20);

    writeString("HMAC/SHA-256", 0);
    writeOptKey(KeyHandle, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(28);

    writeString("datamore_data"sv, 0);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 4, 9}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto TagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);

    EXPECT_EQ(testRun<Symmetric::TagLen>({TagHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);

    EXPECT_EQ(testRun<Symmetric::TagPull>({TagHandle, 0, 32, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    std::vector<uint8_t> Tag{MemInst.getPointer<uint8_t *>(0),
                             MemInst.getPointer<uint8_t *>(0) + 32};

    EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto NewTagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);

    writeSpan(Tag, 0);
    EXPECT_EQ(testRun<Symmetric::TagVerify>({NewTagHandle, 0, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    // close
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::TagClose>({NewTagHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // import key hmac
  {
    writeString("HMAC/SHA-256", 0);
    writeSpan("00000000000000000000000000000000"_u8, 12);
    EXPECT_EQ(testRun<Symmetric::KeyImport>({0, 12, 12, 32, 44}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(44);

    writeString("HMAC/SHA-256", 0);
    writeOptKey(KeyHandle, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(28);

    {
      // absorb "data"
      writeSpan("data"_u8, 0);
      EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 4}),
                __WASI_CRYPTO_ERRNO_SUCCESS);

      // squeezetag "data"
      EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto TagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);
      EXPECT_EQ(testRun<Symmetric::TagLen>({TagHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
      EXPECT_EQ(testRun<Symmetric::TagPull>({TagHandle, 0, 32, 32}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      std::vector<uint8_t> Out{MemInst.getPointer<uint8_t *>(0, 32),
                               MemInst.getPointer<uint8_t *>(0, 32) + 32};
      EXPECT_EQ(
          Out,
          "7f12a3d914ec4d1ee67dd35ff04df5a725d11a6bb78a4aafd1093f5bfbd86887"_u8v);
    }

    {
      // abosorb "more_data"
      writeSpan("more_data"_u8, 0);
      EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 9}),
                __WASI_CRYPTO_ERRNO_SUCCESS);

      // squeezetag "datamore_data"
      EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto TagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);
      EXPECT_EQ(testRun<Symmetric::TagLen>({TagHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 32);
      EXPECT_EQ(testRun<Symmetric::TagPull>({TagHandle, 0, 32, 32}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      std::vector<uint8_t> Out{MemInst.getPointer<uint8_t *>(0, 32),
                               MemInst.getPointer<uint8_t *>(0, 32) + 32};
      EXPECT_EQ(
          Out,
          "77af4875ffb3932cba0c8bc5da18410c42c85eeb07072918629675e054fbc42d"_u8v);
    }

    // close
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  {
    // runtime generate key size required 32
    writeString("HMAC/SHA-256", 0);
    writeOptOptions(std::nullopt, 12);
    EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 12, 12, 20}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(20);
    EXPECT_EQ(testRun<Symmetric::KeyExport>({KeyHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyOutputHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({KeyOutputHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_symmetric_key_t *>(0), 32);

    // a test statehandle for error case check
    writeString("HMAC/SHA-256", 0);
    writeOptKey(KeyHandle, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(28);

    // error case check
    writeString("HMAC/SHA-256", 0);
    writeOptKey(std::nullopt, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
    EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

    writeString("HMAC/SHA-256", 0);
    EXPECT_EQ(testRun<Symmetric::StateSqueezeKey>({StateHandle, 0, 12, 12}),
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
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }
}

TEST_F(WasiCryptoTest, HmacSha512) {
  // generate key hmac
  {
    writeString("HMAC/SHA-512", 0);
    writeOptOptions(std::nullopt, 12);
    EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 12, 12, 20}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(20);

    writeString("HMAC/SHA-512", 0);
    writeOptKey(KeyHandle, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(28);

    writeString("datamore_data"sv, 0);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 4, 9}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto TagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);

    EXPECT_EQ(testRun<Symmetric::TagLen>({TagHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);

    EXPECT_EQ(testRun<Symmetric::TagPull>({TagHandle, 0, 64, 64}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    std::vector<uint8_t> Tag{MemInst.getPointer<uint8_t *>(0),
                             MemInst.getPointer<uint8_t *>(0) + 64};

    EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto NewTagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);

    writeSpan(Tag, 0);
    EXPECT_EQ(testRun<Symmetric::TagVerify>({NewTagHandle, 0, 64}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    // close
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::TagClose>({NewTagHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  // import key hmac
  {
    writeString("HMAC/SHA-512", 0);
    writeSpan(
        "0000000000000000000000000000000000000000000000000000000000000000"_u8,
        12);
    EXPECT_EQ(testRun<Symmetric::KeyImport>({0, 12, 12, 64, 76}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(76);

    writeString("HMAC/SHA-512", 0);
    writeOptKey(KeyHandle, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(28);

    {
      // absorb "data"
      writeSpan("data"_u8, 0);
      EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 4}),
                __WASI_CRYPTO_ERRNO_SUCCESS);

      // squeezetag "data"
      EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto TagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);
      EXPECT_EQ(testRun<Symmetric::TagLen>({TagHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
      EXPECT_EQ(testRun<Symmetric::TagPull>({TagHandle, 0, 64, 64}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      std::vector<uint8_t> Out{MemInst.getPointer<uint8_t *>(0, 64),
                               MemInst.getPointer<uint8_t *>(0, 64) + 64};
      EXPECT_EQ(
          Out,
          "52fbafda16189e63730604e49c747c8281d2420e7aae34c927927e7c3cddfcea62fea554d1962a0c0d1c8177884787d8b2a88bd396d5780e3fb82b11ab33c5cc"_u8v);
    }

    {
      // abosorb "more_data"
      writeSpan("more_data"_u8, 0);
      EXPECT_EQ(testRun<Symmetric::StateAbsorb>({StateHandle, 0, 9}),
                __WASI_CRYPTO_ERRNO_SUCCESS);

      // squeezetag "datamore_data"
      EXPECT_EQ(testRun<Symmetric::StateSqueezeTag>({StateHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      auto TagHandle = *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);
      EXPECT_EQ(testRun<Symmetric::TagLen>({TagHandle, 0}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<__wasi_size_t *>(0), 64);
      EXPECT_EQ(testRun<Symmetric::TagPull>({TagHandle, 0, 64, 64}),
                __WASI_CRYPTO_ERRNO_SUCCESS);
      std::vector<uint8_t> Out{MemInst.getPointer<uint8_t *>(0, 64),
                               MemInst.getPointer<uint8_t *>(0, 64) + 64};
      EXPECT_EQ(
          Out,
          "36d2dbfb50768b963fe243535bcda302750297b361b7eb079978b27177adc40338dab5c244ae90e2f11a3518ac31126a52eb5ec715c0a9476b98f73e7ff7682e"_u8v);
    }

    // close
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }

  {
    // runtime generate key size required 64
    writeString("HMAC/SHA-512", 0);
    writeOptOptions(std::nullopt, 12);
    EXPECT_EQ(testRun<Symmetric::KeyGenerate>({0, 12, 12, 20}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(20);
    EXPECT_EQ(testRun<Symmetric::KeyExport>({KeyHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto KeyOutputHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(0);
    EXPECT_EQ(testRun<Common::ArrayOutputLen>({KeyOutputHandle, 0}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(*MemInst.getPointer<__wasi_symmetric_key_t *>(0), 64);

    // a test statehandle for error case check
    writeString("HMAC/SHA-512", 0);
    writeOptKey(KeyHandle, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto StateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(28);

    // error case check
    writeString("HMAC/SHA-512", 0);
    writeOptKey(std::nullopt, 12);
    writeOptOptions(std::nullopt, 20);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 12, 12, 20, 28}),
              __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
    EXPECT_EQ(testRun<Symmetric::StateSqueeze>({StateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);

    writeString("HMAC/SHA-512", 0);
    EXPECT_EQ(testRun<Symmetric::StateSqueezeKey>({StateHandle, 0, 12, 12}),
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
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({StateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }
}
