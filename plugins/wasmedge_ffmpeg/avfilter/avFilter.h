// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

class AVFilterNameLength : public HostFunction<AVFilterNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterName : public HostFunction<AVFilterName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVFilterDescriptionLength
    : public HostFunction<AVFilterDescriptionLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterDescription : public HostFunction<AVFilterDescription> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t DescPtr, uint32_t DescLen);
};

class AVFilterNbInputs : public HostFunction<AVFilterNbInputs> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterNbOutputs : public HostFunction<AVFilterNbOutputs> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterFlags : public HostFunction<AVFilterFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterInOutSetName : public HostFunction<AVFilterInOutSetName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVFilterInOutSetFilterCtx
    : public HostFunction<AVFilterInOutSetFilterCtx> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       uint32_t FilterCtxId);
};

class AVFilterInOutSetPadIdx : public HostFunction<AVFilterInOutSetPadIdx> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       int32_t PadIdx);
};

class AVFilterInOutSetNext : public HostFunction<AVFilterInOutSetNext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       uint32_t NextInOutId);
};

class AVFilterGetInputsFilterPad
    : public HostFunction<AVFilterGetInputsFilterPad> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t FilterPadPtr);
};

class AVFilterGetOutputsFilterPad
    : public HostFunction<AVFilterGetOutputsFilterPad> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t FilterPadPtr);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
