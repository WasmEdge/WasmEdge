// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasiCryptoMock {

using namespace std::literals;
static inline constexpr const uint32_t kWASICryptoError = 1U;

namespace Common {
class ArrayOutputLen : public Runtime::HostFunction<ArrayOutputLen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class ArrayOutputPull : public Runtime::HostFunction<ArrayOutputPull> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class OptionsOpen : public Runtime::HostFunction<OptionsOpen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class OptionsClose : public Runtime::HostFunction<OptionsClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class OptionsSet : public Runtime::HostFunction<OptionsSet> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class OptionsSetU64 : public Runtime::HostFunction<OptionsSetU64> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint64_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class OptionsSetGuestBuffer
    : public Runtime::HostFunction<OptionsSetGuestBuffer> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class SecretsManagerOpen : public Runtime::HostFunction<SecretsManagerOpen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class SecretsManagerClose : public Runtime::HostFunction<SecretsManagerClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class SecretsManagerInvalidate
    : public Runtime::HostFunction<SecretsManagerInvalidate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint64_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};
} // namespace Common

namespace AsymmetricCommon {
class KeypairGenerate : public Runtime::HostFunction<KeypairGenerate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairImport : public Runtime::HostFunction<KeypairImport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairGenerateManaged
    : public Runtime::HostFunction<KeypairGenerateManaged> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairStoreManaged : public Runtime::HostFunction<KeypairStoreManaged> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, int32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairReplaceManaged
    : public Runtime::HostFunction<KeypairReplaceManaged> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, int32_t,
                        int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairId : public Runtime::HostFunction<KeypairId> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairFromId : public Runtime::HostFunction<KeypairFromId> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint64_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairFromPkAndSk : public Runtime::HostFunction<KeypairFromPkAndSk> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, int32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairExport : public Runtime::HostFunction<KeypairExport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairPublickey : public Runtime::HostFunction<KeypairPublickey> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairSecretkey : public Runtime::HostFunction<KeypairSecretkey> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeypairClose : public Runtime::HostFunction<KeypairClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class PublickeyImport : public Runtime::HostFunction<PublickeyImport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class PublickeyExport : public Runtime::HostFunction<PublickeyExport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class PublickeyVerify : public Runtime::HostFunction<PublickeyVerify> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class PublickeyFromSecretkey
    : public Runtime::HostFunction<PublickeyFromSecretkey> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class PublickeyClose : public Runtime::HostFunction<PublickeyClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class SecretkeyImport : public Runtime::HostFunction<SecretkeyImport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class SecretkeyExport : public Runtime::HostFunction<SecretkeyExport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class SecretkeyClose : public Runtime::HostFunction<SecretkeyClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};
} // namespace AsymmetricCommon

namespace Kx {
class Dh : public Runtime::HostFunction<Dh> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, int32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class Encapsulate : public Runtime::HostFunction<Encapsulate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class Decapsulate : public Runtime::HostFunction<Decapsulate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};
} // namespace Kx

namespace Signatures {
class Export : public Runtime::HostFunction<Export> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class Import : public Runtime::HostFunction<Import> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateOpen : public Runtime::HostFunction<StateOpen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateUpdate : public Runtime::HostFunction<StateUpdate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateSign : public Runtime::HostFunction<StateSign> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateClose : public Runtime::HostFunction<StateClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class VerificationStateOpen
    : public Runtime::HostFunction<VerificationStateOpen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class VerificationStateUpdate
    : public Runtime::HostFunction<VerificationStateUpdate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class VerificationStateVerify
    : public Runtime::HostFunction<VerificationStateVerify> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class VerificationStateClose
    : public Runtime::HostFunction<VerificationStateClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class Close : public Runtime::HostFunction<Close> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

} // namespace Signatures

namespace Symmetric {
class KeyGenerate : public Runtime::HostFunction<KeyGenerate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyImport : public Runtime::HostFunction<KeyImport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyExport : public Runtime::HostFunction<KeyExport> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyClose : public Runtime::HostFunction<KeyClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyGenerateManaged : public Runtime::HostFunction<KeyGenerateManaged> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyStoreManaged : public Runtime::HostFunction<KeyStoreManaged> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, int32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyReplaceManaged : public Runtime::HostFunction<KeyReplaceManaged> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, int32_t,
                        int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyId : public Runtime::HostFunction<KeyId> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class KeyFromId : public Runtime::HostFunction<KeyFromId> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint64_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateOpen : public Runtime::HostFunction<StateOpen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateClone : public Runtime::HostFunction<StateClone> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateOptionsGet : public Runtime::HostFunction<StateOptionsGet> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateOptionsGetU64 : public Runtime::HostFunction<StateOptionsGetU64> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateClose : public Runtime::HostFunction<StateClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateAbsorb : public Runtime::HostFunction<StateAbsorb> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateSqueeze : public Runtime::HostFunction<StateSqueeze> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateSqueezeTag : public Runtime::HostFunction<StateSqueezeTag> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateSqueezeKey : public Runtime::HostFunction<StateSqueezeKey> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateMaxTagLen : public Runtime::HostFunction<StateMaxTagLen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateEncrypt : public Runtime::HostFunction<StateEncrypt> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateEncryptDetached
    : public Runtime::HostFunction<StateEncryptDetached> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateDecrypt : public Runtime::HostFunction<StateDecrypt> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateDecryptDetached
    : public Runtime::HostFunction<StateDecryptDetached> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class StateRatchet : public Runtime::HostFunction<StateRatchet> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class TagLen : public Runtime::HostFunction<TagLen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class TagPull : public Runtime::HostFunction<TagPull> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class TagVerify : public Runtime::HostFunction<TagVerify> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};

class TagClose : public Runtime::HostFunction<TagClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("WASI-Crypto"sv);
    return kWASICryptoError;
  }
};
} // namespace Symmetric

} // namespace WasiCryptoMock
} // namespace Host
} // namespace WasmEdge
