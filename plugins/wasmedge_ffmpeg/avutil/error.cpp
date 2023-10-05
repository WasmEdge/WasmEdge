#include "error.h"

extern "C" {
#include "libavutil/error.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t> AVUtilAVStrError::body(const Runtime::CallingFrame &Frame,
                                       int32_t Errnum, uint32_t Errbuf,
                                       uint32_t BufLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(buffer, MemInst, uint8_t, Errbuf, BufLen, "");
  return av_strerror(Errnum, reinterpret_cast<char *>(buffer.data()), BufLen);
}

Expect<int32_t> AVUtilAVError::body(const Runtime::CallingFrame &Frame
                                    __attribute__((unused)),
                                    int32_t errnum) {
  return AVERROR(errnum);
}

Expect<int32_t> AVUtilAVUNError::body(const Runtime::CallingFrame &Frame
                                      __attribute__((unused)),
                                      int32_t errnum) {
  return AVUNERROR(errnum);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
