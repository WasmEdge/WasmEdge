// SPDX-License-Identifier: Apache-2.0
#include "signature/sig_algorithm.h"

namespace WasmEdge {
namespace Signature {

std::vector<Byte> SigAlgorithm::keygen(Span<const Byte> Code) {
  unsigned char PublicKey[32], PrivateKey[64], Seed[32];
  unsigned char Signature[64];
  // const unsigned char message[] = "Hello, world!";
  const int MessageLen = 12;
  const unsigned char *Message = &Code[0];

  /* create a random seed, and a keypair out of that seed */
  ed25519_create_seed(Seed);
  ed25519_create_keypair(PublicKey, PrivateKey, Seed);

  /* sign bytes */
  ed25519_sign(Signature, Message, MessageLen, PublicKey, PrivateKey);
  std::vector<Byte> Sig(Signature, Signature + 64);
  return Sig;
}

int SigAlgorithm::verify() { return 0; }

} // namespace Signature
} // namespace WasmEdge