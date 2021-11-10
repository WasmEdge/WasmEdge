// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "host/wasi_crypto/symmetric/ctx.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {

template <typename T>
class WasiCryptoSymmetric : public Runtime::HostFunction<T> {
public:
  WasiCryptoSymmetric(WASICrypto::SymmetricContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WASICrypto::SymmetricContext &Ctx;
};

class WasiSymmetricKeyGenerate
    : public WasiCryptoSymmetric<WasiSymmetricKeyGenerate> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        const_uint8_t_ptr AlgPtr,
                        __wasi_size_t AlgLen, uint32_t OptionsPtr,
                        uint32_t /* Out */ KeyPtr);
};

class WasiSymmetricKeyImport
    : public WasiCryptoSymmetric<WasiSymmetricKeyImport> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        const_uint8_t_ptr AlgPtr,
                        __wasi_size_t AlgLen, const_uint8_t_ptr RawPtr,
                        __wasi_size_t RawLen, uint32_t /* Out */ KeyPtr);
};

class WasiSymmetricKeyExport
    : public WasiCryptoSymmetric<WasiSymmetricKeyExport> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_key_t SymmetricKey,
                        uint32_t /* Out */ ArrayOutputPtr);
};

class WasiSymmetricKeyClose
    : public WasiCryptoSymmetric<WasiSymmetricKeyClose> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_key_t SymmetricKey);
};

class WasiSymmetricKeyGenerateManaged
    : public WasiCryptoSymmetric<WasiSymmetricKeyGenerateManaged> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        const_uint8_t_ptr AlgPtr,
                        __wasi_size_t AlgLen, uint32_t OptOptionsPtr,
                        uint32_t /* Out */ KeyPtr);
};

class WasiSymmetricKeyStoreManaged
    : public WasiCryptoSymmetric<WasiSymmetricKeyStoreManaged> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        __wasi_symmetric_key_t SymmetricKey,
                        uint8_t_ptr SymmetricKeyIdPtr,
                        __wasi_size_t SymmetricKeyIdMaxLen);
};

class WasiSymmetricKeyReplaceManaged
    : public WasiCryptoSymmetric<WasiSymmetricKeyReplaceManaged> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        __wasi_symmetric_key_t SymmetricKeyOld,
                        __wasi_symmetric_key_t SymmetricKeyNew,
                        uint32_t /* Out */ VersionPtr);
};

class WasiSymmetricKeyId : public WasiCryptoSymmetric<WasiSymmetricKeyId> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_key_t SymmetricKey,
                        uint8_t_ptr SymmetricKeyId,
                        __wasi_size_t SymmetricKeyIdMaxLen,
                        uint32_t /* Out */ SizePtr,
                        uint32_t /* Out */ VersionPtr);
};

class WasiSymmetricKeyFromId
    : public WasiCryptoSymmetric<WasiSymmetricKeyFromId> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        uint8_t_ptr SymmetricKeyIdPtr,
                        __wasi_size_t SymmetricKeyIdLen,
                        __wasi_version_t SymmetricKeyVersion,
                        uint32_t /* Out */ KeyPtr);
};

class WasiSymmetricStateOpen
    : public WasiCryptoSymmetric<WasiSymmetricStateOpen> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        const_uint8_t_ptr AlgPtr,
                        __wasi_size_t AlgLen, uint32_t OptKeyPtr,
                        uint32_t OptOptionsPtr, uint32_t /* Out */ StatePtr);
};

class WasiSymmetricStateOptionsGet
    : public WasiCryptoSymmetric<WasiSymmetricStateOptionsGet> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, uint8_t_ptr ValuePtr,
                        __wasi_size_t ValueLen, uint32_t /* Out */ SizePtr);
};

class WasiSymmetricStateOptionsGetU64
    : public WasiCryptoSymmetric<WasiSymmetricStateOptionsGetU64> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr NamePtr, __wasi_size_t NameLen,
                        uint32_t /* Out */ U64Ptr);
};

class WasiSymmetricStateClose
    : public WasiCryptoSymmetric<WasiSymmetricStateClose> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle);
};

class WasiSymmetricStateAbsorb
    : public WasiCryptoSymmetric<WasiSymmetricStateAbsorb> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr DataPtr, __wasi_size_t DataLen);
};

class WasiSymmetricStateSqueeze
    : public WasiCryptoSymmetric<WasiSymmetricStateSqueeze> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen);
};

class WasiSymmetricStateSqueezeTag
    : public WasiCryptoSymmetric<WasiSymmetricStateSqueezeTag> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        uint32_t /* Out */ TagPtr);
};

class WasiSymmetricStateSqueezeKey
    : public WasiCryptoSymmetric<WasiSymmetricStateSqueezeKey> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr AlgPtr,
                        __wasi_size_t AlgLen, uint32_t /* Out */ KeyPtr);
};

class WasiSymmetricStateMaxTagLen
    : public WasiCryptoSymmetric<WasiSymmetricStateMaxTagLen> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        uint32_t /* Out */ SizePtr);
};

class WasiSymmetricStateEncrypt
    : public WasiCryptoSymmetric<WasiSymmetricStateEncrypt> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint32_t /* Out */ SizePtr);
};

class WasiSymmetricStateEncryptDetached
    : public WasiCryptoSymmetric<WasiSymmetricStateEncryptDetached> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint32_t /* Out */ KeyPtr);
};

class WasiSymmetricStateDecrypt
    : public WasiCryptoSymmetric<WasiSymmetricStateDecrypt> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint32_t /* Out */ SizePtr);
};

class WasiSymmetricStateDecryptDetached
    : public WasiCryptoSymmetric<WasiSymmetricStateDecryptDetached> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint8_t_ptr RawTagPtr,
                        __wasi_size_t RawTagLen, uint32_t /* Out */ SizePtr);
};

class WasiSymmetricStateRatchet
    : public WasiCryptoSymmetric<WasiSymmetricStateRatchet> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle);
};

class WasiSymmetricTagLen : public WasiCryptoSymmetric<WasiSymmetricTagLen> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_tag_t SymmetricTag,
                        uint32_t /* Out */ SizePtr);
};

class WasiSymmetricTagPull : public WasiCryptoSymmetric<WasiSymmetricTagPull> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_tag_t SymmetricTag, uint8_t_ptr BufPtr,
                        __wasi_size_t BufLen, uint32_t /* Out */ SizePtr);
};

class WasiSymmetricTagVerify
    : public WasiCryptoSymmetric<WasiSymmetricTagVerify> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_tag_t SymmetricTag,
                        uint8_t_ptr RawTagPtr, __wasi_size_t RawTagLen);
};

class WasiSymmetricTagClose
    : public WasiCryptoSymmetric<WasiSymmetricTagClose> {
public:
  using WasiCryptoSymmetric::WasiCryptoSymmetric;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,__wasi_symmetric_tag_t SymmetricTag);
};

} // namespace Host
} // namespace WasmEdge
