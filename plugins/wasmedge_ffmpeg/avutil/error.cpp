#include "error.h"

extern "C" {
    #include "libavutil/error.h"
    #include "libavutil/avutil.h"
}


namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil{

Expect<int32_t> AVUtilAVStrError::body(const Runtime::CallingFrame &Frame, int32_t errnum,uint32_t errbuf,uint32_t bufLen){

  auto* MemInst = Frame.getMemoryByIndex(0);
  auto buffer =  MemInst->getSpan<uint8_t>(errbuf,bufLen);
  return av_strerror(errnum, reinterpret_cast<char *>(buffer.data()),bufLen);
}

Expect<int32_t> AVUtilAVError::body(const Runtime::CallingFrame &Frame  __attribute__((unused)) , int32_t errnum){
  return AVERROR(errnum);
}

Expect<int32_t> AVUtilAVUNError::body(const Runtime::CallingFrame &Frame  __attribute__((unused)), int32_t errnum){
  return AVUNERROR(errnum);
}

}
}
}
}
