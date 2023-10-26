#include "avutil_func.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libavutil/time.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<void> AVLogSetLevel::body(const Runtime::CallingFrame &,
                                 int32_t LogLevelId) {
  av_log_set_level(LogLevelId);
  return {};
}

Expect<int32_t> AVLogGetLevel::body(const Runtime::CallingFrame &) {
  return av_log_get_level();
}

Expect<int32_t> AVLogGetFlags::body(const Runtime::CallingFrame &) {
  return av_log_get_flags();
}

Expect<void> AVLogSetFlags::body(const Runtime::CallingFrame &,
                                 int32_t FlagId) {
  av_log_set_flags(FlagId);
  return {};
}

Expect<int64_t> AVRescaleQ::body(const Runtime::CallingFrame &, int64_t A,
                                 int32_t BNum, int32_t BDen, int32_t CNum,
                                 int32_t CDen) {

  AVRational const B = av_make_q(BNum, BDen);
  AVRational const C = av_make_q(CNum, CDen);
  return av_rescale_q(A, B, C);
}

Expect<int64_t> AVRescaleQRnd::body(const Runtime::CallingFrame &, int64_t A,
                                    int32_t BNum, int32_t BDen, int32_t CNum,
                                    int32_t CDen, int32_t RoundingId) {

  AVRational const B = av_make_q(BNum, BDen);
  AVRational const C = av_make_q(CNum, CDen);
  AVRounding const Rounding = FFmpegUtils::Rounding::intoAVRounding(RoundingId);
  return av_rescale_q_rnd(A, B, C, Rounding);
}

Expect<uint32_t> AVUtilVersion::body(const Runtime::CallingFrame &) {
  return avutil_version();
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
