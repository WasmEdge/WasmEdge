// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"

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

        // Function checking.
        {
          WASI_CRYPTO_EXPECT_SUCCESS(
              KpHandle, keypairGenerate(AlgType, Alg, std::nullopt));
          WASI_CRYPTO_EXPECT_SUCCESS(PkHandle, keypairPublickey(KpHandle));
          WASI_CRYPTO_EXPECT_SUCCESS(SkHandle, keypairSecretkey(KpHandle));
          WASI_CRYPTO_EXPECT_TRUE(keypairClose(KpHandle));
          WASI_CRYPTO_EXPECT_TRUE(publickeyClose(PkHandle));
          WASI_CRYPTO_EXPECT_TRUE(secretkeyClose(SkHandle));
        }

        // Encoding checking.
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
        "-----END PUBLIC KEY-----"_u8}},
      {{__WASI_SECRETKEY_ENCODING_RAW,
        "b9aa5c28ef96d750e47f4ba44d5d6a7ac3ab6988d292e7819e362a4b0ac8e250"_u8v},
       {__WASI_SECRETKEY_ENCODING_PKCS8,
        "308184020100301006072a8648ce3d020106052b8104000a046d306b02010104"
        "207778b8225c02cc7f2ebcd0a47e2c4fcebd6716a329bdf2e4f961fa35041cba"
        "97a1440342000434e2dea3923666bc28779bcd84fba5b4ee97bb8f6ec3cdc0d8"
        "6609f6c8b8b9ca81592cdf4d3aeccdacb092e94e8f814265f46e3eefb49ad43c"
        "3968e69d4faef4"_u8v},
       {__WASI_SECRETKEY_ENCODING_PEM,
        "-----BEGIN PRIVATE KEY-----\n"
        "MIGEAgEAMBAGByqGSM49AgEGBSuBBAAKBG0wawIBAQQguapcKO+W11Dkf0ukTV1q\n"
        "esOraYjSkueBnjYqSwrI4lChRANCAAR/744haGNwx9NDmS8UstRaJizWpcdQMnNv\n"
        "y7AvRqme3w4dEUzck5Vsx1ZIv9OPqDKoITXVwrpjR2aodT9tiKrl\n"
        "-----END PRIVATE KEY-----\n"_u8}},
      {});
  EncodingCheck(
      "ECDSA_P384_SHA384"sv, __WASI_ALGORITHM_TYPE_SIGNATURES,
      {{__WASI_PUBLICKEY_ENCODING_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEUIwwEUeBvHWigFKAYgPs93Pb21mUr2Oa\n"
        "CTSiuK0Hmjc25OO5m4Gk92s4HG22R5596FB7nXlZljqEnETE4xDc4Dugv5ZzlTCf\n"
        "HLeAvmfv+hFMRzroZ+GS1Xwgl434yH9r\n"
        "-----END PUBLIC KEY-----\n"_u8}},
      {{__WASI_SECRETKEY_ENCODING_PEM,
        "-----BEGIN PRIVATE KEY-----\n"
        "MIG2AgEAMBAGByqGSM49AgEGBSuBBAAiBIGeMIGbAgEBBDAHfLyuXwp7DoNPIvxg\n"
        "B5k8zOAyXHFpJ4FF7CIg4zH/UBFb5m8AyT+c9rvvyVcHlEKhZANiAARQjDARR4G8\n"
        "daKAUoBiA+z3c9vbWZSvY5oJNKK4rQeaNzbk47mbgaT3azgcbbZHnn3oUHudeVmW\n"
        "OoScRMTjENzgO6C/lnOVMJ8ct4C+Z+/6EUxHOuhn4ZLVfCCXjfjIf2s=\n"
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

  auto RsaCheck =
      [EncodingCheck](
          std::string_view Bit,
          std::map<__wasi_publickey_encoding_e_t, std::vector<uint8_t>>
              SupportPk,
          std::map<__wasi_secretkey_encoding_e_t, std::vector<uint8_t>>
              SupportSk,
          std::map<__wasi_keypair_encoding_e_t, std::vector<uint8_t>>
              SupportKp) {
        if (Bit == "2048"sv) {
          EncodingCheck("RSA_PSS_2048_SHA256"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PSS_2048_SHA384"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PSS_2048_SHA512"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PKCS1_2048_SHA256"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PKCS1_2048_SHA384"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PKCS1_2048_SHA512"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
        }
        if (Bit == "3072"sv) {
          EncodingCheck("RSA_PSS_3072_SHA384"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PSS_3072_SHA512"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PKCS1_3072_SHA384"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PKCS1_3072_SHA512"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
        }
        if (Bit == "4096"sv) {
          EncodingCheck("RSA_PSS_4096_SHA512"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
          EncodingCheck("RSA_PKCS1_4096_SHA512"sv,
                        __WASI_ALGORITHM_TYPE_SIGNATURES, SupportPk, SupportSk,
                        SupportKp);
        }
      };

  RsaCheck(
      "2048"sv,
      {{__WASI_PUBLICKEY_ENCODING_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtoGqtGXL6bqbDK0IJ2pJ\n"
        "fM7hfMpyYVXWxtEdv5oNErvGWpHKiH1NIeQxn0ks1QFmkHoK8Quvzyn5/anHLxiZ\n"
        "ecWbzRA01MTfs1y7HMBlrToZhRTZPFe7VRGJ+Liv1jpIiRHPzqabZrssgs3Kj5fG\n"
        "0EYXITaQRfn0kfZcYtJLYi0OvU18DBi64MrLwABr3wqn2UZgMgiw3MhKvyXRybca\n"
        "x0ASO1RTxAJIm21XuFWTztHcJBvl66ygDAzzRdOJyPWvG+TuhNXvZ7dtA0N4iU8p\n"
        "SwJljzLEzWzwKOgAizx3Q3EdS+9P+pTdKtei9UGWVunoj46kCw+0QasQE958NPa3\n"
        "uQIDAQAB\n"
        "-----END PUBLIC KEY-----\n"_u8},
       {__WASI_PUBLICKEY_ENCODING_PKCS8,
        "30820122300d06092a864886f70d01010105000382010f003082010a0282"
        "010100b681aab465cbe9ba9b0cad08276a497ccee17cca726155d6c6d11d"
        "bf9a0d12bbc65a91ca887d4d21e4319f492cd50166907a0af10bafcf29f9"
        "fda9c72f189979c59bcd1034d4c4dfb35cbb1cc065ad3a198514d93c57bb"
        "551189f8b8afd63a488911cfcea69b66bb2c82cdca8f97c6d04617213690"
        "45f9f491f65c62d24b622d0ebd4d7c0c18bae0cacbc0006bdf0aa7d94660"
        "3208b0dcc84abf25d1c9b71ac740123b5453c402489b6d57b85593ced1dc"
        "241be5ebaca00c0cf345d389c8f5af1be4ee84d5ef67b76d034378894f29"
        "4b02658f32c4cd6cf028e8008b3c7743711d4bef4ffa94dd2ad7a2f54196"
        "56e9e88f8ea40b0fb441ab1013de7c34f6b7b90203010001"_u8v}},
      {{__WASI_SECRETKEY_ENCODING_PEM,
        "-----BEGIN PRIVATE KEY-----\n"
        "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC2gaq0ZcvpupsM\n"
        "rQgnakl8zuF8ynJhVdbG0R2/mg0Su8ZakcqIfU0h5DGfSSzVAWaQegrxC6/PKfn9\n"
        "qccvGJl5xZvNEDTUxN+zXLscwGWtOhmFFNk8V7tVEYn4uK/WOkiJEc/OpptmuyyC\n"
        "zcqPl8bQRhchNpBF+fSR9lxi0ktiLQ69TXwMGLrgysvAAGvfCqfZRmAyCLDcyEq/\n"
        "JdHJtxrHQBI7VFPEAkibbVe4VZPO0dwkG+XrrKAMDPNF04nI9a8b5O6E1e9nt20D\n"
        "Q3iJTylLAmWPMsTNbPAo6ACLPHdDcR1L70/6lN0q16L1QZZW6eiPjqQLD7RBqxAT\n"
        "3nw09re5AgMBAAECggEAJds7p3O+GltEshpqKJLZb3QSPapYk2wUwuS5gPbZY1tj\n"
        "x4GaOzmSeEc3K80n6X8C4VEPV/SOoTAZ1M4UrOYzX5jnul90NfYoWLIRdeNKs+Xr\n"
        "STmL3gJsrzaWIetdPdiVFymEq17PuT11/CPnsmVPLgB7573Dq2AvpN8vRqhMTq6j\n"
        "6JoaErGi6nqhfOZ0J7GfwKEJtMGPWbOwyjsMpLxrW3PAy0YH99UZD8Ocxw39hERH\n"
        "8i9SGSjk1ty5/NnpC6K8wJmb7BOUXFl1g00FkH9nI/rydjV3Xc6L+vLeLrTvDE59\n"
        "uPtRKoFSCbHmxoARkzm8RapX+R/tsJOUxwwPRMjaAQKBgQDsja822AFkjKoDtK4k\n"
        "R6R9FKcvW/mUVeKz4ldMQynA+5adU2cyNI7UVRWYcbJI7eSefIaaFe/aHhA2NSny\n"
        "83+IhcPfa+z10TBz93CcLs7szYgBddrjYsQ8Q7gc73dGBRqm4oQW+HpuFqPlU5z1\n"
        "NZ7eBtlJJBCdxpilR/bC+GyQYQKBgQDFgo2vg1dMKEIm7YfRSXojpRUcAxS4qCJe\n"
        "hOcZePkkd1mHmwBkOkX6MMlUozNiCXTmwXjRMWRgZJktbH0I9pwSHCEzJbmfnF5W\n"
        "GUwVdfCEvZ4gIcgDZyhsm4A9+L2qC5Nm3dr30NR7c34K1ZpiK3AJTWoI19uEmY46\n"
        "1ryOGH5GWQKBgQCxKdAPGCm636q5ScmefFWSJDSuQIkkckpuhNby095inUqJG5zP\n"
        "OhO6rNqWqJhpDFpL5GF+520Sg6+KmbiIL5vVaLFxFEiNNhW+1JPvNRNewPPafCTq\n"
        "Zd8ob2NlsGc49rumP0HEXmZ7KtOm/j8wWu9Xw/NaVvtm3wUVzFbgYOQWIQKBgQC+\n"
        "T2mOcJOxQilbsQxpUM9rgSmx8BYLR5a2VIEJPlNyG74ct/HMoYnD5TZZY1ejY1FM\n"
        "96ceiuUZLFWcOyjPdjA0Ev66deNCND2B4KY7F4VFoh+2/lXnUYLWA4+yJvc53iWN\n"
        "vL+8gW/78/DDJ8a2SPyPOhStqLBQOFWfxEGy+U7TIQKBgA+cdl5pm1YcVNhcfGbx\n"
        "zjN75MLGkq9QfL6zxqWIv4xUsjyYkwGgqwazMeNmi5KvhgpfUM8A8tJQixXmq/oe\n"
        "m8MDtOovmQ3I1S6jYK7V8wyxyqgj/2oetPhRIjvht8IaHuFKQkjv6424vdoukuoM\n"
        "3o5BHs2kyvh9kuSthBY9XZnN\n"
        "-----END PRIVATE KEY-----\n"_u8},
       {__WASI_SECRETKEY_ENCODING_PKCS8,
        "308204a40201000282010100b681aab465cbe9ba9b0cad08276a497ccee1"
        "7cca726155d6c6d11dbf9a0d12bbc65a91ca887d4d21e4319f492cd50166"
        "907a0af10bafcf29f9fda9c72f189979c59bcd1034d4c4dfb35cbb1cc065"
        "ad3a198514d93c57bb551189f8b8afd63a488911cfcea69b66bb2c82cdca"
        "8f97c6d0461721369045f9f491f65c62d24b622d0ebd4d7c0c18bae0cacb"
        "c0006bdf0aa7d946603208b0dcc84abf25d1c9b71ac740123b5453c40248"
        "9b6d57b85593ced1dc241be5ebaca00c0cf345d389c8f5af1be4ee84d5ef"
        "67b76d034378894f294b02658f32c4cd6cf028e8008b3c7743711d4bef4f"
        "fa94dd2ad7a2f5419656e9e88f8ea40b0fb441ab1013de7c34f6b7b90203"
        "0100010282010025db3ba773be1a5b44b21a6a2892d96f74123daa58936c"
        "14c2e4b980f6d9635b63c7819a3b39927847372bcd27e97f02e1510f57f4"
        "8ea13019d4ce14ace6335f98e7ba5f7435f62858b21175e34ab3e5eb4939"
        "8bde026caf369621eb5d3dd895172984ab5ecfb93d75fc23e7b2654f2e00"
        "7be7bdc3ab602fa4df2f46a84c4eaea3e89a1a12b1a2ea7aa17ce67427b1"
        "9fc0a109b4c18f59b3b0ca3b0ca4bc6b5b73c0cb4607f7d5190fc39cc70d"
        "fd844447f22f521928e4d6dcb9fcd9e90ba2bcc0999bec13945c5975834d"
        "05907f6723faf27635775dce8bfaf2de2eb4ef0c4e7db8fb512a815209b1"
        "e6c680119339bc45aa57f91fedb09394c70c0f44c8da0102818100ec8daf"
        "36d801648caa03b4ae2447a47d14a72f5bf99455e2b3e2574c4329c0fb96"
        "9d536732348ed455159871b248ede49e7c869a15efda1e10363529f2f37f"
        "8885c3df6becf5d13073f7709c2eceeccd880175dae362c43c43b81cef77"
        "46051aa6e28416f87a6e16a3e5539cf5359ede06d94924109dc698a547f6"
        "c2f86c906102818100c5828daf83574c284226ed87d1497a23a5151c0314"
        "b8a8225e84e71978f9247759879b00643a45fa30c954a333620974e6c178"
        "d131646064992d6c7d08f69c121c213325b99f9c5e56194c1575f084bd9e"
        "2021c80367286c9b803df8bdaa0b9366dddaf7d0d47b737e0ad59a622b70"
        "094d6a08d7db84998e3ad6bc8e187e465902818100b129d00f1829badfaa"
        "b949c99e7c55922434ae408924724a6e84d6f2d3de629d4a891b9ccf3a13"
        "baacda96a898690c5a4be4617ee76d1283af8a99b8882f9bd568b1711448"
        "8d3615bed493ef35135ec0f3da7c24ea65df286f6365b06738f6bba63f41"
        "c45e667b2ad3a6fe3f305aef57c3f35a56fb66df0515cc56e060e4162102"
        "818100be4f698e7093b142295bb10c6950cf6b8129b1f0160b4796b65481"
        "093e53721bbe1cb7f1cca189c3e536596357a363514cf7a71e8ae5192c55"
        "9c3b28cf76303412feba75e342343d81e0a63b178545a21fb6fe55e75182"
        "d6038fb226f739de258dbcbfbc816ffbf3f0c327c6b648fc8f3a14ada8b0"
        "5038559fc441b2f94ed3210281800f9c765e699b561c54d85c7c66f1ce33"
        "7be4c2c692af507cbeb3c6a588bf8c54b23c989301a0ab06b331e3668b92"
        "af860a5f50cf00f2d2508b15e6abfa1e9bc303b4ea2f990dc8d52ea360ae"
        "d5f30cb1caa823ff6a1eb4f851223be1b7c21a1ee14a4248efeb8db8bdda"
        "2e92ea0cde8e411ecda4caf87d92e4ad84163d5d99cd"_u8v}},
      {});
  RsaCheck(
      "3072"sv,
      {{__WASI_PUBLICKEY_ENCODING_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEAnvafMCYCGPGOhpvpA3FG\n"
        "8EtN6P/wIQv+fC3MuqrOo3JBYyYVGIxMrpxX/iGEDOEDWkq9Lp7ANFKMu2P2cR+O\n"
        "udZ+7WvIB8nINcvPZCGdqndOL9PoJV2zJveQ6FVMZpbh7Nc+dj5Mc2WFiE1LwkEe\n"
        "aZD4c5r5UsT2CUvfdDTUoRqNnYVAjnQWfurrfo/o9gjietrvvGKT/dOtdDh/WvEl\n"
        "3+HRwVrEeffsO/1tEPbAPHHvtnd1gTNJbNtzVAAUsHT7a4OxPK7JS3hmtv6JRp/Z\n"
        "j9VUkDie86a1nD7dFW+S7a3N0W/0EMBiWJduiYye1Qitf2Uf0Dpo78J8lnJce7zR\n"
        "1UwQc6EIuNlZh4EODIzz+Pm39locuuFgVVq38dcNStCeSX03GuL75SN3sBT6vXms\n"
        "PeEhjhQxxlkGICbfuhCk1TPHj2Q8UROUAvZzbgspbhrtYYaVGrOc+eWBsqlgRzzV\n"
        "bF//cJ9yRZy0PzsQSVTonAGpNXLHKTpB438hz5auwfNfAgMBAAE=\n"
        "-----END PUBLIC KEY-----\n"_u8},
       {__WASI_PUBLICKEY_ENCODING_PKCS8,
        "308201a2300d06092a864886f70d01010105000382018f003082018a0282"
        "0181009ef69f30260218f18e869be9037146f04b4de8fff0210bfe7c2dcc"
        "baaacea37241632615188c4cae9c57fe21840ce1035a4abd2e9ec034528c"
        "bb63f6711f8eb9d67eed6bc807c9c835cbcf64219daa774e2fd3e8255db3"
        "26f790e8554c6696e1ecd73e763e4c736585884d4bc2411e6990f8739af9"
        "52c4f6094bdf7434d4a11a8d9d85408e74167eeaeb7e8fe8f608e27adaef"
        "bc6293fdd3ad74387f5af125dfe1d1c15ac479f7ec3bfd6d10f6c03c71ef"
        "b677758133496cdb73540014b074fb6b83b13caec94b7866b6fe89469fd9"
        "8fd55490389ef3a6b59c3edd156f92edadcdd16ff410c06258976e898c9e"
        "d508ad7f651fd03a68efc27c96725c7bbcd1d54c1073a108b8d95987810e"
        "0c8cf3f8f9b7f65a1cbae160555ab7f1d70d4ad09e497d371ae2fbe52377"
        "b014fabd79ac3de1218e1431c659062026dfba10a4d533c78f643c511394"
        "02f6736e0b296e1aed6186951ab39cf9e581b2a960473cd56c5fff709f72"
        "459cb43f3b104954e89c01a93572c7293a41e37f21cf96aec1f35f020301"
        "0001"_u8v}},
      {{__WASI_SECRETKEY_ENCODING_PEM,
        "-----BEGIN PRIVATE KEY-----\n"
        "MIIG/AIBADANBgkqhkiG9w0BAQEFAASCBuYwggbiAgEAAoIBgQCe9p8wJgIY8Y6G\n"
        "m+kDcUbwS03o//AhC/58Lcy6qs6jckFjJhUYjEyunFf+IYQM4QNaSr0unsA0Uoy7\n"
        "Y/ZxH4651n7ta8gHycg1y89kIZ2qd04v0+glXbMm95DoVUxmluHs1z52PkxzZYWI\n"
        "TUvCQR5pkPhzmvlSxPYJS990NNShGo2dhUCOdBZ+6ut+j+j2COJ62u+8YpP90610\n"
        "OH9a8SXf4dHBWsR59+w7/W0Q9sA8ce+2d3WBM0ls23NUABSwdPtrg7E8rslLeGa2\n"
        "/olGn9mP1VSQOJ7zprWcPt0Vb5Ltrc3Rb/QQwGJYl26JjJ7VCK1/ZR/QOmjvwnyW\n"
        "clx7vNHVTBBzoQi42VmHgQ4MjPP4+bf2Why64WBVWrfx1w1K0J5JfTca4vvlI3ew\n"
        "FPq9eaw94SGOFDHGWQYgJt+6EKTVM8ePZDxRE5QC9nNuCyluGu1hhpUas5z55YGy\n"
        "qWBHPNVsX/9wn3JFnLQ/OxBJVOicAak1cscpOkHjfyHPlq7B818CAwEAAQKCAYBK\n"
        "w+QLWVUTNkm6tgnaPKUIz+JM/FOMt39yGHh6M2wNI+ftIjQ534MRfSdFt63MAOj6\n"
        "xrxD+RadhVX7rQB0JEuUzHXWZSMnxpgL9VgN2GG3k3WKuTgumutwIHBfVf8hIUYR\n"
        "hwsxwgtjGxS7Dt/a9ZXAQRcaCIHLlCfEJ5NprI91Vm/U7p92YNNTzloEpNsFHRio\n"
        "f+DR0euZLr4eM5RyyYjuy99D+dT/KMRLUt7BY8z2oQAF6hmyMtUOBgkwMPmKJPp9\n"
        "xBLHH/25jcY9t3+PM9vT3xkrJ3GrcF9kHHd4R03lLdxuvf0B/AvHlZUphwYcf6ET\n"
        "eKS1H9t1CII2Z0ZKHlv2C4Zy3ZH06yIFPNjJI9wCJcq9QnH1SN3Kf1Jak7iXT0xC\n"
        "rOrUE4N2mcp8nF3HhgeZZ0vPCW/NBvwHmZgdDIrZI7m5aGq+MZr2JOqrcHgVDMC6\n"
        "KCvMpkJRN8Ine6ICpBKd4tl79mrRBbXWK/hCEIwSG9o7C8CPV6NrnZMfgVMUqgEC\n"
        "gcEAzWoCtDSsn08CUivWoLdwcvXkCXWihRYM45EwSY3Zkz1YLsjp7NNGoX0mOzWW\n"
        "aDa6Ofm2KSQv2pBw3oVcR7ns/IUg/2GSP+v1aZpWGx7wPUu3dbuMfpnt8PKUazbL\n"
        "JoBF83bMXZN5LKijEW4HWqO5WsjUwwLUJm0ouvSabnMof6sCeviiDe38Be2lXGfJ\n"
        "jCsj9hvmGUqPdFCQJ9c6IjXaP20qvOYNDdVYJ5AJLZywfH7RDGHd4+GH5GToHXTr\n"
        "9h7hAoHBAMYcM7jj2NKaNkEYnf7YTnjNymdDYLwm7UCFDKHmluYzleNHuyQu53Y+\n"
        "hSyNhKRRVtdxuSKybDPKzTSw5gN4YP1kCGITcDTIQkt9W70Z/U4JL5WSaTyFelZY\n"
        "Wg8MACPZhIDexZ+f+bNA82VBinE40kTXAsp+I/dNZHusZJnHR0Q0bnarfCz8UuQe\n"
        "qFO4vVH71MpJvthVD/wHuhHZT/NpRXDwgrkKVOC8VwK5U2ZTtVf8uppdsBr0gmyX\n"
        "FHI+kz2aPwKBwQCF1E2SrsbQvB8c/ibFav4+R+mcKCIMZ0NaeFtncJ2SimMLiCav\n"
        "/y6DRBBGfzFREGbgIssFnuf2lCiVMXnf2UiHdQz8lcs9DjRD6yOyY8PNi6kpcVml\n"
        "mhAl7UW5XGea2/O3HW0kglJuQCiN0IvGB+lZNoM30n350yC4PWjoEOsP0pC5IYgj\n"
        "XyvViPE1dQEg63Jwg9i0HZm9BEgHTPg5FbDtpeg0TgWvP5JBpFv2daGeWtlEIfb4\n"
        "4xUwPnXjyyt4nMECgb9cFr/0MfWX8BdIKylGTUYs4Xw0hB1zWKTwWOiGWanLWC9U\n"
        "dwOGzkbJsEY3b5E40JaNj09/0XB6osrAs3o4IrzzDIzZCjAeWPh4Hs2GGY6lt59m\n"
        "56gDeghkGq3CUNG/2Fy/is5SZQqtSIPbjZvNBZy4Yzno5rnROyh6VKhu0zNNgRHY\n"
        "F96hCql9YMLeKAHZGjbP0XflF6VWgkD8CwgfHdApr6MUYLkTvnizy3H5HvAs9k3H\n"
        "c8Vowj/eOlxGvs+y0wKBwHOIGyQ49ethiJMUyt9orVfV04luQgvOOi2VRu8ZQesp\n"
        "hA3tuBVilKq7G9EnH2EHy9/xpOql4SzqvboCivh4oKf0W8UyAKQ3Yv7/h4T/obRY\n"
        "QzPUB3vc657B+5DzQEhjnP88BF6poznANEmuK9YJ4OXeA3RQaQiIEIFbY+GgaO7u\n"
        "znZv+HE2UJcfnbGW3m/naIKfIhcIaq2bfWOG5hYd1L6Dazkfttc2GtRYaBpyo0bz\n"
        "gpvbM5ETOLFocm1MM1hcOA==\n"
        "-----END PRIVATE KEY-----\n"_u8},
       {__WASI_SECRETKEY_ENCODING_PKCS8,
        "308206e202010002820181009ef69f30260218f18e869be9037146f04b4d"
        "e8fff0210bfe7c2dccbaaacea37241632615188c4cae9c57fe21840ce103"
        "5a4abd2e9ec034528cbb63f6711f8eb9d67eed6bc807c9c835cbcf64219d"
        "aa774e2fd3e8255db326f790e8554c6696e1ecd73e763e4c736585884d4b"
        "c2411e6990f8739af952c4f6094bdf7434d4a11a8d9d85408e74167eeaeb"
        "7e8fe8f608e27adaefbc6293fdd3ad74387f5af125dfe1d1c15ac479f7ec"
        "3bfd6d10f6c03c71efb677758133496cdb73540014b074fb6b83b13caec9"
        "4b7866b6fe89469fd98fd55490389ef3a6b59c3edd156f92edadcdd16ff4"
        "10c06258976e898c9ed508ad7f651fd03a68efc27c96725c7bbcd1d54c10"
        "73a108b8d95987810e0c8cf3f8f9b7f65a1cbae160555ab7f1d70d4ad09e"
        "497d371ae2fbe52377b014fabd79ac3de1218e1431c659062026dfba10a4"
        "d533c78f643c51139402f6736e0b296e1aed6186951ab39cf9e581b2a960"
        "473cd56c5fff709f72459cb43f3b104954e89c01a93572c7293a41e37f21"
        "cf96aec1f35f0203010001028201804ac3e40b5955133649bab609da3ca5"
        "08cfe24cfc538cb77f7218787a336c0d23e7ed223439df83117d2745b7ad"
        "cc00e8fac6bc43f9169d8555fbad0074244b94cc75d6652327c6980bf558"
        "0dd861b793758ab9382e9aeb7020705f55ff21214611870b31c20b631b14"
        "bb0edfdaf595c041171a0881cb9427c4279369ac8f75566fd4ee9f7660d3"
        "53ce5a04a4db051d18a87fe0d1d1eb992ebe1e339472c988eecbdf43f9d4"
        "ff28c44b52dec163ccf6a10005ea19b232d50e06093030f98a24fa7dc412"
        "c71ffdb98dc63db77f8f33dbd3df192b2771ab705f641c7778474de52ddc"
        "6ebdfd01fc0bc795952987061c7fa11378a4b51fdb7508823667464a1e5b"
        "f60b8672dd91f4eb22053cd8c923dc0225cabd4271f548ddca7f525a93b8"
        "974f4c42acead413837699ca7c9c5dc7860799674bcf096fcd06fc079998"
        "1d0c8ad923b9b9686abe319af624eaab7078150cc0ba282bcca6425137c2"
        "277ba202a4129de2d97bf66ad105b5d62bf842108c121bda3b0bc08f57a3"
        "6b9d931f815314aa010281c100cd6a02b434ac9f4f02522bd6a0b77072f5"
        "e40975a285160ce39130498dd9933d582ec8e9ecd346a17d263b35966836"
        "ba39f9b629242fda9070de855c47b9ecfc8520ff61923febf5699a561b1e"
        "f03d4bb775bb8c7e99edf0f2946b36cb268045f376cc5d93792ca8a3116e"
        "075aa3b95ac8d4c302d4266d28baf49a6e73287fab027af8a20dedfc05ed"
        "a55c67c98c2b23f61be6194a8f74509027d73a2235da3f6d2abce60d0dd5"
        "582790092d9cb07c7ed10c61dde3e187e464e81d74ebf61ee10281c100c6"
        "1c33b8e3d8d29a3641189dfed84e78cdca674360bc26ed40850ca1e696e6"
        "3395e347bb242ee7763e852c8d84a45156d771b922b26c33cacd34b0e603"
        "7860fd640862137034c8424b7d5bbd19fd4e092f9592693c857a56585a0f"
        "0c0023d98480dec59f9ff9b340f365418a7138d244d702ca7e23f74d647b"
        "ac6499c74744346e76ab7c2cfc52e41ea853b8bd51fbd4ca49bed8550ffc"
        "07ba11d94ff3694570f082b90a54e0bc5702b9536653b557fcba9a5db01a"
        "f4826c9714723e933d9a3f0281c10085d44d92aec6d0bc1f1cfe26c56afe"
        "3e47e99c28220c67435a785b67709d928a630b8826afff2e834410467f31"
        "511066e022cb059ee7f69428953179dfd94887750cfc95cb3d0e3443eb23"
        "b263c3cd8ba9297159a59a1025ed45b95c679adbf3b71d6d2482526e4028"
        "8dd08bc607e959368337d27df9d320b83d68e810eb0fd290b92188235f2b"
        "d588f135750120eb727083d8b41d99bd0448074cf83915b0eda5e8344e05"
        "af3f9241a45bf675a19e5ad94421f6f8e315303e75e3cb2b789cc10281bf"
        "5c16bff431f597f017482b29464d462ce17c34841d7358a4f058e88659a9"
        "cb582f54770386ce46c9b046376f9138d0968d8f4f7fd1707aa2cac0b37a"
        "3822bcf30c8cd90a301e58f8781ecd86198ea5b79f66e7a8037a08641aad"
        "c250d1bfd85cbf8ace52650aad4883db8d9bcd059cb86339e8e6b9d13b28"
        "7a54a86ed3334d8111d817dea10aa97d60c2de2801d91a36cfd177e517a5"
        "568240fc0b081f1dd029afa31460b913be78b3cb71f91ef02cf64dc773c5"
        "68c23fde3a5c46becfb2d30281c073881b2438f5eb61889314cadf68ad57"
        "d5d3896e420bce3a2d9546ef1941eb29840dedb8156294aabb1bd1271f61"
        "07cbdff1a4eaa5e12ceabdba028af878a0a7f45bc53200a43762feff8784"
        "ffa1b4584333d4077bdceb9ec1fb90f34048639cff3c045ea9a339c03449"
        "ae2bd609e0e5de03745069088810815b63e1a068eeeece766ff871365097"
        "1f9db196de6fe768829f2217086aad9b7d6386e6161dd4be836b391fb6d7"
        "361ad458681a72a346f3829bdb33911338b168726d4c33585c38"_u8v}},
      {});
  RsaCheck(
      "4096"sv,
      {{__WASI_PUBLICKEY_ENCODING_PEM,
        "-----BEGIN PUBLIC KEY-----\n"
        "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEArcIlHK14T1jiaKxiiS3E\n"
        "jOcQvxErqnc/aQLPXNIx/d21D1/Ii1vODoSTt5KAvWE87ah0rE5oAs0wMVRWSAV+\n"
        "evUAG+WVjBiN8crGilTVWyylCNVBM2Kwrt95rDYLz68OL7ZOgJ393KfZ2HbMq51Q\n"
        "d6AJsvTkEGVdbbO3q9Opx1htoVm4Crac3TgKpNQceQ6i7b3gFBMaIjGNhD8fuWgK\n"
        "5FAYQgstzURXB5B/vzgOSgkG70q1Ksr8iq5q5UpQRoVaUwq7TXvFR0hARoE0lS7T\n"
        "WeAA5GksGwLYPDfg7WvZn1LmCLU0OAAdPJAegFOZa87rKZTo8PXWMgbQM15/rz1G\n"
        "DFVbJRToGr1pgO0lNZ19uBFfmStcQowHNR1is9GCv08yb6ZQBszA3JhGet2Aaqy9\n"
        "HgN6nS8bzJnYljz3q5MTwzLvtBYl7Bw4MBDypbPnqHeGyUY1LJ8BkRbX0b39FPbl\n"
        "9ywWXDnjJ8sly1D3FQmtOiN54grrYW8Ee9kTPQchdgogOX1pJRTRuEnGJI5aRiDE\n"
        "gVkrx12jSymdeUpuy7UdvPkWKQYzwgABgNc7RZs6+qWkQZBsU96jSxMayKTsP5U6\n"
        "fT2rkSAsmXH6HPjtD2gvpbvl32MUBGlbNOUYh4gePv01GiztuONCjASfhRvjJagK\n"
        "4zn18PfVWWpgqas4rrFDyLECAwEAAQ==\n"
        "-----END PUBLIC KEY-----\n"_u8},
       {__WASI_PUBLICKEY_ENCODING_PKCS8,
        "30820222300d06092a864886f70d01010105000382020f003082020a0282"
        "020100adc2251cad784f58e268ac62892dc48ce710bf112baa773f6902cf"
        "5cd231fdddb50f5fc88b5bce0e8493b79280bd613ceda874ac4e6802cd30"
        "31545648057e7af5001be5958c188df1cac68a54d55b2ca508d5413362b0"
        "aedf79ac360bcfaf0e2fb64e809dfddca7d9d876ccab9d5077a009b2f4e4"
        "10655d6db3b7abd3a9c7586da159b80ab69cdd380aa4d41c790ea2edbde0"
        "14131a22318d843f1fb9680ae45018420b2dcd445707907fbf380e4a0906"
        "ef4ab52acafc8aae6ae54a5046855a530abb4d7bc5474840468134952ed3"
        "59e000e4692c1b02d83c37e0ed6bd99f52e608b53438001d3c901e805399"
        "6bceeb2994e8f0f5d63206d0335e7faf3d460c555b2514e81abd6980ed25"
        "359d7db8115f992b5c428c07351d62b3d182bf4f326fa65006ccc0dc9846"
        "7add806aacbd1e037a9d2f1bcc99d8963cf7ab9313c332efb41625ec1c38"
        "3010f2a5b3e7a87786c946352c9f019116d7d1bdfd14f6e5f72c165c39e3"
        "27cb25cb50f71509ad3a2379e20aeb616f047bd9133d0721760a20397d69"
        "2514d1b849c6248e5a4620c481592bc75da34b299d794a6ecbb51dbcf916"
        "290633c2000180d73b459b3afaa5a441906c53dea34b131ac8a4ec3f953a"
        "7d3dab91202c9971fa1cf8ed0f682fa5bbe5df631404695b34e51887881e"
        "3efd351a2cedb8e3428c049f851be325a80ae339f5f0f7d5596a60a9ab38"
        "aeb143c8b10203010001"_u8v}},
      {{__WASI_SECRETKEY_ENCODING_PEM,
        "-----BEGIN PRIVATE KEY-----\n"
        "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\n"
        "Kah5KuUlkoLBFrMqKJCKqhvP72HZUp8=\n"
        "-----END PRIVATE KEY-----\n"_u8},
       {__WASI_SECRETKEY_ENCODING_PKCS8,
        "308209290201000282020100adc2251cad784f58e268ac62892dc48ce710"
        "bf112baa773f6902cf5cd231fdddb50f5fc88b5bce0e8493b79280bd613c"
        "eda874ac4e6802cd3031545648057e7af5001be5958c188df1cac68a54d5"
        "5b2ca508d5413362b0aedf79ac360bcfaf0e2fb64e809dfddca7d9d876cc"
        "ab9d5077a009b2f4e410655d6db3b7abd3a9c7586da159b80ab69cdd380a"
        "a4d41c790ea2edbde014131a22318d843f1fb9680ae45018420b2dcd4457"
        "07907fbf380e4a0906ef4ab52acafc8aae6ae54a5046855a530abb4d7bc5"
        "474840468134952ed359e000e4692c1b02d83c37e0ed6bd99f52e608b534"
        "38001d3c901e8053996bceeb2994e8f0f5d63206d0335e7faf3d460c555b"
        "2514e81abd6980ed25359d7db8115f992b5c428c07351d62b3d182bf4f32"
        "6fa65006ccc0dc98467add806aacbd1e037a9d2f1bcc99d8963cf7ab9313"
        "c332efb41625ec1c383010f2a5b3e7a87786c946352c9f019116d7d1bdfd"
        "14f6e5f72c165c39e327cb25cb50f71509ad3a2379e20aeb616f047bd913"
        "3d0721760a20397d692514d1b849c6248e5a4620c481592bc75da34b299d"
        "794a6ecbb51dbcf916290633c2000180d73b459b3afaa5a441906c53dea3"
        "4b131ac8a4ec3f953a7d3dab91202c9971fa1cf8ed0f682fa5bbe5df6314"
        "04695b34e51887881e3efd351a2cedb8e3428c049f851be325a80ae339f5"
        "f0f7d5596a60a9ab38aeb143c8b1020301000102820201009fcb41b203dd"
        "f6aab95ca5dbe06814afb7f7f09eebd752df1fc593c9bb0c7a791fffc988"
        "7690b1092ce76414f90c309685c13bbb124818fb766c8730e9ff13782444"
        "3b63818a5b327ec08aa1c0ae8db09afd6a91119e9af9d74ee00ebc01fced"
        "40f7996e32ddb9c52b5424bcc8be5db80597a5da0cbaff5d527bca57dcc9"
        "f027e47a54362ab411c267bd72241723455094eedf59d6cf5bced2646260"
        "dc735040f35fbbace82c33c30d93d7c794d79f4279fc2a6a3db67b55565d"
        "bc0c01933923fde68aa6114335f0be1b98cb30bee5636bdba6330a7ab4bd"
        "037428086ffcacc6e201f412d7c5531dd53417b3ca0e1936af00e0d06864"
        "d990e67bb7561964723b73cac9c0ec9c54a0c4f58d29aed239a32c941789"
        "1b3a42023bb68788e697457d4956c5151d5d9c939fd441f032ab65d9fd0f"
        "7d8fcac72581e608e068cbb10243ced24ad194ff2a729773c2df569ff254"
        "c442937e6a3fc1db2ab635a6847e8bb07d6f6434d3247e2cc943a681e2e6"
        "326badef64174062567c4f729b8221054cf3ba026d37062a51782843b5f4"
        "0f94d97178a718c4a5f0129f9d743bbe7cd997a30a497a2b856dcdffb524"
        "bc5af7cc403995bb48a4386e4c7c3cb5b4bdb6178db096136756db759703"
        "76a29dff59fc9fcaba0da8c03ea53d81aa78f3007fdc0212a723a0d4fbc6"
        "d31bc8e63e93277ed5c793dd4e5148ee7037bea4b2c9d20744f902820101"
        "00e7a5aaff6c7c76b1a50188684664bc94995db54db31940979c11e4ae86"
        "553a446f1b46c3653a240649fb647847f3fb8222321468d2516e9143ce56"
        "ed4a1c58a61cee9c8ac9abc3a14a8004bca3aa548239ac343d5a566f2086"
        "78e9241c83ab856c0ea5f79862d963f08e5e449f3737b30d62e9f12fe1ec"
        "e23d615a2441e565aa27b88123a3cb87299061806de75b9c39a1870fcd33"
        "8cde881c6b60ccd716b5841cc98b43f6f199fe34906d24f9df662c87973b"
        "17740b6526dbd3be95649c8a709e37cbf2f33d442935f847d65c6aa0ae7f"
        "e92352257d09f564f45dff127ffaac193764ad45e1e353767e2a3351faba"
        "002e9055fc08643dddfaae68685ea1cf4f0282010100c006837d3d94e8cc"
        "d5d1cb9d075435531add16ab32c727085969317a79ff2dd3942de70dd2e8"
        "e85bc4d24f3e0f5ac5d10c086d7514b49ec8385da94a6fd5189eace884d4"
        "1caafd2b251fccd47699e1ca1400265c2027e201d1563f54a215982ef4f2"
        "56fc4509bb6cf4f2ddf988e1b018bc5f1c53e88996a8980565c8bfa49045"
        "c0f005b09ace500998dff8866e4ff45b50ae2a833711c5b62cbf47239b29"
        "8239bac53508e3b1e7500f280e9a3d2b03c5f543fb4d7cab31f3a6095760"
        "ac12b5895b845464ffe15acf64478d25d8dbc99d66441ec9c844d3c5809f"
        "7435abd7bd4c18e1512dd03f409fd8e25972d71793690a1162581ebff04b"
        "e0c22c9d421ae7ff0282010100ca4e716ce9be399b23d496e11ab957c91f"
        "b82b63548b355233479d449ea5486203f6fa7223b2074c46c87b126124bf"
        "ff030661b4ba19cc4aa9c14741ad7bdc20171c7d32e8b64e004b244afec4"
        "a13975121146fe5e2b269a6d56a3a69a109477cdcb6d3f33a300e0bb725a"
        "f9eb633a0df21ef4d9634c18a9ed995c345628960568f346356e138e53ca"
        "789cc55f4d2ebf5646b2922ff75351e42013465282cebc36b7fe1cb94a7f"
        "86f7393b5913b0fb76e0643f835821ab91a862e10b6ff717210bcd071d83"
        "397a91d344a6d89b95b4073246d64623df741710b82a6f5e24cf340641ce"
        "775594d6084a701d42fcd1d027e082c5d57b2eb0a9710968bdde07cb1902"
        "8201007084208dc504b8f8351d8e023fe61eacc863ddc188aa5afbb0704b"
        "f6a8ff55d9d0cca8c357def32fba7f44c1677bd7c76b1691147682733b7c"
        "939cfba9d5a26c6f827b3a5265fa9c4a4f7cd4cd7dd3687619b4606cd311"
        "c1e0e879895cc3ec7d2f37c4b262bd961dfcd5462dcdfff8103668409006"
        "0480cbdeefe2b9235a08d3667efdc6829efb14f487dff6dd326a4b0b5652"
        "40ba86e6639d8aa2e3812a6c4ce95c5e7c0bf71543baedccc8a1a8cfb831"
        "c398bfc99a0255f2e72c54cfded2925e968660fbf20f24d06c808f39a767"
        "569c32b41ba606765416ced7e80efe05c44e1bd05658dc8740627416d78c"
        "a31ad4047b3535cf26c6659f98074e4a0abbff02820100396f9627e90f47"
        "64210dae75f1bbc148b43979a3f6c7a943bde4d27f142f35f53d5de83329"
        "271ace78d0a952286820ff8ed87b3bc8e01ba9059d33568c7466d822960d"
        "a81dd4259840061a20c160d2cefcd77d168b72b5c02e823c9a9e7808e486"
        "97cca544016e901594e47886af97f4b8e0db41dfd32333bb5169a24dbbad"
        "0f699fd43a589ccb23634593d2d89ad89b2992a3d2dadbbb22f7263d2ff8"
        "170b0d322f6ff7b1b47970f124a05374a3d5c60ece415ac9ef30bcc7530b"
        "5bb660b63f0e24a7c933464db5707ef9625ff083d3a2afde6e2f40bcc380"
        "7b15a7023e28cd3c23ecbe66098bd0a029a8792ae5259282c116b32a2890"
        "8aaa1bcfef61d9529f"_u8v}},
      {});
}

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
