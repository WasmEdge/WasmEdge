#include "module.h"
#include "avDevice_func.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVDevice {

WasmEdgeFFmpegAVDeviceModule::WasmEdgeFFmpegAVDeviceModule(
    std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_avdevice") {

  addHostFunc("wasmedge_ffmpeg_avdevice_avdevice_register_all",
              std::make_unique<AVDeviceRegisterAll>(Env));
  addHostFunc("wasmedge_ffmpeg_avdevice_avdevice_version",
              std::make_unique<AVDeviceVersion>(Env));
  addHostFunc("wasmedge_ffmpeg_avdevice_avdevice_list_devices",
              std::make_unique<AVDeviceListDevices>(Env));
  addHostFunc("wasmedge_ffmpeg_avdevice_avdevice_free_list_devices",
              std::make_unique<AVDeviceFreeListDevices>(Env));
  addHostFunc("wasmedge_ffmpeg_avdevice_avdevice_nb_devices",
              std::make_unique<AVDeviceNbDevices>(Env));
  addHostFunc("wasmedge_ffmpeg_avdevice_avdevice_default_device",
              std::make_unique<AVDeviceDefaultDevice>(Env));
}

} // namespace AVDevice
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
