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
                        uint32_t urlPtr, uint32_t urlSize,uint32_t avInputFormatId,uint32_t avDictionaryId){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(urlId,MemInst,char,urlPtr,"Failed when accessing the return SessionID memory",true);
  MEM_PTR_CHECK(avFormatCtxId,MemInst,uint32_t,avFormatCtxPtr,"Failed when accessing the return AVFormatContext Memory",true);

  std::string targetUrl;
  std::copy_n(urlId,urlSize,std::back_inserter(targetUrl));

  AVDictionary** avDictionary = NULL;
  AVInputFormat* avInputFormat = NULL;
  AVFormatContext* avFormatContext = NULL;

  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();
  if(avDictionaryId)
    avDictionary = static_cast<AVDictionary **>(ffmpegMemory->fetchData(avDictionaryId));
  if(avInputFormatId)
    avInputFormat = static_cast<AVInputFormat *>(ffmpegMemory->fetchData(avInputFormatId));

  // Think of proper error handling.
  printf("Executed all if statement\n");
  int res =  avformat_open_input(&avFormatContext,targetUrl.c_str(),avInputFormat,avDictionary);
  ffmpegMemory->alloc(avFormatContext,avFormatCtxId);
  return res;
}


Expect<int32_t>
AVFormatFindStreamInfo::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId,uint32_t avDictionaryId){
  printf("Inside Find Stream Info\n");

  AVDictionary** avDictionary = NULL;
  AVFormatContext* avFormatContext = NULL;
  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();
  if(avFormatCtxId == 0)
    return static_cast<int32_t>(ErrNo::InvalidArgument);

  avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
  if(avDictionaryId)
    avDictionary = static_cast<AVDictionary**>(ffmpegMemory->fetchData(avDictionaryId));

  return avformat_find_stream_info(avFormatContext,avDictionary);
}

Expect<void>
AVFormatCloseInput::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId) {
  if(!avFormatCtxId) {

  }

  WasmEdgeFFmpegEnv* ffmpegMemory = Env.get();
  AVFormatContext* avFormatContext =  static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
  avformat_close_input(&avFormatContext);
  ffmpegMemory->dealloc(avFormatCtxId);
  return {};
}

Expect<int32_t>
AVReadPause::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId) {

    auto* ffmpegMemory = Env.get();
    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
    return av_read_pause(avFormatContext);
}

Expect<int32_t>
AVReadPlay::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId) {

    auto* ffmpegMemory = Env.get();
    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
    return av_read_play(avFormatContext);
}

Expect<int32_t > AVFormatSeekFile::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId,uint32_t streamIdx,int64_t min_ts,int64_t ts,int64_t max_ts, int32_t flags){

    auto* ffmpegMemory = Env.get();
    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
    return avformat_seek_file(avFormatContext,streamIdx,min_ts,ts,max_ts,flags);
}

Expect<void>
AVDumpFormat::body(const Runtime::CallingFrame &Frame, uint32_t avFormatCtxId,int32_t idx,uint32_t urlPtr,uint32_t urlSize,int32_t isOutput) {
    std::string targetUrl;

    auto* MemInst = Frame.getMemoryByIndex(0);
    char* buf = MemInst->getPointer<char*>(urlPtr);
    std::copy_n(buf,urlSize,std::back_inserter(targetUrl));
    auto* ffmpegMemory = Env.get();

    if(!avFormatCtxId){
        // Error handling...
    }


    AVFormatContext* avFormatContext = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));

    av_dump_format(avFormatContext,idx,targetUrl.c_str(),isOutput);
    return {};
}

Expect<void>
AVFormatFreeContext::body(const Runtime::CallingFrame &, uint32_t avFormatCtxId) {

    auto* ffmpegMemory = Env.get();
    AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
    avformat_free_context(avFormatCtx);
    ffmpegMemory->dealloc(avFormatCtxId);
    return {};
}

Expect<int32_t> AVFindBestStream::body(const Runtime::CallingFrame &,uint32_t avFormatCtxId,int32_t mediaTypeId,int32_t wanted_stream,int32_t related_stream,uint32_t decoderRetId,int32_t flags){

    if(!avFormatCtxId){
      // Error handling
    }

    auto* ffmpegMemory = Env.get();
    const AVCodec** decoderRet = NULL;
    if(decoderRetId)
      decoderRet = static_cast<const AVCodec**>(ffmpegMemory->fetchData(decoderRetId));

    AVFormatContext* avFormatCtx = static_cast<AVFormatContext*>(ffmpegMemory->fetchData(avFormatCtxId));
    AVMediaType avMediaType = FFmpegUtils::MediaType::intoMediaType(mediaTypeId);
    return av_find_best_stream(avFormatCtx, avMediaType,wanted_stream,related_stream,decoderRet,flags);
}

}
}
}
}
