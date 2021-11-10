// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/ctx.h"
#include "gtest/gtest.h"

using namespace WasmEdge::Host::WASICrypto;
using namespace std::literals;
namespace {

WasmEdge::Span<uint8_t const> operator"" _u8(const char *Str,
                                             std::size_t Len) noexcept {
  return {reinterpret_cast<uint8_t const *>(Str), Len};
}

} // namespace

TEST(WasiCryptoTest, Hash) {

  CommonContext C;
  SymmetricContext Ctx{C};

  auto StateHandle =
      Ctx.symmetricStateOpen("SHA-256"sv, std::nullopt, std::nullopt).value();
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

std::vector<uint8_t> tagToVec(WasmEdge::Host::WASICrypto::SymmetricContext &Ctx,
                              __wasi_symmetric_tag_t TagHandle) {
  auto SymmetricTagSize = Ctx.symmetricTagLen(TagHandle).value();
  std::vector<uint8_t> Bytes(SymmetricTagSize, 0);
  Ctx.symmetricTagPull(TagHandle, Bytes).value();
  return Bytes;
}

TEST(WasiCryptoTest, Hmac) {
  CommonContext C;
  SymmetricContext Ctx{C};

  auto KeyHandle =
      Ctx.symmetricKeyGenerate("HMAC/SHA-512"sv, std::nullopt).value();

  auto StateHandle =
      Ctx.symmetricStateOpen("HMAC/SHA-512"sv, KeyHandle, std::nullopt).value();
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
  CommonContext C;
  SymmetricContext Ctx{C};

  std::array<uint8_t, 32> Out;
  auto KeyHandle =
      Ctx.symmetricKeyImport("HKDF-EXTRACT/SHA-512"sv, "IKM"_u8).value();
  auto StateHandle =
      Ctx.symmetricStateOpen("HKDF-EXTRACT/SHA-512"sv, KeyHandle, std::nullopt)
          .value();
  EXPECT_TRUE(Ctx.symmetricStateAbsorb(StateHandle, "salt"_u8).has_value());

  auto NewKeyHandle =
      Ctx.symmetricStateSqueezeKey(StateHandle, "HKDF-EXPAND/SHA-512"sv)
          .value();
  EXPECT_TRUE(Ctx.symmetricStateClose(StateHandle).has_value());
  EXPECT_TRUE(Ctx.symmetricKeyClose(KeyHandle).has_value());

  // --------------------EXPAND----------------------
  auto NewStateHandle = Ctx.symmetricStateOpen("HKDF-EXPAND/SHA-512"sv,
                                               NewKeyHandle, std::nullopt)
                            .value();

  EXPECT_TRUE(Ctx.symmetricStateAbsorb(NewStateHandle, "info"_u8).has_value());
  EXPECT_TRUE(Ctx.symmetricStateSqueeze(NewStateHandle, Out).has_value());
  EXPECT_TRUE(Ctx.symmetricStateClose(NewStateHandle).has_value());
}

TEST(WasiCryptoTest, Encryption) {
  CommonContext C;
  SymmetricContext Ctx{C};

  auto Sp = "test"_u8;
  std::vector<uint8_t> Msg{Sp.begin(), Sp.end()};
  std::array<uint8_t, 12> InNonce;
  InNonce.fill(42);
  auto KeyHandle =
      Ctx.symmetricKeyGenerate("AES-256-GCM"sv, std::nullopt).value();

  auto OptionsHandle = C.optionsOpen(__WASI_ALGORITHM_TYPE_SYMMETRIC).value();
  EXPECT_TRUE(C.optionsSet(OptionsHandle, "nonce"sv, InNonce).has_value());

  // ----- state 1, Test Nonce -----------
  auto State1 =
      Ctx.symmetricStateOpen("AES-256-GCM"sv, KeyHandle, OptionsHandle).value();

  std::array<uint8_t, 12> OutNonce;
  Ctx.symmetricStateOptionsGet(State1, "nonce"sv, OutNonce).value();

  EXPECT_EQ(InNonce, OutNonce);

  auto TagMaxSize = Ctx.symmetricStateMaxTagLen(State1).value();
  std::vector<uint8_t> CiphertextWithTag(Msg.size() + TagMaxSize, 0);
  Ctx.symmetricStateEncrypt(State1, CiphertextWithTag, Msg).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State1).has_value());

  // ----- state 2 -----------
  auto State2 =
      Ctx.symmetricStateOpen("AES-256-GCM"sv, KeyHandle, OptionsHandle).value();
  std::vector<uint8_t> Msg2(Msg.size(), 0);
  Ctx.symmetricStateDecrypt(State2, Msg2, CiphertextWithTag).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State2).has_value());
  EXPECT_EQ(Msg, Msg2);

  // ----- state 3 -----------
  auto State3 =
      Ctx.symmetricStateOpen("AES-256-GCM"sv, KeyHandle, OptionsHandle).value();
  std::vector<uint8_t> Ciphertext(Msg.size(), 0);
  auto TagHandle =
      Ctx.symmetricStateEncryptDetached(State3, Ciphertext, Msg).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State3).has_value());

  // ----- state 4 -----------
  auto RawTag = tagToVec(Ctx, TagHandle);

  auto State4 =
      Ctx.symmetricStateOpen("AES-256-GCM"sv, KeyHandle, OptionsHandle).value();
  std::vector<uint8_t> Msg3(Msg.size(), 0);
  Ctx.symmetricStateDecryptDetached(State4, Msg3, Ciphertext, RawTag).value();
  EXPECT_TRUE(Ctx.symmetricStateClose(State4).has_value());
  EXPECT_EQ(Msg, Msg3);
}

//TEST(WasiCryptoTest, Session) {
//  CommonContext C;
//  SymmetricContext Ctx{C};
//
//  auto Msg = "test"_u8;
//  std::vector<uint8_t> Msg2(Msg.size(), 0);
//
//  std::array<uint8_t, 32> Squeezed;
//  std::array<uint8_t, 32> Squeezed2;
//  auto KeyHandle =
//      Ctx.symmetricKeyGenerate("XOODYAK-128", std::nullopt).value();
//
//  auto SymmetricState =
//      Ctx.symmetricStateOpen("XOODYAK-128", KeyHandle, std::nullopt).value();
//
//  EXPECT_TRUE(Ctx.symmetricStateAbsorb(SymmetricState, "data"_u8).has_value());
//  EXPECT_TRUE(Ctx.symmetricStateSqueeze(SymmetricState, Squeezed).has_value());
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
//  EXPECT_TRUE(Ctx.symmetricStateSqueeze(SymmetricState, Squeezed).has_value());
//  EXPECT_TRUE(Ctx.symmetricStateClose(SymmetricState).has_value());
//
//  //
//
//  auto SymmetricState2 =
//      Ctx.symmetricStateOpen("XOODYAK-128", KeyHandle, std::nullopt).value();
//  EXPECT_TRUE(Ctx.symmetricStateAbsorb(SymmetricState2, "data"_u8).has_value());
//  EXPECT_TRUE(
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