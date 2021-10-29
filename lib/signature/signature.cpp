// SPDX-License-Identifier: Apache-2.0
#include "signature/signature.h"

namespace WasmEdge {
namespace Signature {

Expect<void> Signature::sign(std::filesystem::path Path,
                             const std::vector<uint8_t> Signature) {
  std::ofstream File(Path.string(), std::ios::binary | std::ios::ate);
  try {
    File.exceptions(File.failbit);
  } catch (const std::ios_base::failure &e) {
    // Failure handling
  }
  uint8_t SectionId = 0;
  uint8_t SectionLen = Signature.size();
  uint8_t NameLen = sizeof DEFAULT_CUSOTM_SECTION_NAME;
  File.write(reinterpret_cast<const char *>(&SectionId), 1);
  File.write(reinterpret_cast<const char *>(&SectionLen), 1);
  File.write(reinterpret_cast<const char *>(&NameLen), 1);
  File.write(reinterpret_cast<const char *>(&DEFAULT_CUSOTM_SECTION_NAME),
             NameLen);
  File.write(reinterpret_cast<const char *>(&Signature[0]), SectionLen);
  return {};
}

Expect<void> Signature::verify(const AST::Module &Mod) {
  /// Return void for this template
  for (const auto &Elem : Mod.getCustomSections()) {
    if (Elem.getName() == DEFAULT_CUSOTM_SECTION_NAME)
      return {};
  }
  return {};
}

// Expect<Span<Byte>> keygen(Span<const uint8_t> Code) {
Expect<const std::vector<unsigned char>>
Signature::keygen(Span<const uint8_t> Code) {
  return Alg.keygen(Code);
}

} // namespace Signature
} // namespace WasmEdge