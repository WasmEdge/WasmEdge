#include "avformat_func.h"

extern "C"{
  #include "libavformat/avformat.h"
}

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t>
AVFormatOpenInput::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,
                        uint32_t urlPtr, uint32_t urlSize,uint32_t avInputFormatPtr,uint32_t avDictionaryPtr){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(urlId,MemInst,char,urlPtr,"Failed when accessing the return SessionID memory",true);
  MEM_PTR_CHECK(avDictionaryId,MemInst,uint32_t,avDictionaryPtr,"Failed when accessing the return AVDictionary Memory",false);
  MEM_PTR_CHECK(avInputFormatId,MemInst,uint32_t,avInputFormatPtr,"Failed when accessing the return AVInputFormat Memory",false);
  MEM_PTR_CHECK(avFormatCtxId,MemInst,uint32_t,avFormatCtxPtr,"Failed when accessing the return AVFormatContext Memory",true);

  std::string targetUrl;
  std::copy_n(urlId,urlSize,std::back_inserter(targetUrl));

  AVDictionary** avDictionary = NULL;
  AVInputFormat* avInputFormat = NULL;
  AVFormatContext* avFormatContext = NULL;

  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();

  if(avDictionaryId != nullptr)
    avDictionary = static_cast<AVDictionary **>(ffmpegMemory->fetchData(*avDictionaryId));
  if(avInputFormatId != nullptr)
    avInputFormat = static_cast<AVInputFormat *>(ffmpegMemory->fetchData(*avInputFormatId));

  // Think of proper error handling.
  printf("Executed all if statement\n");
  int res =  avformat_open_input(&avFormatContext,targetUrl.c_str(),avInputFormat,avDictionary);
  ffmpegMemory->alloc(avFormatContext,avFormatCtxId);
  return res;
}


Expect<int32_t>
AVFormatFindStreamInfo::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t avDictionaryPtr){
  printf("Inside Find Stream Info\n");
  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(avFormatCtxId,MemInst,uint32_t,avFormatCtxPtr,"Failed when accessing the return AVFormatContext Memory",true);
  MEM_PTR_CHECK(avDictionaryId,MemInst,uint32_t,avDictionaryPtr,"Failed when accessing the return AVDictionary Memory",false);
  printf("AVFormatCtxIdx %d\n",*avFormatCtxId);

  AVDictionary** avDictionary = NULL;
  AVFormatContext* avFormatContext = NULL;

  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();

  avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxId));
  if(avDictionaryId != nullptr)
    avDictionary = static_cast<AVDictionary**>(ffmpegMemory->fetchData(*avDictionaryId));

  return avformat_find_stream_info(avFormatContext,avDictionary);
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

    MEMINST_CHECK(MemInst,Frame,0);
    MEM_PTR_CHECK(avFormatCtxId,MemInst,uint32_t,avFormatCtxPtr,"Failed when accessing the return AVFormatContext Memory",true);

    auto* ffmpegMemory = Env.get();
    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxId));
    return av_read_pause(avFormatContext);
}

Expect<int32_t>
AVReadPlay::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr) {
    MEMINST_CHECK(MemInst,Frame,0);
    MEM_PTR_CHECK(avFormatCtxId,MemInst,uint32_t,avFormatCtxPtr,"Failed when accessing the return AVFormatContext Memory",true);

    auto* ffmpegMemory = Env.get();
    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxId));
    return av_read_play(avFormatContext);
}

Expect<int32_t > AVFormatSeekFile::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxPtr,uint32_t streamIdx,int64_t min_ts,int64_t ts,int64_t max_ts, int32_t flags){

    MEMINST_CHECK(MemInst,Frame,0);
    MEM_PTR_CHECK(avFormatCtxId,MemInst,uint32_t,avFormatCtxPtr,"Failed when accessing the return AVFormatContext Memory",true);

    auto* ffmpegMemory = Env.get();
    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxId));
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

Expect<int32_t> AVFindBestStream::body(const Runtime::CallingFrame &Frame,uint32_t avFormatCtxPtr,int32_t mediaTypeId,int32_t wanted_stream,int32_t related_stream,uint32_t decoderRetPtr,int32_t flags){

    MEMINST_CHECK(MemInst,Frame,0);
    MEM_PTR_CHECK(avFormatCtxId,MemInst,uint32_t,avFormatCtxPtr,"Failed when accessing the return AVFormatContext Memory",true);
    MEM_PTR_CHECK(decoderRetId,MemInst,uint32_t,decoderRetPtr,"Failed when accessing the return AVCodec Memory",false);

    auto* ffmpegMemory = Env.get();
    const AVCodec** decoderRet = NULL;
    if(decoderRetId != nullptr)
      decoderRet = static_cast<const AVCodec**>(ffmpegMemory->fetchData(*decoderRetId));

    AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(*avFormatCtxId));
    AVMediaType avMediaType = FFmpegUtils::MediaType::getMediaType(mediaTypeId);
    return av_find_best_stream(avFormatCtx, avMediaType,wanted_stream,related_stream,decoderRet,flags);
}

}
}
}
}
