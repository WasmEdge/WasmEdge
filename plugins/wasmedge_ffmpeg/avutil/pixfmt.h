// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AvPixFmtDescriptorNbComponents
    : public HostFunction<AvPixFmtDescriptorNbComponents> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class AvPixFmtDescriptorLog2ChromaW
    : public HostFunction<AvPixFmtDescriptorLog2ChromaW> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class AvPixFmtDescriptorLog2ChromaH
    : public HostFunction<AvPixFmtDescriptorLog2ChromaH> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class AVColorRangeNameLength : public HostFunction<AVColorRangeNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t RangeId);
};

class AVColorRangeName : public HostFunction<AVColorRangeName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t RangeId,
                       uint32_t RangeName, uint32_t RangeLength);
};

class AVColorTransferNameLength
    : public HostFunction<AVColorTransferNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t TransferId);
};

class AVColorTransferName : public HostFunction<AVColorTransferName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t TransferId,
                       uint32_t TransferNamePtr, uint32_t TransferLength);
};

class AVColorSpaceNameLength : public HostFunction<AVColorSpaceNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       int32_t ColorSpaceId);
};

class AVColorSpaceName : public HostFunction<AVColorSpaceName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ColorSpaceId,
                       uint32_t ColorSpaceNamePtr, uint32_t ColorSpaceLen);
};

class AVColorPrimariesNameLength
    : public HostFunction<AVColorPrimariesNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       int32_t ColorPrimariesId);
};

class AVColorPrimariesName : public HostFunction<AVColorPrimariesName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       int32_t ColorPrimariesId, uint32_t ColorPrimariesNamePtr,
                       uint32_t ColorPrimariesLen);
};

class AVPixelFormatNameLength : public HostFunction<AVPixelFormatNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvPixFormatId);
};

class AVPixelFormatName : public HostFunction<AVPixelFormatName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t PixFormatId,
                       uint32_t PixFormatNamePtr, uint32_t PixFormatNameLen);
};

class AVPixelFormatMask : public HostFunction<AVPixelFormatMask> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
