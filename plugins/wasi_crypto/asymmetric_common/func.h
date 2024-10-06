// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/asymmetric_common/func.h -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the asymmetric common host functions of wasi-crypto.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "utils/hostfunction.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace AsymmetricCommon {

class KeypairGenerate : public HostFunction<KeypairGenerate> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgType,
                        uint32_t AlgPtr, uint32_t AlgLen,
                        uint32_t OptOptionsHandlePtr,
                        uint32_t /* Out */ KpHandlePtr);
};

class KeypairImport : public HostFunction<KeypairImport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgType,
                        uint32_t AlgPtr, uint32_t AlgLen, uint32_t EncodedPtr,
                        uint32_t EncodedLen, uint32_t Encoding,
                        uint32_t /* Out */ KpHandlePtr);
};

class KeypairGenerateManaged : public HostFunction<KeypairGenerateManaged> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, uint32_t AlgType,
                        uint32_t AlgPtr, uint32_t AlgLen,
                        uint32_t OptOptionsHandlePtr, uint32_t KpHandlePtr);
};

class KeypairStoreManaged : public HostFunction<KeypairStoreManaged> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, int32_t KpHandle,
                        uint32_t KpIdPtr, uint32_t KpIdMaxLen);
};

class KeypairReplaceManaged : public HostFunction<KeypairReplaceManaged> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, int32_t OldKpHandle,
                        int32_t NewKpHandle, uint32_t /* Out */ KpVersionPtr);
};

class KeypairId : public HostFunction<KeypairId> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KpHandle,
                        uint32_t KpIdPtr, uint32_t KpIdMaxLen,
                        uint32_t /* Out */ SizePtr,
                        uint32_t /* Out */ KpVersionPtr);
};

class KeypairFromId : public HostFunction<KeypairFromId> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, uint32_t KpIdPtr,
                        uint32_t KpIdLen, uint64_t KpVersion,
                        uint32_t /* Out */ KpHandlePtr);
};

class KeypairFromPkAndSk : public HostFunction<KeypairFromPkAndSk> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t PkHandle,
                        int32_t SkHandle, uint32_t /* Out */ KpHandlePtr);
};

class KeypairExport : public HostFunction<KeypairExport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KpHandle,
                        uint32_t KpEncoding,
                        uint32_t /* Out */ ArrayOutputHandlePtr);
};

class KeypairPublickey : public HostFunction<KeypairPublickey> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KpHandle,
                        uint32_t /* Out */ PkHandlePtr);
};

class KeypairSecretkey : public HostFunction<KeypairSecretkey> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KpHandle,
                        uint32_t /* Out */ SkHandlePtr);
};

class KeypairClose : public HostFunction<KeypairClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KpHandle);
};

class PublickeyImport : public HostFunction<PublickeyImport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgType,
                        uint32_t AlgPtr, uint32_t AlgLen, uint32_t EncodedPtr,
                        uint32_t EncodedLen, uint32_t Encoding,
                        uint32_t /* Out */ PkHandlePtr);
};

class PublickeyExport : public HostFunction<PublickeyExport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t PkHandle,
                        uint32_t PkEncoding,
                        uint32_t /* Out */ ArrayOutputHandlePtr);
};

class PublickeyVerify : public HostFunction<PublickeyVerify> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t PkHandle);
};

class PublickeyFromSecretkey : public HostFunction<PublickeyFromSecretkey> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t SkHandle,
                        uint32_t /* Out */ PkHandlePtr);
};

class PublickeyClose : public HostFunction<PublickeyClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t PkHandle);
};

class SecretkeyImport : public HostFunction<SecretkeyImport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgType,
                        uint32_t AlgPtr, uint32_t AlgLen, uint32_t EncodedPtr,
                        uint32_t EncodedLen, uint32_t Encoding,
                        uint32_t /* Out */ SkHandlePtr);
};

class SecretkeyExport : public HostFunction<SecretkeyExport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t SkHandle,
                        uint32_t SkEncoding,
                        uint32_t /* Out */ ArrayOutputHandlePtr);
};

class SecretkeyClose : public HostFunction<SecretkeyClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t SkHandle);
};

} // namespace AsymmetricCommon
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
