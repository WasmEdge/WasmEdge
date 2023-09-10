#include "avStream.h"

extern "C" {
  #include "libavformat/avformat.h"
}

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVStreamId::body(const Runtime::CallingFrame &Frame, uint32_t avStreamPtr,uint32_t idx){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(avStreamId,MemInst,uint32_t,avStreamPtr,"Failed when accessing the return AVStream Memory",true);

  auto ffmpegMemory = Env.get();
  AVStream** avStream = static_cast<AVStream**>(ffmpegMemory->fetchData(*avStreamId));

  // No check here (Check)
  AVStream* requiredStream;
  for(unsigned int i=0;i<= idx;i++){
    requiredStream = *avStream;
    avStream++;
  }

  return requiredStream->id;
}

Expect<int32_t> AVStreamIndex::body(const Runtime::CallingFrame &Frame, uint32_t avStreamPtr,uint32_t idx){

    MEMINST_CHECK(MemInst,Frame,0);
    MEM_PTR_CHECK(avStreamId,MemInst,uint32_t,avStreamPtr,"Failed when accessing the return AVStream Memory",true);

    auto ffmpegMemory = Env.get();
    AVStream** avStream = static_cast<AVStream**>(ffmpegMemory->fetchData(*avStreamId));

    // No check here (Check)
    AVStream* requiredStream;
    for(unsigned int i=0;i<= idx;i++){
      requiredStream = *avStream;
      avStream++;
    }

    return requiredStream->index;
}

}
}
}
}
