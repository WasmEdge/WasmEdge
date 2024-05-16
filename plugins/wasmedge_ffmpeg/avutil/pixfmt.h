#pragma once
#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AvPixFmtDescriptorNbComponents
    : public WasmEdgeFFmpegAVUtil<AvPixFmtDescriptorNbComponents> {
public:
  AvPixFmtDescriptorNbComponents(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class AvPixFmtDescriptorLog2ChromaW
    : public WasmEdgeFFmpegAVUtil<AvPixFmtDescriptorLog2ChromaW> {
public:
  AvPixFmtDescriptorLog2ChromaW(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class AvPixFmtDescriptorLog2ChromaH
    : public WasmEdgeFFmpegAVUtil<AvPixFmtDescriptorLog2ChromaH> {
public:
  AvPixFmtDescriptorLog2ChromaH(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

class AVColorRangeNameLength
    : public WasmEdgeFFmpegAVUtil<AVColorRangeNameLength> {
public:
  AVColorRangeNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t RangeId);
};

class AVColorRangeName : public WasmEdgeFFmpegAVUtil<AVColorRangeName> {
public:
  AVColorRangeName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t RangeId,
                       uint32_t RangeName, uint32_t RangeLength);
};

class AVColorTransferNameLength
    : public WasmEdgeFFmpegAVUtil<AVColorTransferNameLength> {
public:
  AVColorTransferNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t TransferId);
};

class AVColorTransferName : public WasmEdgeFFmpegAVUtil<AVColorTransferName> {
public:
  AVColorTransferName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t TransferId,
                       uint32_t TransferNamePtr, uint32_t TransferLength);
};

class AVColorSpaceNameLength
    : public WasmEdgeFFmpegAVUtil<AVColorSpaceNameLength> {
public:
  AVColorSpaceNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       int32_t ColorSpaceId);
};

class AVColorSpaceName : public WasmEdgeFFmpegAVUtil<AVColorSpaceName> {
public:
  AVColorSpaceName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ColorSpaceId,
                       uint32_t ColorSpaceNamePtr, uint32_t ColorSpaceLen);
};

class AVColorPrimariesNameLength
    : public WasmEdgeFFmpegAVUtil<AVColorPrimariesNameLength> {
public:
  AVColorPrimariesNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       int32_t ColorPrimariesId);
};

class AVColorPrimariesName : public WasmEdgeFFmpegAVUtil<AVColorPrimariesName> {
public:
  AVColorPrimariesName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       int32_t ColorPrimariesId, uint32_t ColorPrimariesNamePtr,
                       uint32_t ColorPrimariesLen);
};

class AVPixelFormatNameLength
    : public WasmEdgeFFmpegAVUtil<AVPixelFormatNameLength> {
public:
  AVPixelFormatNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvPixFormatId);
};

class AVPixelFormatName : public WasmEdgeFFmpegAVUtil<AVPixelFormatName> {
public:
  AVPixelFormatName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t PixFormatId,
                       uint32_t PixFormatNamePtr, uint32_t PixFormatNameLen);
};

class AVPixelFormatMask : public WasmEdgeFFmpegAVUtil<AVPixelFormatMask> {
public:
  AVPixelFormatMask(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t PixFormatId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
