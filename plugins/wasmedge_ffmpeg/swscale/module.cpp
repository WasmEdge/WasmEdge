#include "module.h"
#include "swscale_func.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale{

WasmEdgeFFmpegSWScaleModule::
WasmEdgeFFmpegSWScaleModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_swscale"){

    addHostFunc("wasmedge_ffmpeg_swscale_sws_getContext",std::make_unique<SwsGetContext>(Env));
    addHostFunc("wasmedge_ffmpeg_swscale_sws_freeContext",std::make_unique<SwsFreeContext>(Env));
    addHostFunc("wasmedge_ffmpeg_swscale_sws_scale",std::make_unique<SwsScale>(Env));
    addHostFunc("wasmedge_ffmpeg_swscale_sws_getCachedContext",std::make_unique<SwsGetCachedContext>(Env));
    addHostFunc("wasmedge_ffmpeg_swscale_sws_isSupportedInput",std::make_unique<SwsIsSupportedInput>(Env));
    addHostFunc("wasmedge_ffmpeg_swscale_sws_isSupportedOutput",std::make_unique<SwsIsSupportedOutput>(Env));
    addHostFunc("wasmedge_ffmpeg_swscale_sws_isSupportedEndiannessConversion",std::make_unique<SwsIsSupportedEndiannessConversion>(Env));
}

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
