// SPDX-License-Identifier: Apache-2.0
#include "signature/sig_algorithm.h"

namespace WasmEdge {
namespace Signature {

std::vector<Byte> Sig_algorithm::sign(std::vector<Byte> data) {
  unsigned char public_key[32], private_key[64], seed[32];
  unsigned char signature[64];
  // const unsigned char message[] = "Hello, world!";
  const int message_len = 12;
  const unsigned char *message = &data[0];
  auto sign_sec_raw = FileMgr();
  
  /* create a random seed, and a keypair out of that seed */
  ed25519_create_seed(seed);
  ed25519_create_keypair(public_key, private_key, seed);

  /* sign bytes */
  ed25519_sign(signature, message, message_len, public_key, private_key);
  std::vector<Byte> vec(signature, signature + 64);
  sign_sec_raw.setCode(vec);
  return vec;
}

int Sig_algorithm::verify() { return 0; }

} // namespace Signature
} // namespace WasmEdge