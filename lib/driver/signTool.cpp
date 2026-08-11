// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "driver/signTool.h"
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

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Read the entire contents of a binary file.
std::vector<uint8_t> readFile(const std::string &Path) noexcept {
  std::FILE *F = std::fopen(Path.c_str(), "rb");
  if (!F) {
    spdlog::error("sign: cannot open file: {}"sv, Path);
    return {};
  }
  if (std::fseek(F, 0, SEEK_END) != 0) {
    spdlog::error("sign: fseek failed on: {}"sv, Path);
    std::fclose(F);
    return {};
  }
  const long Size = std::ftell(F);
  if (Size < 0) {
    spdlog::error("sign: ftell failed on: {}"sv, Path);
    std::fclose(F);
    return {};
  }
  std::rewind(F);
  std::vector<uint8_t> Buf(static_cast<std::size_t>(Size));
  if (Size > 0 && std::fread(Buf.data(), 1, static_cast<std::size_t>(Size),
                             F) != static_cast<std::size_t>(Size)) {
    spdlog::error("sign: read error on: {}"sv, Path);
    std::fclose(F);
    return {};
  }
  std::fclose(F);
  return Buf;
}

/// Write a buffer to a file (atomic-ish via rename if possible, otherwise
/// direct).
bool writeFile(const std::string &Path,
               const std::vector<uint8_t> &Data) noexcept {
  std::FILE *F = std::fopen(Path.c_str(), "wb");
  if (!F) {
    spdlog::error("sign: cannot open output file: {}"sv, Path);
    return false;
  }
  bool Ok = std::fwrite(Data.data(), 1, Data.size(), F) == Data.size();
  std::fclose(F);
  if (!Ok) {
    spdlog::error("sign: write error on: {}"sv, Path);
  }
  return Ok;
}

/// Decode a LEB128 unsigned 32-bit integer from Buf starting at Pos.
/// Advances Pos past the encoded bytes.  Returns false on overflow/truncation.
bool decodeUleb128(const std::vector<uint8_t> &Buf, std::size_t &Pos,
                   uint32_t &Out) noexcept {
  uint32_t Result = 0;
  unsigned Shift = 0;
  while (Pos < Buf.size()) {
    const uint8_t Byte = Buf[Pos++];
    if (Shift >= 32) {
      return false; // overflow
    }
    Result |= static_cast<uint32_t>(Byte & 0x7Fu) << Shift;
    Shift += 7;
    if ((Byte & 0x80u) == 0) {
      Out = Result;
      return true;
    }
  }
  return false; // truncated
}

/// Encode a string-vector name as a name-section name (LEB128 len + bytes).
std::vector<uint8_t> buildNameBytes(std::string_view Name) noexcept {
  std::vector<uint8_t> Out;
  auto Leb = encodeUleb128(static_cast<uint32_t>(Name.size()));
  Out.insert(Out.end(), Leb.begin(), Leb.end());
  Out.insert(Out.end(), reinterpret_cast<const uint8_t *>(Name.data()),
             reinterpret_cast<const uint8_t *>(Name.data()) + Name.size());
  return Out;
}

/// Build a Wasm custom section byte sequence:
///   section_id(0x00) | section_size(uleb128) | name_len(uleb128) | name |
///   payload
std::vector<uint8_t>
buildCustomSection(std::string_view Name,
                   const std::vector<uint8_t> &Payload) noexcept {
  auto NameBytes = buildNameBytes(Name);
  // Content = name_bytes + payload
  std::vector<uint8_t> Content;
  Content.insert(Content.end(), NameBytes.begin(), NameBytes.end());
  Content.insert(Content.end(), Payload.begin(), Payload.end());

  auto ContentSizeBytes = encodeUleb128(static_cast<uint32_t>(Content.size()));

  std::vector<uint8_t> Section;
  Section.push_back(0x00u); // custom section id
  Section.insert(Section.end(), ContentSizeBytes.begin(),
                 ContentSizeBytes.end());
  Section.insert(Section.end(), Content.begin(), Content.end());
  return Section;
}

/// Parse the first section of `Wasm` (starting at offset 8, just after the
/// 4-byte magic + 4-byte version), and return true if it is a custom section
/// named "signature".  `SigSectionEnd` is set to the byte offset immediately
/// past that section.
bool hasLeadingSignatureSection(const std::vector<uint8_t> &Wasm,
                                std::size_t &SigSectionEnd) noexcept {
  if (Wasm.size() < 8) {
    return false;
  }
  std::size_t Pos = 8;
  if (Pos >= Wasm.size()) {
    return false;
  }
  if (Wasm[Pos] != 0x00u) {
    return false; // not a custom section
  }
  Pos++;
  uint32_t ContentSize = 0;
  if (!decodeUleb128(Wasm, Pos, ContentSize)) {
    return false;
  }
  const std::size_t ContentStart = Pos;
  if (ContentStart + ContentSize > Wasm.size()) {
    return false;
  }
  // Read name length
  uint32_t NameLen = 0;
  if (!decodeUleb128(Wasm, Pos, NameLen)) {
    return false;
  }
  if (Pos + NameLen > ContentStart + ContentSize) {
    return false;
  }
  std::string_view SectionName(
      reinterpret_cast<const char *>(Wasm.data() + Pos), NameLen);
  if (SectionName != "signature"sv) {
    return false;
  }
  SigSectionEnd = ContentStart + ContentSize;
  return true;
}

/// Build the `signature` custom section payload (the raw signed data).
/// Phase 1: one part, one hash, one signature, no key_id.
///
/// Payload layout (per Signatures.md):
///   0x01           spec_version
///   0x01           content_type (Wasm module)
///   0x01           hash_fn (SHA-256)
///   varuint32      signed_hashes_count = 1
///   --- signed_hashes block ---
///   varuint32      hashes_count = 1
///   32 bytes       h1 = SHA-256(all non-sig content)
///   varuint32      signatures_count = 1
///   --- signature record ---
///   varuint32      key_id_len = 0   (no key id for Phase 1)
///   0x01           signature_id (Ed25519)
///   varuint32      signature_len = 64
///   64 bytes       Ed25519 signature over sig_msg
///
/// sig_msg = "wasmsig" | 0x01 | 0x01 | 0x01 | hashes
std::vector<uint8_t>
buildSignaturePayload(const std::array<uint8_t, kSha256DigestLen> &Hash,
                      const std::vector<uint8_t> &Sig) noexcept {
  // Build the hashes blob (single hash for Phase 1)
  std::vector<uint8_t> Hashes(Hash.begin(), Hash.end());

  std::vector<uint8_t> Payload;
  // Header bytes
  Payload.push_back(0x01u); // spec_version
  Payload.push_back(0x01u); // content_type
  Payload.push_back(0x01u); // hash_fn (SHA-256)
  // signed_hashes_count = 1
  for (auto B : encodeUleb128(1u)) {
    Payload.push_back(B);
  }
  // --- signed_hashes ---
  // hashes_count = 1
  for (auto B : encodeUleb128(1u)) {
    Payload.push_back(B);
  }
  // hashes (32 bytes)
  Payload.insert(Payload.end(), Hashes.begin(), Hashes.end());
  // signatures_count = 1
  for (auto B : encodeUleb128(1u)) {
    Payload.push_back(B);
  }
  // --- signature record ---
  // key_id_len = 0
  for (auto B : encodeUleb128(0u)) {
    Payload.push_back(B);
  }
  // signature_id = 0x01 (Ed25519)
  Payload.push_back(0x01u);
  // signature_len
  for (auto B : encodeUleb128(static_cast<uint32_t>(Sig.size()))) {
    Payload.push_back(B);
  }
  // signature bytes
  Payload.insert(Payload.end(), Sig.begin(), Sig.end());
  return Payload;
}

/// Build the message that is actually signed:
///   "wasmsig" || spec_version(0x01) || content_type(0x01) || hash_fn(0x01)
///   || hashes
std::vector<uint8_t>
buildSigMessage(const std::array<uint8_t, kSha256DigestLen> &Hash) noexcept {
  std::vector<uint8_t> Msg;
  static constexpr std::string_view kPrefix = "wasmsig"sv;
  Msg.insert(Msg.end(), reinterpret_cast<const uint8_t *>(kPrefix.data()),
             reinterpret_cast<const uint8_t *>(kPrefix.data()) +
                 kPrefix.size());
  Msg.push_back(0x01u); // spec_version
  Msg.push_back(0x01u); // content_type
  Msg.push_back(0x01u); // hash_fn
  // Append hashes
  Msg.insert(Msg.end(), Hash.begin(), Hash.end());
  return Msg;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// SignTool — public entry point
// ---------------------------------------------------------------------------

int SignTool(struct DriverToolOptions &Opt) noexcept {
  // --- Validate options ---
  const std::string &WasmPath = Opt.SoName.value();
  if (WasmPath.empty()) {
    spdlog::error("sign: no input file specified. Usage: wasmedge sign "
                  "<wasm> --key <keyfile> [--output <out.wasm>]"sv);
    return EXIT_FAILURE;
  }

  const std::string &KeyPath = Opt.KeyFile.value();
  if (KeyPath.empty()) {
    spdlog::error(
        "sign: --key is required. Provide an Ed25519 keypair file."sv);
    return EXIT_FAILURE;
  }

  // Derive output path
  std::string OutPath = Opt.OutputFile.value();
  if (OutPath.empty()) {
    // Default: <input>.signed.wasm
    OutPath = WasmPath + ".signed.wasm"s;
  }

  // --- Load input Wasm ---
  const auto Wasm = readFile(WasmPath);
  if (Wasm.empty()) {
    return EXIT_FAILURE;
  }

  // Validate Wasm magic + version
  static constexpr std::array<uint8_t, 8> kWasmHeader = {
      0x00, 0x61, 0x73, 0x6D, // \0asm
      0x01, 0x00, 0x00, 0x00  // version 1
  };
  if (Wasm.size() < 8 ||
      !std::equal(kWasmHeader.begin(), kWasmHeader.end(), Wasm.begin())) {
    spdlog::error("sign: not a valid WebAssembly binary: {}"sv, WasmPath);
    return EXIT_FAILURE;
  }

  // Reject already-signed modules to keep Phase 1 simple (idempotency guard).
  std::size_t ExistingSigEnd = 0;
  if (hasLeadingSignatureSection(Wasm, ExistingSigEnd)) {
    spdlog::error("sign: module already contains a signature section. "
                  "Re-signing is not supported in Phase 1."sv);
    return EXIT_FAILURE;
  }

  // --- Load keypair ---
  const auto KeyBlob = readFile(KeyPath);
  if (KeyBlob.empty()) {
    return EXIT_FAILURE;
  }
  WasmSigKey Key;
  if (!Key.loadFromBytes(KeyBlob)) {
    spdlog::error(
        "sign: failed to load key from: {}. "
        "Expected format: 0x81 | secret(32) | public(32) = 65 bytes."sv,
        KeyPath);
    return EXIT_FAILURE;
  }
  if (!Key.canSign()) {
    spdlog::error("sign: key file contains a public key only. Signing requires "
                  "a keypair (65-byte file, first byte 0x81)."sv);
    return EXIT_FAILURE;
  }

  // --- Compute rolling hash over the payload bytes ---
  // Per Signatures.md Phase 1 (no delimiters):
  //   h1 = SHA-256(p1 || d1) where p1 is all sections and d1 is the
  //   trailing delimiter.
  //
  // For Phase 1 with a single part and no delimiter, we compute:
  //   h1 = SHA-256(all bytes after the 8-byte Wasm header)
  //
  // This matches the spec example: "SHA-256(sections 1..end)".
  RollingHasher Hasher;
  // Feed everything after the magic+version header.
  if (!Hasher.update(Wasm.data() + 8, Wasm.size() - 8)) {
    spdlog::error("sign: hash computation failed."sv);
    return EXIT_FAILURE;
  }
  const auto Hash = Hasher.digest();

  // --- Build and sign the message ---
  const auto Msg = buildSigMessage(Hash);
  const auto Sig = Key.sign(Msg.data(), Msg.size());
  if (Sig.size() != kEd25519SigLen) {
    spdlog::error("sign: signing failed."sv);
    return EXIT_FAILURE;
  }

  // --- Build the signature custom section ---
  const auto Payload = buildSignaturePayload(Hash, Sig);
  const auto SigSection = buildCustomSection("signature"sv, Payload);

  // --- Assemble signed module: header | sigSection | rest ---
  std::vector<uint8_t> SignedWasm;
  SignedWasm.reserve(Wasm.size() + SigSection.size());
  // 8-byte Wasm header
  SignedWasm.insert(SignedWasm.end(), Wasm.begin(), Wasm.begin() + 8);
  // Prepend signature section (first custom section after header)
  SignedWasm.insert(SignedWasm.end(), SigSection.begin(), SigSection.end());
  // Remaining sections unchanged
  SignedWasm.insert(SignedWasm.end(), Wasm.begin() + 8, Wasm.end());

  // --- Write output ---
  if (!writeFile(OutPath, SignedWasm)) {
    return EXIT_FAILURE;
  }

  spdlog::info("sign: signed module written to: {}"sv, OutPath);
  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
