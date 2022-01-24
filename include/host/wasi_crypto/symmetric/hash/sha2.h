// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/hash/hash_state.h"
#include "host/wasi_crypto/symmetric/options.h"

#include "host/wasi_crypto/evpwrapper.h"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <uint32_t Sha> class Sha2State final : public HashState {
public:
  Sha2State(std::shared_ptr<Options> OptOption, EvpMdCtxPtr Ctx)
      : OptOption(OptOption), Ctx(std::move(Ctx)) {}

  static WasiCryptoExpect<std::unique_ptr<Sha2State>>
  open(std::shared_ptr<Key> OptKey, std::shared_ptr<Options> OptOption);

  WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) override;

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) override;

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data) override;

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override;

private:
  std::shared_ptr<Options> OptOption;
  EvpMdCtxPtr Ctx;
};

using Sha256State = Sha2State<256>;
using Sha512State = Sha2State<512>;
using Sha512_256State = Sha2State<512256>;

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
