// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVUtilAVStrError : public HostFunction<AVUtilAVStrError> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ErrNum,
                       uint32_t ErrBuf, uint32_t BufLen);
};

class AVUtilAVError : public HostFunction<AVUtilAVError> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ErrNum);
};

class AVUtilAVUNError : public HostFunction<AVUtilAVUNError> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ErrNum);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
