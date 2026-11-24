// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_base.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

// Every function except abi_version returns an ErrNo.
class SDAbiVersion : public Func<SDAbiVersion> {
public:
  using Func::Func;
  Expect<uint32_t> body(const Runtime::CallingFrame &);
};

class SDSessionOpen : public Func<SDSessionOpen> {
public:
  using Func::Func;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                        uint32_t ConfigLen, uint32_t SessionPtr);
};

class SDSessionClose : public Func<SDSessionClose> {
public:
  using Func::Func;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId);
};

class SDGenerate : public Func<SDGenerate> {
public:
  using Func::Func;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t ReqPtr, uint32_t ReqLen, uint32_t BlobsPtr,
                        uint32_t BlobsLen, uint32_t CountPtr);
};

class SDResultInfo : public Func<SDResultInfo> {
public:
  using Func::Func;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t Index, uint32_t Format, uint32_t InfoPtr);
};

class SDResultRead : public Func<SDResultRead> {
public:
  using Func::Func;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t SessionId,
                        uint32_t Index, uint32_t Format, uint32_t BufPtr,
                        uint32_t BufLen, uint32_t WrittenPtr);
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
