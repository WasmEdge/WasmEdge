// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avfilter_func.h"
#include "avFilter.h"

extern "C" {
#include "libavfilter/avfilter.h"
}

#include <unordered_set>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

Expect<int32_t> AVFilterGraphAlloc::body(const Runtime::CallingFrame &Frame,
                                         uint32_t FilterGraphPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(FilterGraphId, MemInst, uint32_t, FilterGraphPtr, "")

  FFMPEG_PTR_FETCH(FilterGraph, *FilterGraphId, AVFilterGraph);

  FilterGraph = avfilter_graph_alloc();
  if (FilterGraph == nullptr) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  FFMPEG_PTR_STORE(FilterGraph, FilterGraphId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterGraphConfig::body(const Runtime::CallingFrame &,
                                          uint32_t FilterGraphId) {
  FFMPEG_PTR_FETCH(FilterGraph, FilterGraphId, AVFilterGraph);
  return avfilter_graph_config(FilterGraph,
                               nullptr); // log_ctx always NULL on Rust SDK.
}

Expect<int32_t> AVFilterGraphFree::body(const Runtime::CallingFrame &,
                                        uint32_t FilterGraphId) {
  FFMPEG_PTR_FETCH(FilterGraph, FilterGraphId, AVFilterGraph);
  avfilter_graph_free(&FilterGraph);
  // The graph owns its filter contexts, so every child context id minted by
  // AVFilterGraphCreateFilter or AVFilterGraphGetFilter is dropped with it;
  // otherwise a stale id would later resolve to a freed AVFilterContext.
  Env.get()->deallocChildren(FilterGraphId);
  FFMPEG_PTR_DELETE(FilterGraphId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterGraphGetFilter::body(const Runtime::CallingFrame &Frame,
                                             uint32_t FilterCtxPtr,
                                             uint32_t FilterGraphId,
                                             uint32_t NamePtr,
                                             uint32_t NameSize) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameId, MemInst, char, NamePtr, NameSize,
                 "Failed when accessing the return Name memory"sv);
  MEM_PTR_CHECK(FilterCtxId, MemInst, uint32_t, FilterCtxPtr, "");

  FFMPEG_PTR_FETCH(FilterGraph, FilterGraphId, AVFilterGraph);
  FFMPEG_PTR_CHECK(FilterGraph, static_cast<int32_t>(ErrNo::InternalError));
  FFMPEG_PTR_FETCH(FilterCtx, *FilterCtxId, AVFilterContext);

  std::string Name(NameId.data(), NameSize);

  FilterCtx = avfilter_graph_get_filter(FilterGraph, Name.c_str());
  if (FilterCtx == nullptr) {
    *FilterCtxId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }
  FFMPEG_PTR_STORE_CHILD(FilterCtx, FilterCtxId, FilterGraphId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterGraphParsePtr::body(const Runtime::CallingFrame &Frame,
                                            uint32_t FilterGraphId,
                                            uint32_t FiltersString,
                                            uint32_t FiltersSize,
                                            uint32_t InputsId,
                                            uint32_t OutputsId) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(FiltersId, MemInst, char, FiltersString, FiltersSize, "");

  FFMPEG_PTR_FETCH(Inputs, InputsId, AVFilterInOut);
  FFMPEG_PTR_FETCH(Outputs, OutputsId, AVFilterInOut);
  FFMPEG_PTR_FETCH(FiltersGraph, FilterGraphId, AVFilterGraph);
  FFMPEG_PTR_CHECK(FiltersGraph, static_cast<int32_t>(ErrNo::InternalError));

  // avfilter_graph_parse_ptr() takes ownership of the supplied in/out lists: it
  // frees the nodes it links into the graph and returns the still-open pads via
  // *Inputs/*Outputs. Record every original node first (a guest may chain
  // several via AVFilterInOutSetNext, each with its own id) so every handle id
  // that aliases them can be dropped afterwards; otherwise those ids would
  // resolve to freed memory and a later accessor would be a use-after-free. The
  // same walk rejects a cyclic chain or an Inputs/Outputs pair that shares a
  // node: passing one list as both arguments makes FFmpeg free a node through
  // one head while the other still references it, crashing the host.
  std::vector<AVFilterInOut *> Consumed;
  std::unordered_set<AVFilterInOut *> Seen;
  for (AVFilterInOut *Node = Inputs; Node != nullptr; Node = Node->next) {
    if (!Seen.insert(Node).second) {
      return static_cast<int32_t>(ErrNo::InternalError);
    }
    Consumed.push_back(Node);
  }
  for (AVFilterInOut *Node = Outputs; Node != nullptr; Node = Node->next) {
    if (!Seen.insert(Node).second) {
      return static_cast<int32_t>(ErrNo::InternalError);
    }
    Consumed.push_back(Node);
  }

  std::string Filters(FiltersId.data(), FiltersSize);
  int32_t const Res = avfilter_graph_parse_ptr(FiltersGraph, Filters.c_str(),
                                               &Inputs, &Outputs, nullptr);

  // On failure FFmpeg frees every filter context already in the graph (those
  // minted by AVFilterGraphCreateFilter or AVFilterGraphGetFilter included), so
  // drop their child handle ids; otherwise they would resolve to freed
  // AVFilterContext objects on a later call.
  if (Res < 0) {
    Env.get()->deallocChildren(FilterGraphId);
  }

  for (AVFilterInOut *const Node : Consumed) {
    Env.get()->deallocByValue(Node);
  }
  // The lists returned in *Inputs/*Outputs (surviving original nodes plus any
  // newly minted open-pad nodes) are unreachable from the guest now that their
  // ids are dropped, so free them here per the documented contract; otherwise
  // they leak.
  avfilter_inout_free(&Inputs);
  avfilter_inout_free(&Outputs);
  return Res;
}

Expect<int32_t> AVFilterInOutFree::body(const Runtime::CallingFrame &,
                                        uint32_t InOutId) {
  FFMPEG_PTR_FETCH(InOut, InOutId, AVFilterInOut);
  if (InOut == nullptr) {
    FFMPEG_PTR_DELETE(InOutId);
    return static_cast<int32_t>(ErrNo::Success);
  }
  // A node linked into another node's chain via AVFilterInOutSetNext is owned
  // by that chain's head; avfilter_inout_free() on the head releases the whole
  // list. Its id is marked borrowed, so refuse to free it on its own: freeing
  // it here would leave the head's ->next dangling and let the head's later
  // free double-free this node. It is released when the head is freed.
  if (Env.get()->isBorrowed(InOutId)) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  // avfilter_inout_free releases the entire linked list, so drop the handle
  // ids of every node in the chain, not just the head; otherwise an id for a
  // linked node would keep resolving to freed memory.
  for (AVFilterInOut *Node = InOut; Node != nullptr; Node = Node->next) {
    Env.get()->deallocByValue(Node);
  }
  avfilter_inout_free(&InOut);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVFilterVersion::body(const Runtime::CallingFrame &) {
  return avfilter_version();
}

Expect<int32_t> AVFilterGetByName::body(const Runtime::CallingFrame &Frame,
                                        uint32_t FilterPtr, uint32_t StrPtr,
                                        uint32_t StrLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(StrId, MemInst, char, StrPtr, StrLen,
                 "Failed when accessing the return Str memory"sv);
  MEM_PTR_CHECK(FilterId, MemInst, uint32_t, FilterPtr,
                "Failed when accessing the return Filter memory"sv);

  FFMPEG_PTR_FETCH(Filter, *FilterId, const struct AVFilter);
  std::string Name(StrId.data(), StrLen);

  Filter = avfilter_get_by_name(Name.c_str());
  if (Filter == nullptr) {
    *FilterId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }

  FFMPEG_PTR_STORE(const_cast<struct AVFilter *>(Filter), FilterId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVFilterConfigurationLength::body(const Runtime::CallingFrame &) {
  const char *Config = avfilter_configuration();
  return strlen(Config);
}

Expect<int32_t> AVFilterConfiguration::body(const Runtime::CallingFrame &Frame,
                                            uint32_t ConfigPtr,
                                            uint32_t ConfigLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ConfigBuf, MemInst, char, ConfigPtr, ConfigLen, "");

  const char *Config = avfilter_configuration();
  copyCStringToBuffer(ConfigBuf.data(), ConfigLen, Config);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterLicenseLength::body(const Runtime::CallingFrame &) {
  const char *License = avfilter_license();
  return strlen(License);
}

Expect<int32_t> AVFilterLicense::body(const Runtime::CallingFrame &Frame,
                                      uint32_t LicensePtr,
                                      uint32_t LicenseLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LicenseBuf, MemInst, char, LicensePtr, LicenseLen, "");

  const char *License = avfilter_license();
  copyCStringToBuffer(LicenseBuf.data(), LicenseLen, License);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterGraphCreateFilter::body(
    const Runtime::CallingFrame &Frame, uint32_t FilterCtxPtr,
    uint32_t FilterId, uint32_t NamePtr, uint32_t NameLen, uint32_t ArgsPtr,
    uint32_t ArgsLen, uint32_t FilterGraphId) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");
  MEM_SPAN_CHECK(ArgsBuf, MemInst, char, ArgsPtr, ArgsLen, "");
  MEM_PTR_CHECK(FilterCtxId, MemInst, uint32_t, FilterCtxPtr, "")

  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_FETCH(FilterGraph, FilterGraphId, AVFilterGraph);
  if (Filter == nullptr || FilterGraph == nullptr) {
    *FilterCtxId = 0;
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  std::string Name(NameBuf.data(), NameLen);
  std::string Args(ArgsBuf.data(), ArgsLen);

  AVFilterContext *FilterCtx = nullptr;
  int Res = avfilter_graph_create_filter(&FilterCtx, Filter, Name.c_str(),
                                         Args.c_str(), nullptr, FilterGraph);
  if (Res < 0) {
    *FilterCtxId = 0;
    return Res;
  }

  FFMPEG_PTR_STORE_CHILD(FilterCtx, FilterCtxId, FilterGraphId);
  return Res;
}

Expect<int32_t> AVFilterInOutAlloc::body(const Runtime::CallingFrame &Frame,
                                         uint32_t InOutPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(InOutId, MemInst, uint32_t, InOutPtr, "")

  FFMPEG_PTR_FETCH(InOut, *InOutId, AVFilterInOut);
  InOut = avfilter_inout_alloc();
  if (InOut == nullptr) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  FFMPEG_PTR_STORE(InOut, InOutId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterPadGetNameLength::body(const Runtime::CallingFrame &,
                                               uint32_t FilterPadId,
                                               int32_t Idx) {
  FFMPEG_PTR_FETCH(FilterPad, FilterPadId, FilterPadView);
  if (FilterPad == nullptr || Idx < 0 ||
      static_cast<unsigned>(Idx) >= FilterPad->NbPads) {
    return 0;
  }

  const char *Name = avfilter_pad_get_name(FilterPad->Pads, Idx);
  if (Name == nullptr) {
    return 0;
  }
  return strlen(Name);
}

Expect<int32_t> AVFilterPadGetName::body(const Runtime::CallingFrame &Frame,
                                         uint32_t FilterPadId, int32_t Idx,
                                         uint32_t NamePtr, uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");

  FFMPEG_PTR_FETCH(FilterPad, FilterPadId, FilterPadView);
  if (FilterPad == nullptr || Idx < 0 ||
      static_cast<unsigned>(Idx) >= FilterPad->NbPads) {
    spdlog::error("[WasmEdge-FFmpeg] AVFilterPadGetName: invalid filter pad "
                  "id {} or pad index {} (nb_pads={})"sv,
                  FilterPadId, Idx, FilterPad ? FilterPad->NbPads : 0);
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  const char *Name = avfilter_pad_get_name(FilterPad->Pads, Idx);
  if (!copyCStringToBuffer(NameBuf.data(), NameLen, Name)) {
    spdlog::error("[WasmEdge-FFmpeg] AVFilterPadGetName: pad name is null "
                  "(filter pad id {}, pad index {})"sv,
                  FilterPadId, Idx);
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterPadGetType::body(const Runtime::CallingFrame &,
                                         uint32_t FilterPadId, int32_t Idx) {
  FFMPEG_PTR_FETCH(FilterPad, FilterPadId, FilterPadView);
  if (FilterPad == nullptr || Idx < 0 ||
      static_cast<unsigned>(Idx) >= FilterPad->NbPads) {
    return FFmpegUtils::MediaType::fromMediaType(AVMEDIA_TYPE_UNKNOWN);
  }
  AVMediaType const MediaType = avfilter_pad_get_type(FilterPad->Pads, Idx);
  return FFmpegUtils::MediaType::fromMediaType(MediaType);
}

Expect<int32_t> AVFilterGraphDumpLength::body(const Runtime::CallingFrame &,
                                              uint32_t FilterGraphId) {
  FFMPEG_PTR_FETCH(FilterGraph, FilterGraphId, AVFilterGraph);
  FFMPEG_PTR_CHECK(FilterGraph, 0);
  std::unique_ptr<char, decltype(&av_free)> Graph{
      avfilter_graph_dump(FilterGraph, nullptr), &av_free};
  if (Graph == nullptr) {
    return 0;
  }
  return static_cast<int32_t>(strlen(Graph.get()));
}

Expect<int32_t> AVFilterGraphDump::body(const Runtime::CallingFrame &Frame,
                                        uint32_t FilterGraphId,
                                        uint32_t GraphStrPtr,
                                        uint32_t GraphStrLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(GraphStr, MemInst, char, GraphStrPtr, GraphStrLen, "");

  FFMPEG_PTR_FETCH(FilterGraph, FilterGraphId, AVFilterGraph);
  FFMPEG_PTR_CHECK(FilterGraph, static_cast<int32_t>(ErrNo::InternalError));

  std::unique_ptr<char, decltype(&av_free)> Graph{
      avfilter_graph_dump(FilterGraph, nullptr), &av_free};
  if (Graph == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  copyCStringToBuffer(GraphStr.data(), GraphStrLen, Graph.get());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterFreeGraphStr::body(const Runtime::CallingFrame &,
                                           uint32_t FilterGraphId) {
  FFMPEG_PTR_FETCH(FilterGraph, FilterGraphId, AVFilterGraph);
  FFMPEG_PTR_CHECK(FilterGraph, static_cast<int32_t>(ErrNo::Success));

  std::unique_ptr<char, decltype(&av_free)> Graph{
      avfilter_graph_dump(FilterGraph, nullptr), &av_free};
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterDrop::body(const Runtime::CallingFrame &,
                                   uint32_t FilterId) {
  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  if (Filter == nullptr) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  FFMPEG_PTR_DELETE(FilterId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterPadDrop::body(const Runtime::CallingFrame &,
                                      uint32_t FilterPadId) {
  FFMPEG_PTR_FETCH(FilterPad, FilterPadId, FilterPadView);
  if (FilterPad == nullptr) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  delete FilterPad;
  FFMPEG_PTR_DELETE(FilterPadId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterContextDrop::body(const Runtime::CallingFrame &,
                                          uint32_t FilterCtxId) {
  FFMPEG_PTR_FETCH(FilterCtx, FilterCtxId, AVFilterContext);
  if (FilterCtx == nullptr) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  FFMPEG_PTR_DELETE(FilterCtxId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
