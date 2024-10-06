// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "helper.h"
#include "asymmetric_common/func.h"
#include "common/func.h"
#include "kx/func.h"
#include "signatures/func.h"
#include "symmetric/func.h"
#include "utils/error.h"

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

namespace {
template <typename T, typename M>
inline T *getHostFunc(M &Mod, const char *Name) {
  if (Mod) {
    auto *FuncInst = Mod->findFuncExports(Name);
    if (FuncInst && FuncInst->isHostFunction()) {
      return dynamic_cast<T *>(&FuncInst->getHostFunc());
    }
  }
  return nullptr;
}
} // namespace

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
  std::fill_n(MemInst->getPointer<uint8_t *>(0), 64, UINT8_C(0xa5));
}

void WasiCryptoTest::writeString(std::string_view String, uint32_t Ptr) {
  std::copy(String.begin(), String.end(), MemInst->getPointer<uint8_t *>(Ptr));
}

void WasiCryptoTest::writeSpan(Span<const uint8_t> Content, uint32_t Ptr) {
  std::copy(Content.begin(), Content.end(),
            MemInst->getPointer<uint8_t *>(Ptr));
}

void WasiCryptoTest::writeOptKey(std::optional<int32_t> OptKey, uint32_t Ptr) {
  __wasi_opt_symmetric_key_t Key;
  if (OptKey) {
    Key.tag = __WASI_OPT_SYMMETRIC_KEY_U_SOME;
    Key.u = {*OptKey};
  } else {
    Key.tag = __WASI_OPT_SYMMETRIC_KEY_U_NONE;
  }
  auto *BeginPlace = MemInst->getPointer<__wasi_opt_symmetric_key_t *>(Ptr);
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
  auto *BeginPlace = MemInst->getPointer<__wasi_opt_options_t *>(Ptr);
  *BeginPlace = Options;
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Common::ArrayOutputLen>(WasiCryptoCommonMod,
                                                   "array_output_len");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ArrayOutputHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_size_t *>(0);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                                Span<uint8_t> Buf) {
  writeDummyMemoryContent();
  writeSpan(Buf, 0);
  uint32_t BufSize = static_cast<uint32_t>(Buf.size());

  auto *Func = getHostFunc<Common::ArrayOutputPull>(WasiCryptoCommonMod,
                                                    "array_output_pull");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            ArrayOutputHandle, 0, BufSize, BufSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(BufSize), Buf.begin());
  return *MemInst->getPointer<__wasi_size_t *>(BufSize);
}

WasiCryptoExpect<__wasi_options_t>
WasiCryptoTest::optionsOpen(__wasi_algorithm_type_e_t AlgorithmType) {
  writeDummyMemoryContent();

  auto *Func =
      getHostFunc<Common::OptionsOpen>(WasiCryptoCommonMod, "options_open");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            static_cast<uint32_t>(AlgorithmType), 0},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_options_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::optionsClose(__wasi_options_t OptionsHandle) {
  writeDummyMemoryContent();

  auto *Func =
      getHostFunc<Common::OptionsClose>(WasiCryptoCommonMod, "options_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{OptionsHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void>
WasiCryptoTest::optionsSet(__wasi_options_t OptionsHandle,
                           std::string_view Name, Span<const uint8_t> Value) {
  writeDummyMemoryContent();
  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());
  writeSpan(Value, NameSize);
  uint32_t ValueSize = static_cast<uint32_t>(Value.size());

  auto *Func =
      getHostFunc<Common::OptionsSet>(WasiCryptoCommonMod, "options_set");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
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

  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());

  auto *Func = getHostFunc<Common::OptionsSetU64>(WasiCryptoCommonMod,
                                                  "options_set_u64");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            OptionsHandle, 0, NameSize, Value},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_secrets_manager_t> WasiCryptoTest::secretsManagerOpen(
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();
  writeOptOptions(OptOptionsHandle, 0);

  auto *Func = getHostFunc<Common::SecretsManagerOpen>(WasiCryptoCommonMod,
                                                       "secrets_manager_open");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{0, 8}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_secrets_manager_t *>(8);
}

WasiCryptoExpect<void> WasiCryptoTest::secretsManagerClose(
    __wasi_secrets_manager_t SecretsManagerHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Common::SecretsManagerClose>(
      WasiCryptoCommonMod, "secrets_manager_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{SecretsManagerHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void> WasiCryptoTest::secretsManagerInvalidate(
    __wasi_secrets_manager_t SecretsManagerHandle, Span<const uint8_t> KeyId,
    __wasi_version_t Version) {
  writeDummyMemoryContent();
  writeSpan(KeyId, 0);
  uint32_t KeyIdSize = static_cast<uint32_t>(KeyId.size());

  auto *Func = getHostFunc<Common::SecretsManagerInvalidate>(
      WasiCryptoCommonMod, "secrets_manager_invalidate");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            SecretsManagerHandle, 0, KeyIdSize, Version},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_key_t> WasiCryptoTest::symmetricKeyGenerate(
    std::string_view Alg, std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);

  auto *Func = getHostFunc<Symmetric::KeyGenerate>(WasiCryptoSymmMod,
                                                   "symmetric_key_generate");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            0, AlgSize, AlgSize, AlgSize + 8},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_symmetric_key_t *>(AlgSize + 8);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoTest::symmetricKeyImport(std::string_view Alg,
                                   Span<const uint8_t> Raw) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Raw, AlgSize);
  uint32_t RawSize = static_cast<uint32_t>(Raw.size());

  auto *Func = getHostFunc<Symmetric::KeyImport>(WasiCryptoSymmMod,
                                                 "symmetric_key_import");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            0, AlgSize, AlgSize, RawSize, AlgSize + RawSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_symmetric_key_t *>(AlgSize + RawSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::symmetricKeyExport(__wasi_symmetric_key_t KeyHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::KeyExport>(WasiCryptoSymmMod,
                                                 "symmetric_key_export");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{KeyHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricKeyClose(__wasi_symmetric_key_t KeyHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::KeyClose>(WasiCryptoSymmMod,
                                                "symmetric_key_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{KeyHandle},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoTest::symmetricKeyGenerateManaged(
    __wasi_secrets_manager_t SecretsManagerHandle, std::string_view Alg,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);

  auto *Func = getHostFunc<Symmetric::KeyGenerateManaged>(
      WasiCryptoSymmMod, "symmetric_key_generate_managed");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(
      Func->run(CallFrame,
                std::initializer_list<WasmEdge::ValVariant>{
                    SecretsManagerHandle, 0, AlgSize, AlgSize, AlgSize + 8},
                Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_keypair_t *>(AlgSize + 8);
}

WasiCryptoExpect<void> WasiCryptoTest::symmetricKeyStoreManaged(
    __wasi_secrets_manager_t SecretsManagerHandle,
    __wasi_symmetric_key_t KeyHandle, Span<uint8_t> KeyId) {
  writeDummyMemoryContent();
  writeSpan(KeyId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KeyId.size());

  auto *Func = getHostFunc<Symmetric::KeyStoreManaged>(
      WasiCryptoSymmMod, "symmetric_key_store_managed");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            SecretsManagerHandle, KeyHandle, 0, KpIdSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(KpIdSize), KeyId.begin());

  return {};
}

WasiCryptoExpect<__wasi_version_t> WasiCryptoTest::symmetricKeyReplaceManaged(
    __wasi_secrets_manager_t SecretsManagerHandle,
    __wasi_symmetric_key_t OldKeyHandle, __wasi_symmetric_key_t NewKeyHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::KeyReplaceManaged>(
      WasiCryptoSymmMod, "symmetric_key_replace_managed");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(
      Func->run(CallFrame,
                std::initializer_list<WasmEdge::ValVariant>{
                    SecretsManagerHandle, OldKeyHandle, NewKeyHandle, 0},
                Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_version_t *>(0);
}

WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
WasiCryptoTest::symmetricKeyId(__wasi_symmetric_key_t KeyHandle,
                               Span<uint8_t> KeyId) {
  writeDummyMemoryContent();
  writeSpan(KeyId, 0);
  uint32_t KeyIdSize = static_cast<uint32_t>(KeyId.size());

  auto *Func =
      getHostFunc<Symmetric::KeyId>(WasiCryptoSymmMod, "symmetric_key_id");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            KeyHandle, 0, KeyIdSize, KeyIdSize, KeyIdSize + 1},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(KeyIdSize), KeyId.begin());

  return std::make_tuple(
      *MemInst->getPointer<size_t *>(KeyIdSize),
      *MemInst->getPointer<__wasi_version_t *>(KeyIdSize + 1));
}

WasiCryptoExpect<__wasi_symmetric_key_t> WasiCryptoTest::symmetricKeyFromId(
    __wasi_secrets_manager_t SecretsManagerHandle, Span<uint8_t> KeyId,
    __wasi_version_t KeyVersion) {
  writeDummyMemoryContent();
  writeSpan(KeyId, 0);
  uint32_t KeyIdSize = static_cast<uint32_t>(KeyId.size());

  auto *Func = getHostFunc<Symmetric::KeyFromId>(WasiCryptoSymmMod,
                                                 "symmetric_key_from_id");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(
      Func->run(CallFrame,
                std::initializer_list<WasmEdge::ValVariant>{
                    SecretsManagerHandle, 0, KeyIdSize, KeyVersion, KeyIdSize},
                Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_symmetric_key_t *>(KeyIdSize);
}

WasiCryptoExpect<__wasi_symmetric_state_t> WasiCryptoTest::symmetricStateOpen(
    std::string_view Alg, std::optional<__wasi_symmetric_key_t> OptKeyHandle,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptKey(OptKeyHandle, AlgSize);
  writeOptOptions(OptOptionsHandle, AlgSize + 8);

  auto *Func = getHostFunc<Symmetric::StateOpen>(WasiCryptoSymmMod,
                                                 "symmetric_state_open");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            0, AlgSize, AlgSize, AlgSize + 8, AlgSize + 16},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_symmetric_state_t *>(AlgSize + 16);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateOptionsGet(__wasi_symmetric_state_t StateHandle,
                                         std::string_view Name,
                                         Span<uint8_t> Value) {
  writeDummyMemoryContent();
  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());
  writeSpan(Value, NameSize);
  uint32_t ValueSize = static_cast<uint32_t>(Value.size());

  auto *Func = getHostFunc<Symmetric::StateOptionsGet>(
      WasiCryptoSymmMod, "symmetric_state_options_get");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, NameSize, NameSize, ValueSize, NameSize + ValueSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(NameSize),
            MemInst->getPointer<uint8_t *>(NameSize + ValueSize),
            Value.begin());

  return *MemInst->getPointer<__wasi_size_t *>(NameSize + ValueSize);
}

WasiCryptoExpect<uint64_t> WasiCryptoTest::symmetricStateOptionsGetU64(
    __wasi_symmetric_state_t StateHandle, std::string_view Name) {
  writeDummyMemoryContent();
  writeString(Name, 0);
  uint32_t NameSize = static_cast<uint32_t>(Name.size());

  auto *Func = getHostFunc<Symmetric::StateOptionsGetU64>(
      WasiCryptoSymmMod, "symmetric_state_options_get_u64");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            StateHandle, 0, NameSize, NameSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<uint64_t *>(NameSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateClose(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::StateClose>(WasiCryptoSymmMod,
                                                  "symmetric_state_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateAbsorb(__wasi_symmetric_state_t StateHandle,
                                     Span<const uint8_t> Data) {
  writeDummyMemoryContent();
  writeSpan(Data, 0);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  auto *Func = getHostFunc<Symmetric::StateAbsorb>(WasiCryptoSymmMod,
                                                   "symmetric_state_absorb");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_state_t>
WasiCryptoTest::symmetricStateClone(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::StateClone>(WasiCryptoSymmMod,
                                                  "symmetric_state_clone");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_symmetric_state_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateSqueeze(__wasi_symmetric_state_t StateHandle,
                                      Span<uint8_t> Out) {
  writeDummyMemoryContent();
  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());

  auto *Func = getHostFunc<Symmetric::StateSqueeze>(WasiCryptoSymmMod,
                                                    "symmetric_state_squeeze");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, OutSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(OutSize), Out.begin());

  return {};
}

WasiCryptoExpect<__wasi_symmetric_tag_t>
WasiCryptoTest::symmetricStateSqueezeTag(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::StateSqueezeTag>(
      WasiCryptoSymmMod, "symmetric_state_squeeze_tag");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_symmetric_tag_t *>(0);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoTest::symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                                         std::string_view Alg) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());

  auto *Func = getHostFunc<Symmetric::StateSqueezeKey>(
      WasiCryptoSymmMod, "symmetric_state_squeeze_key");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            StateHandle, 0, AlgSize, AlgSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_symmetric_key_t *>(AlgSize);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateMaxTagLen(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::StateMaxTagLen>(
      WasiCryptoSymmMod, "symmetric_state_max_tag_len");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_size_t *>(0);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) {
  writeDummyMemoryContent();
  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  auto *Func = getHostFunc<Symmetric::StateEncrypt>(WasiCryptoSymmMod,
                                                    "symmetric_state_encrypt");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, OutSize, OutSize, DataSize, OutSize + DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(OutSize), Out.begin());

  return *MemInst->getPointer<__wasi_size_t *>(OutSize + DataSize);
}

WasiCryptoExpect<__wasi_symmetric_tag_t>
WasiCryptoTest::symmetricStateEncryptDetached(
    __wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
    Span<const uint8_t> Data) {
  writeDummyMemoryContent();
  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  auto *Func = getHostFunc<Symmetric::StateEncryptDetached>(
      WasiCryptoSymmMod, "symmetric_state_encrypt_detached");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, OutSize, OutSize, DataSize, OutSize + DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(OutSize), Out.begin());

  return *MemInst->getPointer<__wasi_symmetric_tag_t *>(OutSize + DataSize);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle,
                                      Span<uint8_t> Out,
                                      Span<const uint8_t> Data) {
  writeDummyMemoryContent();
  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());

  auto *Func = getHostFunc<Symmetric::StateDecrypt>(WasiCryptoSymmMod,
                                                    "symmetric_state_decrypt");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          StateHandle, 0, OutSize, OutSize, DataSize, OutSize + DataSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(OutSize), Out.begin());

  return *MemInst->getPointer<__wasi_size_t *>(OutSize + DataSize);
}

WasiCryptoExpect<__wasi_size_t> WasiCryptoTest::symmetricStateDecryptDetached(
    __wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
    Span<const uint8_t> Data, Span<uint8_t> RawTag) {
  writeDummyMemoryContent();
  writeSpan(Out, 0);
  uint32_t OutSize = static_cast<uint32_t>(Out.size());
  writeSpan(Data, OutSize);
  uint32_t DataSize = static_cast<uint32_t>(Data.size());
  writeSpan(RawTag, OutSize + DataSize);
  uint32_t RawTagSize = static_cast<uint32_t>(RawTag.size());

  auto *Func = getHostFunc<Symmetric::StateDecryptDetached>(
      WasiCryptoSymmMod, "symmetric_state_decrypt_detached");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            StateHandle, 0, OutSize, OutSize, DataSize,
                            OutSize + DataSize, RawTagSize,
                            OutSize + DataSize + RawTagSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(OutSize), Out.begin());
  std::copy(MemInst->getPointer<uint8_t *>(OutSize + DataSize),
            MemInst->getPointer<uint8_t *>(OutSize + DataSize + RawTagSize),
            RawTag.begin());

  return *MemInst->getPointer<__wasi_size_t *>(OutSize + DataSize + RawTagSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricStateRatchet(__wasi_symmetric_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::StateRatchet>(WasiCryptoSymmMod,
                                                    "symmetric_state_ratchet");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricMaxTagLen(__wasi_symmetric_tag_t TagHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::StateMaxTagLen>(
      WasiCryptoSymmMod, "symmetric_state_max_tag_len");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{TagHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_size_t *>(0);
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoTest::symmetricTagPull(__wasi_symmetric_tag_t TagHandle,
                                 Span<uint8_t> Buf) {
  writeDummyMemoryContent();
  writeSpan(Buf, 0);
  uint32_t BufSize = static_cast<uint32_t>(Buf.size());

  auto *Func =
      getHostFunc<Symmetric::TagPull>(WasiCryptoSymmMod, "symmetric_tag_pull");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            TagHandle, 0, BufSize, BufSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(BufSize), Buf.begin());

  return *MemInst->getPointer<__wasi_size_t *>(BufSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                                   Span<const uint8_t> RawTag) {
  writeDummyMemoryContent();
  writeSpan(RawTag, 0);
  uint32_t RawTagSize = static_cast<uint32_t>(RawTag.size());

  auto *Func = getHostFunc<Symmetric::TagVerify>(WasiCryptoSymmMod,
                                                 "symmetric_tag_verify");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{TagHandle, 0, RawTagSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void>
WasiCryptoTest::symmetricTagClose(__wasi_symmetric_tag_t TagHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Symmetric::TagClose>(WasiCryptoSymmMod,
                                                "symmetric_tag_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{TagHandle},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoTest::keypairGenerate(
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);

  auto *Func = getHostFunc<AsymmetricCommon::KeypairGenerate>(
      WasiCryptoAsymCommonMod, "keypair_generate");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize, AlgSize + 8},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_keypair_t *>(AlgSize + 8);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoTest::keypairImport(__wasi_algorithm_type_e_t AlgType,
                              std::string_view Alg, Span<const uint8_t> Encoded,
                              __wasi_keypair_encoding_e_t Encoding) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  auto *Func = getHostFunc<AsymmetricCommon::KeypairImport>(
      WasiCryptoAsymCommonMod, "keypair_import");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize,
                            EncodedSize, static_cast<uint32_t>(Encoding),
                            AlgSize + EncodedSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_keypair_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<__wasi_keypair_t> WasiCryptoTest::keypairGenerateManaged(
    __wasi_secrets_manager_t SecretsManagerHandle,
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    std::optional<__wasi_options_t> OptOptionsHandle) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeOptOptions(OptOptionsHandle, AlgSize);

  auto *Func = getHostFunc<AsymmetricCommon::KeypairGenerateManaged>(
      WasiCryptoAsymCommonMod, "keypair_generate_managed");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(
      Func->run(CallFrame,
                std::initializer_list<WasmEdge::ValVariant>{
                    SecretsManagerHandle, static_cast<uint32_t>(AlgType), 0,
                    AlgSize, AlgSize, AlgSize + 8},
                Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_keypair_t *>(AlgSize + 8);
}

WasiCryptoExpect<void> WasiCryptoTest::keypairStoreManaged(
    __wasi_secrets_manager_t SecretsManagerHandle, __wasi_keypair_t KpHandle,
    Span<uint8_t> KpId) {
  writeDummyMemoryContent();
  writeSpan(KpId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KpId.size());

  auto *Func = getHostFunc<AsymmetricCommon::KeypairStoreManaged>(
      WasiCryptoAsymCommonMod, "keypair_store_managed");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            SecretsManagerHandle, KpHandle, 0, KpIdSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(KpIdSize), KpId.begin());

  return {};
}

WasiCryptoExpect<__wasi_version_t> WasiCryptoTest::keypairReplaceManaged(
    __wasi_secrets_manager_t SecretsManagerHandle, __wasi_keypair_t OldKpHandle,
    __wasi_keypair_t NewKpHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::KeypairReplaceManaged>(
      WasiCryptoAsymCommonMod, "keypair_replace_managed");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            SecretsManagerHandle, OldKpHandle, NewKpHandle, 0},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_version_t *>(0);
}

WasiCryptoExpect<std::tuple<size_t, __wasi_version_t>>
WasiCryptoTest::keypairId(__wasi_keypair_t KpHandle, Span<uint8_t> KpId) {
  writeDummyMemoryContent();
  writeSpan(KpId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KpId.size());

  auto *Func = getHostFunc<AsymmetricCommon::KeypairId>(WasiCryptoAsymCommonMod,
                                                        "keypair_id");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            KpHandle, 0, KpIdSize, KpIdSize, KpIdSize + 1},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  std::copy(MemInst->getPointer<uint8_t *>(0),
            MemInst->getPointer<uint8_t *>(KpIdSize), KpId.begin());

  return std::make_tuple(
      *MemInst->getPointer<size_t *>(KpIdSize),
      *MemInst->getPointer<__wasi_version_t *>(KpIdSize + 1));
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoTest::keypairFromId(__wasi_secrets_manager_t SecretsManagerHandle,
                              Span<const uint8_t> KpId,
                              __wasi_version_t KpIdVersion) {
  writeDummyMemoryContent();
  writeSpan(KpId, 0);
  uint32_t KpIdSize = static_cast<uint32_t>(KpId.size());

  auto *Func = getHostFunc<AsymmetricCommon::KeypairFromId>(
      WasiCryptoAsymCommonMod, "keypair_from_id");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(
      Func->run(CallFrame,
                std::initializer_list<WasmEdge::ValVariant>{
                    SecretsManagerHandle, 0, KpIdSize, KpIdVersion, KpIdSize},
                Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_keypair_t *>(KpIdSize);
}

WasiCryptoExpect<__wasi_keypair_t>
WasiCryptoTest::keypairFromPkAndSk(__wasi_publickey_t PkHandle,
                                   __wasi_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::KeypairFromPkAndSk>(
      WasiCryptoAsymCommonMod, "keypair_from_pk_and_sk");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PkHandle, SkHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_keypair_t *>(0);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::keypairExport(__wasi_keypair_t KpHandle,
                              __wasi_keypair_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::KeypairExport>(
      WasiCryptoAsymCommonMod, "keypair_export");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            KpHandle, static_cast<uint32_t>(Encoding), 0},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoTest::keypairPublickey(__wasi_keypair_t KpHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::KeypairPublickey>(
      WasiCryptoAsymCommonMod, "keypair_publickey");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{KpHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_publickey_t *>(0);
}

WasiCryptoExpect<__wasi_secretkey_t>
WasiCryptoTest::keypairSecretkey(__wasi_keypair_t KpHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::KeypairSecretkey>(
      WasiCryptoAsymCommonMod, "keypair_secretkey");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{KpHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_secretkey_t *>(0);
}

WasiCryptoExpect<void> WasiCryptoTest::keypairClose(__wasi_keypair_t KpHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::KeypairClose>(
      WasiCryptoAsymCommonMod, "keypair_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{KpHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_publickey_t> WasiCryptoTest::publickeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    Span<const uint8_t> Encoded, __wasi_publickey_encoding_e_t Encoding) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  auto *Func = getHostFunc<AsymmetricCommon::PublickeyImport>(
      WasiCryptoAsymCommonMod, "publickey_import");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize,
                            EncodedSize, static_cast<uint32_t>(Encoding),
                            AlgSize + EncodedSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_publickey_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::publickeyExport(__wasi_publickey_t PkHandle,
                                __wasi_publickey_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::PublickeyExport>(
      WasiCryptoAsymCommonMod, "publickey_export");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            PkHandle, static_cast<uint32_t>(Encoding), 0},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::publickeyVerify(__wasi_publickey_t PkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::PublickeyVerify>(
      WasiCryptoAsymCommonMod, "publickey_verify");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PkHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_publickey_t>
WasiCryptoTest::publickeyFromSecretkey(__wasi_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::PublickeyFromSecretkey>(
      WasiCryptoAsymCommonMod, "publickey_from_secretkey");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SkHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_publickey_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::publickeyClose(__wasi_publickey_t PkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::PublickeyClose>(
      WasiCryptoAsymCommonMod, "publickey_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PkHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_secretkey_t> WasiCryptoTest::secretkeyImport(
    __wasi_algorithm_type_e_t AlgType, std::string_view Alg,
    Span<const uint8_t> Encoded, __wasi_secretkey_encoding_e_t Encoding) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  auto *Func = getHostFunc<AsymmetricCommon::SecretkeyImport>(
      WasiCryptoAsymCommonMod, "secretkey_import");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            static_cast<uint32_t>(AlgType), 0, AlgSize, AlgSize,
                            EncodedSize, static_cast<uint32_t>(Encoding),
                            AlgSize + EncodedSize},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_secretkey_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::secretkeyExport(__wasi_secretkey_t SkHandle,
                                __wasi_secretkey_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::SecretkeyExport>(
      WasiCryptoAsymCommonMod, "secretkey_export");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            SkHandle, static_cast<uint32_t>(Encoding), 0},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_publickey_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::secretkeyClose(__wasi_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<AsymmetricCommon::SecretkeyClose>(
      WasiCryptoAsymCommonMod, "secretkey_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SkHandle}, Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::kxDh(__wasi_kx_publickey_t PkHandle,
                     __wasi_kx_secretkey_t SkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Kx::Dh>(WasiCryptoKxMod, "kx_dh");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PkHandle, SkHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
WasiCryptoTest::kxEncapsulate(__wasi_kx_publickey_t PkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Kx::Encapsulate>(WasiCryptoKxMod, "kx_encapsulate");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PkHandle, 0, 1},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return std::make_tuple(*MemInst->getPointer<__wasi_array_output_t *>(0),
                         *MemInst->getPointer<__wasi_array_output_t *>(1));
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::kxDecapsulate(__wasi_kx_secretkey_t SkHandle,
                              Span<const uint8_t> EncapsulatedSecret) {
  writeDummyMemoryContent();
  writeSpan(EncapsulatedSecret, 0);
  uint32_t EncapsulatedSecretSize =
      static_cast<uint32_t>(EncapsulatedSecret.size());

  auto *Func = getHostFunc<Kx::Decapsulate>(WasiCryptoKxMod, "kx_decapsulate");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          SkHandle, 0, EncapsulatedSecretSize, EncapsulatedSecretSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_array_output_t *>(EncapsulatedSecretSize);
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoTest::signatureExport(__wasi_signature_t SigHandle,
                                __wasi_signature_encoding_e_t Encoding) {
  writeDummyMemoryContent();

  auto *Func =
      getHostFunc<Signatures::Export>(WasiCryptoSignMod, "signature_export");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            SigHandle, static_cast<uint32_t>(Encoding), 0},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_array_output_t *>(0);
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoTest::signatureImport(std::string_view Alg,
                                Span<const uint8_t> Encoded,
                                __wasi_signature_encoding_e_t Encoding) {
  writeDummyMemoryContent();
  writeString(Alg, 0);
  uint32_t AlgSize = static_cast<uint32_t>(Alg.size());
  writeSpan(Encoded, AlgSize);
  uint32_t EncodedSize = static_cast<uint32_t>(Encoded.size());

  auto *Func =
      getHostFunc<Signatures::Import>(WasiCryptoSignMod, "signature_import");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(
      Func->run(CallFrame,
                std::initializer_list<WasmEdge::ValVariant>{
                    0, AlgSize, AlgSize, EncodedSize,
                    static_cast<uint32_t>(Encoding), AlgSize + EncodedSize},
                Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_signature_t *>(AlgSize + EncodedSize);
}

WasiCryptoExpect<void>
WasiCryptoTest::signatureClose(__wasi_signature_t SigHandle) {
  writeDummyMemoryContent();

  auto *Func =
      getHostFunc<Signatures::Close>(WasiCryptoSignMod, "signature_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{SigHandle},
                        Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_signature_state_t>
WasiCryptoTest::signatureStateOpen(__wasi_signature_keypair_t KpHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Signatures::StateOpen>(WasiCryptoSignMod,
                                                  "signature_state_open");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{KpHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_signature_state_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::signatureStateUpdate(__wasi_signature_state_t StateHandle,
                                     Span<const uint8_t> Input) {
  writeDummyMemoryContent();
  writeSpan(Input, 0);
  uint32_t InputSize = static_cast<uint32_t>(Input.size());

  auto *Func = getHostFunc<Signatures::StateUpdate>(WasiCryptoSignMod,
                                                    "signature_state_update");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, InputSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_signature_t>
WasiCryptoTest::signatureStateSign(__wasi_signature_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Signatures::StateSign>(WasiCryptoSignMod,
                                                  "signature_state_sign");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_signature_t *>(0);
}

WasiCryptoExpect<void>
WasiCryptoTest::signatureStateClose(__wasi_signature_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Signatures::StateClose>(WasiCryptoSignMod,
                                                   "signature_state_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<__wasi_signature_verification_state_t>
WasiCryptoTest::signatureVerificationStateOpen(
    __wasi_signature_publickey_t PkHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Signatures::VerificationStateOpen>(
      WasiCryptoSignMod, "signature_verification_state_open");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PkHandle, 0},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return *MemInst->getPointer<__wasi_signature_verification_state_t *>(0);
}

WasiCryptoExpect<void> WasiCryptoTest::signatureVerificationStateUpdate(
    __wasi_signature_verification_state_t StateHandle,
    Span<const uint8_t> Input) {
  writeDummyMemoryContent();
  writeSpan(Input, 0);
  uint32_t InputSize = static_cast<uint32_t>(Input.size());

  auto *Func = getHostFunc<Signatures::VerificationStateUpdate>(
      WasiCryptoSignMod, "signature_verification_state_update");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, 0, InputSize},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void> WasiCryptoTest::signatureVerificationStateVerify(
    __wasi_signature_verification_state_t StateHandle,
    __wasi_signature_t SigHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Signatures::VerificationStateVerify>(
      WasiCryptoSignMod, "signature_verification_state_verify");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{StateHandle, SigHandle},
      Errno));
  ensureOrReturnOnTest(Errno[0].get<int32_t>());

  return {};
}

WasiCryptoExpect<void> WasiCryptoTest::signatureVerificationStateClose(
    __wasi_signature_verification_state_t StateHandle) {
  writeDummyMemoryContent();

  auto *Func = getHostFunc<Signatures::VerificationStateClose>(
      WasiCryptoSignMod, "signature_verification_state_close");
  EXPECT_NE(Func, nullptr);
  EXPECT_TRUE(Func->run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StateHandle},
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
//   return *MemInst->getPointer<__wasi_signature_keypair_t *>(AlgStr.size() +
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
//   return *MemInst->getPointer<__wasi_signature_keypair_t *>(0);
// }

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
