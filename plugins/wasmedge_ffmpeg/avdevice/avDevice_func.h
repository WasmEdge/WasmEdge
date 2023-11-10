#pragma once

#include "avDevice_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVDevice {

class AVDeviceRegisterAll : public WasmEdgeFFmpegAVDevice<AVDeviceRegisterAll> {
public:
  AVDeviceRegisterAll(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceVersion : public WasmEdgeFFmpegAVDevice<AVDeviceVersion> {
public:
  AVDeviceVersion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceListDevices : public WasmEdgeFFmpegAVDevice<AVDeviceListDevices> {
public:
  AVDeviceListDevices(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFormatCtxId, uint32_t AVDeviceInfoListPtr);
};

class AVInputAudioDeviceNext
    : public WasmEdgeFFmpegAVDevice<AVInputAudioDeviceNext> {
public:
  AVInputAudioDeviceNext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVInputVideoDeviceNext
    : public WasmEdgeFFmpegAVDevice<AVInputVideoDeviceNext> {
public:
  AVInputVideoDeviceNext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVOutputAudioDeviceNext
    : public WasmEdgeFFmpegAVDevice<AVOutputAudioDeviceNext> {
public:
  AVOutputAudioDeviceNext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVOutputVideoDeviceNext
    : public WasmEdgeFFmpegAVDevice<AVOutputVideoDeviceNext> {
public:
  AVOutputVideoDeviceNext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &);
};

class AVDeviceFreeListDevices
    : public WasmEdgeFFmpegAVDevice<AVDeviceFreeListDevices> {
public:
  AVDeviceFreeListDevices(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVDeviceInfoListId);
};

class AVDeviceNbDevices : public WasmEdgeFFmpegAVDevice<AVDeviceNbDevices> {
public:
  AVDeviceNbDevices(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVDeviceInfoListId);
};

class AVDeviceDefaultDevice
    : public WasmEdgeFFmpegAVDevice<AVDeviceDefaultDevice> {
public:
  AVDeviceDefaultDevice(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVDeviceInfoListId);
};

class AVDeviceConfigurationLength
    : public WasmEdgeFFmpegAVDevice<AVDeviceConfigurationLength> {
public:
  AVDeviceConfigurationLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceConfiguration
    : public WasmEdgeFFmpegAVDevice<AVDeviceConfiguration> {
public:
  AVDeviceConfiguration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVDeviceLicenseLength
    : public WasmEdgeFFmpegAVDevice<AVDeviceLicenseLength> {
public:
  AVDeviceLicenseLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVDeviceLicense : public WasmEdgeFFmpegAVDevice<AVDeviceLicense> {
public:
  AVDeviceLicense(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVDevice(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVDevice
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
