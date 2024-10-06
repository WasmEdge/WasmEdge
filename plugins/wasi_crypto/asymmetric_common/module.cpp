// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "asymmetric_common/module.h"
#include "asymmetric_common/func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiCryptoAsymmetricCommonModule::WasiCryptoAsymmetricCommonModule(
    std::shared_ptr<WasiCrypto::Context> C)
    : ModuleInstance("wasi_ephemeral_crypto_asymmetric_common"), Ctx(C) {
  using namespace WasiCrypto;

  addHostFunc("keypair_generate",
              std::make_unique<AsymmetricCommon::KeypairGenerate>(*Ctx));
  addHostFunc("keypair_import",
              std::make_unique<AsymmetricCommon::KeypairImport>(*Ctx));
  addHostFunc("keypair_generate_managed",
              std::make_unique<AsymmetricCommon::KeypairGenerateManaged>(*Ctx));
  addHostFunc("keypair_store_managed",
              std::make_unique<AsymmetricCommon::KeypairStoreManaged>(*Ctx));
  addHostFunc("keypair_replace_managed",
              std::make_unique<AsymmetricCommon::KeypairReplaceManaged>(*Ctx));
  addHostFunc("keypair_id",
              std::make_unique<AsymmetricCommon::KeypairId>(*Ctx));
  addHostFunc("keypair_from_id",
              std::make_unique<AsymmetricCommon::KeypairFromId>(*Ctx));
  addHostFunc("keypair_from_pk_and_sk",
              std::make_unique<AsymmetricCommon::KeypairFromPkAndSk>(*Ctx));
  addHostFunc("keypair_export",
              std::make_unique<AsymmetricCommon::KeypairExport>(*Ctx));
  addHostFunc("keypair_publickey",
              std::make_unique<AsymmetricCommon::KeypairPublickey>(*Ctx));
  addHostFunc("keypair_secretkey",
              std::make_unique<AsymmetricCommon::KeypairSecretkey>(*Ctx));
  addHostFunc("keypair_close",
              std::make_unique<AsymmetricCommon::KeypairClose>(*Ctx));
  addHostFunc("publickey_import",
              std::make_unique<AsymmetricCommon::PublickeyImport>(*Ctx));
  addHostFunc("publickey_export",
              std::make_unique<AsymmetricCommon::PublickeyExport>(*Ctx));
  addHostFunc("publickey_verify",
              std::make_unique<AsymmetricCommon::PublickeyVerify>(*Ctx));
  addHostFunc("publickey_from_secretkey",
              std::make_unique<AsymmetricCommon::PublickeyFromSecretkey>(*Ctx));
  addHostFunc("publickey_close",
              std::make_unique<AsymmetricCommon::PublickeyClose>(*Ctx));
  addHostFunc("secretkey_import",
              std::make_unique<AsymmetricCommon::SecretkeyImport>(*Ctx));
  addHostFunc("secretkey_export",
              std::make_unique<AsymmetricCommon::SecretkeyExport>(*Ctx));
  addHostFunc("secretkey_close",
              std::make_unique<AsymmetricCommon::SecretkeyClose>(*Ctx));
}

} // namespace Host
} // namespace WasmEdge
