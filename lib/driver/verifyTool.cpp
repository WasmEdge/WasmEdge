// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "driver/verifyTool.h"
#include "common/spdlog.h"
#include "driver/tool.h"
#include "signature_crypto.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

namespace {

/// Read entire file into a byte vector.
std::vector<uint8_t> readFileV(const std::string &Path) noexcept {
  std::FILE *F = std::fopen(Path.c_str(), "rb");
  if (!F) {
    spdlog::error("verify: cannot open file: {}"sv, Path);
    return {};
  }
  if (std::fseek(F, 0, SEEK_END) != 0) {
    std::fclose(F);
    return {};
  }
  const long Size = std::ftell(F);
  if (Size < 0) {
    std::fclose(F);
    return {};
  }
  std::rewind(F);
  std::vector<uint8_t> Buf(static_cast<std::size_t>(Size));
  if (Size > 0 && std::fread(Buf.data(), 1, static_cast<std::size_t>(Size),
                             F) != static_cast<std::size_t>(Size)) {
    spdlog::error("verify: read error on: {}"sv, Path);
    std::fclose(F);
    return {};
  }
  std::fclose(F);
  return Buf;
}

/// Decode a LEB128 unsigned 32-bit integer from Buf at Pos.
bool decodeUleb128V(const std::vector<uint8_t> &Buf, std::size_t &Pos,
                    uint32_t &Out) noexcept {
  uint32_t Result = 0;
  unsigned Shift = 0;
  while (Pos < Buf.size()) {
    const uint8_t Byte = Buf[Pos++];
    if (Shift >= 32) {
      return false;
    }
    Result |= static_cast<uint32_t>(Byte & 0x7Fu) << Shift;
    Shift += 7;
    if ((Byte & 0x80u) == 0) {
      Out = Result;
      return true;
    }
  }
  return false;
}

/// Parsed representation of a single signature record.
struct SigRecord {
  std::vector<uint8_t> KeyId;    // may be empty
  uint8_t SigAlgorithm = 0;      // 0x01 = Ed25519
  std::vector<uint8_t> SigBytes; // raw signature
};

/// Parsed representation of a signed_hashes block.
struct SignedHashBlock {
  std::vector<std::array<uint8_t, kSha256DigestLen>> Hashes;
  std::vector<SigRecord> Signatures;
};

/// Parsed signature section structure.
struct SignatureSection {
  uint8_t SpecVersion = 0;
  uint8_t ContentType = 0;
  uint8_t HashFn = 0;
  std::vector<SignedHashBlock> Blocks;
  /// Byte offset in the outer Wasm buffer where module content begins
  /// (i.e., immediately after the signature custom section ends).
  std::size_t ContentStart = 0;
};

/// Attempt to parse a `signature` custom section from `Wasm`.
/// On success returns true and fills `Out`; ContentStart is set to the first
/// byte of non-signature module content.
bool parseSignatureSection(const std::vector<uint8_t> &Wasm,
                           SignatureSection &Out) noexcept {
  if (Wasm.size() < 8) {
    return false;
  }
  std::size_t Pos = 8; // after magic+version
  if (Pos >= Wasm.size()) {
    return false;
  }
  // Must be a custom section (id=0)
  if (Wasm[Pos] != 0x00u) {
    spdlog::error("verify: first section is not a custom section."sv);
    return false;
  }
  Pos++;
  uint32_t ContentSize = 0;
  if (!decodeUleb128V(Wasm, Pos, ContentSize)) {
    return false;
  }
  const std::size_t ContentStart = Pos;
  const std::size_t ContentEnd = ContentStart + ContentSize;
  if (ContentEnd > Wasm.size()) {
    return false;
  }

  // Parse name
  uint32_t NameLen = 0;
  if (!decodeUleb128V(Wasm, Pos, NameLen)) {
    return false;
  }
  if (Pos + NameLen > ContentEnd) {
    return false;
  }
  std::string_view Name(reinterpret_cast<const char *>(Wasm.data() + Pos),
                        NameLen);
  if (Name != "signature"sv) {
    spdlog::error(
        "verify: leading custom section is named '{}', not 'signature'."sv,
        std::string(Name));
    return false;
  }
  Pos += NameLen;

  // Parse signature payload
  if (Pos >= ContentEnd) {
    return false;
  }
  Out.SpecVersion = Wasm[Pos++];
  if (Pos >= ContentEnd) {
    return false;
  }
  Out.ContentType = Wasm[Pos++];
  if (Pos >= ContentEnd) {
    return false;
  }
  Out.HashFn = Wasm[Pos++];

  if (Out.SpecVersion != 0x01u) {
    spdlog::error("verify: unsupported spec_version: 0x{:02x}."sv,
                  Out.SpecVersion);
    return false;
  }
  if (Out.ContentType != 0x01u) {
    spdlog::error("verify: unsupported content_type: 0x{:02x}."sv,
                  Out.ContentType);
    return false;
  }
  if (Out.HashFn != 0x01u) {
    spdlog::error("verify: unsupported hash_fn: 0x{:02x}. Only SHA-256 (0x01) "
                  "is supported."sv,
                  Out.HashFn);
    return false;
  }

  uint32_t BlockCount = 0;
  if (!decodeUleb128V(Wasm, Pos, BlockCount)) {
    return false;
  }

  for (uint32_t Bi = 0; Bi < BlockCount; ++Bi) {
    SignedHashBlock Block;

    uint32_t HashCount = 0;
    if (!decodeUleb128V(Wasm, Pos, HashCount)) {
      return false;
    }
    for (uint32_t Hi = 0; Hi < HashCount; ++Hi) {
      if (Pos + kSha256DigestLen > ContentEnd) {
        return false;
      }
      std::array<uint8_t, kSha256DigestLen> H;
      std::copy_n(Wasm.data() + Pos, kSha256DigestLen, H.data());
      Pos += kSha256DigestLen;
      Block.Hashes.push_back(H);
    }

    uint32_t SigCount = 0;
    if (!decodeUleb128V(Wasm, Pos, SigCount)) {
      return false;
    }
    for (uint32_t Si = 0; Si < SigCount; ++Si) {
      SigRecord Rec;
      uint32_t KeyIdLen = 0;
      if (!decodeUleb128V(Wasm, Pos, KeyIdLen)) {
        return false;
      }
      if (Pos + KeyIdLen > ContentEnd) {
        return false;
      }
      Rec.KeyId.assign(Wasm.data() + Pos, Wasm.data() + Pos + KeyIdLen);
      Pos += KeyIdLen;

      if (Pos >= ContentEnd) {
        return false;
      }
      Rec.SigAlgorithm = Wasm[Pos++];

      uint32_t SigLen = 0;
      if (!decodeUleb128V(Wasm, Pos, SigLen)) {
        return false;
      }
      if (Pos + SigLen > ContentEnd) {
        return false;
      }
      Rec.SigBytes.assign(Wasm.data() + Pos, Wasm.data() + Pos + SigLen);
      Pos += SigLen;

      Block.Signatures.push_back(std::move(Rec));
    }

    Out.Blocks.push_back(std::move(Block));
  }

  Out.ContentStart = ContentEnd;
  return true;
}

/// Build the canonical message that was signed:
///   "wasmsig" || spec_version || content_type || hash_fn || hashes
std::vector<uint8_t> buildVerifyMessage(
    uint8_t SpecVer, uint8_t ContentType, uint8_t HashFn,
    const std::vector<std::array<uint8_t, kSha256DigestLen>> &Hashes) noexcept {
  std::vector<uint8_t> Msg;
  static constexpr std::string_view kPrefix = "wasmsig"sv;
  Msg.insert(Msg.end(), reinterpret_cast<const uint8_t *>(kPrefix.data()),
             reinterpret_cast<const uint8_t *>(kPrefix.data()) +
                 kPrefix.size());
  Msg.push_back(SpecVer);
  Msg.push_back(ContentType);
  Msg.push_back(HashFn);
  for (const auto &H : Hashes) {
    Msg.insert(Msg.end(), H.begin(), H.end());
  }
  return Msg;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// VerifyTool — public entry point
// ---------------------------------------------------------------------------

int VerifyTool(struct DriverToolOptions &Opt) noexcept {
  // --- Validate options ---
  const std::string &WasmPath = Opt.SoName.value();
  if (WasmPath.empty()) {
    spdlog::error("verify: no input file specified. Usage: wasmedge verify "
                  "<wasm> [--key <pubkey>]"sv);
    return EXIT_FAILURE;
  }

  // --- Load Wasm ---
  const auto Wasm = readFileV(WasmPath);
  if (Wasm.empty()) {
    return EXIT_FAILURE;
  }

  static constexpr std::array<uint8_t, 8> kWasmHeader = {
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00};
  if (Wasm.size() < 8 ||
      !std::equal(kWasmHeader.begin(), kWasmHeader.end(), Wasm.begin())) {
    spdlog::error("verify: not a valid WebAssembly binary: {}"sv, WasmPath);
    return EXIT_FAILURE;
  }

  // --- Parse signature section ---
  SignatureSection SigSec;
  if (!parseSignatureSection(Wasm, SigSec)) {
    spdlog::error("verify: no valid signature section found in: {}"sv,
                  WasmPath);
    return EXIT_FAILURE;
  }

  if (SigSec.Blocks.empty()) {
    spdlog::error(
        "verify: signature section contains no signed_hashes blocks."sv);
    return EXIT_FAILURE;
  }

  // --- Optionally load public key for filtering ---
  std::unique_ptr<WasmSigKey> PubKey;
  const std::string &KeyPath = Opt.KeyFile.value();
  if (!KeyPath.empty()) {
    const auto KeyBlob = readFileV(KeyPath);
    if (KeyBlob.empty()) {
      return EXIT_FAILURE;
    }
    PubKey = std::make_unique<WasmSigKey>();
    if (!PubKey->loadFromBytes(KeyBlob)) {
      spdlog::error("verify: failed to load public key from: {}. "
                    "Expected format: 0x01 | public(32) = 33 bytes, or "
                    "0x81 | secret(32) | public(32) = 65 bytes."sv,
                    KeyPath);
      return EXIT_FAILURE;
    }
  }

  // --- Recompute rolling hash over content after the signature section ---
  // Phase 1: one part, the hash covers everything after the sig section.
  // This matches what signTool.cpp committed: h1 = SHA-256(bytes after header).
  //
  // However, the signed module's "bytes after header" now includes the
  // signature section itself prepended.  When verifying, we must hash
  // everything from ContentStart to end of file (i.e. the original payload).
  const std::size_t ContentOff = SigSec.ContentStart;
  if (ContentOff > Wasm.size()) {
    spdlog::error("verify: malformed signature section offsets."sv);
    return EXIT_FAILURE;
  }

  RollingHasher Hasher;
  if (Wasm.size() > ContentOff) {
    if (!Hasher.update(Wasm.data() + ContentOff, Wasm.size() - ContentOff)) {
      spdlog::error("verify: hash computation failed."sv);
      return EXIT_FAILURE;
    }
  }
  const auto ComputedHash = Hasher.digest();

  // --- Verify each block ---
  // For Phase 1, expect exactly one block with exactly one hash.
  bool AnyBlockVerified = false;
  for (const auto &Block : SigSec.Blocks) {
    if (Block.Hashes.size() != 1) {
      spdlog::warn(
          "verify: block has {} hashes; Phase 1 expects 1. Skipping."sv,
          Block.Hashes.size());
      continue;
    }

    // Check that the embedded hash matches the recomputed hash.
    if (Block.Hashes[0] != ComputedHash) {
      spdlog::error(
          "verify: content hash mismatch — module has been modified."sv);
      return EXIT_FAILURE;
    }

    // Build the message that was originally signed.
    const auto Msg = buildVerifyMessage(SigSec.SpecVersion, SigSec.ContentType,
                                        SigSec.HashFn, Block.Hashes);

    // Try to verify at least one signature in the block.
    bool BlockVerified = false;
    for (const auto &Rec : Block.Signatures) {
      if (Rec.SigAlgorithm != 0x01u) {
        spdlog::warn(
            "verify: unknown signature algorithm 0x{:02x}, skipping."sv,
            Rec.SigAlgorithm);
        continue;
      }

      if (PubKey) {
        // Verify against the user-supplied key.
        if (PubKey->verify(Msg.data(), Msg.size(), Rec.SigBytes.data(),
                           Rec.SigBytes.size())) {
          BlockVerified = true;
          break;
        }
      } else {
        // No user-supplied key: we need to recover the public key from the
        // key_id embedded in the signature record.  When key_id_len=0 (Phase
        // 1), there is no embedded public key, so verification without a
        // supplied key file is not possible.
        if (Rec.KeyId.empty()) {
          spdlog::error("verify: signature record carries no key identifier. "
                        "Provide --key <pubkey> to verify."sv);
          return EXIT_FAILURE;
        }
        // Future: parse key_id as a serialised public key.
        spdlog::warn(
            "verify: key_id present but key loading from key_id is not "
            "yet implemented. Provide --key <pubkey> to verify."sv);
        continue;
      }
    }

    if (!BlockVerified) {
      spdlog::error("verify: no valid signature found in block."sv);
      return EXIT_FAILURE;
    }
    AnyBlockVerified = true;
  }

  if (!AnyBlockVerified) {
    spdlog::error(
        "verify: signature verification failed — no block verified."sv);
    return EXIT_FAILURE;
  }

  spdlog::info("verify: signature verified OK: {}"sv, WasmPath);
  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
