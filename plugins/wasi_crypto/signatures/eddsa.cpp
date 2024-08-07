// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "signatures/eddsa.h"

#include <openssl/evp.h>
#include <openssl/x509.h>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

namespace {
inline constexpr size_t PkSize = 32;
inline constexpr size_t SkSize = 32;
inline constexpr size_t KpSize = 64;
inline constexpr size_t SigSize = 64;
} // namespace

WasiCryptoExpect<Eddsa::PublicKey>
Eddsa::PublicKey::import(Span<const uint8_t> Encoded,
                         __wasi_publickey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    EvpPkeyPtr Ctx{EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                               Encoded.data(), Encoded.size())};
    ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

    return Ctx;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<void> Eddsa::PublicKey::verify() const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<std::vector<uint8_t>> Eddsa::PublicKey::exportData(
    __wasi_publickey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_PUBLICKEY_ENCODING_RAW: {
    size_t Size = PkSize;
    std::vector<uint8_t> Res(PkSize);
    opensslCheck(EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<Eddsa::VerificationState>
Eddsa::PublicKey::openVerificationState() const noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};

  opensslCheck(EVP_DigestVerifyInit(SignCtx.get(), nullptr, nullptr, nullptr,
                                    Ctx.get()));
  return SignCtx;
}

WasiCryptoExpect<Eddsa::SecretKey>
Eddsa::SecretKey::import(Span<const uint8_t> Encoded,
                         __wasi_secretkey_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    EvpPkeyPtr Ctx{EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, Encoded.data(), Encoded.size())};
    ensureOrReturn(Ctx, __WASI_CRYPTO_ERRNO_INVALID_KEY);

    return Ctx;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<Eddsa::PublicKey>
Eddsa::SecretKey::publicKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

WasiCryptoExpect<Eddsa::KeyPair>
Eddsa::SecretKey::toKeyPair(const PublicKey &) const noexcept {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED);
}

WasiCryptoExpect<SecretVec> Eddsa::SecretKey::exportData(
    __wasi_secretkey_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SECRETKEY_ENCODING_RAW: {
    size_t Size = SkSize;
    SecretVec Res(SkSize);
    opensslCheck(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<Eddsa::KeyPair>
Eddsa::KeyPair::generate(OptionalRef<const Options>) noexcept {
  // Generate Key.
  EvpPkeyCtxPtr KCtx{EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr)};
  opensslCheck(KCtx);
  opensslCheck(EVP_PKEY_keygen_init(KCtx.get()));

  EVP_PKEY *Key = nullptr;
  opensslCheck(EVP_PKEY_keygen(KCtx.get(), &Key));

  return EvpPkeyPtr{Key};
}

// Refer to: https://github.com/openssl/openssl/issues/8960
WasiCryptoExpect<Eddsa::KeyPair>
Eddsa::KeyPair::import(Span<const uint8_t> Encoded,
                       __wasi_keypair_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == KpSize, __WASI_CRYPTO_ERRNO_INVALID_KEY);
    // PublicKey can auto generate from SecretKey.
    EvpPkeyPtr SkCtx{EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr,
                                                  Encoded.data(), SkSize)};
    ensureOrReturn(SkCtx, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

    return SkCtx;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<SecretVec> Eddsa::KeyPair::exportData(
    __wasi_keypair_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_KEYPAIR_ENCODING_RAW: {
    SecretVec Res(KpSize);
    size_t Size = SkSize;
    opensslCheck(EVP_PKEY_get_raw_private_key(Ctx.get(), Res.data(), &Size));
    ensureOrReturn(Size == SkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    Size = PkSize;
    opensslCheck(
        EVP_PKEY_get_raw_public_key(Ctx.get(), Res.data() + SkSize, &Size));
    ensureOrReturn(Size == PkSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    return Res;
  }
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<Eddsa::PublicKey> Eddsa::KeyPair::publicKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

WasiCryptoExpect<Eddsa::SecretKey> Eddsa::KeyPair::secretKey() const noexcept {
  // Since the inner is always `const`, we just increase the ref count.
  return Ctx;
}

WasiCryptoExpect<Eddsa::SignState>
Eddsa::KeyPair::openSignState() const noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslCheck(SignCtx);

  opensslCheck(
      EVP_DigestSignInit(SignCtx.get(), nullptr, nullptr, nullptr, Ctx.get()));

  return SignCtx;
}

WasiCryptoExpect<Eddsa::Signature>
Eddsa::Signature::import(Span<const uint8_t> Encoded,
                         __wasi_signature_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    ensureOrReturn(Encoded.size() == SigSize,
                   __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    return std::vector<uint8_t>(Encoded.begin(), Encoded.end());
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<std::vector<uint8_t>> Eddsa::Signature::exportData(
    __wasi_signature_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW:
    return Data;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING);
  }
}

WasiCryptoExpect<void>
Eddsa::SignState::update(Span<const uint8_t> Input) noexcept {
  // Notice: Ecdsa is oneshot in OpenSSL, we need a cache for updating instead
  // of calling `EVP_DigestSignUpdate`.
  std::scoped_lock Lock{Ctx->Mutex};

  Ctx->Data.insert(Ctx->Data.end(), Input.begin(), Input.end());
  return {};
}

WasiCryptoExpect<Eddsa::Signature> Eddsa::SignState::sign() noexcept {
  size_t Size = SigSize;
  std::vector<uint8_t> Res(Size);

  std::scoped_lock Lock{Ctx->Mutex};
  opensslCheck(EVP_DigestSign(Ctx->RawCtx.get(), Res.data(), &Size,
                              Ctx->Data.data(), Ctx->Data.size()));
  ensureOrReturn(Size == SigSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  return Res;
}

WasiCryptoExpect<void>
Eddsa::VerificationState::update(Span<const uint8_t> Input) noexcept {
  // Also oneshot.
  std::scoped_lock Lock{Ctx->Mutex};

  Ctx->Data.insert(Ctx->Data.end(), Input.begin(), Input.end());
  return {};
}

WasiCryptoExpect<void>
Eddsa::VerificationState::verify(const Signature &Sig) noexcept {
  std::scoped_lock Lock{Ctx->Mutex};
  // The invocation to EVP_DigestVerifyFinal() internally finalizes a copy of
  // the digest context. This means that EVP_VerifyUpdate() and
  // EVP_VerifyFinal() can be called later to digest and verify the additional
  // data.
  ensureOrReturn(EVP_DigestVerify(Ctx->RawCtx.get(), Sig.ref().data(),
                                  Sig.ref().size(), Ctx->Data.data(),
                                  Ctx->Data.size()),
                 __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);

  return {};
}

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
