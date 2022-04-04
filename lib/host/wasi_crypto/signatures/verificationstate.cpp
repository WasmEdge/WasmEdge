// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_crypto/signatures/verificationstate.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

WasiCryptoExpect<VerificationStateVariant>
verificationStateOpen(const PkVariant &PkVariant) noexcept {
  return std::visit(
      [](const auto &Pk) noexcept {
        return Pk.openVerificationState().map(
            [](auto &&VerificationState) noexcept {
              return VerificationStateVariant{std::move(VerificationState)};
            });
      },
      PkVariant);
}

WasiCryptoExpect<void>
verificationStateUpdate(VerificationStateVariant &VerificationStateVariant,
                        Span<const uint8_t> Input) noexcept {
  return std::visit(
      [Input](auto &VerificationState) noexcept {
        return VerificationState.update(Input);
      },
      VerificationStateVariant);
}

namespace {
template <class P> struct VerifySigType;
template <class VerificationStateType, class SigType>
struct VerifySigType<WasiCryptoExpect<void> (VerificationStateType::*)(
    const SigType &) noexcept> {
  using Sig = SigType;
};

} // namespace

WasiCryptoExpect<void>
verificationStateVerify(VerificationStateVariant &VerificationStateVariant,
                        const SigVariant &SigVariant) noexcept {
  return std::visit(
      [](auto &VerificationState,
         const auto &Sig) noexcept -> WasiCryptoExpect<void> {
        using RequiredSigType = typename VerifySigType<
            decltype(&std::decay_t<decltype(VerificationState)>::verify)>::Sig;
        if constexpr (std::is_same_v<RequiredSigType,
                                     std::decay_t<decltype(Sig)>>) {
          return VerificationState.verify(Sig);
        } else {
          return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_SIGNATURE);
        }
      },
      VerificationStateVariant, SigVariant);
}

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge