#include "avfilter_func.h"

extern "C" {
#include "libavfilter/avfilter.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

Expect<int32_t> AVFilterGraphAlloc::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AVFilterGraphPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(AVFilterGraphId, MemInst, uint32_t, AVFilterGraphPtr, "")

  FFMPEG_PTR_FETCH(AvFilterGraph, *AVFilterGraphId, AVFilterGraph);

  AvFilterGraph = avfilter_graph_alloc();
  FFMPEG_PTR_STORE(AvFilterGraph, AVFilterGraphId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterGraphConfig::body(const Runtime::CallingFrame &,
                                          uint32_t AVFilterGraphId) {

  FFMPEG_PTR_FETCH(AvFilterGraph, AVFilterGraphId, AVFilterGraph);
  return avfilter_graph_config(AvFilterGraph,
                               NULL); // log_ctx always NULL on Rust SDK.
}

Expect<int32_t> AVFilterGraphFree::body(const Runtime::CallingFrame &,
                                        uint32_t AVFilterGraphId) {

  FFMPEG_PTR_FETCH(AvFilterGraph, AVFilterGraphId, AVFilterGraph);
  avfilter_graph_free(&AvFilterGraph);
  FFMPEG_PTR_DELETE(AVFilterGraphId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterGraphGetFilter::body(const Runtime::CallingFrame &Frame,
                                             uint32_t AVFilterCtxPtr,
                                             uint32_t AVFilterGraphId,
                                             uint32_t NamePtr,
                                             uint32_t NameSize) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(NameId, MemInst, char, NamePtr,
                "Failed when accessing the return Name memory");
  MEM_PTR_CHECK(AvFilterCtxId, MemInst, uint32_t, AVFilterCtxPtr, "");

  FFMPEG_PTR_FETCH(AvFilterGraph, AVFilterGraphId, AVFilterGraph);
  FFMPEG_PTR_FETCH(AvFilterCtx, *AvFilterCtxId, AVFilterContext);

  std::string Name;
  std::copy_n(NameId, NameSize, std::back_inserter(Name));

  AvFilterCtx = avfilter_graph_get_filter(AvFilterGraph, Name.c_str());
  FFMPEG_PTR_STORE(
      AvFilterCtx,
      AvFilterCtxId); // Not deleting this from MEMORY. WILL get AUTO Deleted
                      // from memory when app close. When AVFilterGraph is
                      // deleted, This points to NULL.
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterGraphParsePtr::body(const Runtime::CallingFrame &Frame,
                                            uint32_t AVFilterGraphId,
                                            uint32_t FiltersString,
                                            uint32_t FiltersSize,
                                            uint32_t InputsId,
                                            uint32_t OutputsId) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(FiltersId, MemInst, char, FiltersString, "");

  FFMPEG_PTR_FETCH(Inputs, InputsId, AVFilterInOut);
  FFMPEG_PTR_FETCH(Outputs, OutputsId, AVFilterInOut);
  FFMPEG_PTR_FETCH(AvFiltersGraph, AVFilterGraphId, AVFilterGraph);

  std::string Filters;
  std::copy_n(FiltersId, FiltersSize, std::back_inserter(Filters));
  int Res = avfilter_graph_parse_ptr(AvFiltersGraph, Filters.c_str(), &Inputs,
                                     &Outputs, NULL);
  return Res;
}

Expect<int32_t> AVFilterInOutFree::body(const Runtime::CallingFrame &,
                                        uint32_t InoutId) {

  FFMPEG_PTR_FETCH(InOuts, InoutId, AVFilterInOut);
  avfilter_inout_free(&InOuts);
  FFMPEG_PTR_DELETE(InoutId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVFilterVersion::body(const Runtime::CallingFrame &) {
  return avfilter_version();
}

Expect<int32_t> AVFilterGetByName::body(const Runtime::CallingFrame &Frame,
                                        uint32_t FilterPtr, uint32_t StrPtr,
                                        uint32_t StrLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(StrId, MemInst, char, StrPtr,
                "Failed when accessing the return Str memory");
  MEM_PTR_CHECK(FilterId, MemInst, uint32_t, FilterPtr,
                "Failed when accessing the return Filter memory");

  FFMPEG_PTR_FETCH(AvFilter, *FilterId, const struct AVFilter);
  std::string Name;
  std::copy_n(StrId, StrLen, std::back_inserter(Name));

  AvFilter = avfilter_get_by_name(Name.c_str());
  FFMPEG_PTR_STORE((void *)AvFilter, FilterId); // Check this...
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
