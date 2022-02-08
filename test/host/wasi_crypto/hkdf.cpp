#include "helper.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/symmetric/func.h"

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;

TEST_F(WasiCryptoTest, HkdfSha256) {
  {
    writeString("HKDF-EXTRACT/SHA-512", 0);
    writeSpan("IKM"_u8, 20);
    EXPECT_EQ(testRun<Symmetric::KeyImport>({0, 20, 20, 3, 23}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(23);

    writeString("HKDF-EXTRACT/SHA-512", 0);
    writeOptKey(KeyHandle, 20);
    writeOptOptions(std::nullopt, 28);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 20, 20, 28, 36}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto ExtractStateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(36);

    writeSpan("salt"_u8, 0);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({ExtractStateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("HKDF-EXPAND/SHA-512", 0);
    EXPECT_EQ(testRun<Symmetric::StateSqueezeKey>({ExtractStateHandle, 0, 19, 19}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto PrkHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(19);

    EXPECT_EQ(testRun<Symmetric::StateClose>({ExtractStateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("HKDF-EXPAND/SHA-512", 0);
    writeOptKey(PrkHandle, 19);
    writeOptOptions(std::nullopt, 27);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 19, 19, 27, 35}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto ExpandStateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(35);

    writeSpan("info"_u8, 0);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({ExpandStateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateSqueeze>({ExpandStateHandle, 0, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({ExpandStateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }
}


TEST_F(WasiCryptoTest, HkdfSha512) {
  {
    writeString("HKDF-EXTRACT/SHA-512", 0);
    writeSpan("IKM"_u8, 20);
    EXPECT_EQ(testRun<Symmetric::KeyImport>({0, 20, 20, 3, 23}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    auto KeyHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(23);

    writeString("HKDF-EXTRACT/SHA-512", 0);
    writeOptKey(KeyHandle, 20);
    writeOptOptions(std::nullopt, 28);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 20, 20, 28, 36}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto ExtractStateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(36);

    writeSpan("salt"_u8, 0);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({ExtractStateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("HKDF-EXPAND/SHA-512", 0);
    EXPECT_EQ(testRun<Symmetric::StateSqueezeKey>({ExtractStateHandle, 0, 19, 19}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto PrkHandle = *MemInst.getPointer<__wasi_symmetric_key_t *>(19);

    EXPECT_EQ(testRun<Symmetric::StateClose>({ExtractStateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::KeyClose>({KeyHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);

    writeString("HKDF-EXPAND/SHA-512", 0);
    writeOptKey(PrkHandle, 19);
    writeOptOptions(std::nullopt, 27);
    EXPECT_EQ(testRun<Symmetric::StateOpen>({0, 19, 19, 27, 35}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    auto ExpandStateHandle = *MemInst.getPointer<__wasi_symmetric_state_t *>(35);

    writeSpan("info"_u8, 0);
    EXPECT_EQ(testRun<Symmetric::StateAbsorb>({ExpandStateHandle, 0, 4}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateSqueeze>({ExpandStateHandle, 0, 32}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
    EXPECT_EQ(testRun<Symmetric::StateClose>({ExpandStateHandle}),
              __WASI_CRYPTO_ERRNO_SUCCESS);
  }
}
