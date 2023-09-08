#include "avformatContext.h"

extern "C" {
    #include "libavformat/avformat.h"
}

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {


Expect<int32_t> AVFormatCtxIFormat::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avInputFormatPtr){
    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);
    uint32_t* avInputFormatIdx = MemInst->getPointerOrNull<uint32_t*>(avInputFormatPtr);

    if(avFormatCtxIdx == nullptr) {
      // Error Handling...
    }


    auto ffmpegMemory = Env.get();

    AVFormatContext* avFormatCtx = static_cast<AVFormatContext *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    const AVInputFormat* avInputFormat = avFormatCtx->iformat;
    ffmpegMemory->alloc(const_cast<AVInputFormat*>(avInputFormat),avInputFormatIdx);

    // Think of strategy to return value.
    return 0;
}

Expect<int32_t> AVFormatCtxProbeScore::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr){

    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
      // Error Management
    }

    auto ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    return avFormatCtx->probe_score;
}


Expect<uint32_t> AVFormatCtxNbStreams::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr){
    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
      // Error Management
    }

    auto ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext *>(ffmpegMemory->fetchData(*avFormatCtxIdx));
    return avFormatCtx->nb_streams;
};


Expect<int64_t> AVFormatCtxBitRate::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr){
    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
      // Error Management
    }

    auto ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    return avFormatCtx->bit_rate;
}

Expect<int64_t> AVFormatCtxDuration::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr){
    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
      // Error Management
    }

    auto ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    return avFormatCtx->duration;
}

Expect<uint32_t> AVFormatCtxNbChapters::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr){
    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
      // Error Management
    }

    auto ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    return avFormatCtx->nb_chapters;
}

}
}
}
}

