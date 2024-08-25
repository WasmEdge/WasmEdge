// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVDictSet : public HostFunction<AVDictSet> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId,
                       uint32_t KeyPtr, uint32_t KeyLen, uint32_t ValuePtr,
                       uint32_t ValueLen, int32_t Flags);
};

class AVDictGet : public HostFunction<AVDictGet> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId,
                       uint32_t KeyPtr, uint32_t KeyLen,
                       uint32_t PrevDictEntryIdx, uint32_t Flags,
                       uint32_t KeyLenPtr, uint32_t ValueLenPtr);
};

class AVDictGetKeyValue : public HostFunction<AVDictGetKeyValue> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId,
                       uint32_t KeyPtr, uint32_t KeyLen, uint32_t ValBufPtr,
                       uint32_t ValBufLen, uint32_t KeyBufPtr,
                       uint32_t KeyBufLen, uint32_t PrevDictEntryIdx,
                       uint32_t Flags);
};

class AVDictCopy : public HostFunction<AVDictCopy> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestDictId,
                       uint32_t SrcDictId, uint32_t Flags);
};

class AVDictFree : public HostFunction<AVDictFree> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
