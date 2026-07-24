// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avFilter.h"

extern "C" {
#include "libavfilter/avfilter.h"
}

#include <new>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

Expect<int32_t> AVFilterNameLength::body(const Runtime::CallingFrame &,
                                         uint32_t FilterId) {
  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, 0);
  return strlen(Filter->name);
}

Expect<int32_t> AVFilterName::body(const Runtime::CallingFrame &Frame,
                                   uint32_t FilterId, uint32_t NamePtr,
                                   uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");

  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, static_cast<int32_t>(ErrNo::InternalError));
  const char *Name = Filter->name;
  copyCStringToBuffer(NameBuf.data(), NameLen, Name);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterDescriptionLength::body(const Runtime::CallingFrame &,
                                                uint32_t FilterId) {
  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, 0);
  if (Filter->description == nullptr) {
    return 0;
  }
  return strlen(Filter->description);
}

Expect<int32_t> AVFilterDescription::body(const Runtime::CallingFrame &Frame,
                                          uint32_t FilterId, uint32_t DescPtr,
                                          uint32_t DescLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(DescBuf, MemInst, char, DescPtr, DescLen, "");

  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, static_cast<int32_t>(ErrNo::InternalError));
  const char *Desc = Filter->description;
  copyCStringToBuffer(DescBuf.data(), DescLen, Desc);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVFilterNbInputs::body(const Runtime::CallingFrame &,
                                        uint32_t FilterId) {
  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, 0);
  return Filter->nb_inputs;
}

Expect<uint32_t> AVFilterNbOutputs::body(const Runtime::CallingFrame &,
                                         uint32_t FilterId) {
  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, 0);
  return Filter->nb_outputs;
}

Expect<int32_t> AVFilterFlags::body(const Runtime::CallingFrame &,
                                    uint32_t FilterId) {
  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, 0);
  return Filter->flags;
}

Expect<int32_t> AVFilterInOutSetName::body(const Runtime::CallingFrame &Frame,
                                           uint32_t InOutId, uint32_t NamePtr,
                                           uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");

  FFMPEG_PTR_FETCH(InOut, InOutId, AVFilterInOut);
  FFMPEG_PTR_CHECK(InOut, static_cast<int32_t>(ErrNo::InternalError));

  std::string Name(NameBuf.data(), NameLen);
  char *CName = av_strdup(Name.c_str());
  if (CName == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  // The InOut node owns its name (avfilter_inout_free releases it with
  // av_freep), so drop any previous allocation before installing the new one.
  av_freep(&InOut->name);
  InOut->name = CName;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterInOutSetFilterCtx::body(const Runtime::CallingFrame &,
                                                uint32_t InOutId,
                                                uint32_t FilterCtxId) {
  FFMPEG_PTR_FETCH(InOut, InOutId, AVFilterInOut);
  FFMPEG_PTR_FETCH(FilterCtx, FilterCtxId, AVFilterContext);
  FFMPEG_PTR_CHECK(InOut, static_cast<int32_t>(ErrNo::InternalError));
  // id 0 clears filter_ctx; a nonzero id must resolve, else a stale id would be
  // silently treated as a clear.
  if (FilterCtxId != 0) {
    FFMPEG_PTR_CHECK(FilterCtx, static_cast<int32_t>(ErrNo::InternalError));
  }

  InOut->filter_ctx = FilterCtx;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterInOutSetPadIdx::body(const Runtime::CallingFrame &,
                                             uint32_t InOutId, int32_t PadIdx) {
  FFMPEG_PTR_FETCH(InOut, InOutId, AVFilterInOut);
  FFMPEG_PTR_CHECK(InOut, static_cast<int32_t>(ErrNo::InternalError));
  InOut->pad_idx = PadIdx;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVFilterInOutSetNext::body(const Runtime::CallingFrame &,
                                           uint32_t InOutId,
                                           uint32_t NextInOutId) {
  FFMPEG_PTR_FETCH(InOut, InOutId, AVFilterInOut);
  FFMPEG_PTR_FETCH(NextInOut, NextInOutId, AVFilterInOut);
  FFMPEG_PTR_CHECK(InOut, static_cast<int32_t>(ErrNo::InternalError));
  // id 0 clears the link; a nonzero id must resolve, else a stale id would be
  // silently treated as a clear and detach the previous next node.
  if (NextInOutId != 0) {
    FFMPEG_PTR_CHECK(NextInOut, static_cast<int32_t>(ErrNo::InternalError));
  }
  AVFilterInOut *const OldNext = InOut->next;
  // Relinking to the node already installed is a no-op: leave ownership and the
  // borrowed mark untouched so an idempotent call neither detaches nor rejects.
  if (OldNext == NextInOut) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  if (NextInOut != nullptr) {
    // The in/out nodes form a forest of disjoint singly linked chains; a next
    // node already owned by another chain (its id is borrowed) would gain two
    // owning heads. Reject it.
    if (Env.get()->isBorrowed(NextInOutId)) {
      return static_cast<int32_t>(ErrNo::InternalError);
    }
    // Reject a link that would close a cycle, which avfilter_inout_free()
    // would walk forever or free twice. The chain is acyclic (every prior
    // link passed this guard), so the walk terminates.
    for (AVFilterInOut *Node = NextInOut; Node != nullptr; Node = Node->next) {
      if (Node == InOut) {
        return static_cast<int32_t>(ErrNo::InternalError);
      }
    }
  }
  InOut->next = NextInOut;
  // Relinking detaches the previous next node; no chain head owns it anymore,
  // so restore its id's right to free it.
  if (OldNext != nullptr) {
    Env.get()->unmarkBorrowedByValue(OldNext);
  }
  // NextInOut is now owned by InOut's chain and released with its head, so
  // mark its id borrowed; a null next just clears the link.
  if (NextInOut != nullptr) {
    Env.get()->markBorrowed(NextInOutId);
  }
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVFilterGetInputsFilterPad::body(const Runtime::CallingFrame &Frame,
                                 uint32_t FilterId, uint32_t FilterPadPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(FilterPadId, MemInst, uint32_t, FilterPadPtr, "")

  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, static_cast<int32_t>(ErrNo::InternalError));
  const AVFilterPad *FilterPad = Filter->inputs;
  if (FilterPad == nullptr) {
    *FilterPadId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }
  auto *FilterPadHandle =
      new (std::nothrow) FilterPadView{FilterPad, Filter->nb_inputs};
  if (FilterPadHandle == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  FFMPEG_PTR_STORE(FilterPadHandle, FilterPadId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVFilterGetOutputsFilterPad::body(const Runtime::CallingFrame &Frame,
                                  uint32_t FilterId, uint32_t FilterPadPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(FilterPadId, MemInst, uint32_t, FilterPadPtr, "")

  FFMPEG_PTR_FETCH(Filter, FilterId, struct AVFilter);
  FFMPEG_PTR_CHECK(Filter, static_cast<int32_t>(ErrNo::InternalError));
  const AVFilterPad *FilterPad = Filter->outputs;
  if (FilterPad == nullptr) {
    *FilterPadId = 0;
    return static_cast<int32_t>(ErrNo::Success);
  }
  auto *FilterPadHandle =
      new (std::nothrow) FilterPadView{FilterPad, Filter->nb_outputs};
  if (FilterPadHandle == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  FFMPEG_PTR_STORE(FilterPadHandle, FilterPadId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
