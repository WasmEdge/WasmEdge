#include "avStream.h"

extern "C" {
  #include "libavformat/avformat.h"
}

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVStreamId::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId,uint32_t streamIdx){

  auto ffmpegMemory = Env.get();
  AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));

  AVStream** avStream = avFormatCtx->streams;

  // No check here (Check)
  for(unsigned int i=1;i<= streamIdx;i++){
    avStream++;
  }

  return static_cast<AVStream*>(*avStream)->id;
}

Expect<int32_t> AVStreamIndex::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId,uint32_t streamIdx){

    auto ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));

    AVStream** avStream = avFormatCtx->streams;

    // No check here (Check)
    // Get the Requried AVStream
    for(unsigned int i=1;i<=streamIdx;i++){
      avStream++;
    }

    return static_cast<AVStream*>(*avStream)->index;
}

Expect<int32_t> AVStreamCodecPar::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxId,uint32_t streamIdx, uint32_t codecParameterPtr){
  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(codecParamId,MemInst,uint32_t,codecParameterPtr,"Failed when accessing the return CodecParameter Memory",true);

  auto ffmpegMemory = Env.get();
  AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));

  AVStream** avStream = avFormatCtx->streams;

  // No check here (Check)
  // Get the Requried AVStream
  for(unsigned int i=1;i<=streamIdx;i++){
    avStream++;
  }

  AVCodecParameters* codecpar =  (static_cast<AVStream*>(*avStream))->codecpar;

  ffmpegMemory->alloc(codecpar,codecParamId);
  return 0;
}

}
}
}
}
