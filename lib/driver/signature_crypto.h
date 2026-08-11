// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/driver/signature_crypto.h - OpenSSL RAII wrappers --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// RAII wrappers for OpenSSL EVP objects used by the Wasm signature tool.
/// Only compiled when WASMEDGE_BUILD_SIGNATURE_TOOLS is ON.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

// Forward declarations to avoid pulling full OpenSSL headers into every TU
// that includes this header.  The actual definitions only appear in .cpp files.
struct evp_pkey_st;
struct evp_md_ctx_st;

namespace WasmEdge {
namespace Driver {

/// Ed25519 signature length in bytes.
static constexpr std::size_t kEd25519SigLen = 64;
/// SHA-256 digest length in bytes.
static constexpr std::size_t kSha256DigestLen = 32;

/// RAII deleter for EVP_PKEY.
struct EvpPkeyDeleter {
  void operator()(evp_pkey_st *Ptr) const noexcept;
};
using EvpPkeyPtr = std::unique_ptr<evp_pkey_st, EvpPkeyDeleter>;

/// RAII deleter for EVP_MD_CTX.
struct EvpMdCtxDeleter {
  void operator()(evp_md_ctx_st *Ptr) const noexcept;
};
using EvpMdCtxPtr = std::unique_ptr<evp_md_ctx_st, EvpMdCtxDeleter>;

/// Rolling SHA-256 state that accumulates bytes fed into it.
/// Call digest() to obtain the intermediate hash without resetting state.
class RollingHasher {
public:
  RollingHasher() noexcept;

  /// Feed raw bytes into the hash state.
  /// Returns false on OpenSSL error.
  bool update(const uint8_t *Data, std::size_t Len) noexcept;

  /// Snapshot the current hash without finalising (copy-finalize trick).
  /// Returns an empty vector on OpenSSL error.
  std::array<uint8_t, kSha256DigestLen> digest() const noexcept;

private:
  EvpMdCtxPtr Ctx;
};

/// Ed25519 key loaded from the raw serialized format defined in Signatures.md.
///
/// Accepted key file formats (raw binary):
///   - Keypair  (Phase 1 signing):  0x81 | secret(32) | public(32)  => 65 B
///   - Public   (verification):     0x01 | public(32)               => 33 B
class WasmSigKey {
public:
  WasmSigKey() noexcept = default;

  /// Load from a binary blob whose first byte is the key-type identifier.
  /// Returns false when the blob is malformed or the key type is unsupported.
  bool loadFromBytes(const std::vector<uint8_t> &Blob) noexcept;

  /// True after a successful loadFromBytes() call.
  bool isLoaded() const noexcept { return Key != nullptr; }

  /// True if this key can sign (i.e. it carries the private scalar).
  bool canSign() const noexcept { return CanSign; }

  /// Sign `Msg` with this key.  Returns 64-byte signature or empty on error.
  std::vector<uint8_t> sign(const uint8_t *Msg,
                            std::size_t MsgLen) const noexcept;

  /// Verify `Sig` over `Msg` with this key.
  /// Returns true iff verification succeeds.
  bool verify(const uint8_t *Msg, std::size_t MsgLen, const uint8_t *Sig,
              std::size_t SigLen) const noexcept;

private:
  EvpPkeyPtr Key;
  bool CanSign = false;
};

/// LEB128 varuint32 encoding helpers used when building the custom section.
std::vector<uint8_t> encodeUleb128(uint32_t Value) noexcept;

} // namespace Driver
} // namespace WasmEdge
