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
  EncodingCheck(
      "ECDSA_P256_SHA256"sv, __WASI_ALGORITHM_TYPE_SIGNATURES,
      {{__WASI_PUBLICKEY_ENCODING_SEC,
        "0460FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB67903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299"_u8v},
       {__WASI_PUBLICKEY_ENCODING_PKCS8,
        "3059301306072a8648ce3d020106082a8648ce3d0301070342000460FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB67903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299"_u8v},
       {__WASI_PUBLICKEY_ENCODING_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEYP7UuiVanTHJYet0xjVtaMBJuJI7\n"
        "Yfps5mliLmDyn7Z5A/4QCLi8maQa6elWKLxk8vGyDC1+n1F3o8KU1EYimQ==\n"
        "-----END PUBLIC KEY-----\n"_u8},
       {__WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC,
        "0360FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6"_u8v},
       {__WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8,
        "3039301306072a8648ce3d020106082a8648ce3d0301070322000360FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6"_u8v},
       {__WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MDkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDIgADYP7UuiVanTHJYet0xjVtaMBJuJI7\n"
        "Yfps5mliLmDyn7Y=\n"
        "-----END PUBLIC KEY-----\n"_u8}},
      {{__WASI_SECRETKEY_ENCODING_RAW,
        "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721"_u8v},
       {__WASI_SECRETKEY_ENCODING_PEM,
        "-----BEGIN PRIVATE KEY-----\n"
        "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgya+p2EW6dRZrXCFX\n"
        "Z7HWk05Qw9s26JsSe4piKxIPZyGhRANCAARg/tS6JVqdMclh63TGNW1owEm4kjth\n"
        "+mzmaWIuYPKftnkD/hAIuLyZpBrp6VYovGTy8bIMLX6fUXejwpTURiKZ\n"
        "-----END PRIVATE KEY-----\n"_u8}},
      {});
  EncodingCheck(
      "ECDSA_K256_SHA256"sv, __WASI_ALGORITHM_TYPE_SIGNATURES,
      {{__WASI_PUBLICKEY_ENCODING_SEC,
        "04b838ff44e5bc177bf21189d0766082fc9d843226887fc9760371100b7ee20a6ff0c9d75bfba7b31a6bca1974496eeb56de357071955d83c4b1badaa0b21832e9"_u8v},
       {__WASI_PUBLICKEY_ENCODING_PKCS8,
        "3056301006072a8648ce3d020106052b8104000a03420004b838ff44e5bc177bf21189d0766082fc9d843226887fc9760371100b7ee20a6ff0c9d75bfba7b31a6bca1974496eeb56de357071955d83c4b1badaa0b21832e9"_u8v},
       {__WASI_PUBLICKEY_ENCODING_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEuDj/ROW8F3vyEYnQdmCC/J2EMiaIf8l2\n"
        "A3EQC37iCm/wyddb+6ezGmvKGXRJbutW3jVwcZVdg8Sxutqgshgy6Q==\n"
        "-----END PUBLIC KEY-----"_u8},
       {__WASI_PUBLICKEY_ENCODING_COMPRESSED_SEC,
        "03b838ff44e5bc177bf21189d0766082fc9d843226887fc9760371100b7ee20a6f"_u8v},
       {__WASI_PUBLICKEY_ENCODING_COMPRESSED_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MDYwEAYHKoZIzj0CAQYFK4EEAAoDIgADuDj/ROW8F3vyEYnQdmCC/J2EMiaIf8l2\n"
        "A3EQC37iCm8=\n"
        "-----END PUBLIC KEY-----\n"_u8},
       {__WASI_PUBLICKEY_ENCODING_COMPRESSED_PKCS8,
        "3036301006072a8648ce3d020106052b8104000a03220003b838ff44e5bc177bf21189d0766082fc9d843226887fc9760371100b7ee20a6f"_u8v}},
      {{__WASI_SECRETKEY_ENCODING_RAW,
        "b9aa5c28ef96d750e47f4ba44d5d6a7ac3ab6988d292e7819e362a4b0ac8e250"_u8v},
       {__WASI_SECRETKEY_ENCODING_PKCS8,
        "30740201010420b9aa5c28ef96d750e47f4ba44d5d6a7ac3ab6988d292e7819e362a4b"
        "0ac8e250a00706052b8104000aa144034200047fef8e21686370c7d343992f14b2d45a"
        "262cd6a5c75032736fcbb02f46a99edf0e1d114cdc93956cc75648bfd38fa832a82135"
        "d5c2ba634766a8753f6d88aae5"_u8v},
       {__WASI_SECRETKEY_ENCODING_PEM,
        "-----BEGIN PRIVATE KEY-----\n"
        "MIGEAgEAMBAGByqGSM49AgEGBSuBBAAKBG0wawIBAQQguapcKO+W11Dkf0ukTV1q\n"
        "esOraYjSkueBnjYqSwrI4lChRANCAAR/744haGNwx9NDmS8UstRaJizWpcdQMnNv\n"
        "y7AvRqme3w4dEUzck5Vsx1ZIv9OPqDKoITXVwrpjR2aodT9tiKrl\n"
        "-----END PRIVATE KEY-----\n"_u8}},
      {});
  EncodingCheck(
      "Ed25519"sv, __WASI_ALGORITHM_TYPE_SIGNATURES,
      {{__WASI_PUBLICKEY_ENCODING_RAW,
        "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"_u8v}},
      {{__WASI_SECRETKEY_ENCODING_RAW,
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"_u8v}},
      {{__WASI_KEYPAIR_ENCODING_RAW,
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"_u8v}});
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge