// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "swresample_func.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample {

WasmEdgeFFmpegSWResampleModule::WasmEdgeFFmpegSWResampleModule(
    std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_swresample") {
  addHostFunc("wasmedge_ffmpeg_swresample_swresample_version",
              std::make_unique<SWResampleVersion>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swr_get_delay",
              std::make_unique<SWRGetDelay>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swr_init",
              std::make_unique<SWRInit>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swr_alloc_set_opts",
              std::make_unique<SWRAllocSetOpts>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_av_opt_set_dict",
              std::make_unique<AVOptSetDict>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swr_convert_frame",
              std::make_unique<SWRConvertFrame>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swr_free",
              std::make_unique<SWRFree>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swresample_configuration_length",
              std::make_unique<SWResampleConfigurationLength>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swresample_configuration",
              std::make_unique<SWResampleConfiguration>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swresample_license_length",
              std::make_unique<SWResampleLicenseLength>(Env));
  addHostFunc("wasmedge_ffmpeg_swresample_swresample_license",
              std::make_unique<SWResampleLicense>(Env));
}

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
