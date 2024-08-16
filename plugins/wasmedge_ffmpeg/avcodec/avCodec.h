// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

class AVCodecID : public HostFunction<AVCodecID> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecType : public HostFunction<AVCodecType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecMaxLowres : public HostFunction<AVCodecMaxLowres> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecCapabilities : public HostFunction<AVCodecCapabilities> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecGetNameLen : public HostFunction<AVCodecGetNameLen> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecGetName : public HostFunction<AVCodecGetName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVCodecGetLongNameLen : public HostFunction<AVCodecGetLongNameLen> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecGetLongName : public HostFunction<AVCodecGetLongName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t LongNamePtr, uint32_t LongNameLen);
};

class AVCodecProfiles : public HostFunction<AVCodecProfiles> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecPixFmtsIsNull : public HostFunction<AVCodecPixFmtsIsNull> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecPixFmtsIter : public HostFunction<AVCodecPixFmtsIter> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                        uint32_t Idx);
};

class AVCodecSupportedFrameratesIsNull
    : public HostFunction<AVCodecSupportedFrameratesIsNull> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecSupportedFrameratesIter
    : public HostFunction<AVCodecSupportedFrameratesIter> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t Idx, uint32_t NumPtr, uint32_t DenPtr);
};

class AVCodecSupportedSampleRatesIsNull
    : public HostFunction<AVCodecSupportedSampleRatesIsNull> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecSupportedSampleRatesIter
    : public HostFunction<AVCodecSupportedSampleRatesIter> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                       uint32_t Idx);
};

class AVCodecChannelLayoutIsNull
    : public HostFunction<AVCodecChannelLayoutIsNull> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecChannelLayoutIter : public HostFunction<AVCodecChannelLayoutIter> {
public:
  using HostFunction::HostFunction;
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                        uint32_t Idx);
};

class AVCodecSampleFmtsIsNull : public HostFunction<AVCodecSampleFmtsIsNull> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId);
};

class AVCodecSampleFmtsIter : public HostFunction<AVCodecSampleFmtsIter> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AvCodecId,
                        uint32_t Idx);
};

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
