#pragma once
#include "avutil_base.h"

#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVLogSetLevel : public WasmEdgeFFmpegAVUtil<AVLogSetLevel> {
public:
  AVLogSetLevel(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, int32_t LogLevelId);
};

class AVLogGetLevel : public WasmEdgeFFmpegAVUtil<AVLogGetLevel> {
public:
  AVLogGetLevel(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVLogSetFlags : public WasmEdgeFFmpegAVUtil<AVLogSetFlags> {
public:
  AVLogSetFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, int32_t FlagsId);
};

class AVLogGetFlags : public WasmEdgeFFmpegAVUtil<AVLogGetFlags> {
public:
  AVLogGetFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

// Option funcs.
class AVOptSetBin : public WasmEdgeFFmpegAVUtil<AVOptSetBin> {
public:
  AVOptSetBin(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSet : public WasmEdgeFFmpegAVUtil<AVOptSet> {
public:
  AVOptSet(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetInt : public WasmEdgeFFmpegAVUtil<AVOptSetInt> {
public:
  AVOptSetInt(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetDouble : public WasmEdgeFFmpegAVUtil<AVOptSetDouble> {
public:
  AVOptSetDouble(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetQ : public WasmEdgeFFmpegAVUtil<AVOptSetQ> {
public:
  AVOptSetQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetImageSize : public WasmEdgeFFmpegAVUtil<AVOptSetImageSize> {
public:
  AVOptSetImageSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetPixelFmt : public WasmEdgeFFmpegAVUtil<AVOptSetPixelFmt> {
public:
  AVOptSetPixelFmt(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetSampleFmt : public WasmEdgeFFmpegAVUtil<AVOptSetSampleFmt> {
public:
  AVOptSetSampleFmt(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVOptSetChannelLayout
    : public WasmEdgeFFmpegAVUtil<AVOptSetChannelLayout> {
public:
  AVOptSetChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVRescaleQ : public WasmEdgeFFmpegAVUtil<AVRescaleQ> {
public:
  AVRescaleQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame, int64_t A,
                       int32_t BNum, int32_t BDen, int32_t CNum, int32_t CDen);
};

class AVRescaleQRnd : public WasmEdgeFFmpegAVUtil<AVRescaleQRnd> {
public:
  AVRescaleQRnd(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &, int64_t A, int32_t BNum,
                       int32_t BDen, int32_t CNum, int32_t CDen,
                       int32_t RoundingId);
};

class AVUtilVersion : public WasmEdgeFFmpegAVUtil<AVUtilVersion> {
public:
  AVUtilVersion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &);
};

class AVGetChannelLayoutNbChannels
    : public WasmEdgeFFmpegAVUtil<AVGetChannelLayoutNbChannels> {
public:
  AVGetChannelLayoutNbChannels(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint64_t ChannelLayoutId);
};

class AVGetChannelLayoutNameLen
    : public WasmEdgeFFmpegAVUtil<AVGetChannelLayoutNameLen> {
public:
  AVGetChannelLayoutNameLen(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint64_t ChannelLayoutId);
};

class AVGetChannelLayoutName
    : public WasmEdgeFFmpegAVUtil<AVGetChannelLayoutName> {
public:
  AVGetChannelLayoutName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint64_t ChannelLayoutId, uint32_t NamePtr,
                       uint32_t NameLen);
};

class AVGetChannelLayoutMask
    : public WasmEdgeFFmpegAVUtil<AVGetChannelLayoutMask> {
public:
  AVGetChannelLayoutMask(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        uint64_t ChannelLayoutId);
};

class AVGetDefaultChannelLayout
    : public WasmEdgeFFmpegAVUtil<AVGetDefaultChannelLayout> {
public:
  AVGetDefaultChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint64_t> body(const Runtime::CallingFrame &Frame,
                        int32_t ChannelLayoutId);
};

class AVUtilConfigurationLength
    : public WasmEdgeFFmpegAVUtil<AVUtilConfigurationLength> {
public:
  AVUtilConfigurationLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVUtilConfiguration : public WasmEdgeFFmpegAVUtil<AVUtilConfiguration> {
public:
  AVUtilConfiguration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVUtilLicenseLength : public WasmEdgeFFmpegAVUtil<AVUtilLicenseLength> {
public:
  AVUtilLicenseLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVUtilLicense : public WasmEdgeFFmpegAVUtil<AVUtilLicense> {
public:
  AVUtilLicense(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
