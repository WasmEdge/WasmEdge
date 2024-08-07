// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "signatures/ecdsa.h"

#include <openssl/pem.h>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

namespace {
inline const size_t RawSigSize = 64;
} // namespace

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::VerificationState>
Ecdsa<CurveNid>::PublicKey::openVerificationState() const noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslCheck(EVP_DigestVerifyInit(SignCtx.get(), nullptr, EVP_sha256(),
                                    nullptr, this->Ctx.get()));
  return SignCtx;
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::SignState>
Ecdsa<CurveNid>::KeyPair::openSignState() const noexcept {
  EvpMdCtxPtr SignCtx{EVP_MD_CTX_create()};
  opensslCheck(EVP_DigestSignInit(SignCtx.get(), nullptr, EVP_sha256(), nullptr,
                                  this->Ctx.get()));

  return SignCtx;
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::Signature>
Ecdsa<CurveNid>::Signature::import(
    Span<const uint8_t> Encoded,
    __wasi_signature_encoding_e_t Encoding) noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW: {
    ensureOrReturn(Encoded.size() == RawSigSize,
                   __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    EcdsaSigPtr Sig{o2iEcdsaSig(Encoded)};
    ensureOrReturn(Sig, __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
    return i2dEcdsaSig(Sig.get());
  }
  case __WASI_SIGNATURE_ENCODING_DER: {
    return std::vector<uint8_t>(Encoded.begin(), Encoded.end());
  }
  default:
    assumingUnreachable();
  }
}

template <int CurveNid>
WasiCryptoExpect<std::vector<uint8_t>> Ecdsa<CurveNid>::Signature::exportData(
    __wasi_signature_encoding_e_t Encoding) const noexcept {
  switch (Encoding) {
  case __WASI_SIGNATURE_ENCODING_RAW: {
    EcdsaSigPtr Sig{d2iEcdsaSig(Data)};
    ensureOrReturn(Sig, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
    return i2oEcdsaSig(Sig.get());
  }
  case __WASI_SIGNATURE_ENCODING_DER: {
    return Data;
  }
  default:
    assumingUnreachable();
  }
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::SignState::update(Span<const uint8_t> Data) noexcept {
  std::scoped_lock Lock{Ctx->Mutex};
  opensslCheck(
      EVP_DigestSignUpdate(Ctx->RawCtx.get(), Data.data(), Data.size()));
  return {};
}

template <int CurveNid>
WasiCryptoExpect<typename Ecdsa<CurveNid>::Signature>
Ecdsa<CurveNid>::SignState::sign() noexcept {
  size_t Size;
  // For ecdsa, OpenSSL produce a der format signatures which means the size is
  // not fixed. Here is an answer talk about it:
  // https://bitcoin.stackexchange.com/questions/77191/what-is-the-maximum-size-of-a-der-encoded-ecdsa-signature
  // So instead of fixing size, just read.
  std::scoped_lock Lock{Ctx->Mutex};
  opensslCheck(EVP_DigestSignFinal(Ctx->RawCtx.get(), nullptr, &Size));

  std::vector<uint8_t> Res(Size);
  opensslCheck(EVP_DigestSignFinal(Ctx->RawCtx.get(), Res.data(), &Size));

  return Res;
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::VerificationState::update(Span<const uint8_t> Data) noexcept {
  std::scoped_lock Lock{Ctx->Mutex};
  opensslCheck(
      EVP_DigestVerifyUpdate(Ctx->RawCtx.get(), Data.data(), Data.size()));
  return {};
}

template <int CurveNid>
WasiCryptoExpect<void>
Ecdsa<CurveNid>::VerificationState::verify(const Signature &Sig) noexcept {
  std::scoped_lock Lock{Ctx->Mutex};
  ensureOrReturn(EVP_DigestVerifyFinal(Ctx->RawCtx.get(), Sig.ref().data(),
                                       Sig.ref().size()),
                 __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED);
  return {};
}

template class Ecdsa<NID_X9_62_prime256v1>;
template class Ecdsa<NID_secp256k1>;
template class Ecdsa<NID_secp384r1>;

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
