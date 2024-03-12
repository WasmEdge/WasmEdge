#pragma once

#include "avfilter_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

class AVFilterGraphAlloc : public WasmEdgeFFmpegAVFilter<AVFilterGraphAlloc> {
public:
  AVFilterGraphAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphPtr);
};

class AVFilterGraphConfig : public WasmEdgeFFmpegAVFilter<AVFilterGraphConfig> {
public:
  AVFilterGraphConfig(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterGraphFree : public WasmEdgeFFmpegAVFilter<AVFilterGraphFree> {
public:
  AVFilterGraphFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterGraphGetFilter
    : public WasmEdgeFFmpegAVFilter<AVFilterGraphGetFilter> {
public:
  AVFilterGraphGetFilter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterCtxPtr, uint32_t FilterGraphId,
                       uint32_t NamePtr, uint32_t NameSize);
};

class AVFilterGraphParsePtr
    : public WasmEdgeFFmpegAVFilter<AVFilterGraphParsePtr> {
public:
  AVFilterGraphParsePtr(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId, uint32_t FiltersString,
                       uint32_t FiltersSize, uint32_t InputsId,
                       uint32_t OutputsId);
};

class AVFilterInOutFree : public WasmEdgeFFmpegAVFilter<AVFilterInOutFree> {
public:
  AVFilterInOutFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId);
};

class AVFilterVersion : public WasmEdgeFFmpegAVFilter<AVFilterVersion> {
public:
  AVFilterVersion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFilterGetByName : public WasmEdgeFFmpegAVFilter<AVFilterGetByName> {
public:
  AVFilterGetByName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPtr,
                       uint32_t StrPtr, uint32_t StrLen);
};

class AVFilterConfigurationLength
    : public WasmEdgeFFmpegAVFilter<AVFilterConfigurationLength> {
public:
  AVFilterConfigurationLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFilterConfiguration
    : public WasmEdgeFFmpegAVFilter<AVFilterConfiguration> {
public:
  AVFilterConfiguration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVFilterLicenseLength
    : public WasmEdgeFFmpegAVFilter<AVFilterLicenseLength> {
public:
  AVFilterLicenseLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFilterLicense : public WasmEdgeFFmpegAVFilter<AVFilterLicense> {
public:
  AVFilterLicense(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

class AVFilterGraphCreateFilter
    : public WasmEdgeFFmpegAVFilter<AVFilterGraphCreateFilter> {
public:
  AVFilterGraphCreateFilter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterCtxPtr, uint32_t FilterId,
                       uint32_t NamePtr, uint32_t NameLen, uint32_t ArgsPtr,
                       uint32_t ArgsLen, uint32_t FilterGraphId);
};

class AVFilterInOutAlloc : public WasmEdgeFFmpegAVFilter<AVFilterInOutAlloc> {
public:
  AVFilterInOutAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutPtr);
};

class AVFilterPadGetNameLength
    : public WasmEdgeFFmpegAVFilter<AVFilterPadGetNameLength> {
public:
  AVFilterPadGetNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPadId,
                       int32_t Idx);
};

class AVFilterPadGetName : public WasmEdgeFFmpegAVFilter<AVFilterPadGetName> {
public:
  AVFilterPadGetName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPadId,
                       int32_t Idx, uint32_t NamePtr, uint32_t NameLen);
};

class AVFilterPadGetType : public WasmEdgeFFmpegAVFilter<AVFilterPadGetType> {
public:
  AVFilterPadGetType(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterPadId,
                       int32_t Idx);
};

class AVFilterGraphDumpLength
    : public WasmEdgeFFmpegAVFilter<AVFilterGraphDumpLength> {
public:
  AVFilterGraphDumpLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterGraphDump : public WasmEdgeFFmpegAVFilter<AVFilterGraphDump> {
public:
  AVFilterGraphDump(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId, uint32_t GraphStrPtr,
                       uint32_t GraphStrLen);
};

class AVFilterFreeGraphStr
    : public WasmEdgeFFmpegAVFilter<AVFilterFreeGraphStr> {
public:
  AVFilterFreeGraphStr(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterGraphId);
};

class AVFilterDrop : public WasmEdgeFFmpegAVFilter<AVFilterDrop> {
public:
  AVFilterDrop(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterPadDrop : public WasmEdgeFFmpegAVFilter<AVFilterPadDrop> {
public:
  AVFilterPadDrop(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterPadId);
};

class AVFilterContextDrop : public WasmEdgeFFmpegAVFilter<AVFilterContextDrop> {
public:
  AVFilterContextDrop(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterCtxId);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
