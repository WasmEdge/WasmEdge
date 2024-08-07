// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avDevice_func.h"

extern "C" {
#include "libavdevice/avdevice.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVDevice {

Expect<void> AVDeviceRegisterAll::body(const Runtime::CallingFrame &) {
  avdevice_register_all();
  return {};
}

Expect<uint32_t> AVDeviceVersion::body(const Runtime::CallingFrame &) {
  return avdevice_version();
}

Expect<int32_t> AVDeviceListDevices::body(const Runtime::CallingFrame &Frame,
                                          uint32_t AVFormatCtxId,
                                          uint32_t AVDeviceInfoListPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AVDeviceInfoListId, MemInst, uint32_t, AVDeviceInfoListPtr, "")

  FFMPEG_PTR_FETCH(AvFormatCtx, AVFormatCtxId, AVFormatContext);

  AVDeviceInfoList **AvDeviceInfoList =
      static_cast<AVDeviceInfoList **>(av_malloc(sizeof(AVDeviceInfoList *)));

  int Res = avdevice_list_devices(AvFormatCtx, AvDeviceInfoList);
  FFMPEG_PTR_STORE(AvDeviceInfoList, AVDeviceInfoListId);
  return Res;
}

Expect<int32_t> AVInputAudioDeviceNext::body(const Runtime::CallingFrame &) {
  spdlog::error("[WasmEdge-FFmpeg] AVInputAudioDeviceNext unimplemented"sv);
  //  av_input_audio_device_next();
  return static_cast<int32_t>(ErrNo::UnImplemented);
}

Expect<int32_t> AVInputVideoDeviceNext::body(const Runtime::CallingFrame &) {
  spdlog::error("[WasmEdge-FFmpeg] AVInputVideoDeviceNext unimplemented"sv);
  //  av_input_video_device_next();
  return static_cast<int32_t>(ErrNo::UnImplemented);
}

Expect<int32_t> AVOutputAudioDeviceNext::body(const Runtime::CallingFrame &) {
  spdlog::error("[WasmEdge-FFmpeg] AVOutputAudioDeviceNext unimplemented"sv);
  //  av_output_audio_device_next();
  return static_cast<int32_t>(ErrNo::UnImplemented);
}

Expect<int32_t> AVOutputVideoDeviceNext::body(const Runtime::CallingFrame &) {
  spdlog::error("[WasmEdge-FFmpeg] AVOutputVideoDeviceNext unimplemented"sv);
  //  av_output_video_device_next();
  return static_cast<int32_t>(ErrNo::UnImplemented);
}

Expect<int32_t> AVDeviceFreeListDevices::body(const Runtime::CallingFrame &,
                                              uint32_t AVDeviceInfoListId) {
  FFMPEG_PTR_FETCH(AvDeviceInfoList, AVDeviceInfoListId, AVDeviceInfoList *);
  avdevice_free_list_devices(AvDeviceInfoList);
  FFMPEG_PTR_DELETE(AVDeviceInfoListId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVDeviceNbDevices::body(const Runtime::CallingFrame &,
                                        uint32_t AVDeviceInfoListId) {
  FFMPEG_PTR_FETCH(AvDeviceInfoList, AVDeviceInfoListId, AVDeviceInfoList *);
  return (*AvDeviceInfoList)->nb_devices;
}

Expect<int32_t> AVDeviceDefaultDevice::body(const Runtime::CallingFrame &,
                                            uint32_t AVDeviceInfoListId) {
  FFMPEG_PTR_FETCH(AvDeviceInfoList, AVDeviceInfoListId, AVDeviceInfoList *);
  return (*AvDeviceInfoList)->default_device;
}

Expect<int32_t>
AVDeviceConfigurationLength::body(const Runtime::CallingFrame &) {
  const char *Config = avdevice_configuration();
  return strlen(Config);
}

Expect<int32_t> AVDeviceConfiguration::body(const Runtime::CallingFrame &Frame,
                                            uint32_t ConfigPtr,
                                            uint32_t ConfigLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ConfigBuf, MemInst, char, ConfigPtr, ConfigLen, "");

  const char *Config = avdevice_configuration();
  std::copy_n(Config, ConfigLen, ConfigBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVDeviceLicenseLength::body(const Runtime::CallingFrame &) {
  const char *License = avdevice_license();
  return strlen(License);
}

Expect<int32_t> AVDeviceLicense::body(const Runtime::CallingFrame &Frame,
                                      uint32_t LicensePtr,
                                      uint32_t LicenseLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LicenseBuf, MemInst, char, LicensePtr, LicenseLen, "");

  const char *License = avdevice_license();
  std::copy_n(License, LicenseLen, LicenseBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVDevice
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
