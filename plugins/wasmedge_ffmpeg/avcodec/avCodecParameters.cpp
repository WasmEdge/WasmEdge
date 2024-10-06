// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avCodecParameters.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVcodec {

Expect<uint32_t> AVCodecParamCodecId::body(const Runtime::CallingFrame &,
                                           uint32_t AvCodecParamId) {
  FFMPEG_PTR_FETCH(AvCodecParams, AvCodecParamId, AVCodecParameters);
  return FFmpegUtils::CodecID::fromAVCodecID(AvCodecParams->codec_id);
}

Expect<int32_t> AVCodecParamCodecType::body(const Runtime::CallingFrame &,
                                            uint32_t AvCodecParamId) {
  FFMPEG_PTR_FETCH(AvCodecParams, AvCodecParamId, AVCodecParameters);
  return FFmpegUtils::MediaType::fromMediaType(AvCodecParams->codec_type);
}

Expect<int32_t> AVCodecParamSetCodecTag::body(const Runtime::CallingFrame &,
                                              uint32_t AvCodecParamId,
                                              uint32_t CodecTag) {
  FFMPEG_PTR_FETCH(AvCodecParams, AvCodecParamId, AVCodecParameters);
  AvCodecParams->codec_tag = CodecTag;
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVcodec
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
