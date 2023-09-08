#include "avformat_func.h"


extern "C"{
#include "libavformat/avformat.h"
}

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {


#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-FFmpeg] Memory instance not found."sv);       \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define SESSION_CHECK(Out, SessionID, Message, ErrNo)                          \
  auto *Out = Env.getContext(SessionID);                                       \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo);                                       \
  }

#define MEM_SPAN_CHECK(OutSpan, MemInst, Type, BufPtr, BufLen, Message)        \
  auto OutSpan = MemInst->getSpan<Type>(BufPtr, BufLen);                       \
  if (unlikely(OutSpan.size() != BufLen)) {                                    \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_SV_CHECK(OutSV, MemInst, BufPtr, BufLen, Message)                  \
  auto OutSV = MemInst->getStringView(BufPtr, BufLen);                         \
  if (unlikely(OutSV.size() != BufLen)) {                                      \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define MEM_PTR_CHECK(OutPtr, MemInst, Type, Offset, Message)                  \
  Type *OutPtr = MemInst->getPointer<Type *>(Offset);                          \
  if (unlikely(OutPtr == nullptr)) {                                           \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                         \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

Expect<int32_t>
AVFormatOpenInput::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,
                        uint32_t urlPtr, uint32_t urlSize,uint32_t avInputFormatPtr,uint32_t avDictonaryPtr){

  printf("I am inside AVFORMAT OPEN\n");
  std::string targetUrl;

  auto* MemInst = Frame.getMemoryByIndex(0);
  char* buf = MemInst->getPointer<char*>(urlPtr);

  if(buf == nullptr){
    // Handle exception.
  }
  std::copy_n(buf,urlSize,std::back_inserter(targetUrl));

  uint32_t* avDictonaryIdx = MemInst->getPointerOrNull<uint32_t*>(avDictonaryPtr);
  uint32_t* avInputFormatIdx = MemInst->getPointerOrNull<uint32_t*>(avInputFormatPtr);
  uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

  AVDictionary** avDictionary = NULL;
  AVInputFormat* avInputFormat = NULL;
  AVFormatContext* avFormatContext = NULL;

  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();

  if(avDictonaryIdx != nullptr)
    avDictionary = static_cast<AVDictionary **>(ffmpegMemory->fetchData(*avDictonaryIdx));
  if(avInputFormatIdx != nullptr)
    avInputFormat = static_cast<AVInputFormat *>(ffmpegMemory->fetchData(*avInputFormatIdx));

  // Think of proper error handling.
  printf("Executed all if statement\n");
  int res =  avformat_open_input(&avFormatContext,targetUrl.c_str(),avInputFormat,avDictionary);
  ffmpegMemory->alloc(avFormatContext,avFormatCtxIdx);
  return res;
}


Expect<int32_t>
AVFormatFindStreamInfo::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avDictionaryPtr){
  printf("Inside Find Stream Info\n");
  auto* MemInst = Frame.getMemoryByIndex(0);

  uint32_t* avDictionaryIdx = MemInst->getPointerOrNull<uint32_t*>(avDictionaryPtr);
  uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);
  printf("AVFormatCtxIdx %d\n",*avFormatCtxIdx);

  AVDictionary** avDictionary = NULL;
  AVFormatContext* avFormatContext = NULL;

  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();
  if(avFormatCtxIdx == nullptr){
      // Error Handling...
  }

    avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));
  if(avDictionaryIdx != nullptr)
    avDictionary = static_cast<AVDictionary**>(ffmpegMemory->fetchData(*avDictionaryIdx));

  int res = avformat_find_stream_info(avFormatContext,avDictionary);
  printf("Results: %d\n",res);

  // Need to see if error handling on rust side or c++ side.
  return res;
}

Expect<void>
AVFormatCloseInput::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr) {

  auto* MemInst = Frame.getMemoryByIndex(0);
  uint32_t* avFormatCtxIdx = MemInst->getPointer<uint32_t*>(avFormatCtxPtr);

  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();

  if(avFormatCtxIdx == nullptr){
    // Error Handling...
  }

  AVFormatContext* avFormatContext =  static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));
  avformat_close_input(&avFormatContext);

  return {};
}

Expect<int32_t>
AVReadPause::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr) {
    auto* MemInst = Frame.getMemoryByIndex(0);
    auto* ffmpegMemory = Env.get();

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
        // Error handling...
    }

    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    return av_read_pause(avFormatContext);
}

Expect<int32_t>
AVReadPlay::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr) {
    auto* MemInst = Frame.getMemoryByIndex(0);
    auto* ffmpegMemory = Env.get();

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
        // Error handling...
    }

    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    return av_read_play(avFormatContext);
}

Expect<int32_t > AVFormatSeekFile::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t streamIdx,int64_t min_ts,int64_t ts,int64_t max_ts, int32_t flags){

    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);
    auto* ffmpegMemory = Env.get();
    if(avFormatCtxIdx == nullptr){
        // Error handling...
    }

    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    return avformat_seek_file(avFormatContext,streamIdx,min_ts,ts,max_ts,flags);
}

Expect<void>
AVDumpFormat::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,int32_t idx,uint32_t urlPtr,uint32_t urlSize,int32_t isOutput) {
    std::string targetUrl;

    auto* MemInst = Frame.getMemoryByIndex(0);
    char* buf = MemInst->getPointer<char*>(urlPtr);
    std::copy_n(buf,urlSize,std::back_inserter(targetUrl));
    auto* ffmpegMemory = Env.get();

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);

    if(avFormatCtxIdx == nullptr){
        // Error handling...
    }


    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    av_dump_format(avFormatContext,idx,targetUrl.c_str(),isOutput);
    return {};
}

Expect<void>
AVFormatFreeContext::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr) {
    auto* MemInst = Frame.getMemoryByIndex(0);

    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avFormatCtxPtr);
    if(avFormatCtxIdx == nullptr){
        // Error handling...
    }

    auto* ffmpegMemory = Env.get();

    AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    avformat_free_context(avFormatCtx);
    return {};
}

}
}
}
}
