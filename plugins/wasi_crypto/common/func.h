// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/common/func.h - Common func ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the common host functions of wasi-crypto.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/hostfunction.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Common {

class ArrayOutputLen : public HostFunction<ArrayOutputLen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t ArrayOutputHandle, uint32_t /* Out */ SizePtr);
};

class ArrayOutputPull : public HostFunction<ArrayOutputPull> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t ArrayOutputHandle, uint32_t BufPtr,
                        uint32_t BufLen, uint32_t /* Out */ SizePtr);
};

class OptionsOpen : public HostFunction<OptionsOpen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgType,
                        uint32_t /* Out */ OptionsHandlePtr);
};

class OptionsClose : public HostFunction<OptionsClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t OptionsHandle);
};

class OptionsSet : public HostFunction<OptionsSet> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t OptionsHandle, uint32_t NamePtr,
                        uint32_t NameLen, uint32_t ValuePtr, uint32_t ValueLen);
};

class OptionsSetU64 : public HostFunction<OptionsSetU64> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t OptionsHandle, uint32_t NamePtr,
                        uint32_t NameLen, uint64_t Value);
};

class OptionsSetGuestBuffer : public HostFunction<OptionsSetGuestBuffer> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t OptionsHandle, uint32_t NamePtr,
                        uint32_t NameLen, uint32_t BufPtr, uint32_t BufLen);
};

class SecretsManagerOpen : public HostFunction<SecretsManagerOpen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t OptOptionsHandlePtr,
                        uint32_t /* Out */ SecretsManagerHandlePtr);
};

class SecretsManagerClose : public HostFunction<SecretsManagerClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle);
};

class SecretsManagerInvalidate : public HostFunction<SecretsManagerInvalidate> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, uint32_t KeyIdPtr,
                        uint32_t KeyIdLen, uint64_t Version);
};

} // namespace Common
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
