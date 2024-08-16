// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

class AVFilterGraphAlloc : public HostFunction<AVFilterGraphAlloc> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphPtr);
};

class AVFilterGraphConfig : public HostFunction<AVFilterGraphConfig> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterGraphFree : public HostFunction<AVFilterGraphFree> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterGraphGetFilter : public HostFunction<AVFilterGraphGetFilter> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterCtxPtr, uint32_t FilterGraphId,
                       uint32_t NamePtr, uint32_t NameSize);
};

class AVFilterGraphParsePtr : public HostFunction<AVFilterGraphParsePtr> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId, uint32_t FiltersString,
                       uint32_t FiltersSize, uint32_t InputsId,
                       uint32_t OutputsId);
};

class AVFilterInOutFree : public HostFunction<AVFilterInOutFree> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId);
};

class AVFilterVersion : public HostFunction<AVFilterVersion> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFilterGetByName : public HostFunction<AVFilterGetByName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPtr,
                       uint32_t StrPtr, uint32_t StrLen);
};

class AVFilterConfigurationLength
    : public HostFunction<AVFilterConfigurationLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFilterConfiguration : public HostFunction<AVFilterConfiguration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVFilterLicenseLength : public HostFunction<AVFilterLicenseLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFilterLicense : public HostFunction<AVFilterLicense> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

class AVFilterGraphCreateFilter
    : public HostFunction<AVFilterGraphCreateFilter> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterCtxPtr, uint32_t FilterId,
                       uint32_t NamePtr, uint32_t NameLen, uint32_t ArgsPtr,
                       uint32_t ArgsLen, uint32_t FilterGraphId);
};

class AVFilterInOutAlloc : public HostFunction<AVFilterInOutAlloc> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutPtr);
};

class AVFilterPadGetNameLength : public HostFunction<AVFilterPadGetNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPadId,
                       int32_t Idx);
};

class AVFilterPadGetName : public HostFunction<AVFilterPadGetName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPadId,
                       int32_t Idx, uint32_t NamePtr, uint32_t NameLen);
};

class AVFilterPadGetType : public HostFunction<AVFilterPadGetType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPadId,
                       int32_t Idx);
};

class AVFilterGraphDumpLength : public HostFunction<AVFilterGraphDumpLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterGraphDump : public HostFunction<AVFilterGraphDump> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId, uint32_t GraphStrPtr,
                       uint32_t GraphStrLen);
};

class AVFilterFreeGraphStr : public HostFunction<AVFilterFreeGraphStr> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterDrop : public HostFunction<AVFilterDrop> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterPadDrop : public HostFunction<AVFilterPadDrop> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterPadId);
};

class AVFilterContextDrop : public HostFunction<AVFilterContextDrop> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterCtxId);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
