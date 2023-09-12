#include "avio_func.h"

extern "C"{
    #include "libavformat/avio.h"
    #include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<void> AVIOClose::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId){

    auto ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
    avio_close(avFormatCtx->pb);
    return {};
}

}
}
}
}

