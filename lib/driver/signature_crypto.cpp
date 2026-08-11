// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "signature_crypto.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <cstring>

namespace WasmEdge {
namespace Driver {

// ---------------------------------------------------------------------------
// EvpPkeyDeleter / EvpMdCtxDeleter
// ---------------------------------------------------------------------------

void EvpPkeyDeleter::operator()(evp_pkey_st *Ptr) const noexcept {
  EVP_PKEY_free(Ptr);
}

void EvpMdCtxDeleter::operator()(evp_md_ctx_st *Ptr) const noexcept {
  EVP_MD_CTX_free(Ptr);
}

// ---------------------------------------------------------------------------
// RollingHasher
// ---------------------------------------------------------------------------

RollingHasher::RollingHasher() noexcept : Ctx(EVP_MD_CTX_new()) {
  if (Ctx) {
    if (EVP_DigestInit_ex(Ctx.get(), EVP_sha256(), nullptr) != 1) {
      Ctx.reset();
    }
  }
}

bool RollingHasher::update(const uint8_t *Data, std::size_t Len) noexcept {
  if (!Ctx) {
    return false;
  }
  return EVP_DigestUpdate(Ctx.get(), Data, Len) == 1;
}

std::array<uint8_t, kSha256DigestLen> RollingHasher::digest() const noexcept {
  std::array<uint8_t, kSha256DigestLen> Out{};
  if (!Ctx) {
    return Out;
  }
  // Clone so we can finalize without disturbing the rolling state.
  EvpMdCtxPtr Copy(EVP_MD_CTX_new());
  if (!Copy) {
    return Out;
  }
  if (EVP_MD_CTX_copy_ex(Copy.get(), Ctx.get()) != 1) {
    return Out;
  }
  unsigned int Len = 0;
  EVP_DigestFinal_ex(Copy.get(), Out.data(), &Len);
  return Out;
}

// ---------------------------------------------------------------------------
// WasmSigKey
// ---------------------------------------------------------------------------

bool WasmSigKey::loadFromBytes(const std::vector<uint8_t> &Blob) noexcept {
  if (Blob.empty()) {
    return false;
  }
  const uint8_t Tag = Blob[0];
  if (Tag == 0x81) {
    // Keypair: 0x81 | secret(32) | public(32)
    if (Blob.size() != 65) {
      return false;
    }
    const uint8_t *Secret = Blob.data() + 1;
    // OpenSSL Ed25519 raw private key is the 32-byte seed.
    EVP_PKEY *Pk =
        EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, Secret, 32);
    if (!Pk) {
      return false;
    }
    Key.reset(Pk);
    CanSign = true;
    return true;
  }
  if (Tag == 0x01) {
    // Public only: 0x01 | public(32)
    if (Blob.size() != 33) {
      return false;
    }
    const uint8_t *Pub = Blob.data() + 1;
    EVP_PKEY *Pk =
        EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, Pub, 32);
    if (!Pk) {
      return false;
    }
    Key.reset(Pk);
    CanSign = false;
    return true;
  }
  return false;
}

std::vector<uint8_t> WasmSigKey::sign(const uint8_t *Msg,
                                      std::size_t MsgLen) const noexcept {
  if (!Key || !CanSign) {
    return {};
  }
  EvpMdCtxPtr Ctx(EVP_MD_CTX_new());
  if (!Ctx) {
    return {};
  }
  if (EVP_DigestSignInit(Ctx.get(), nullptr, nullptr, nullptr, Key.get()) !=
      1) {
    return {};
  }
  std::size_t SigLen = 0;
  if (EVP_DigestSign(Ctx.get(), nullptr, &SigLen, Msg, MsgLen) != 1) {
    return {};
  }
  std::vector<uint8_t> Sig(SigLen);
  if (EVP_DigestSign(Ctx.get(), Sig.data(), &SigLen, Msg, MsgLen) != 1) {
    return {};
  }
  Sig.resize(SigLen);
  return Sig;
}

bool WasmSigKey::verify(const uint8_t *Msg, std::size_t MsgLen,
                        const uint8_t *Sig, std::size_t SigLen) const noexcept {
  if (!Key) {
    return false;
  }
  EvpMdCtxPtr Ctx(EVP_MD_CTX_new());
  if (!Ctx) {
    return false;
  }
  if (EVP_DigestVerifyInit(Ctx.get(), nullptr, nullptr, nullptr, Key.get()) !=
      1) {
    return false;
  }
  return EVP_DigestVerify(Ctx.get(), Sig, SigLen, Msg, MsgLen) == 1;
}

// ---------------------------------------------------------------------------
// encodeUleb128
// ---------------------------------------------------------------------------

std::vector<uint8_t> encodeUleb128(uint32_t Value) noexcept {
  std::vector<uint8_t> Out;
  do {
    uint8_t Byte = static_cast<uint8_t>(Value & 0x7Fu);
    Value >>= 7u;
    if (Value != 0) {
      Byte |= 0x80u;
    }
    Out.push_back(Byte);
  } while (Value != 0);
  return Out;
}

} // namespace Driver
} // namespace WasmEdge
