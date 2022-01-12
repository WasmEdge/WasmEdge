// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/symmetric/alg.h"

#include <array>
#include <vector>

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;
namespace {
WasmEdge::Span<uint8_t const> operator"" _u8(const char *Str,
                                             std::size_t Len) noexcept {
  return {reinterpret_cast<uint8_t const *>(Str), Len};
}

std::vector<uint8_t> tagToVec(WasiCryptoContext &Ctx,
                              __wasi_symmetric_tag_t TagHandle) {
  auto SymmetricTagSize = Ctx.symmetricTagLen(TagHandle).value();
  std::vector<uint8_t> Bytes(SymmetricTagSize, 0);
  Ctx.symmetricTagPull(TagHandle, Bytes).value();
  return Bytes;
}

} // namespace

TEST(WasiCryptoTest, Hash) {

  WasiCryptoContext Ctx;

  auto StateHandle = Ctx.symmetricStateOpen(SymmetricAlgorithm::Sha256,
                                            std::nullopt, std::nullopt)
                         .value();
  Ctx.symmetricStateAbsorb(StateHandle, "data"_u8);
  Ctx.symmetricStateAbsorb(StateHandle, "more_data"_u8);

  std::array<uint8_t, 32> Out;
  Ctx.symmetricStateSqueeze(StateHandle, Out);

  std::array<uint8_t, 32> Expected = {19,  196, 14,  236, 34,  84,  26,  21,
                                      94,  23,  32,  16,  199, 253, 110, 246,
                                      84,  228, 225, 56,  160, 194, 9,   35,
                                      249, 169, 16,  98,  162, 127, 87,  182};
  EXPECT_EQ(Out, Expected);
  EXPECT_TRUE(Ctx.symmetricStateClose(StateHandle));
}

TEST(WasiCryptoTest, Hmac) {
  WasiCryptoContext Ctx;

  auto KeyHandle =
      Ctx.symmetricKeyGenerate(SymmetricAlgorithm::HmacSha512, std::nullopt)
          .value();

  auto StateHandle = Ctx.symmetricStateOpen(SymmetricAlgorithm::HmacSha512,
                                            KeyHandle, std::nullopt)
                         .value();
  Ctx.symmetricStateAbsorb(StateHandle, "data"_u8);
  Ctx.symmetricStateAbsorb(StateHandle, "more_data"_u8);

  auto TagHandle = Ctx.symmetricStateSqueezeTag(StateHandle).value();
  auto RawTag = tagToVec(Ctx, TagHandle);

  auto TagHandle2 = Ctx.symmetricStateSqueezeTag(StateHandle).value();
  EXPECT_TRUE(Ctx.symmetricTagVerify(TagHandle2, RawTag).has_value());

  EXPECT_TRUE(Ctx.symmetricStateClose(StateHandle).has_value());
  EXPECT_TRUE(Ctx.symmetricKeyClose(KeyHandle).has_value());
  EXPECT_TRUE(Ctx.symmetricTagClose(TagHandle2).has_value());
}

TEST(WasiCryptoTest, Hkdf) {
  WasiCryptoContext Ctx;

  std::array<uint8_t, 32> Out;
  auto KeyHandle =
      Ctx.symmetricKeyImport(SymmetricAlgorithm::HkdfSha512Extract, "IKM"_u8)
          .value();
  auto StateHandle =
      Ctx.symmetricStateOpen(SymmetricAlgorithm::HkdfSha512Extract, KeyHandle,
                             std::nullopt)
          .value();
  EXPECT_TRUE(Ctx.symmetricStateAbsorb(StateHandle, "salt"_u8).has_value());

  // --------------------EXPAND----------------------
  auto PrkHandle = Ctx.symmetricStateSqueezeKey(
                          StateHandle, SymmetricAlgorithm::HkdfSha512Expand)
                       .value();
  EXPECT_TRUE(Ctx.symmetricStateClose(StateHandle).has_value());
  EXPECT_TRUE(Ctx.symmetricKeyClose(KeyHandle).has_value());

  auto NewStateHandle =
      Ctx.symmetricStateOpen(SymmetricAlgorithm::HkdfSha512Expand, PrkHandle,
                             std::nullopt)
          .value();

  EXPECT_TRUE(Ctx.symmetricStateAbsorb(NewStateHandle, "info"_u8).has_value());
  EXPECT_TRUE(Ctx.symmetricStateSqueeze(NewStateHandle, Out).has_value());
  EXPECT_TRUE(Ctx.symmetricStateClose(NewStateHandle).has_value());
}

TEST(WasiCryptoTest, Encryption) {
  WasiCryptoContext Ctx;

  auto Sp = "test"_u8;
  std::vector<uint8_t> Msg{Sp.begin(), Sp.end()};
  std::array<uint8_t, 12> InNonce;
  InNonce.fill(42);
  auto KeyHandle =
      Ctx.symmetricKeyGenerate(SymmetricAlgorithm::Aes256Gcm, std::nullopt)
          .value();

  auto OptionsHandle = Ctx.optionsOpen(__WASI_ALGORITHM_TYPE_SYMMETRIC).value();
  EXPECT_TRUE(Ctx.optionsSet(OptionsHandle, "nonce"sv, InNonce).has_value());

  // ----- state 1, Test Nonce -----------
  __wasi_symmetric_state_t State1 =
      Ctx.symmetricStateOpen(SymmetricAlgorithm::Aes256Gcm, KeyHandle,
                             OptionsHandle)
          .value();

  std::array<uint8_t, 12> OutNonce;
  EXPECT_EQ(12,
            Ctx.symmetricStateOptionsGet(State1, "nonce"sv, OutNonce).value());

  EXPECT_EQ(InNonce, OutNonce);

  std::vector<uint8_t> CiphertextWithTag(
      Msg.size() + Ctx.symmetricStateMaxTagLen(State1).value(), 0);
  Ctx.symmetricStateEncrypt(State1, CiphertextWithTag, Msg).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State1).has_value());

  // ----- state 2 -----------
  auto State2 = Ctx.symmetricStateOpen(SymmetricAlgorithm::Aes256Gcm, KeyHandle,
                                       OptionsHandle)
                    .value();
  std::vector<uint8_t> Msg2(Msg.size(), 0);
  Ctx.symmetricStateDecrypt(State2, Msg2, CiphertextWithTag).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State2).has_value());
  EXPECT_EQ(Msg, Msg2);

  // ----- state 3 -----------
  auto State3 = Ctx.symmetricStateOpen(SymmetricAlgorithm::Aes256Gcm, KeyHandle,
                                       OptionsHandle)
                    .value();
  std::vector<uint8_t> Ciphertext(Msg.size(), 0);
  auto TagHandle =
      Ctx.symmetricStateEncryptDetached(State3, Ciphertext, Msg).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State3).has_value());

  // ----- state 4 -----------
  auto RawTag = tagToVec(Ctx, TagHandle);

  auto State4 = Ctx.symmetricStateOpen(SymmetricAlgorithm::Aes256Gcm, KeyHandle,
                                       OptionsHandle)
                    .value();
  std::vector<uint8_t> Msg3(Msg.size(), 0);
  Ctx.symmetricStateDecryptDetached(State4, Msg3, Ciphertext, RawTag).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State4).has_value());
  EXPECT_EQ(Msg, Msg3);
}

TEST(WasiCryptoTest, EncryptionCharchaPoly) {
  WasiCryptoContext Ctx;

  auto Sp = "test"_u8;
  std::vector<uint8_t> Msg{Sp.begin(), Sp.end()};
  std::array<uint8_t, 12> InNonce;
  InNonce.fill(42);
  auto KeyHandle = Ctx.symmetricKeyGenerate(
                          SymmetricAlgorithm::ChaCha20Poly1305, std::nullopt)
                       .value();

  auto OptionsHandle = Ctx.optionsOpen(__WASI_ALGORITHM_TYPE_SYMMETRIC).value();
  EXPECT_TRUE(Ctx.optionsSet(OptionsHandle, "nonce"sv, InNonce).has_value());

  // ----- state 1, Test Nonce -----------
  __wasi_symmetric_state_t State1 =
      Ctx.symmetricStateOpen(SymmetricAlgorithm::ChaCha20Poly1305, KeyHandle,
                             OptionsHandle)
          .value();

  std::array<uint8_t, 12> OutNonce;
  EXPECT_EQ(12,
            Ctx.symmetricStateOptionsGet(State1, "nonce"sv, OutNonce).value());

  EXPECT_EQ(InNonce, OutNonce);

  std::vector<uint8_t> CiphertextWithTag(
      Msg.size() + Ctx.symmetricStateMaxTagLen(State1).value(), 0);
  Ctx.symmetricStateEncrypt(State1, CiphertextWithTag, Msg).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State1).has_value());

  // ----- state 2 -----------
  auto State2 = Ctx.symmetricStateOpen(SymmetricAlgorithm::ChaCha20Poly1305,
                                       KeyHandle, OptionsHandle)
                    .value();
  std::vector<uint8_t> Msg2(Msg.size(), 0);
  Ctx.symmetricStateDecrypt(State2, Msg2, CiphertextWithTag).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State2).has_value());
  EXPECT_EQ(Msg, Msg2);

  // ----- state 3 -----------
  auto State3 = Ctx.symmetricStateOpen(SymmetricAlgorithm::ChaCha20Poly1305,
                                       KeyHandle, OptionsHandle)
                    .value();
  std::vector<uint8_t> Ciphertext(Msg.size(), 0);
  auto TagHandle =
      Ctx.symmetricStateEncryptDetached(State3, Ciphertext, Msg).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State3).has_value());

  // ----- state 4 -----------
  auto RawTag = tagToVec(Ctx, TagHandle);

  auto State4 = Ctx.symmetricStateOpen(SymmetricAlgorithm::ChaCha20Poly1305,
                                       KeyHandle, OptionsHandle)
                    .value();
  std::vector<uint8_t> Msg3(Msg.size(), 0);
  Ctx.symmetricStateDecryptDetached(State4, Msg3, Ciphertext, RawTag).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State4).has_value());
  EXPECT_EQ(Msg, Msg3);
}
// Not Implementation
// TEST(WasiCryptoTest, Session) {
//  WasiCryptoContext Ctx;
//
//  auto Msg = "test"_u8;
//  std::vector<uint8_t> Msg2(Msg.size(), 0);
//
//  std::array<uint8_t, 32> Squeezed;
//  std::array<uint8_t, 32> Squeezed2;
//  auto KeyHandle =
//      Ctx.symmetricKeyGenerate(SymmetricAlgorithm::Xoodyak128, std::nullopt)
//          .value();
//
//  auto SymmetricState = Ctx.symmetricStateOpen(SymmetricAlgorithm::Xoodyak128,
//                                               KeyHandle, std::nullopt)
//                            .value();
//
//  EXPECT_TRUE(Ctx.symmetricStateAbsorb(SymmetricState,
//  "data"_u8).has_value());
//  EXPECT_TRUE(Ctx.symmetricStateSqueeze(SymmetricState,
//  Squeezed).has_value());
//
//  std::vector<uint8_t> CiphertextWithTag(
//      Msg.size() + Ctx.symmetricStateMaxTagLen(SymmetricState).value());
//  Ctx.symmetricStateEncrypt(SymmetricState, CiphertextWithTag, Msg).value();
//
//  EXPECT_TRUE(
//      Ctx.symmetricStateAbsorb(SymmetricState, "more_data"_u8).has_value());
//
//  EXPECT_TRUE(Ctx.symmetricStateRatchet(SymmetricState).has_value());
//
//  EXPECT_TRUE(Ctx.symmetricStateSqueeze(SymmetricState,
//  Squeezed).has_value());
//  EXPECT_TRUE(Ctx.symmetricStateClose(SymmetricState).has_value());
//
//  //
//
//  auto SymmetricState2 =
//  Ctx.symmetricStateOpen(SymmetricAlgorithm::Xoodyak128,
//                                                KeyHandle, std::nullopt)
//                             .value();
//  EXPECT_TRUE(Ctx.symmetricStateAbsorb(SymmetricState2,
//  "data"_u8).has_value()); EXPECT_TRUE(
//      Ctx.symmetricStateSqueeze(SymmetricState2, Squeezed2).has_value());
//
//  Ctx.symmetricStateDecrypt(SymmetricState2, Msg2, CiphertextWithTag).value();
//
//  EXPECT_TRUE(
//      Ctx.symmetricStateAbsorb(SymmetricState2, "more_data"_u8).has_value());
//
//  EXPECT_TRUE(Ctx.symmetricStateRatchet(SymmetricState2).has_value());
//
//  EXPECT_TRUE(
//      Ctx.symmetricStateSqueeze(SymmetricState2, Squeezed2).has_value());
//  EXPECT_TRUE(Ctx.symmetricStateClose(SymmetricState2).has_value());
//  EXPECT_EQ(Squeezed, Squeezed2);
//}