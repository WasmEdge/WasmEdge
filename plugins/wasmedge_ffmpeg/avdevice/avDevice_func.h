// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVDevice {

class AVDeviceRegisterAll : public HostFunction<AVDeviceRegisterAll> {
public:
  using HostFunction::HostFunction;
  Expect<void> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceVersion : public HostFunction<AVDeviceVersion> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceListDevices : public HostFunction<AVDeviceListDevices> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFormatCtxId, uint32_t AVDeviceInfoListPtr);
};

class AVInputAudioDeviceNext : public HostFunction<AVInputAudioDeviceNext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVInputVideoDeviceNext : public HostFunction<AVInputVideoDeviceNext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVOutputAudioDeviceNext : public HostFunction<AVOutputAudioDeviceNext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVOutputVideoDeviceNext : public HostFunction<AVOutputVideoDeviceNext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVDeviceFreeListDevices : public HostFunction<AVDeviceFreeListDevices> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVDeviceInfoListId);
};

class AVDeviceNbDevices : public HostFunction<AVDeviceNbDevices> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVDeviceInfoListId);
};

class AVDeviceDefaultDevice : public HostFunction<AVDeviceDefaultDevice> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVDeviceInfoListId);
};

class AVDeviceConfigurationLength
    : public HostFunction<AVDeviceConfigurationLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceConfiguration : public HostFunction<AVDeviceConfiguration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVDeviceLicenseLength : public HostFunction<AVDeviceLicenseLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceLicense : public HostFunction<AVDeviceLicense> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVDevice
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
