// SPDX-License-Identifier: Apache-2.0
#include "signature/signature.h"

namespace WasmEdge {
namespace Signature {

Expect<void> Signature::sign(const AST::Module &Mod, Span<Byte> signature) {
  auto cs = Mod.getCustomSections();
  auto sign_sec_raw = FileMgr();
  sign_sec_raw.setCode(signature);
  AST::CustomSection sign_sec;
  auto empty_conf = Configure();
  sign_sec.loadBinary(sign_sec_raw, empty_conf);
  cs.push_back(sign_sec);
  return {};
}

Expect<void> Signature::verify(const AST::Module &Mod) {
  auto cs = Mod.getCustomSections();
  return {};
}

// Expect<Span<Byte>> keygen(Span<const uint8_t> Code) {
int keygen(Span<const uint8_t> Code) {
  if (Code.size()) return 0;
  return 1;
}

} // namespace Signature
} // namespace WasmEdge