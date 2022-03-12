#include "gtest/gtest.h"
#include <optional>

#include "helper.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
using namespace std::literals;

TEST_F(WasiCryptoTest, Asymmetric) {
  auto EncodingCheck =
      [this](std::string_view Alg, __wasi_algorithm_type_e_t AlgType,
             std::map<__wasi_publickey_encoding_e_t, std::vector<uint8_t>>
                 SupportPk,
             std::map<__wasi_secretkey_encoding_e_t, std::vector<uint8_t>>
                 SupportSk,
             std::map<__wasi_keypair_encoding_e_t, std::vector<uint8_t>>
                 SupportKp) {
        SCOPED_TRACE(Alg);

        /// function check
        {
          WASI_CRYPTO_EXPECT_SUCCESS(
              KpHandle, keypairGenerate(AlgType, Alg, std::nullopt));
          WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
          WASI_CRYPTO_EXPECT_SUCCESS(SkHandle, keypairSecretkey(KpHandle));
          WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
          // expectedSuccess(keypairClose(
          //     expectedSuccess(keypairFromPkAndSk(PkHandle, SkHandle))));
          WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
          WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(SkHandle));
        }

        /// encoding check
        for (auto &&[PkEncoding, Pk] : SupportPk) {
          SCOPED_TRACE("Public key encoding");
          SCOPED_TRACE(PkEncoding);
          WASI_CRYPTO_EXPECT_SUCCESS(
              PkHandle, publickeyImport(AlgType, Alg, Pk, PkEncoding));

          std::vector<uint8_t> ExportPk(Pk.size());
          WASI_CRYPTO_EXPECT_SUCCESS(PkOutputHandle,
                                     publickeyExport(PkHandle, PkEncoding));
          WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(PkOutputHandle, ExportPk));
          EXPECT_EQ(ExportPk, Pk);
        }
        for (auto &&[SkEncoding, Sk] : SupportSk) {
          SCOPED_TRACE("Secret key encoding");
          SCOPED_TRACE(SkEncoding);
          WASI_CRYPTO_EXPECT_SUCCESS(
              SkHandle, secretkeyImport(AlgType, Alg, Sk, SkEncoding));

          std::vector<uint8_t> ExportSk(Sk.size());
          WASI_CRYPTO_EXPECT_SUCCESS(SkOutputHandle,
                                     secretkeyExport(SkHandle, SkEncoding));
          WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(SkOutputHandle, ExportSk));
          EXPECT_EQ(ExportSk, Sk);
        }
        for (auto &&[KpEncoding, Kp] : SupportKp) {
          SCOPED_TRACE("Key Pair encoding");
          SCOPED_TRACE(KpEncoding);
          WASI_CRYPTO_EXPECT_SUCCESS(
              KpHandle, keypairImport(AlgType, Alg, Kp, KpEncoding));

          std::vector<uint8_t> ExportKp(Kp.size());
          WASI_CRYPTO_EXPECT_SUCCESS(KpOutputHandle,
                                     keypairExport(KpHandle, KpEncoding));
          WASI_CRYPTO_EXPECT_TRUE(arrayOutputPull(KpOutputHandle, ExportKp));
          EXPECT_EQ(ExportKp, Kp);
        }
      };
  EncodingCheck(
      "X25519"sv, __WASI_ALGORITHM_TYPE_KEY_EXCHANGE,
      {{__WASI_PUBLICKEY_ENCODING_RAW,
        "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"_u8v}},
      {{__WASI_SECRETKEY_ENCODING_RAW,
        "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"_u8v}},
      {{__WASI_KEYPAIR_ENCODING_RAW,
        "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"_u8v}});
  //   EncodingCheck("ECDSA_P256_SHA256"sv, __WASI_ALGORITHM_TYPE_SIGNATURES,
  //   {{}},
  //                 {{}}, {{}});
  //   EncodingCheck("ECDSA_P256_SHA256"sv, __WASI_ALGORITHM_TYPE_SIGNATURES,
  //   {{}},
  //                 {{}}, {{}});
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge