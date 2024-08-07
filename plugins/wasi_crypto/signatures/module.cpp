// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "asymmetric_common/func.h"
#include "common/func.h"
#include "kx/func.h"
#include "signatures/func.h"
#include "symmetric/func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiCryptoSignaturesModule::WasiCryptoSignaturesModule(
    std::shared_ptr<WasiCrypto::Context> C)
    : ModuleInstance("wasi_ephemeral_crypto_signatures"), Ctx(C) {
  using namespace WasiCrypto;

  addHostFunc("signature_export", std::make_unique<Signatures::Export>(*Ctx));
  addHostFunc("signature_import", std::make_unique<Signatures::Import>(*Ctx));
  addHostFunc("signature_state_open",
              std::make_unique<Signatures::StateOpen>(*Ctx));
  addHostFunc("signature_state_update",
              std::make_unique<Signatures::StateUpdate>(*Ctx));
  addHostFunc("signature_state_sign",
              std::make_unique<Signatures::StateSign>(*Ctx));
  addHostFunc("signature_state_close",
              std::make_unique<Signatures::StateClose>(*Ctx));
  addHostFunc("signature_verification_state_open",
              std::make_unique<Signatures::VerificationStateOpen>(*Ctx));
  addHostFunc("signature_verification_state_update",
              std::make_unique<Signatures::VerificationStateUpdate>(*Ctx));
  addHostFunc("signature_verification_state_verify",
              std::make_unique<Signatures::VerificationStateVerify>(*Ctx));
  addHostFunc("signature_verification_state_close",
              std::make_unique<Signatures::VerificationStateClose>(*Ctx));
  addHostFunc("signature_close", std::make_unique<Signatures::Close>(*Ctx));
}

} // namespace Host
} // namespace WasmEdge
