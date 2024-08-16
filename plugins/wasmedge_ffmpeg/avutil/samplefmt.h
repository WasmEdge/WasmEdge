// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVGetPlanarSampleFmt : public HostFunction<AVGetPlanarSampleFmt> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t SampleFormatId);
};

class AVGetPackedSampleFmt : public HostFunction<AVGetPackedSampleFmt> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t SampleFormatId);
};

class AVSampleFmtIsPlanar : public HostFunction<AVSampleFmtIsPlanar> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t SampleFormatId);
};

class AVGetBytesPerSample : public HostFunction<AVGetBytesPerSample> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SampleFormatId);
};

class AVGetSampleFmt : public HostFunction<AVGetSampleFmt> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t Str,
                       uint32_t StrLen);
};

class AVSamplesGetBufferSize : public HostFunction<AVSamplesGetBufferSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t NbChannels,
                       int32_t NbSamples, uint32_t SampleFormatId,
                       int32_t Align);
};

class AVSamplesAllocArrayAndSamples
    : public HostFunction<AVSamplesAllocArrayAndSamples> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufferPtr,
                       uint32_t LinesizePtr, int32_t NbChannels,
                       int32_t NbSamples, uint32_t SampleFmtId, int32_t Align);
};

class AVGetSampleFmtNameLength : public HostFunction<AVGetSampleFmtNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SampleFmtId);
};

class AVGetSampleFmtName : public HostFunction<AVGetSampleFmtName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SampleFmtId,
                       uint32_t SampleFmtNamePtr, uint32_t SampleFmtNameLen);
};

class AVGetSampleFmtMask : public HostFunction<AVGetSampleFmtMask> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SampleFmtId);
};

class AVFreep : public HostFunction<AVFreep> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufferId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
