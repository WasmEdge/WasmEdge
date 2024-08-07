// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/asymmetric_common/ecdsa.h - Ecdsa alg-===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of ecdsa algorithm.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "kx/options.h"
#include "utils/error.h"
#include "utils/evp_wrapper.h"
#include "utils/optional.h"
#include "utils/secret_vec.h"

#include "common/span.h"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

template <int CurveNid, typename PublicKey, typename SecretKey,
          typename KeyPair, typename OptionsType>
class Ecdsa {
  inline static const size_t UnCompressedPkSize = 65;
  inline static const size_t CompressedPkSize = 33;
  constexpr static size_t getRawPkSize(bool Compressed) {
    return Compressed ? CompressedPkSize : UnCompressedPkSize;
  }

  inline static const size_t SkSize = 32;

  constexpr static point_conversion_form_t getForm(bool Compressed) noexcept {
    return Compressed ? POINT_CONVERSION_COMPRESSED
                      : POINT_CONVERSION_UNCOMPRESSED;
  }

public:
  class PublicKeyBase {
  public:
    PublicKeyBase(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    PublicKeyBase(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<PublicKey>
    import(Span<const uint8_t> Encoded,
           __wasi_publickey_encoding_e_t Encoding) noexcept {
      switch (Encoding) {
      case __WASI_PUBLICKEY_ENCODING_PKCS8:
        return importPkcs8(Encoded);
      case __WASI_PUBLICKEY_ENCODING_PEM:
        return importPem(Encoded);
      case __WASI_PUBLICKEY_ENCODING_SEC:
        return importSec(Encoded);
      default:
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      }
    }

    WasiCryptoExpect<std::vector<uint8_t>>
    exportData(__wasi_publickey_encoding_e_t Encoding) const noexcept {
      switch (Encoding) {
      case __WASI_PUBLICKEY_ENCODING_SEC:
        return exportSec(false);
      case __WASI_PUBLICKEY_ENCODING_PEM:
        return exportPem(false);
      case __WASI_PUBLICKEY_ENCODING_PKCS8:
        return exportPkcs8(false);
      default:
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      }
    }

    WasiCryptoExpect<void> verify() const noexcept {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    }

  protected:
    static WasiCryptoExpect<PublicKey>
    importPkcs8(Span<const uint8_t> Encoded) noexcept {
      return checkValid(EvpPkeyPtr{d2iPUBKEY(Encoded)});
    }

    static WasiCryptoExpect<PublicKey>
    importPem(Span<const uint8_t> Encoded) noexcept {
      return checkValid(EvpPkeyPtr{pemReadPUBKEY(Encoded)});
    }

    static WasiCryptoExpect<PublicKey>
    importSec(Span<const uint8_t> Encoded) noexcept {
      EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
      EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
      ensureOrReturn(EC_POINT_oct2point(EC_KEY_get0_group(EcCtx.get()),
                                        Pk.get(), Encoded.data(),
                                        Encoded.size(), nullptr),
                     __WASI_CRYPTO_ERRNO_INVALID_KEY);
      opensslCheck(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

      EvpPkeyPtr Ctx{EVP_PKEY_new()};
      opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));

      return checkValid(std::move(Ctx));
    }

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept {
      ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
      const EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
      ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

      const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
      ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
      ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                     __WASI_CRYPTO_ERRNO_INVALID_KEY);
      return {std::move(Ctx)};
    }

    WasiCryptoExpect<std::vector<uint8_t>>
    exportSec(bool Compressed) const noexcept {
      const EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
      std::vector<uint8_t> Res(getRawPkSize(Compressed));
      opensslCheck(EC_POINT_point2oct(
          EC_KEY_get0_group(EcCtx), EC_KEY_get0_public_key(EcCtx),
          getForm(Compressed), Res.data(), Res.size(), nullptr));
      return Res;
    }

    WasiCryptoExpect<std::vector<uint8_t>>
    exportPem(bool Compressed) const noexcept {
      EC_KEY_set_conv_form(
          const_cast<EC_KEY *>(EVP_PKEY_get0_EC_KEY(Ctx.get())),
          getForm(Compressed));

      return pemWritePUBKEY(Ctx.get());
    }

    WasiCryptoExpect<std::vector<uint8_t>>
    exportPkcs8(bool Compressed) const noexcept {
      EC_KEY_set_conv_form(
          const_cast<EC_KEY *>(EVP_PKEY_get0_EC_KEY(Ctx.get())),
          getForm(Compressed));

      return i2dPUBKEY(Ctx.get());
    }

    SharedEvpPkey Ctx;
  };

  class SecretKeyBase {
  public:
    SecretKeyBase(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    SecretKeyBase(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<SecretKey>
    import(Span<const uint8_t> Encoded,
           __wasi_secretkey_encoding_e_t Encoding) noexcept {
      switch (Encoding) {
      case __WASI_SECRETKEY_ENCODING_RAW:
        return importRaw(Encoded);
      case __WASI_SECRETKEY_ENCODING_PKCS8:
        return importPkcs8(Encoded);
      case __WASI_SECRETKEY_ENCODING_PEM:
        return importPem(Encoded);
      default:
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      }
    }

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_secretkey_encoding_e_t Encoding) const noexcept {
      switch (Encoding) {
      case __WASI_SECRETKEY_ENCODING_RAW:
        return exportRaw();
      case __WASI_SECRETKEY_ENCODING_PKCS8:
        return exportPkcs8();
      case __WASI_SECRETKEY_ENCODING_PEM:
        return exportPem();
      default:
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      }
    }

    WasiCryptoExpect<PublicKey> publicKey() const noexcept {
      // Since the inner is always `const`, we just increase the ref count.
      return Ctx;
    }

    WasiCryptoExpect<KeyPair> toKeyPair(const PublicKey &) const noexcept {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
    }

  protected:
    static WasiCryptoExpect<SecretKey>
    importPkcs8(Span<const uint8_t> Encoded) noexcept {
      return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)});
    }

    static WasiCryptoExpect<SecretKey>
    importPem(Span<const uint8_t> Encoded) noexcept {
      return checkValid(EvpPkeyPtr{pemReadPrivateKey(Encoded)});
    }

    static WasiCryptoExpect<SecretKey>
    importRaw(Span<const uint8_t> Encoded) noexcept {
      EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
      ensureOrReturn(Encoded.size() == SkSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);

      BnPtr Sk{
          BN_bin2bn(Encoded.data(), static_cast<int>(Encoded.size()), nullptr)};
      ensureOrReturn(EC_KEY_set_private_key(EcCtx.get(), Sk.get()),
                     __WASI_CRYPTO_ERRNO_INVALID_KEY);

      EvpPkeyPtr Ctx{EVP_PKEY_new()};
      opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));

      return Ctx;
    }

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept {
      ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
      const EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
      ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

      const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
      ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
      ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                     __WASI_CRYPTO_ERRNO_INVALID_KEY);
      return {std::move(Ctx)};
    }

    WasiCryptoExpect<SecretVec> exportPkcs8() const noexcept {
      EVP_PKEY *Key = Ctx.get();
      BioPtr Bio{BIO_new(BIO_s_mem())};
      opensslCheck(i2d_PKCS8PrivateKey_bio(Bio.get(), Key, nullptr, nullptr, 0,
                                           nullptr, nullptr));

      BUF_MEM *Mem = nullptr;
      opensslCheck(BIO_get_mem_ptr(Bio.get(), &Mem));
      SecretVec Ret(Mem->length);

      if (size_t Size; BIO_read_ex(Bio.get(), Ret.data(), Ret.size(), &Size)) {
        ensureOrReturn(Size == Ret.size(),
                       __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
      } else {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
      }

      return Ret;
    }

    WasiCryptoExpect<SecretVec> exportPem() const noexcept {
      return pemWritePrivateKey(Ctx.get());
    }

    WasiCryptoExpect<SecretVec> exportRaw() const noexcept {
      // Must equal to SkSize, not check.
      const BIGNUM *Sk =
          EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(Ctx.get()));
      SecretVec Res(SkSize);
      opensslCheck(BN_bn2bin(Sk, Res.data()));

      return Res;
    }

    SharedEvpPkey Ctx;
  };

  class KeyPairBase {
  public:
    KeyPairBase(EvpPkeyPtr Ctx) noexcept : Ctx(std::move(Ctx)) {}

    KeyPairBase(SharedEvpPkey Ctx) noexcept : Ctx(std::move(Ctx)) {}

    static WasiCryptoExpect<KeyPair>
    generate(OptionalRef<const OptionsType>) noexcept {
      EvpPkeyCtxPtr ParamCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr)};
      EVP_PKEY_keygen_init(ParamCtx.get());
      EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ParamCtx.get(), CurveNid);

      EVP_PKEY *Key = nullptr;
      opensslCheck(EVP_PKEY_keygen(ParamCtx.get(), &Key));

      return EvpPkeyPtr{Key};
    }

    static WasiCryptoExpect<KeyPair>
    import(Span<const uint8_t> Encoded,
           __wasi_keypair_encoding_e_t Encoding) noexcept {
      switch (Encoding) {
      case __WASI_KEYPAIR_ENCODING_RAW:
        return importRaw(Encoded);
      case __WASI_KEYPAIR_ENCODING_PKCS8:
        return importPkcs8(Encoded);
      case __WASI_KEYPAIR_ENCODING_PEM:
        return importPem(Encoded);
      default:
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      }
    }

    WasiCryptoExpect<PublicKey> publicKey() const noexcept {
      // Since the inner is always `const`, we just increase the ref count.
      return Ctx;
    }

    WasiCryptoExpect<SecretKey> secretKey() const noexcept {
      // Since the inner is always `const`, we just increase the ref count.
      return Ctx;
    }

    WasiCryptoExpect<SecretVec>
    exportData(__wasi_keypair_encoding_e_t Encoding) const noexcept {
      switch (Encoding) {
      case __WASI_KEYPAIR_ENCODING_RAW:
        return exportRaw();
      case __WASI_KEYPAIR_ENCODING_PKCS8:
        return exportPkcs8();
      case __WASI_KEYPAIR_ENCODING_PEM:
        return exportPem();
      default:
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
      }
    }

  protected:
    static WasiCryptoExpect<KeyPair>
    importPkcs8(Span<const uint8_t> Encoded) noexcept {
      return checkValid(EvpPkeyPtr{d2iPrivateKey(Encoded)});
    }

    static WasiCryptoExpect<KeyPair>
    importPem(Span<const uint8_t> Encoded) noexcept {
      return checkValid(EvpPkeyPtr{pemReadPrivateKey(Encoded)});
    }

    static WasiCryptoExpect<KeyPair>
    importRaw(Span<const uint8_t> Encoded) noexcept {
      ensureOrReturn(Encoded.size() == SkSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);
      EcKeyPtr EcCtx{EC_KEY_new_by_curve_name(CurveNid)};
      BnPtr Sk{
          BN_bin2bn(Encoded.data(), static_cast<int>(Encoded.size()), nullptr)};
      ensureOrReturn(EC_KEY_set_private_key(EcCtx.get(), Sk.get()),
                     __WASI_CRYPTO_ERRNO_INVALID_KEY);

      // Calculate and set Pk.
      EcPointPtr Pk{EC_POINT_new(EC_KEY_get0_group(EcCtx.get()))};
      opensslCheck(EC_POINT_mul(EC_KEY_get0_group(EcCtx.get()), Pk.get(),
                                Sk.get(), nullptr, nullptr, nullptr));
      opensslCheck(EC_KEY_set_public_key(EcCtx.get(), Pk.get()));

      EvpPkeyPtr Ctx{EVP_PKEY_new()};
      opensslCheck(EVP_PKEY_set1_EC_KEY(Ctx.get(), EcCtx.get()));
      ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

      return Ctx;
    }

    static WasiCryptoExpect<EvpPkeyPtr> checkValid(EvpPkeyPtr Ctx) noexcept {
      ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);
      const EC_KEY *EcCtx = EVP_PKEY_get0_EC_KEY(Ctx.get());
      ensureOrReturn(EcCtx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

      // Curve id check.
      const EC_GROUP *Group = EC_KEY_get0_group(EcCtx);
      ensureOrReturn(Group, __WASI_CRYPTO_ERRNO_INVALID_KEY);
      ensureOrReturn(EC_GROUP_get_curve_name(Group) == CurveNid,
                     __WASI_CRYPTO_ERRNO_INVALID_KEY);
      // Have public key.
      ensureOrReturn(EC_KEY_get0_public_key(EcCtx),
                     __WASI_CRYPTO_ERRNO_INVALID_KEY);
      return {std::move(Ctx)};
    }

    WasiCryptoExpect<SecretVec> exportPkcs8() const noexcept {
      return i2dPrivateKey(Ctx.get());
    }

    WasiCryptoExpect<SecretVec> exportPem() const noexcept {
      return pemWritePrivateKey(Ctx.get());
    }

    WasiCryptoExpect<SecretVec> exportRaw() const noexcept {
      const BIGNUM *Sk =
          EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(Ctx.get()));
      SecretVec Res(SkSize);
      opensslCheck(BN_bn2bin(Sk, Res.data()));

      return Res;
    }

    SharedEvpPkey Ctx;
  };
};

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
