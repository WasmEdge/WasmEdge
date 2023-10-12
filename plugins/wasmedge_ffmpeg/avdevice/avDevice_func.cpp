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
      (AVDeviceInfoList **)malloc(sizeof(AVDeviceInfoList **));

  int res = avdevice_list_devices(AvFormatCtx, AvDeviceInfoList);
  FFMPEG_PTR_STORE(AvDeviceInfoList, AVDeviceInfoListId);
  return res;
}

Expect<int32_t> AVInputAudioDeviceNext::body(const Runtime::CallingFrame &) {
  return 1;
  //  av_input_audio_device_next();
}

Expect<int32_t> AVInputVideoDeviceNext::body(const Runtime::CallingFrame &) {
  return 1;
  //  av_input_video_device_next();
}

Expect<int32_t> AVOutputAudioDeviceNext::body(const Runtime::CallingFrame &) {
  return 1;
  //  av_output_audio_device_next();
}

Expect<int32_t> AVOutputVideoDeviceNext::body(const Runtime::CallingFrame &) {
  return 1;
  //  av_output_video_device_next();
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

} // namespace AVDevice
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge