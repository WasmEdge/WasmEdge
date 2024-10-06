// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample {

class SWResampleVersion : public HostFunction<SWResampleVersion> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class SWRGetDelay : public HostFunction<SWRGetDelay> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SWRContextId, int64_t Base);
};

class SWRInit : public HostFunction<SWRInit> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SWRContextId);
};

class SWRAllocSetOpts : public HostFunction<SWRAllocSetOpts> {
public:
  using HostFunction::HostFunction;

  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SwrCtxPtr,
                       uint32_t SWRContextId, uint64_t OutChLayout,
                       uint32_t OutSampleFmtId, int32_t OutSampleRate,
                       uint64_t InChLayout, uint32_t InSampleFmtId,
                       int32_t InSampleRate, int32_t LogOffset);
};

class AVOptSetDict : public HostFunction<AVOptSetDict> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SWRContextId, uint32_t DictId);
};

class SWRConvertFrame : public HostFunction<SWRConvertFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SWRContextId, uint32_t FrameOutputId,
                       uint32_t FrameInputId);
};

class SWRFree : public HostFunction<SWRFree> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SWRContextId);
};

class SWResampleConfigurationLength
    : public HostFunction<SWResampleConfigurationLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class SWResampleConfiguration : public HostFunction<SWResampleConfiguration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class SWResampleLicenseLength : public HostFunction<SWResampleLicenseLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class SWResampleLicense : public HostFunction<SWResampleLicense> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
