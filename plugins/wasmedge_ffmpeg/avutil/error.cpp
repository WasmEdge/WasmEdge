// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "error.h"

extern "C" {
#include "libavutil/error.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t> AVUtilAVStrError::body(const Runtime::CallingFrame &Frame,
                                       int32_t ErrNum, uint32_t ErrBuf,
                                       uint32_t BufLen) {
  MEMINST_CHECK(MemInst, Frame, 0);

  MEM_PTR_CHECK(ErrId, MemInst, char, ErrBuf,
                "Failed when accessing the return URL memory"sv);

  std::string Error;
  std::copy_n(ErrId, BufLen, std::back_inserter(Error));
  return av_strerror(ErrNum, const_cast<char *>(Error.c_str()), BufLen);
}

Expect<int32_t> AVUtilAVError::body(const Runtime::CallingFrame &,
                                    int32_t ErrNum) {
  return AVERROR(ErrNum);
}

Expect<int32_t> AVUtilAVUNError::body(const Runtime::CallingFrame &,
                                      int32_t ErrNum) {
  return AVUNERROR(ErrNum);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
