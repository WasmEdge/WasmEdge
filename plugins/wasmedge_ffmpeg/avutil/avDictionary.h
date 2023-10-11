#pragma once

#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVDictSet : public WasmEdgeFFmpegAVUtil<AVDictSet> {
public:
  AVDictSet(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId,
                       uint32_t KeyPtr, uint32_t KeyLen, uint32_t ValuePtr,
                       uint32_t ValueLen, int32_t Flags);
};

class AVDictGet : public WasmEdgeFFmpegAVUtil<AVDictGet> {
public:
  AVDictGet(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId,
                       uint32_t KeyPtr, uint32_t KeyLen,
                       uint32_t PrevDictEntryIdx, uint32_t Flags,
                       uint32_t KeyLenPtr, uint32_t ValueLenPtr);
};

class AVDictGetKeyValue : public WasmEdgeFFmpegAVUtil<AVDictGetKeyValue> {
public:
  AVDictGetKeyValue(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId,
                       uint32_t KeyPtr, uint32_t KeyLen, uint32_t ValBufPtr,
                       uint32_t ValBufLen, uint32_t KeyBufPtr,
                       uint32_t KeyBufLen, uint32_t PrevDictEntryIdx,
                       uint32_t Flags);
};

class AVDictCopy : public WasmEdgeFFmpegAVUtil<AVDictCopy> {
public:
  AVDictCopy(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestDictId,
                       uint32_t SrcDictId, uint32_t Flags);
};

class AVDictFree : public WasmEdgeFFmpegAVUtil<AVDictFree> {
public:
  AVDictFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DictId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
