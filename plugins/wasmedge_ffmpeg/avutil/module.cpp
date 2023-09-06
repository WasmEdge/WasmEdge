#include "module.h"
#include "error.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg{
namespace AVUtil{

WasmEdgeFFmpegAVUtilModule::
    WasmEdgeFFmpegAVUtilModule(std::shared_ptr<WasmEdgeFFmpegEnv> Env)
    : ModuleInstance("wasmedge_ffmpeg_avutil"){

    addHostFunc("wasmedge_ffmpeg_avutil_av_strerror",
              std::make_unique<AVUtilAVStrError>(Env));

    addHostFunc("wasmedge_ffmpeg_avutil_AVERROR",
                std::make_unique<AVUtilAVError>(Env));

    addHostFunc("wasmedge_ffmpeg_avutil_AVUNERROR",
                std::make_unique<AVUtilAVUNError>(Env));
}

}
}
}
}
