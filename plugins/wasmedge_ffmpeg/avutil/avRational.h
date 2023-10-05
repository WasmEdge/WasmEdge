#pragma once
#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVAddQ : public WasmEdgeFFmpegAVUtil<AVAddQ> {
public:
  AVAddQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVSubQ : public WasmEdgeFFmpegAVUtil<AVSubQ> {
public:
  AVSubQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVMulQ : public WasmEdgeFFmpegAVUtil<AVMulQ> {
public:
  AVMulQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVDivQ : public WasmEdgeFFmpegAVUtil<AVDivQ> {
public:
  AVDivQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVCmpQ : public WasmEdgeFFmpegAVUtil<AVCmpQ> {
public:
  AVCmpQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen);
};

class AVNearerQ : public WasmEdgeFFmpegAVUtil<AVNearerQ> {
public:
  AVNearerQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen, int32_t CNum,
                       int32_t CDen);
};

class AVQ2d : public WasmEdgeFFmpegAVUtil<AVQ2d> {
public:
  AVQ2d(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<double_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                        int32_t ADen);
};

class AVD2Q : public WasmEdgeFFmpegAVUtil<AVD2Q> {
public:
  AVD2Q(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, double_t D,
                       int32_t Max, uint32_t ANumPtr, uint32_t ADenPtr);
};

class AVQ2IntFloat : public WasmEdgeFFmpegAVUtil<AVQ2IntFloat> {
public:
  AVQ2IntFloat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                        int32_t ADen);
};

class AVInvQ : public WasmEdgeFFmpegAVUtil<AVInvQ> {
public:
  AVInvQ(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, uint32_t BNumPtr, uint32_t BDenPtr);
};

class AVReduce : public WasmEdgeFFmpegAVUtil<AVReduce> {
public:
  AVReduce(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ANumPtr,
                       uint32_t ADenPtr, int64_t BNum, int64_t BDen,
                       int64_t Max);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
