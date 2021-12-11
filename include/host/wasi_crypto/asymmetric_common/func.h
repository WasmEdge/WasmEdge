// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/ctx.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace AsymmetricCommon {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(WasiCryptoContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WasiCryptoContext &Ctx;
};

class KeypairGenerate : public HostFunction<KeypairGenerate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                        __wasi_size_t AlgorithmLen, uint32_t Options,
                        uint8_t_ptr /* Out */ KeypairPtr);
};

class KeypairImport : public HostFunction<KeypairImport> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                        __wasi_size_t AlgorithmLen,
                        const_uint8_t_ptr EncodedPtr, __wasi_size_t EncodedLen,
                        uint32_t Encoding, uint8_t_ptr /* Out */ KeypairPtr);
};

class KeypairGenerateManaged : public HostFunction<KeypairGenerateManaged> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                        __wasi_size_t AlgorithmLen, uint8_t_ptr OptOptions,
                        uint8_t_ptr ResultPtr);
};

class KeypairStoreManaged : public HostFunction<KeypairStoreManaged> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        __wasi_keypair_t Kp, uint8_t_ptr KpId,
                        __wasi_size_t KpIdMaxLen);
};

class KeypairReplaceManaged : public HostFunction<KeypairReplaceManaged> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        __wasi_keypair_t KpOld, __wasi_keypair_t KpNew,
                        uint8_t_ptr /* Out */ VersionPtr);
};

class KeypairId : public HostFunction<KeypairId> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_keypair_t Kp, uint8_t_ptr KpId,
                        __wasi_size_t KpIdMaxLen,
                        uint8_t_ptr /* Out */ SizePtr,
                        uint8_t_ptr /* Out */ VersionPtr);
};

class KeypairFromId : public HostFunction<KeypairFromId> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        const_uint8_t_ptr KpId, __wasi_size_t KpIdLen,
                        __wasi_version_t KpVersion,
                        uint8_t_ptr /* Out */ KeypairPtr);
};

class KeypairFromPkAndSk : public HostFunction<KeypairFromPkAndSk> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_publickey_t Publickey,
                        __wasi_secretkey_t Secretkey,
                        uint8_t_ptr /* Out */ KeypairPtr);
};

class KeypairExport : public HostFunction<KeypairExport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_keypair_t Kp, uint32_t Encoding,
                        uint8_t_ptr /* Out */ ArrayOutputPtr);
};

class KeypairPublickey : public HostFunction<KeypairPublickey> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_keypair_t Kp,
                        uint8_t_ptr /* Out */ PublicKeyPtr);
};

class KeypairSecretkey : public HostFunction<KeypairSecretkey> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_keypair_t Kp,
                        uint8_t_ptr /* Out */ SecretKeyPtr);
};

class KeypairClose : public HostFunction<KeypairClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_keypair_t /* Out */ Kp);
};

class PublickeyImport : public HostFunction<PublickeyImport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t AlgorithmType, const_uint8_t_ptr AlgorithmPtr,
                        __wasi_size_t AlgorithmLen, const_uint8_t_ptr EncodedPtr,
                        __wasi_size_t EncodedLen, uint32_t Encoding,
                        uint8_t_ptr /* Out */ PublickeyPtr);
};

class PublickeyExport : public HostFunction<PublickeyExport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_publickey_t Pk,
                        uint32_t Encoding,
                        uint8_t_ptr /* Out */ ArrayOutputPtr);
};

class PublickeyVerify : public HostFunction<PublickeyVerify> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_publickey_t Pk);
};

class PublickeyFromSecretkey : public HostFunction<PublickeyFromSecretkey> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secretkey_t Sk,
                        uint8_t_ptr /* Out */ PublickeyPtr);
};

class PublickeyClose : public HostFunction<PublickeyClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_publickey_t Pk);
};

class SecretkeyImport : public HostFunction<SecretkeyImport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t AlgorithmType,
                        const_uint8_t_ptr AlgorithmPtr,
                        __wasi_size_t AlgorithmLen, const_uint8_t_ptr EncodedPtr,
                        __wasi_size_t EncodedLen,
                        uint32_t Encoding,
                        uint8_t_ptr /* Out */ SecretkeyPtr);
};

class SecretkeyExport : public HostFunction<SecretkeyExport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secretkey_t Sk,
                        uint32_t Encoding,
                        uint8_t_ptr /* Out */ ArrayOutputPtr);
};

class SecretkeyClose : public HostFunction<SecretkeyClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secretkey_t Sk);
};

} // namespace AsymmetricCommon
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
