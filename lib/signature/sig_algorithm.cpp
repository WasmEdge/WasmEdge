// SPDX-License-Identifier: Apache-2.0
#include "signature/sig_algorithm.h"
#include "api/wasmedge/enum_errcode.h"
#include "common/errcode.h"
#include "spdlog/spdlog.h"
#include <cstring>

namespace WasmEdge {
namespace Signature {

Expect<const std::vector<Byte>> SigAlgorithm::keygen(Span<const Byte> Code,
                                                     const fs::path &Path) {
  unsigned char PublicKey[32], PrivateKey[64], Seed[32], Signature[64];

  const int MessageLen = std::size(Code);
  const unsigned char *Message = &Code[0];
  const fs::path PubKeyPath = Path / "./id_ed25519.pub";
  const fs::path PriKeyPath = Path / "./id_ed25519";

  ed25519_create_seed(Seed);
  ed25519_create_keypair(PublicKey, PrivateKey, Seed);
  // if (Prikey.empty() && Pubkey.empty()) {
  //   /* create a random seed, and a keypair out of that seed */
  // ed25519_create_seed(Seed);
  // ed25519_create_keypair(PublicKey, PrivateKey, Seed);
  // }

  // else if (!Prikey.empty() && !Pubkey.empty()) {
  //   &PublicKey[0] = reinterpret_cast<const unsigned char *>(Pubkey.data());
  //   PublicKey = PublicKey.data();
  // }

  /* Save public key and private key */
  std::ofstream PubKeyFile(PubKeyPath, std::ios::binary | std::ios::ate);
  try {
    PubKeyFile.exceptions(PubKeyFile.failbit);
  } catch (const std::ios_base::failure &_) {
    spdlog::error("File open error");
    return Unexpect(ErrCode::IllegalPath);
  }
  PubKeyFile.write(reinterpret_cast<const char *>(&PublicKey), 32);

  std::ofstream PriKeyFile(PriKeyPath, std::ios::binary | std::ios::ate);
  try {
    PriKeyFile.exceptions(PriKeyFile.failbit);
  } catch (const std::ios_base::failure &_) {
    spdlog::error("File open error");
    return Unexpect(ErrCode::IllegalPath);
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