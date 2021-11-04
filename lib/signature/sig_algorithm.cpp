// SPDX-License-Identifier: Apache-2.0
#include "signature/sig_algorithm.h"

namespace WasmEdge {
namespace Signature {

std::vector<Byte> SigAlgorithm::keygen(Span<const Byte> Code) {
  unsigned char PublicKey[32], PrivateKey[64], Seed[32];
  unsigned char Signature[64];
  const int MessageLen = std::size(Code);
  const unsigned char *Message = &Code[0];

  /* create a random seed, and a keypair out of that seed */
  ed25519_create_seed(Seed);
  ed25519_create_keypair(PublicKey, PrivateKey, Seed);

  /* sign bytes */
  ed25519_sign(Signature, Message, MessageLen, PublicKey, PrivateKey);
  std::vector<Byte> Sig(Signature, Signature + 64);
  return Sig;
}

int SigAlgorithm::verify(Span<const Byte> Code, Span<const Byte> Signature,
                         Span<const Byte> PublicKey) {
  const unsigned char *PublicKeyArray = &PublicKey[0];
  const unsigned char *SignatureArray = &Signature[0];
  const unsigned char *Message = &Code[0];
  const int MessageLen = std::size(Code);

  ed25519_verify(SignatureArray, Message, MessageLen, PublicKeyArray);
  return 0;
}

} // namespace Signature
} // namespace WasmEdge