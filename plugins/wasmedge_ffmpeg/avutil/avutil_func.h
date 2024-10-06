// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVLogSetLevel : public HostFunction<AVLogSetLevel> {
public:
  using HostFunction::HostFunction;
  Expect<void> body(const Runtime::CallingFrame &Frame, int32_t LogLevelId);
};

class AVLogGetLevel : public HostFunction<AVLogGetLevel> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVLogSetFlags : public HostFunction<AVLogSetFlags> {
public:
  using HostFunction::HostFunction;
  Expect<void> body(const Runtime::CallingFrame &Frame, int32_t FlagsId);
};

class AVLogGetFlags : public HostFunction<AVLogGetFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

// Option funcs.
class AVOptSetBin : public HostFunction<AVOptSetBin> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSet : public HostFunction<AVOptSet> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetInt : public HostFunction<AVOptSetInt> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetDouble : public HostFunction<AVOptSetDouble> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetQ : public HostFunction<AVOptSetQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetImageSize : public HostFunction<AVOptSetImageSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetPixelFmt : public HostFunction<AVOptSetPixelFmt> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetSampleFmt : public HostFunction<AVOptSetSampleFmt> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetChannelLayout : public HostFunction<AVOptSetChannelLayout> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVRescaleQ : public HostFunction<AVRescaleQ> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, int64_t A,
                       int32_t BNum, int32_t BDen, int32_t CNum, int32_t CDen);
};

class AVRescaleQRnd : public HostFunction<AVRescaleQRnd> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &, int64_t A, int32_t BNum,
                       int32_t BDen, int32_t CNum, int32_t CDen,
                       int32_t RoundingId);
};

class AVUtilVersion : public HostFunction<AVUtilVersion> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &);
};

class AVGetChannelLayoutNbChannels
    : public HostFunction<AVGetChannelLayoutNbChannels> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint64_t ChannelLayoutId);
};

class AVGetChannelLayoutNameLen
    : public HostFunction<AVGetChannelLayoutNameLen> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint64_t ChannelLayoutId);
};

class AVGetChannelLayoutName : public HostFunction<AVGetChannelLayoutName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint64_t ChannelLayoutId, uint32_t NamePtr,
                       uint32_t NameLen);
};

class AVGetChannelLayoutMask : public HostFunction<AVGetChannelLayoutMask> {
public:
  using HostFunction::HostFunction;
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        uint64_t ChannelLayoutId);
};

class AVGetDefaultChannelLayout
    : public HostFunction<AVGetDefaultChannelLayout> {
public:
  using HostFunction::HostFunction;
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        int32_t ChannelLayoutId);
};

class AVUtilConfigurationLength
    : public HostFunction<AVUtilConfigurationLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVUtilConfiguration : public HostFunction<AVUtilConfiguration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVUtilLicenseLength : public HostFunction<AVUtilLicenseLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVUtilLicense : public HostFunction<AVUtilLicense> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
