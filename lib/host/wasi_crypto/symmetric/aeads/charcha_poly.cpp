// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aeads/charcha_poly.h"

#include <cstddef>
#include <map>
#include <openssl/rand.h>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
namespace {
constexpr size_t getNonceSize(SymmetricAlgorithm Alg) {
  switch (Alg) {
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return 12;
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return 24;
  default:
    __builtin_unreachable();
  }
}
inline size_t TagLen = 12;
} // namespace

WasiCryptoExpect<std::unique_ptr<ChaChaPolyState>>
ChaChaPolyState::open(SymmetricAlgorithm Alg, std::shared_ptr<Key> OptKey,
                      std::shared_ptr<Option> OptOption) {
  ensureOrReturn(OptKey, __WASI_CRYPTO_ERRNO_KEY_REQUIRED);
  ensureOrReturn(OptOption, __WASI_CRYPTO_ERRNO_NONCE_REQUIRED);

  // Init unique_ptr
  EVP_CIPHER_CTX *Ctx = EVP_CIPHER_CTX_new();
  opensslAssuming(Ctx);

  std::vector<uint8_t> Key =
      OptKey->inner().locked([](auto &Data) { return Data.Data; });

  auto Nonce = OptOption->get("nonce");
  if (!Nonce) {
    return WasiCryptoUnexpect(Nonce);
  }

  opensslAssuming(EVP_CipherInit_ex(Ctx, EVP_chacha20_poly1305(), nullptr,
                                    Key.data(), Nonce->data(),
                                    Mode::Unchanged));

  return std::make_unique<ChaChaPolyState>(Ctx);
}

WasiCryptoExpect<std::unique_ptr<Key>>
ChaChaPolyKeyBuilder::generate(std::shared_ptr<Option>) {
  auto Len = keyLen();
  if (!Len) {
    return WasiCryptoUnexpect(Len);
  }

  std::vector<uint8_t> Res(*Len, 0);
  ensureOrReturn(RAND_bytes(Res.data(), Res.size()),
                 __WASI_CRYPTO_ERRNO_RNG_ERROR);

  return std::make_unique<Key>(Alg, std::move(Res));
}

WasiCryptoExpect<std::unique_ptr<Key>>
ChaChaPolyKeyBuilder::import(Span<uint8_t const> Raw) {
  return std::make_unique<Key>(Alg,
                               std::vector<uint8_t>{Raw.begin(), Raw.end()});
}

WasiCryptoExpect<__wasi_size_t> ChaChaPolyKeyBuilder::keyLen() {
  switch (Alg) {
  case SymmetricAlgorithm::ChaCha20Poly1305:
    return 16;
  case SymmetricAlgorithm::XChaCha20Poly1305:
    return 32;
  default:
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM);
  }
}

WasiCryptoExpect<std::vector<uint8_t>>
ChaChaPolyState::optionsGet(std::string_view Name) {
  ensureOrReturn(OptOptions, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOptions->get(Name);
}

WasiCryptoExpect<uint64_t>
ChaChaPolyState::optionsGetU64(std::string_view Name) {
  ensureOrReturn(OptOptions, __WASI_CRYPTO_ERRNO_OPTION_NOT_SET);
  return OptOptions->getU64(Name);
}

WasiCryptoExpect<void> ChaChaPolyState::absorb(Span<const uint8_t> Data) {
  int Len;
  // TODO: need change Openssl AAD default length from 12 if beyond?
  opensslAssuming(
      EVP_CipherUpdate(Ctx.get(), nullptr, &Len, Data.data(), Data.size()));

  return {};
}

WasiCryptoExpect<Tag>
ChaChaPolyState::encryptDetachedUnchecked(Span<uint8_t> Out,
                                          Span<const uint8_t> Data) {
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Encrypt));
  //  auto Nonce = Options.get("nonce");
  //  if (!Nonce) {
  //    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_NONCE_REQUIRED);
  //  }

  //  if (Out.data() != Data.data()) {
  //    std::copy(Data.begin(), Data.end(), Out.begin());
  //  }

  int ActualOutSize;
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), Data.size()));

  // we need check the equal.
  if (ActualOutSize < 0 ||
      static_cast<__wasi_size_t>(ActualOutSize) != Out.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_NONCE);
  }

  // Notice: Finalise the encryption. Normally ciphertext bytes may be written
  // at this stage, but this does not occur in GCM mode
  // However, cannot do put nullptr length in it. construct a temp var
  // TODO:Better
  int AL;
  opensslAssuming(EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL));

  // Gen tag
  std::vector<uint8_t> RawTagData;
  RawTagData.reserve(TagLen);
  RawTagData.resize(TagLen);

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_GET_TAG, TagLen,
                                      RawTagData.data()));

  return RawTagData;
}

WasiCryptoExpect<__wasi_size_t> ChaChaPolyState::decryptDetachedUnchecked(
    Span<uint8_t> Out, Span<const uint8_t> Data, Span<uint8_t const> RawTag) {
  opensslAssuming(EVP_CipherInit_ex(Ctx.get(), nullptr, nullptr, nullptr,
                                    nullptr, Mode::Decrypt));

  int ActualOutSize;
  opensslAssuming(EVP_CipherUpdate(Ctx.get(), Out.data(), &ActualOutSize,
                                   Data.data(), Data.size()));

  opensslAssuming(EVP_CIPHER_CTX_ctrl(Ctx.get(), EVP_CTRL_AEAD_SET_TAG, TagLen,
                                      const_cast<uint8_t *>(RawTag.data())));

  // Notice: Finalise the decryption. Normally ciphertext bytes may be written
  // at this stage, but this does not occur in GCM mode
  // However, cannot do put nullptr length in it. construct a temp var
  // TODO:Better
  int AL;
  opensslAssuming(EVP_CipherFinal_ex(Ctx.get(), nullptr, &AL));

  return ActualOutSize;
}

WasiCryptoExpect<__wasi_size_t> ChaChaPolyState::maxTagLen() {
  return WasmEdge::Host::WASICrypto::WasiCryptoExpect<__wasi_size_t>();
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
