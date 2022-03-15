#include "host/wasi_crypto/signatures/signstate.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

WasiCryptoExpect<SignStateVariant> sigStateOpen(KpVariant &KpVariant) noexcept {
  return std::visit(
      [](auto &&Kp) noexcept {
        return Kp.openSignState().map([](auto &&SignState) noexcept {
          return SignStateVariant{SignState};
        });
      },
      KpVariant);
}

WasiCryptoExpect<void> sigStateUpdate(SignStateVariant &SignStateVariant,
                                      Span<const uint8_t> Input) noexcept {
  return std::visit(
      [=](auto &&SignState) noexcept { return SignState.update(Input); },
      SignStateVariant);
}

WasiCryptoExpect<SigVariant>
sigStateSign(SignStateVariant &SignStateVariant) noexcept {
  return std::visit(
      [](auto &&SignState) noexcept {
        return SignState.sign().map(
            [](auto &&Sig) noexcept { return SigVariant{Sig}; });
      },
      SignStateVariant);
}

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge