// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/ctx.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/symmetric/key.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {


WasiCryptoExpect<__wasi_symmetric_key_t> WasiCryptoContext::symmetricKeyGenerate(
    SymmetricAlgorithm Alg, std::optional<__wasi_options_t> OptionsHandle) {
  auto Options = readSymmetricOption(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  auto Key = SymmetricKey::generate(Alg, *Options);
  if (!Key) {
    return WasiCryptoUnexpect(Key);
  }

  return SymmetricKeyManger.registerManger(std::move(*Key));
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoContext::symmetricKeyImport(SymmetricAlgorithm Alg,
                                     Span<uint8_t const> Raw) {
  auto Key = SymmetricKey::import(Alg, Raw);
  if (!Key) {
    return WasiCryptoUnexpect(Key);
  }

  return SymmetricKeyManger.registerManger(std::move(*Key));
}

WasiCryptoExpect<__wasi_array_output_t>
WasiCryptoContext::symmetricKeyExport(__wasi_symmetric_key_t KeyHandle) {
  auto Key = SymmetricKeyManger.get(KeyHandle);
  if (!Key) {
    return WasiCryptoUnexpect(Key);
  }

  auto Raw = (*Key)->raw();
  if (!Raw) {
    return WasiCryptoUnexpect(Raw);
  }

  return allocateArrayOutput({Raw->begin(), Raw->end()});
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricKeyClose(__wasi_symmetric_key_t SymmetricKey) {
  return SymmetricKeyManger.close(SymmetricKey);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoContext::symmetricKeyGenerateManaged(__wasi_secrets_manager_t,
                                              SymmetricAlgorithm,
                                              std::optional<__wasi_options_t>) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricKeyStoreManaged(__wasi_secrets_manager_t,
                                           __wasi_symmetric_key_t, uint8_t_ptr,
                                           __wasi_size_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<__wasi_version_t> WasiCryptoContext::symmetricKeyReplaceManaged(
    __wasi_secrets_manager_t, __wasi_symmetric_key_t, __wasi_symmetric_key_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<std::tuple<__wasi_size_t, __wasi_version_t>>
WasiCryptoContext::symmetricKeyId(__wasi_symmetric_key_t, uint8_t_ptr,
                                 __wasi_size_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoContext::symmetricKeyFromId(__wasi_secrets_manager_t, Span<uint8_t>,
                                     __wasi_version_t) {
  return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE);
}

WasiCryptoExpect<__wasi_symmetric_state_t> WasiCryptoContext::symmetricStateOpen(
    SymmetricAlgorithm Alg, std::optional<__wasi_symmetric_key_t> KeyHandle,
    std::optional<__wasi_options_t> OptionsHandle) {

  auto Key = readSymmetricKey(KeyHandle);
  if (!Key) {
    return WasiCryptoUnexpect(Key);
  }

  auto Options = readSymmetricOption(OptionsHandle);
  if (!Options) {
    return WasiCryptoUnexpect(Options);
  }

  auto State = SymmetricState::make(Alg, *Key, *Options);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  auto Handle = SymmetricStateManger.registerManger(std::move(*State));
  if (!Handle) {
    return WasiCryptoUnexpect(Handle);
  }

  return *Handle;
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoContext::symmetricStateOptionsGet(__wasi_symmetric_state_t Handle,
                                           std::string_view Name,
                                           Span<uint8_t> Value) {
  auto State = SymmetricStateManger.get(Handle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  auto InnerVec = (*State)->optionsGet(Name);
  if (!InnerVec) {
    return WasiCryptoUnexpect(InnerVec);
  }
  //  auto InnerVec =
  //      (*SymmetricState)->locked([&Name](SymmetricStateBase &StateBase) {
  //        return StateBase.optionsGet(Name);
  //      });
  if (InnerVec->size() > Value.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }
  std::copy(InnerVec->begin(), InnerVec->end(), Value.begin());
  return InnerVec->size();
}

WasiCryptoExpect<uint64_t>
WasiCryptoContext::symmetricStateOptionsGetU64(__wasi_symmetric_state_t Handle,
                                              std::string_view Name) {
  auto SymmetricState = SymmetricStateManger.get(Handle);

  if (!SymmetricState) {
    return WasiCryptoUnexpect(SymmetricState);
  }

  return (*SymmetricState)->optionsGetU64(Name);
  //  return (*SymmetricState)->locked([&Name](SymmetricStateBase &StateBase) {
  //    return StateBase.optionsGetU64(Name);
  //  });
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricStateClose(__wasi_symmetric_state_t Handle) {
  return SymmetricStateManger.close(Handle);
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricStateAbsorb(__wasi_symmetric_state_t Handle,
                                       Span<uint8_t const> Data) {
  auto SymmetricState = SymmetricStateManger.get(Handle);
  if (!SymmetricState) {
    return WasiCryptoUnexpect(SymmetricState);
  }

  return (*SymmetricState)->absorb(Data);
  //  return (*SymmetricState)->locked([&Data](SymmetricStateBase &StateBase) {
  //    return StateBase.absorb(Data);
  //  });
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricStateSqueeze(__wasi_symmetric_state_t Handle,
                                        Span<uint8_t> Out) {
  auto SymmetricState = SymmetricStateManger.get(Handle);
  if (!SymmetricState) {
    return WasiCryptoUnexpect(SymmetricState);
  }

  return (*SymmetricState)->squeeze(Out);
  //  return (*SymmetricState)->locked([&Out](SymmetricStateBase &StateBase) {
  //    return StateBase.squeeze(Out);
  //  });
}

WasiCryptoExpect<__wasi_symmetric_tag_t>
WasiCryptoContext::symmetricStateSqueezeTag(__wasi_symmetric_state_t Handle) {
  auto SymmetricState = SymmetricStateManger.get(Handle);
  if (!SymmetricState) {
    return WasiCryptoUnexpect(SymmetricState);
  }
  auto Tag = (*SymmetricState)->squeezeTag();
  //  auto Tag = (*SymmetricState)->locked([](SymmetricStateBase &StateBase) {
  //    return StateBase.squeezeTag();
  //  });

  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }

  return SymmetricTagManger.registerManger(*Tag);
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoContext::symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                                           SymmetricAlgorithm Alg) {
  auto State = SymmetricStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  auto Key = (*State)->squeezeKey(Alg);
  //  auto Key = (*SymmetricState)->locked([&Alg](SymmetricStateBase &StateBase)
  //  {
  //    return StateBase.squeezeKey(Alg);
  //  });
  if (!Key) {
    return WasiCryptoUnexpect(Key);
  }

  return SymmetricKeyManger.registerManger(std::move(*Key));
}

WasiCryptoExpect<__wasi_size_t> WasiCryptoContext::symmetricStateMaxTagLen(
    __wasi_symmetric_state_t StateHandle) {
  auto State = SymmetricStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  return (*State)->maxTagLen();
  //  return (*SymmetricState)->locked([](SymmetricStateBase &StateBase) {
  //    return StateBase.maxTagLen();
  //  });
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoContext::symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle,
                                        Span<uint8_t> Out,
                                        Span<uint8_t const> Data) {
  auto State = SymmetricStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  return (*State)->encrypt(Out, Data);
  //  return (*SymmetricState)->locked([&Out, &Data](SymmetricStateBase
  //  &StateBase) {
  //        return StateBase.encrypt(Out, Data);
  //      });
}

WasiCryptoExpect<__wasi_symmetric_key_t>
WasiCryptoContext::symmetricStateEncryptDetached(
    __wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
    Span<uint8_t const> Data) {
  auto State = SymmetricStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  auto Tag = (*State)->encryptDetached(Out, Data);
  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }

  return SymmetricTagManger.registerManger(*Tag);
  //  return (*SymmetricState)
  //      ->locked([&Out, &Data](SymmetricStateBase &StateBase) {
  //        return StateBase.encryptDetached(Out, Data);
  //      });
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoContext::symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle,
                                        Span<uint8_t> Out,
                                        Span<uint8_t const> Data) {
  auto State = SymmetricStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  return (*State)->decrypt(Out, Data);
  //  return (*SymmetricState)
  //      ->locked([&Out, &Data](SymmetricStateBase &StateBase) {
  //        return StateBase.decrypt(Out, Data);
  //      });
}

WasiCryptoExpect<__wasi_size_t> WasiCryptoContext::symmetricStateDecryptDetached(
    __wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
    Span<uint8_t const> Data, Span<uint8_t> RawTag) {
  auto State = SymmetricStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  return (*State)->decryptDetached(Out, Data, RawTag);
  //  return (*SymmetricState)
  //      ->locked([&Out, &Data, &RawTag](SymmetricStateBase &StateBase) {
  //        return StateBase.decryptDetached(Out, Data, RawTag);
  //      });
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricStateRatchet(__wasi_symmetric_state_t StateHandle) {
  auto State = SymmetricStateManger.get(StateHandle);
  if (!State) {
    return WasiCryptoUnexpect(State);
  }

  return (*State)->ratchet();
  //  return (*SymmetricState)->locked([](SymmetricStateBase &StateBase) {
  //    return StateBase.ratchet();
  //  });
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoContext::symmetricTagLen(__wasi_symmetric_tag_t TagHandle) {
  auto Tag = SymmetricTagManger.get(TagHandle);
  if (!Tag) {
    return Tag.error();
  }
  return Tag->raw().size();
}

WasiCryptoExpect<__wasi_size_t>
WasiCryptoContext::symmetricTagPull(__wasi_symmetric_tag_t TagHandle,
                                   Span<uint8_t> Buf) {
  auto Tag = SymmetricTagManger.get(TagHandle);
  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }
  auto Raw = Tag->raw();
  if (Raw.size() > Buf.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }
  std::copy(Raw.begin(), Raw.end(), Buf.begin());

  auto CloseRes = SymmetricTagManger.close(TagHandle);
  if (!CloseRes) {
    return WasiCryptoUnexpect(CloseRes);
  }

  return Raw.size();
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                                     Span<uint8_t> RawTag) {
  auto Tag = SymmetricTagManger.get(TagHandle);
  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }
  return Tag->verify(RawTag);
}

WasiCryptoExpect<void>
WasiCryptoContext::symmetricTagClose(__wasi_symmetric_tag_t TagHandle) {
  return SymmetricTagManger.close(TagHandle);
}

WasiCryptoExpect<std::shared_ptr<SymmetricOptions>>
WasiCryptoContext::readSymmetricOption(
    std::optional<__wasi_options_t> OptionsHandle) {
  std::shared_ptr<SymmetricOptions> Options = nullptr;

  if (OptionsHandle) {
    auto Res = readOption(*OptionsHandle);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    Options = std::dynamic_pointer_cast<SymmetricOptions>(std::move(*Res));
    if (Options == nullptr) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
  }

  return Options;
}

WasiCryptoExpect<std::shared_ptr<SymmetricKey>>
WasiCryptoContext::readSymmetricKey(
    std::optional<__wasi_symmetric_key_t> KeyHandle) {
  std::shared_ptr<SymmetricKey> Key = nullptr;

  if (KeyHandle) {
    auto Res = SymmetricKeyManger.get(*KeyHandle);
    if (!Res) {
      return WasiCryptoUnexpect(Res);
    }
    Key = std::move(*Res);
  }

  return Key;
}


} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
