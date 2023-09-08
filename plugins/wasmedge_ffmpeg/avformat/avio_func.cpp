#include "avio_func.h"

extern "C"{
    #include "libavformat/avio.h"
    #include "libavformat/avformat.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<void> AVIOClose::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr){
    auto* MemInst = Frame.getMemoryByIndex(0);
    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);
    if(avFormatCtxIdx == nullptr){
      // Error Handling...
    }
    auto ffmpegMemory = Env.get();

    AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));
    avio_close(avFormatCtx->pb);
    return {};
}

}
}
}
}

