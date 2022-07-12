// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "helper.h"
#include "host/wasi_crypto/asymmetric_common/func.h"
#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/kx/func.h"
#include "host/wasi_crypto/signatures/func.h"
#include "host/wasi_crypto/symmetric/func.h"
#include "host/wasi_crypto/utils/error.h"
#include "wasi_crypto/api.hpp"
#include <algorithm>
#include <cstdint>
#include <string_view>

#define ensureOrReturnOnTest(Expr)                                             \
  do {                                                                         \
    if ((static_cast<__wasi_crypto_errno_e_t>(Expr) !=                         \
         __WASI_CRYPTO_ERRNO_SUCCESS)) {                                       \
      return WasiCryptoUnexpect(static_cast<__wasi_crypto_errno_e_t>(Expr));   \
    }                                                                          \
  } while (0)

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

std::vector<uint8_t> operator"" _u8(const char *Str, std::size_t Len) {
  return std::vector<uint8_t>{reinterpret_cast<const uint8_t *>(Str),
                              reinterpret_cast<const uint8_t *>(Str) + Len};
}

std::vector<uint8_t> operator"" _u8v(const char *Str, std::size_t Len) {
  std::vector<uint8_t> Res;
  Res.reserve(Len / 2);
  for (size_t I = 0; I < Len; I += 2) {
    std::string Tran{Str + I, 2};
    uint8_t Byte = static_cast<uint8_t>(std::strtol(Tran.c_str(), nullptr, 16));
    Res.push_back(Byte);
  }
  return Res;
}

void WasiCryptoTest::writeDummyMemoryContent() {
  std::fill_n(MemInst.getPointer<uint8_t *>(0), 64, UINT8_C(0xa5));
}

void WasiCryptoTest::writeString(std::string_view String, uint32_t Ptr) {
  std::copy(String.begin(), String.end(), MemInst.getPointer<uint8_t *>(Ptr));
}

void WasiCryptoTest::writeSpan(Span<const uint8_t> Content, uint32_t Ptr) {
  std::copy(Content.begin(), Content.end(), MemInst.getPointer<uint8_t *>(Ptr));
}

void WasiCryptoTest::writeOptKey(std::optional<int32_t> OptKey, uint32_t Ptr) {
  __wasi_opt_symmetric_key_t Key;
  if (OptKey) {
    Key.tag = __WASI_OPT_SYMMETRIC_KEY_U_SOME;
    Key.u = {*OptKey};
  } else {
    Key.tag = __WASI_OPT_SYMMETRIC_KEY_U_NONE;
  }
  auto *BeginPlace = MemInst.getPointer<__wasi_opt_symmetric_key_t *>(Ptr);
  *BeginPlace = Key;
}

void WasiCryptoTest::writeOptOptions(std::optional<__wasi_options_t> OptOptions,
                                     uint32_t Ptr) {
  __wasi_opt_options_t Options;
  if (OptOptions) {
    Options.tag = __WASI_OPT_OPTIONS_U_SOME;
    Options.u = {*OptOptions};
  } else {
    Options.tag = __WASI_OPT_OPTIONS_U_NONE;
  }
  auto *BeginPlace = MemInst.getPointer<__wasi_opt_options_t *>(Ptr);
  *BeginPlace = Options;
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) {
  writeDummyMemoryContent();

  Common::ArrayOutputLen Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{ArrayOutputHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_size_t *>(0);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                                Span<uint8_t> Buf) {
  writeDummyMemoryContent();

  Common::ArrayOutputPull Func{Ctx};

  writeSpan(Buf, 0);
  uint32_t BufSize = static_cast<uint32_t>(Buf.size());
  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ArrayOutputHandle, 0, BufSize, BufSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(BufSize), Buf.begin());
  return *MemInst.getPointer<__wasi_size_t *>(BufSize);
}

WasiCryptoExpect<__wasi_options_t>
WasiCryptoTest::optionsOpen(__wasi_algorithm_type_e_t AlgorithmType) {
  writeDummyMemoryContent();

  Common::OptionsOpen Func{Ctx};
  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           static_cast<uint32_t>(AlgorithmType), 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_options_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::optionsClose(__wasi_options_t OptionsHandle) {
  writeDummyMemoryContent();

  Common::OptionsClose Func{Ctx};
  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{OptionsHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void>
WasiCryptoTest::optionsSet(__wasi_options_t OptionsHandle,
                           std::string_view Name, Span<const uint8_t> Value) {
  writeDummyMemoryContent();

  Common::OptionsSet Func{Ctx};

  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());
  writeSpan(Value, NameSize);
  uint32_t ValueSize = static_cast<uint32_t>(Value.size());
  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           OptionsHandle, 0, NameSize, NameSize, ValueSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void>
WasiCryptoTest::optionsSetU64(__wasi_options_t OptionsHandle,
                              std::string_view Name, uint64_t Value) {
  writeDummyMemoryContent();

  Common::OptionsSetU64 Func{Ctx};

  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());
  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           OptionsHandle, 0, NameSize, Value},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_secrets_manager_t> WasiCryptoTest::secretsManagerOpen(
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();

  Common::SecretsManagerOpen Func{Ctx};

  writeOptOptions(OptOptionsHandle, 0);
  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{0, 8}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_secrets_manager_t *>(8);
}

WasiCryptoExpect<void> WasiCryptoTest::secretsManagerClose(
    __wasi_secrets_manager_t SecretsManagerHandle) {
  writeDummyMemoryContent();

  Common::SecretsManagerClose Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{SecretsManagerHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void> WasiCryptoTest::secretsManagerInvalidate(
    __wasi_secrets_manager_t SecretsManagerHandle, Span<const uint8_t> KeyId,
    __wasi_version_t Version) {
  writeDummyMemoryContent();

  Common::SecretsManagerInvalidate Func{Ctx};

  writeSpan(KeyId, 0);
  uint32_t KeyIdSize = static_cast<uint32_t>(KeyId.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SecretsManagerHandle, 0, KeyIdSize, Version},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_key_t> WasiCryptoTest::symmetricKeyGenerate(
    std::string_view Alg, std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();

  Symmetric::KeyGenerate Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);
  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           0, AlgSize, AlgSize, AlgSize + 8},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_symmetric_key_t *>(AlgSize + 8);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoTest::symmetricKeyImport(std::string_view Alg,
                                   Span<const uint8_t> Raw) {
  writeDummyMemoryContent();

  Symmetric::KeyImport Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Raw, AlgSize);
  uint32_t RawSize = static_cast<uint32_t>(Raw.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           0, AlgSize, AlgSize, RawSize, AlgSize + RawSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_symmetric_key_t *>(AlgSize + RawSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::symmetricKeyExport(__wasi_symmetric_key_t KeyHandle) {
  writeDummyMemoryContent();

  Symmetric::KeyExport Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{KeyHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricKeyClose(__wasi_symmetric_key_t KeyHandle) {
  writeDummyMemoryContent();

  Symmetric::KeyClose Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{KeyHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoTest::symmetricKeyGenerateManaged(
    __wasi_secrets_manager_t SecretsManagerHandle, std::string_view Alg,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();

  Symmetric::KeyGenerateManaged Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);

  EXPECT_TRUE(
      Func.run(&MemInst,
               std::initializer_list<WasmEdge::ValVariant>{
                   SecretsManagerHandle, 0, AlgSize, AlgSize, AlgSize + 8},
               Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_keypair_t *>(AlgSize + 8);
}

WasiCryptoExpect<void> WasiCryptoTest::symmetricKeyStoreManaged(
    __wasi_secrets_manager_t SecretsManagerHandle,
    __wasi_symmetric_key_t KeyHandle, Span<uint8_t> KeyId) {
  writeDummyMemoryContent();

  Symmetric::KeyStoreManaged Func{Ctx};

  writeSpan(KeyId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KeyId.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SecretsManagerHandle, KeyHandle, 0, KpIdSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(KpIdSize), KeyId.begin());

  return {};
}

WasiCryptoExpect<__wasi_version_t> WasiCryptoTest::symmetricKeyReplaceManaged(
    __wasi_secrets_manager_t SecretsManagerHandle,
    __wasi_symmetric_key_t OldKeyHandle, __wasi_symmetric_key_t NewKeyHandle) {
  writeDummyMemoryContent();

  Symmetric::KeyReplaceManaged Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SecretsManagerHandle, OldKeyHandle, NewKeyHandle, 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_version_t *>(0);
}

WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
WasiCryptoTest::symmetricKeyId(__wasi_symmetric_key_t KeyHandle,
                               Span<uint8_t> KeyId) {
  writeDummyMemoryContent();

  Symmetric::KeyId Func{Ctx};

  writeSpan(KeyId, 0);
  uint32_t KeyIdSize = static_cast<uint32_t>(KeyId.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           KeyHandle, 0, KeyIdSize, KeyIdSize, KeyIdSize + 1},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(KeyIdSize), KeyId.begin());

  return std::make_tuple(
      *MemInst.getPointer<size_t *>(KeyIdSize),
      *MemInst.getPointer<__wasi_version_t *>(KeyIdSize + 1));
}

WasiCryptoExpect<__wasi_symmetric_key_t> WasiCryptoTest::symmetricKeyFromId(
    __wasi_secrets_manager_t SecretsManagerHandle, Span<uint8_t> KeyId,
    __wasi_version_t KeyVersion) {
  writeDummyMemoryContent();

  Symmetric::KeyFromId Func{Ctx};

  writeSpan(KeyId, 0);
  uint32_t KeyIdSize = static_cast<uint32_t>(KeyId.size());

  EXPECT_TRUE(
      Func.run(&MemInst,
               std::initializer_list<WasmEdge::ValVariant>{
                   SecretsManagerHandle, 0, KeyIdSize, KeyVersion, KeyIdSize},
               Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_symmetric_key_t *>(KeyIdSize);
}

WasiCryptoExpect<__wasi_symmetric_state_t> WasiCryptoTest::symmetricStateOpen(
    std::string_view Alg, std::optional<__wasi_symmetric_key_t> OptKeyHandle,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();

  Symmetric::StateOpen Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptKey(OptKeyHandle, AlgSize);
  writeOptOptions(OptOptionsHandle, AlgSize + 8);

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           0, AlgSize, AlgSize, AlgSize + 8, AlgSize + 16},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_symmetric_state_t *>(AlgSize + 16);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateOptionsGet(__wasi_symmetric_state_t StateHandle,
                                         std::string_view Name,
                                         Span<uint8_t> Value) {
  writeDummyMemoryContent();

  Symmetric::StateOptionsGet Func{Ctx};

  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());
  writeSpan(Value, NameSize);
  uint32_t ValueSize = static_cast<uint32_t>(Value.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, NameSize, NameSize, ValueSize, NameSize + ValueSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(NameSize),
            MemInst.getPointer<uint8_t *>(NameSize + ValueSize), Value.begin());

  return *MemInst.getPointer<__wasi_size_t *>(NameSize + ValueSize);
}

WasiCryptoExpect<uint64_t> WasiCryptoTest::symmetricStateOptionsGetU64(
    __wasi_symmetric_state_t StateHandle, std::string_view Name) {
  writeDummyMemoryContent();

  Symmetric::StateOptionsGetU64 Func{Ctx};

  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           StateHandle, 0, NameSize, NameSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<uint64_t *>(NameSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateClose(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  Symmetric::StateClose Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{StateHandle},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateAbsorb(__wasi_symmetric_state_t StateHandle,
                                     Span<const uint8_t> Data) {
  writeDummyMemoryContent();

  Symmetric::StateAbsorb Func{Ctx};

  writeSpan(Data, 0);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_state_t>
WasiCryptoTest::symmetricStateClone(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  Symmetric::StateClone Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_symmetric_state_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateSqueeze(__wasi_symmetric_state_t StateHandle,
                                      Span<uint8_t> Out) {
  writeDummyMemoryContent();

  Symmetric::StateSqueeze Func{Ctx};

  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, OutSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(OutSize), Out.begin());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_tag_t>
WasiCryptoTest::symmetricStateSqueezeTag(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  Symmetric::StateSqueezeTag Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_symmetric_tag_t *>(0);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoTest::symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                                         std::string_view Alg) {
  writeDummyMemoryContent();

  Symmetric::StateSqueezeKey Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           StateHandle, 0, AlgSize, AlgSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_symmetric_key_t *>(AlgSize);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateMaxTagLen(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  Symmetric::StateMaxTagLen Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_size_t *>(0);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) {
  writeDummyMemoryContent();

  Symmetric::StateEncrypt Func{Ctx};

  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, OutSize, OutSize, DataSize, OutSize + DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(OutSize), Out.begin());

  return *MemInst.getPointer<__wasi_size_t *>(OutSize + DataSize);
}

WasiCryptoExpect<__wasi_symmetric_tag_t>
WasiCryptoTest::symmetricStateEncryptDetached(
    __wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
    Span<const uint8_t> Data) {
  writeDummyMemoryContent();

  Symmetric::StateEncryptDetached Func{Ctx};

  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, OutSize, OutSize, DataSize, OutSize + DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(OutSize), Out.begin());

  return *MemInst.getPointer<__wasi_symmetric_tag_t *>(OutSize + DataSize);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) {
  writeDummyMemoryContent();

  Symmetric::StateDecrypt Func{Ctx};

  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, OutSize, OutSize, DataSize, OutSize + DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(OutSize), Out.begin());

  return *MemInst.getPointer<__wasi_size_t *>(OutSize + DataSize);
}

WasiCryptoExpect<__wasi_size_t> WasiCryptoTest::symmetricStateDecryptDetached(
    __wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
    Span<const uint8_t> Data, Span<uint8_t> RawTag) {
  writeDummyMemoryContent();

  Symmetric::StateDecryptDetached Func{Ctx};

  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());
  writeSpan(RawTag, OutSize + DataSize);
  uint32_t RawTagSize = static_cast<uint32_t>(RawTag.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           StateHandle, 0, OutSize, OutSize, DataSize,
                           OutSize + DataSize, RawTagSize,
                           OutSize + DataSize + RawTagSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(OutSize), Out.begin());
  std::copy(MemInst.getPointer<uint8_t *>(OutSize + DataSize),
            MemInst.getPointer<uint8_t *>(OutSize + DataSize + RawTagSize),
            RawTag.begin());

  return *MemInst.getPointer<__wasi_size_t *>(OutSize + DataSize + RawTagSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateRatchet(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  Symmetric::StateRatchet Func{Ctx};
  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{StateHandle},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricMaxTagLen(__wasi_symmetric_tag_t TagHandle) {
  writeDummyMemoryContent();

  Symmetric::StateMaxTagLen Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{TagHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_size_t *>(0);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricTagPull(__wasi_symmetric_tag_t TagHandle,
                                 Span<uint8_t> Buf) {
  writeDummyMemoryContent();

  Symmetric::TagPull Func{Ctx};

  writeSpan(Buf, 0);
  uint32_t BufSize = static_cast<uint32_t>(Buf.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           TagHandle, 0, BufSize, BufSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(BufSize), Buf.begin());

  return *MemInst.getPointer<__wasi_size_t *>(BufSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                                   Span<const uint8_t> RawTag) {
  writeDummyMemoryContent();

  Symmetric::TagVerify Func{Ctx};

  writeSpan(RawTag, 0);
  uint32_t RawTagSize = static_cast<uint32_t>(RawTag.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{TagHandle, 0, RawTagSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricTagClose(__wasi_symmetric_tag_t TagHandle) {
  writeDummyMemoryContent();

  Symmetric::TagClose Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{TagHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoTest::keypairGenerate(
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairGenerate Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{
          static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize, AlgSize + 8},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_keypair_t *>(AlgSize + 8);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoTest::keypairImport(__wasi_algorithm_type_e_t AlgType,
                              std::string_view Alg, Span<const uint8_t> Encoded,
                              __wasi_keypair_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairImport Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize,
                           EncodedSize, static_cast<uint32_t>(Encoding),
                           AlgSize + EncodedSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_keypair_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoTest::keypairGenerateManaged(
    __wasi_secrets_manager_t SecretsManagerHandle,
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairGenerateManaged Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SecretsManagerHandle, static_cast<uint32_t>(AlgType),
                           0, AlgSize, AlgSize, AlgSize + 8},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_keypair_t *>(AlgSize + 8);
}

WasiCryptoExpect<void> WasiCryptoTest::keypairStoreManaged(
    __wasi_secrets_manager_t SecretsManagerHandle, __wasi_keypair_t KpHandle,
    Span<uint8_t> KpId) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairStoreManaged Func{Ctx};

  writeSpan(KpId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KpId.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SecretsManagerHandle, KpHandle, 0, KpIdSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(KpIdSize), KpId.begin());

  return {};
}

WasiCryptoExpect<__wasi_version_t> WasiCryptoTest::keypairReplaceManaged(
    __wasi_secrets_manager_t SecretsManagerHandle, __wasi_keypair_t OldKpHandle,
    __wasi_keypair_t NewKpHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairReplaceManaged Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SecretsManagerHandle, OldKpHandle, NewKpHandle, 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_version_t *>(0);
}

WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
WasiCryptoTest::keypairId(__wasi_keypair_t KpHandle, Span<uint8_t> KpId) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairId Func{Ctx};

  writeSpan(KpId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KpId.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           KpHandle, 0, KpIdSize, KpIdSize, KpIdSize + 1},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst.getPointer<uint8_t *>(0),
            MemInst.getPointer<uint8_t *>(KpIdSize), KpId.begin());

  return std::make_tuple(*MemInst.getPointer<size_t *>(KpIdSize),
                         *MemInst.getPointer<__wasi_version_t *>(KpIdSize + 1));
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoTest::keypairFromId(__wasi_secrets_manager_t SecretsManagerHandle,
                              Span<const uint8_t> KpId,
                              __wasi_version_t KpIdVersion) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairFromId Func{Ctx};

  writeSpan(KpId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KpId.size());

  EXPECT_TRUE(
      Func.run(&MemInst,
               std::initializer_list<WasmEdge::ValVariant>{
                   SecretsManagerHandle, 0, KpIdSize, KpIdVersion, KpIdSize},
               Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_keypair_t *>(KpIdSize);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoTest::keypairFromPkAndSk(__wasi_publickey_t PkHandle,
                                   __wasi_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairFromPkAndSk Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{PkHandle, SkHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_keypair_t *>(0);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::keypairExport(__wasi_keypair_t KpHandle,
                              __wasi_keypair_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairExport Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           KpHandle, static_cast<uint32_t>(Encoding), 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoTest::keypairPublickey(__wasi_keypair_t KpHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairPublickey Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{KpHandle, 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_publickey_t *>(0);
}

WasiCryptoExpect<__wasi_secretkey_t>
WasiCryptoTest::keypairSecretkey(__wasi_keypair_t KpHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairSecretkey Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{KpHandle, 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_secretkey_t *>(0);
}

WasiCryptoExpect<void> WasiCryptoTest::keypairClose(__wasi_keypair_t KpHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::KeypairClose Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{KpHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_publickey_t> WasiCryptoTest::publickeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    Span<const uint8_t> Encoded, __wasi_publickey_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  AsymmetricCommon::PublickeyImport Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize,
                           EncodedSize, static_cast<uint32_t>(Encoding),
                           AlgSize + EncodedSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_publickey_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::publickeyExport(__wasi_publickey_t PkHandle,
                                __wasi_publickey_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  AsymmetricCommon::PublickeyExport Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           PkHandle, static_cast<uint32_t>(Encoding), 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::publickeyVerify(__wasi_publickey_t PkHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::PublickeyVerify Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{PkHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoTest::publickeyFromSecretkey(__wasi_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::PublickeyFromSecretkey Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{SkHandle, 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_publickey_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::publickeyClose(__wasi_publickey_t PkHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::PublickeyClose Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{PkHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_secretkey_t> WasiCryptoTest::secretkeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    Span<const uint8_t> Encoded, __wasi_secretkey_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  AsymmetricCommon::SecretkeyImport Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize,
                           EncodedSize, static_cast<uint32_t>(Encoding),
                           AlgSize + EncodedSize},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_secretkey_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::secretkeyExport(__wasi_secretkey_t SkHandle,
                                __wasi_secretkey_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  AsymmetricCommon::SecretkeyExport Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SkHandle, static_cast<uint32_t>(Encoding), 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_publickey_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::secretkeyClose(__wasi_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  AsymmetricCommon::SecretkeyClose Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{SkHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::kxDh(__wasi_kx_publickey_t PkHandle,
                     __wasi_kx_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  Kx::Dh Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{PkHandle, SkHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
WasiCryptoTest::kxEncapsulate(__wasi_kx_publickey_t PkHandle) {
  writeDummyMemoryContent();

  Kx::Encapsulate Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{PkHandle, 0, 1},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return std::make_tuple(*MemInst.getPointer<__wasi_array_output_t *>(0),
                         *MemInst.getPointer<__wasi_array_output_t *>(1));
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::kxDecapsulate(__wasi_kx_secretkey_t SkHandle,
                              Span<const uint8_t> EncapsulatedSecret) {
  writeDummyMemoryContent();

  Kx::Decapsulate Func{Ctx};

  writeSpan(EncapsulatedSecret, 0);
  uint32_t EncapsulatedSecretSize =
      static_cast<uint32_t>(EncapsulatedSecret.size());

  EXPECT_TRUE(
      Func.run(&MemInst,
               std::initializer_list<WasmEdge::ValVariant>{
                   SkHandle, 0, EncapsulatedSecretSize, EncapsulatedSecretSize},
               Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_array_output_t *>(EncapsulatedSecretSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::signatureExport(__wasi_signature_t SigHandle,
                                __wasi_signature_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  Signatures::Export Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SigHandle, static_cast<uint32_t>(Encoding), 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoTest::signatureImport(std::string_view Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_signature_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  Signatures::Import Func{Ctx};

  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  EXPECT_TRUE(
      Func.run(&MemInst,
               std::initializer_list<WasmEdge::ValVariant>{
                   0, AlgSize, AlgSize, EncodedSize,
                   static_cast<uint32_t>(Encoding), AlgSize + EncodedSize},
               Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_signature_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::signatureClose(__wasi_signature_t SigHandle) {
  writeDummyMemoryContent();

  Signatures::Close Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{SigHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_signature_state_t>
WasiCryptoTest::signatureStateOpen(__wasi_signature_keypair_t KpHandle) {
  writeDummyMemoryContent();

  Signatures::StateOpen Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{KpHandle, 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_signature_state_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::signatureStateUpdate(__wasi_signature_state_t StateHandle,
                                     Span<const uint8_t> Input) {
  writeDummyMemoryContent();

  Signatures::StateUpdate Func{Ctx};

  writeSpan(Input, 0);
  uint32_t InputSize = static_cast<uint32_t>(Input.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, InputSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoTest::signatureStateSign(__wasi_signature_state_t StateHandle) {
  writeDummyMemoryContent();

  Signatures::StateSign Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_signature_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::signatureStateClose(__wasi_signature_state_t StateHandle) {
  writeDummyMemoryContent();

  Signatures::StateClose Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{StateHandle},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_signature_verification_state_t>
WasiCryptoTest::signatureVerificationStateOpen(
    __wasi_signature_publickey_t PkHandle) {
  writeDummyMemoryContent();

  Signatures::VerificationStateOpen Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{PkHandle, 0},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst.getPointer<__wasi_signature_verification_state_t *>(0);
}

WasiCryptoExpect<void> WasiCryptoTest::signatureVerificationStateUpdate(
    __wasi_signature_verification_state_t StateHandle,
    Span<const uint8_t> Input) {
  writeDummyMemoryContent();

  Signatures::VerificationStateUpdate Func{Ctx};

  writeSpan(Input, 0);
  uint32_t InputSize = static_cast<uint32_t>(Input.size());

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, InputSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void> WasiCryptoTest::signatureVerificationStateVerify(
    __wasi_signature_verification_state_t StateHandle,
    __wasi_signature_t SigHandle) {
  writeDummyMemoryContent();

  Signatures::VerificationStateVerify Func{Ctx};

  EXPECT_TRUE(Func.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, SigHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void> WasiCryptoTest::signatureVerificationStateClose(
    __wasi_signature_verification_state_t StateHandle) {
  writeDummyMemoryContent();

  Signatures::VerificationStateClose Func{Ctx};

  EXPECT_TRUE(Func.run(&MemInst,
                       std::initializer_list<WasmEdge::ValVariant>{StateHandle},
                       Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

// WasiCryptoExpect<__wasi_secretkey_t> WasiCryptoTest::secretkeyImport(
//     __wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
//     Span<const uint8_t> Encoded, __wasi_secretkey_encoding_e_t Encoding) {
//   writeString(AlgStr, 0);
//   writeSpan(Encoded, AlgStr.size());
//   auto Res =
//       testRun<AsymmetricCommon::SecretkeyImport>(
//           {static_cast<uint32_t>(AlgType), 0, AlgStr.size(), AlgStr.size(),
//            Encoded.size(), static_cast<uint32_t>(Encoding),
//            AlgStr.size() + Encoded.size()})
//           .value();
//   if (Res != __WASI_CRYPTO_ERRNO_SUCCESS) {
//     return WasiCryptoUnexpect(Res);
//   }
//   return *MemInst.getPointer<__wasi_signature_keypair_t *>(AlgStr.size() +
//                                                            Encoded.size());
// }

// WasiCryptoExpect<__wasi_array_output_t>
// WasiCryptoTest::secretkeyExport(__wasi_secretkey_t SkHandle,
//                                 __wasi_secretkey_encoding_e_t SkEncoding) {
//   auto Res = testRun<AsymmetricCommon::SecretkeyExport>(
//                  {SkHandle, static_cast<uint32_t>(SkEncoding), 0})
//                  .value();
//   if (Res != __WASI_CRYPTO_ERRNO_SUCCESS) {
//     return WasiCryptoUnexpect(Res);
//   }
//   return *MemInst.getPointer<__wasi_signature_keypair_t *>(0);
// }

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
