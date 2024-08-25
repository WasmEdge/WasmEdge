// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "signatures/verificationstate.h"

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
              return VerificationStateVariant{
                  std::forward<decltype(VerificationState)>(VerificationState)};
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
/// Correspond signatures:
/// WasiCryptoExpect<void> VerificationStateType::verify(const SigType&);
/// is used to get `SigType`.
template <class P> struct VerifyTrait;
template <class VerificationStateType, class SigType>
struct VerifyTrait<WasiCryptoExpect<void> (VerificationStateType::*)(
    const SigType &) noexcept> {
  using Sig = SigType;
};

template <typename T>
using SigType = typename VerifyTrait<decltype(&T::verify)>::Sig;
} // namespace

WasiCryptoExpect<void>
verificationStateVerify(VerificationStateVariant &VerificationStateVariant,
                        const SigVariant &SigVariant) noexcept {
  return std::visit(
      [](auto &VerificationState,
         const auto &Sig) noexcept -> WasiCryptoExpect<void> {
        using RequiredSigType =
            SigType<std::decay_t<decltype(VerificationState)>>;
        using InSigType = std::decay_t<decltype(Sig)>;

        if constexpr (std::is_same_v<RequiredSigType, InSigType>) {
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
