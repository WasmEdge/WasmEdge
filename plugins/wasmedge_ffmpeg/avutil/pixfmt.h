#pragma once
#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {
//
// class AVPixFmtDescGet : public WasmEdgeFFmpegAVUtil<AVPixFmtDescGet> {
// public:
//  AVPixFmtDescGet(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
//      : WasmEdgeFFmpegAVUtil(HostEnv) {}
//  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvPixFmtId,
//  uint32_t AvPixFmtDescPtr);
//};

class AvPixFmtDescriptorNbComponents
    : public WasmEdgeFFmpegAVUtil<AvPixFmtDescriptorNbComponents> {
public:
  AvPixFmtDescriptorNbComponents(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvPixFormatId);
};

class AvPixFmtDescriptorLog2ChromaW
    : public WasmEdgeFFmpegAVUtil<AvPixFmtDescriptorLog2ChromaW> {
public:
  AvPixFmtDescriptorLog2ChromaW(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvPixFormatId);
};

class AvPixFmtDescriptorLog2ChromaH
    : public WasmEdgeFFmpegAVUtil<AvPixFmtDescriptorLog2ChromaH> {
public:
  AvPixFmtDescriptorLog2ChromaH(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvPixFormatId);
};
//
// class AvPixFmtDescriptorName : public
// WasmEdgeFFmpegAVUtil<AvPixFmtDescriptorName> { public:
//  AvPixFmtDescriptorName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
//      : WasmEdgeFFmpegAVUtil(HostEnv) {}
//  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t
//  AvPixFormatId);
//};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
