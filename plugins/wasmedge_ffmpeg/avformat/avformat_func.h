#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVFormatOpenInput : public WasmEdgeFFmpegAVFormat<AVFormatOpenInput> {
public:
  AVFormatOpenInput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr, uint32_t urlPtr, uint32_t urlSize,
                        uint32_t avInputFormatPtr,uint32_t avDictonaryPtr);
};

class AVFormatFindStreamInfo : public WasmEdgeFFmpegAVFormat<AVFormatFindStreamInfo> {
public:
  AVFormatFindStreamInfo(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avDictonaryPtr);
};

class AVFormatCloseInput : public WasmEdgeFFmpegAVFormat<AVFormatCloseInput> {
public:
    AVFormatCloseInput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};

class AVReadPause : public WasmEdgeFFmpegAVFormat<AVReadPause> {
public:
    AVReadPause(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};

class AVReadPlay : public WasmEdgeFFmpegAVFormat<AVReadPlay> {
public:
    AVReadPlay(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
            : WasmEdgeFFmpegAVFormat(HostEnv) {}
    Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
};
//
//class AVFormatSeekFile : public WasmEdgeFFmpegAVFormat<AVFormatSeekFile> {
//public:
//    AVFormatSeekFile(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
//            : WasmEdgeFFmpegAVFormat(HostEnv) {}
//    Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr);
//};
//
//class AVDumpFormat : public WasmEdgeFFmpegAVFormat<AVDumpFormat> {
//public:
//    AVDumpFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
//            : WasmEdgeFFmpegAVFormat(HostEnv) {}
//    Expect<int32_t > body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avInputFormatPtr);
//};
//
}
}
}
}
