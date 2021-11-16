// SPDX-License-Identifier: Apache-2.0
#include "signature/sig_algorithm.h"

namespace WasmEdge {
namespace Signature {

const std::vector<Byte>
SigAlgorithm::keygen(Span<const Byte> Code, const std::filesystem::path &Path) {
  unsigned char PublicKey[32], PrivateKey[64], Seed[32];
  unsigned char Signature[64];
  const int MessageLen = std::size(Code);
  const unsigned char *Message = &Code[0];

  /* create a random seed, and a keypair out of that seed */
  ed25519_create_seed(Seed);
  ed25519_create_keypair(PublicKey, PrivateKey, Seed);

  /* Save public key and private key */
  std::ofstream PubKeyFile(Path / "./id_ed25519.pub",
                           std::ios::binary | std::ios::ate);
  try {
    PubKeyFile.exceptions(PubKeyFile.failbit);
  } catch (const std::ios_base::failure &e) {
    // Failure handling
  }
  PubKeyFile.write(reinterpret_cast<const char *>(&PublicKey), 32);

  std::ofstream PriKeyFile(Path / "./id_ed25519",
                           std::ios::binary | std::ios::ate);
  try {
    PriKeyFile.exceptions(PriKeyFile.failbit);
  } catch (const std::ios_base::failure &e) {
    // Failure handling
  }
  PriKeyFile.write(reinterpret_cast<const char *>(&PrivateKey), 32);

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