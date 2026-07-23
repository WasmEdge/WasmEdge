#include "common/secrets_manager.h"
#include "helper.h"

using namespace WasmEdge::Host::WasiCrypto;

TEST(SecretsManagerTest, InvalidateAndCheck) {
  auto Ctx = Context::getInstance();

  __wasi_opt_options_t opt;
  opt.tag = __WASI_OPT_OPTIONS_U_NONE;
  auto smRes = Ctx->secretsManagerOpen(opt);
  ASSERT_TRUE(smRes);
  __wasi_secrets_manager_t smHandle = *smRes;

  std::vector<uint8_t> keyId = {'k', 'e', 'y', '_', '1'};
  __wasi_version_t version = 1;
  auto invRes = Ctx->secretsManagerInvalidate(smHandle, keyId, version);
  ASSERT_TRUE(invRes);

  // TODO: verify that the internal state reflects this (test via
  // the dependent *_from_id functions once implemented)

  auto closeRes = Ctx->secretsManagerClose(smHandle);
  ASSERT_TRUE(closeRes);

  // verify closing an already closed handle returns CLOSED
  auto closeAgainRes = Ctx->secretsManagerClose(smHandle);
  ASSERT_FALSE(closeAgainRes);
  ASSERT_EQ(closeAgainRes.error(), __WASI_CRYPTO_ERRNO_CLOSED);
}