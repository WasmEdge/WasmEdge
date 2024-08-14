// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "swscale_func.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale {

WasmEdgeFFmpegSWScaleModule::WasmEdgeFFmpegSWScaleModule(
    std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_swscale") {
  addHostFunc("wasmedge_ffmpeg_swscale_swscale_version",
              std::make_unique<SwscaleVersion>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_swscale_configuration_length",
              std::make_unique<SwscaleConfigurationLength>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_swscale_configuration",
              std::make_unique<SwscaleConfiguration>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_swscale_license_length",
              std::make_unique<SwscaleLicenseLength>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_swscale_license",
              std::make_unique<SwscaleLicense>(Env));

  // SwsContext
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getContext",
              std::make_unique<SwsGetContext>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_freeContext",
              std::make_unique<SwsFreeContext>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_scale",
              std::make_unique<SwsScale>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getCachedContext",
              std::make_unique<SwsGetCachedContext>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_isSupportedInput",
              std::make_unique<SwsIsSupportedInput>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_isSupportedOutput",
              std::make_unique<SwsIsSupportedOutput>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_isSupportedEndiannessConversion",
              std::make_unique<SwsIsSupportedEndiannessConversion>(Env));

  // SwsFilter
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getDefaultFilter",
              std::make_unique<SwsGetDefaultFilter>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getLumaH",
              std::make_unique<SwsGetLumaH>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getLumaV",
              std::make_unique<SwsGetLumaV>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getChromaH",
              std::make_unique<SwsGetChromaH>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getChromaV",
              std::make_unique<SwsGetChromaV>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_freeFilter",
              std::make_unique<SwsFreeFilter>(Env));

  // SwsVector
  addHostFunc("wasmedge_ffmpeg_swscale_sws_allocVec",
              std::make_unique<SwsAllocVec>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getGaussianVec",
              std::make_unique<SwsGetGaussianVec>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_scaleVec",
              std::make_unique<SwsScaleVec>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_normalizeVec",
              std::make_unique<SwsNormalizeVec>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getCoeffVecLength",
              std::make_unique<SwsGetCoeffVecLength>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_getCoeff",
              std::make_unique<SwsGetCoeff>(Env));
  addHostFunc("wasmedge_ffmpeg_swscale_sws_freeVec",
              std::make_unique<SwsFreeVec>(Env));
}

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
