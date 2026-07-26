// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "helper.h"

#include <openssl/opensslv.h>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, KxDh) {

  auto KxDhTest = [this](std::string_view Alg, const std::vector<uint8_t> &Pk1,
                         const std::vector<uint8_t> &Sk1,
                         const std::vector<uint8_t> &Pk2,
                         const std::vector<uint8_t> &Sk2,
                         const std::vector<uint8_t> &SharedSecret) {
    SCOPED_TRACE(Alg);
    WASI_CRYPTO_EXPECT_SUCCESS(
        Pk1Handle, publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Pk1,
                                   __WASI_PUBLICKEY_ENCODING_RAW));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Sk1Handle, secretkeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Sk1,
                                   __WASI_SECRETKEY_ENCODING_RAW));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Pk2Handle, publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Pk2,
                                   __WASI_PUBLICKEY_ENCODING_RAW));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Sk2Handle, secretkeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Sk2,
                                   __WASI_SECRETKEY_ENCODING_RAW));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Handle, kxDh(Pk1Handle, Sk2Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Size,
                               arrayOutputLen(SharedKey1Handle));
    EXPECT_EQ(SharedKey1Size, 32);
    std::vector<uint8_t> SharedKey1(32);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1PullSize,
                               arrayOutputPull(SharedKey1Handle, SharedKey1));
    EXPECT_EQ(SharedKey1PullSize, 32);
    EXPECT_EQ(SharedKey1, SharedSecret);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Handle, kxDh(Pk2Handle, Sk1Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Size,
                               arrayOutputLen(SharedKey2Handle));
    EXPECT_EQ(SharedKey2Size, 32);
    std::vector<uint8_t> SharedKey2(32);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SharedKey2Handle, SharedKey2));
    EXPECT_EQ(SharedKey2, SharedSecret);

    /// It's only supported in OpenSSL 3.0.
    /// See: https://github.com/openssl/openssl/issues/7616
    WASI_CRYPTO_EXPECT_FAILURE(kxEncapsulate(Pk1Handle),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_FAILURE(kxDecapsulate(Sk1Handle, {}),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk1Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk2Handle));

    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk2Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk1Handle));
  };

  // From: https://datatracker.ietf.org/doc/html/rfc7748#section-6.1
  KxDhTest(
      "X25519"sv,
      "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"_u8v,
      "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"_u8v,
      "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f"_u8v,
      "5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb"_u8v,
      "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742"_u8v);

  auto NewKxDhTest = [this](std::string_view Alg) {
    SCOPED_TRACE(Alg);

    WASI_CRYPTO_EXPECT_SUCCESS(
        Kp1Handle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(
        Kp2Handle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));

    WASI_CRYPTO_EXPECT_SUCCESS(Pk1Handle, keypairPublickey(Kp1Handle));
    WASI_CRYPTO_EXPECT_SUCCESS(Sk1Handle, keypairSecretkey(Kp1Handle));
    WASI_CRYPTO_EXPECT_SUCCESS(Pk2Handle, keypairPublickey(Kp2Handle));
    WASI_CRYPTO_EXPECT_SUCCESS(Sk2Handle, keypairSecretkey(Kp2Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Handle, kxDh(Pk1Handle, Sk2Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1Size,
                               arrayOutputLen(SharedKey1Handle));
    EXPECT_EQ(SharedKey1Size, 32);
    std::vector<uint8_t> SharedKey1(32);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey1PullSize,
                               arrayOutputPull(SharedKey1Handle, SharedKey1));
    EXPECT_EQ(SharedKey1PullSize, 32);

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Handle, kxDh(Pk2Handle, Sk1Handle));

    WASI_CRYPTO_EXPECT_SUCCESS(SharedKey2Size,
                               arrayOutputLen(SharedKey2Handle));
    EXPECT_EQ(SharedKey2Size, 32);
    std::vector<uint8_t> SharedKey2(32);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SharedKey2Handle, SharedKey2));

    EXPECT_EQ(SharedKey1, SharedKey2);

    /// It's only supported in OpenSSL 3.0.
    /// See: https://github.com/openssl/openssl/issues/7616
    WASI_CRYPTO_EXPECT_FAILURE(kxEncapsulate(Pk1Handle),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_FAILURE(kxDecapsulate(Sk1Handle, {}),
                               __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk1Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk2Handle));

    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(Pk2Handle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(Sk1Handle));
  };
  NewKxDhTest("P256-SHA256"sv);
  NewKxDhTest("P384-SHA384"sv);
}

#if OPENSSL_VERSION_NUMBER >= 0x30500000L
TEST_F(WasiCryptoTest, KxMlKemKeypairGenerate) {
  auto MlKemGenerateTest = [this](std::string_view Alg, size_t PkSize) {
    SCOPED_TRACE(Alg);

    auto ExportPk = [this, PkSize](std::string_view InnerAlg) {
      WASI_CRYPTO_EXPECT_SUCCESS(
          KpHandle, keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
                                    InnerAlg, std::nullopt));
      WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
      WASI_CRYPTO_EXPECT_SUCCESS(
          OutputHandle,
          publickeyExport(PkHandle, __WASI_PUBLICKEY_ENCODING_RAW));
      WASI_CRYPTO_EXPECT_SUCCESS(PkLen, arrayOutputLen(OutputHandle));
      EXPECT_EQ(PkLen, PkSize);

      std::vector<uint8_t> Pk(PkSize);
      WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(OutputHandle, Pk));

      WASI_CRYPTO_EXPECT_FAILURE(
          publickeyExport(PkHandle, __WASI_PUBLICKEY_ENCODING_PEM),
          __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);

      WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
      WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
      return Pk;
    };

    const auto Pk1 = ExportPk(Alg);
    const auto Pk2 = ExportPk(Alg);

    EXPECT_EQ(Pk1.size(), PkSize);
    EXPECT_NE(Pk1, std::vector<uint8_t>(PkSize, 0));
    EXPECT_NE(Pk1, Pk2);
  };
  MlKemGenerateTest("ML-KEM-512"sv, 800);
  MlKemGenerateTest("ML-KEM-768"sv, 1184);
  MlKemGenerateTest("ML-KEM-1024"sv, 1568);
}

TEST_F(WasiCryptoTest, KxMlKemKeyAccessors) {
  auto MlKemAccessorTest = [this](std::string_view Alg, size_t PkSize) {
    SCOPED_TRACE(Alg);

    WASI_CRYPTO_EXPECT_SUCCESS(
        KpHandle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));

    WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
    WASI_CRYPTO_EXPECT_SUCCESS(SkHandle, keypairSecretkey(KpHandle));

    WASI_CRYPTO_EXPECT_SUCCESS(DerivedPkHandle,
                               publickeyFromSecretkey(SkHandle));

    auto ExportRaw = [this, PkSize](__wasi_publickey_t Handle) {
      WASI_CRYPTO_EXPECT_SUCCESS(
          OutputHandle, publickeyExport(Handle, __WASI_PUBLICKEY_ENCODING_RAW));
      WASI_CRYPTO_EXPECT_SUCCESS(Len, arrayOutputLen(OutputHandle));
      EXPECT_EQ(Len, PkSize);
      std::vector<uint8_t> Raw(PkSize);
      WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(OutputHandle, Raw));
      return Raw;
    };

    // The decapsulation key embeds the encapsulation key, so the public key
    // derived from the secret key must equal the one from the keypair.
    EXPECT_EQ(ExportRaw(PkHandle), ExportRaw(DerivedPkHandle));

    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(DerivedPkHandle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(SkHandle));
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
    WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
  };
  MlKemAccessorTest("ML-KEM-512"sv, 800);
  MlKemAccessorTest("ML-KEM-768"sv, 1184);
  MlKemAccessorTest("ML-KEM-1024"sv, 1568);
}

TEST_F(WasiCryptoTest, KxMlKemEncapsulate) {
  auto MlKemEncapsulateTest = [this](std::string_view Alg, size_t CtSize) {
    SCOPED_TRACE(Alg);
    constexpr size_t SecretSize = 32;

    WASI_CRYPTO_EXPECT_SUCCESS(
        KpHandle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));

    auto Encapsulate = [this, CtSize, SecretSize, PkHandle]() {
      WASI_CRYPTO_EXPECT_SUCCESS(Handles, kxEncapsulate(PkHandle));
      const auto [SecretHandle, CiphertextHandle] = Handles;

      WASI_CRYPTO_EXPECT_SUCCESS(SecretLen, arrayOutputLen(SecretHandle));
      EXPECT_EQ(SecretLen, SecretSize);
      WASI_CRYPTO_EXPECT_SUCCESS(CiphertextLen,
                                 arrayOutputLen(CiphertextHandle));
      EXPECT_EQ(CiphertextLen, CtSize);

      std::vector<uint8_t> Secret(SecretSize);
      std::vector<uint8_t> Ciphertext(CtSize);
      WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SecretHandle, Secret));
      WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(CiphertextHandle, Ciphertext));
      return std::make_pair(Secret, Ciphertext);
    };

    const auto [Secret1, Ciphertext1] = Encapsulate();
    const auto [Secret2, Ciphertext2] = Encapsulate();

    EXPECT_NE(Secret1, std::vector<uint8_t>(SecretSize, 0));
    EXPECT_NE(Ciphertext1, std::vector<uint8_t>(CtSize, 0));

    // Encapsulation draws fresh randomness, so the same public key must not
    // produce the same ciphertext or secret twice.
    EXPECT_NE(Secret1, Secret2);
    EXPECT_NE(Ciphertext1, Ciphertext2);

    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
    WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
  };
  MlKemEncapsulateTest("ML-KEM-512"sv, 768);
  MlKemEncapsulateTest("ML-KEM-768"sv, 1088);
  MlKemEncapsulateTest("ML-KEM-1024"sv, 1568);
}

TEST_F(WasiCryptoTest, KxMlKemRoundTrip) {
  auto MlKemRoundTripTest = [this](std::string_view Alg, size_t CtSize) {
    SCOPED_TRACE(Alg);
    constexpr size_t SecretSize = 32;

    WASI_CRYPTO_EXPECT_SUCCESS(
        KpHandle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
    WASI_CRYPTO_EXPECT_SUCCESS(SkHandle, keypairSecretkey(KpHandle));

    WASI_CRYPTO_EXPECT_SUCCESS(Handles, kxEncapsulate(PkHandle));
    const auto [SecretHandle, CiphertextHandle] = Handles;

    std::vector<uint8_t> Secret(SecretSize);
    std::vector<uint8_t> Ciphertext(CtSize);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SecretHandle, Secret));
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(CiphertextHandle, Ciphertext));

    auto Decapsulate = [this, SkHandle,
                        SecretSize](Span<const uint8_t> EncapsulatedSecret) {
      WASI_CRYPTO_EXPECT_SUCCESS(OutputHandle,
                                 kxDecapsulate(SkHandle, EncapsulatedSecret));
      WASI_CRYPTO_EXPECT_SUCCESS(Len, arrayOutputLen(OutputHandle));
      EXPECT_EQ(Len, SecretSize);
      std::vector<uint8_t> Out(SecretSize);
      WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(OutputHandle, Out));
      return Out;
    };

    EXPECT_EQ(Decapsulate(Ciphertext), Secret);

    // FIPS 203 uses implicit rejection: decapsulating a corrupted ciphertext
    // succeeds and yields a pseudorandom secret instead of failing.
    std::vector<uint8_t> Corrupted = Ciphertext;
    Corrupted[0] ^= 0xff;
    EXPECT_NE(Decapsulate(Corrupted), Secret);

    // A wrong-length ciphertext is a real error, unlike a corrupted one.
    WASI_CRYPTO_EXPECT_FAILURE(
        kxDecapsulate(SkHandle,
                      Span<const uint8_t>(Ciphertext.data(), CtSize - 1)),
        __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(SkHandle));
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
    WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
  };
  MlKemRoundTripTest("ML-KEM-512"sv, 768);
  MlKemRoundTripTest("ML-KEM-768"sv, 1088);
  MlKemRoundTripTest("ML-KEM-1024"sv, 1568);
}

TEST_F(WasiCryptoTest, KxMlKemPublickeyImport) {
  auto MlKemPkImportTest = [this](std::string_view Alg, size_t PkSize,
                                  size_t CtSize) {
    SCOPED_TRACE(Alg);
    constexpr size_t SecretSize = 32;

    WASI_CRYPTO_EXPECT_SUCCESS(
        KpHandle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
    WASI_CRYPTO_EXPECT_SUCCESS(SkHandle, keypairSecretkey(KpHandle));

    WASI_CRYPTO_EXPECT_SUCCESS(
        OutputHandle, publickeyExport(PkHandle, __WASI_PUBLICKEY_ENCODING_RAW));
    std::vector<uint8_t> Raw(PkSize);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(OutputHandle, Raw));

    WASI_CRYPTO_EXPECT_SUCCESS(
        ImportedPkHandle,
        publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Raw,
                        __WASI_PUBLICKEY_ENCODING_RAW));

    WASI_CRYPTO_EXPECT_SUCCESS(
        ReexportHandle,
        publickeyExport(ImportedPkHandle, __WASI_PUBLICKEY_ENCODING_RAW));
    std::vector<uint8_t> Reexported(PkSize);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(ReexportHandle, Reexported));
    EXPECT_EQ(Reexported, Raw);

    // The imported key must be functionally identical: encapsulating with it
    // has to produce a secret the original secret key can recover.
    WASI_CRYPTO_EXPECT_SUCCESS(Handles, kxEncapsulate(ImportedPkHandle));
    const auto [SecretHandle, CiphertextHandle] = Handles;
    std::vector<uint8_t> Secret(SecretSize);
    std::vector<uint8_t> Ciphertext(CtSize);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SecretHandle, Secret));
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(CiphertextHandle, Ciphertext));

    WASI_CRYPTO_EXPECT_SUCCESS(DecapsulatedHandle,
                               kxDecapsulate(SkHandle, Ciphertext));
    std::vector<uint8_t> Decapsulated(SecretSize);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(DecapsulatedHandle, Decapsulated));
    EXPECT_EQ(Decapsulated, Secret);

    std::vector<uint8_t> Short(Raw.begin(), Raw.end() - 1);
    WASI_CRYPTO_EXPECT_FAILURE(
        publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Short,
                        __WASI_PUBLICKEY_ENCODING_RAW),
        __WASI_CRYPTO_ERRNO_INVALID_KEY);

    std::vector<uint8_t> Long = Raw;
    Long.push_back(0);
    WASI_CRYPTO_EXPECT_FAILURE(
        publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Long,
                        __WASI_PUBLICKEY_ENCODING_RAW),
        __WASI_CRYPTO_ERRNO_INVALID_KEY);

    WASI_CRYPTO_EXPECT_FAILURE(
        publickeyImport(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, Raw,
                        __WASI_PUBLICKEY_ENCODING_PEM),
        __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);

    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(ImportedPkHandle));
    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(SkHandle));
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
    WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
  };
  MlKemPkImportTest("ML-KEM-512"sv, 800, 768);
  MlKemPkImportTest("ML-KEM-768"sv, 1184, 1088);
  MlKemPkImportTest("ML-KEM-1024"sv, 1568, 1568);
}

TEST_F(WasiCryptoTest, KxMlKemSecretkeyExport) {
  auto MlKemSkExportTest = [this](std::string_view Alg, size_t PkSize,
                                  size_t SkSize) {
    SCOPED_TRACE(Alg);

    WASI_CRYPTO_EXPECT_SUCCESS(
        KpHandle,
        keypairGenerate(__WASI_ALGORITHM_TYPE_KEY_EXCHANGE, Alg, std::nullopt));
    WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
    WASI_CRYPTO_EXPECT_SUCCESS(SkHandle, keypairSecretkey(KpHandle));

    WASI_CRYPTO_EXPECT_SUCCESS(
        SkOutputHandle,
        secretkeyExport(SkHandle, __WASI_SECRETKEY_ENCODING_RAW));
    WASI_CRYPTO_EXPECT_SUCCESS(SkLen, arrayOutputLen(SkOutputHandle));
    EXPECT_EQ(SkLen, SkSize);
    std::vector<uint8_t> Sk(SkSize);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SkOutputHandle, Sk));
    EXPECT_NE(Sk, std::vector<uint8_t>(SkSize, 0));

    WASI_CRYPTO_EXPECT_SUCCESS(
        PkOutputHandle,
        publickeyExport(PkHandle, __WASI_PUBLICKEY_ENCODING_RAW));
    std::vector<uint8_t> Pk(PkSize);
    WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(PkOutputHandle, Pk));

    // FIPS 203 lays the decapsulation key out as dk_PKE || ek || H(ek) || z,
    // so the encapsulation key must appear verbatim at that offset. This
    // confirms the export really is the expanded dk.
    const size_t EkOffset = SkSize - PkSize - 64;
    EXPECT_EQ(std::vector<uint8_t>(Sk.begin() + EkOffset,
                                   Sk.begin() + EkOffset + PkSize),
              Pk);

    WASI_CRYPTO_EXPECT_FAILURE(
        secretkeyExport(SkHandle, __WASI_SECRETKEY_ENCODING_PEM),
        __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);

    WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(SkHandle));
    WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
    WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
  };
  MlKemSkExportTest("ML-KEM-512"sv, 800, 1632);
  MlKemSkExportTest("ML-KEM-768"sv, 1184, 2400);
  MlKemSkExportTest("ML-KEM-1024"sv, 1568, 3168);
}
#endif

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
