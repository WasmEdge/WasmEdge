#pragma once

#include "plugin/plugin.h"
#include "utils.h"

#include "vector"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {

class WasmEdgeFFmpegEnv {
public:
  // Singleton
  static std::shared_ptr<WasmEdgeFFmpegEnv> getInstance() noexcept {
    // Do I need a lock here???
    std::shared_ptr<WasmEdgeFFmpegEnv> envPtr = Instance.lock();
    if (!envPtr) {
      envPtr.reset(new WasmEdgeFFmpegEnv());
      Instance = envPtr;
    }
    return envPtr;
  }

  // Avoid copy constructor and overloading functions.
  WasmEdgeFFmpegEnv(const WasmEdgeFFmpegEnv &) = delete;
  void operator=(const WasmEdgeFFmpegEnv &) = delete;

  // Can improvise by implementing a queue or by using a hashmap.
  // For now, using a vector and appending it to end.
  void alloc(void* data,uint32_t* dataPtr) {

    ffmpegLinearMemory.push_back(data);
    *dataPtr = ffmpegLinearMemory.size() - 1;
  }

  void* fetchData(const size_t index) {
    if(index >= ffmpegLinearMemory.size()){
      // Error Handling...
    }
    if(ffmpegLinearMemory[index] == nullptr){
      // Error Handling...
    }

    return ffmpegLinearMemory[index];
  }

  void dealloc(size_t index) {
    size_t memory_size = ffmpegLinearMemory.size();
    if (memory_size == 0) {
      // Memory is Empty.
    }

    if (index >= memory_size) {
      // Throw error.
    }

  // Do I need to clear??? Internally Eg: av_close_input(); will clear the pointer value. Just the pointer exist. However, the rust side will face problem
  // as we have value to sm kind of index. Need to make that value null(RUST).
    // Frequently run a linear search to collect the spaces of index pointing to null pointer.
    ffmpegLinearMemory[index] = nullptr;
  }

  void test(){
    printf("Len of linear memory %zu\n",ffmpegLinearMemory.size());
  }

  WasmEdgeFFmpegEnv() noexcept {}


  // Starting from 200 because, posix codes take values till 131.
  // Hence using 200.
  enum class ErrNo : int32_t {
    Success = 0,         // No error occurred.
    InvalidArgument = -200, // Caller module passed an invalid argument.
    InvalidEncoding = -201, // Invalid encoding.
    MissingMemory = -202,   // Caller module is missing a memory export.
    Busy = -203,            // Device or resource busy.
    RuntimeError = -204,    // Runtime Error.
  };

#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
      auto *Out = CallFrame.getMemoryByIndex(Index);                               \
      if (unlikely(Out == nullptr)) {                                              \
        spdlog::error("[WasmEdge-FFmpeg] Memory instance not found."sv);           \
        return static_cast<int32_t >(WasmEdgeFFmpegEnv::ErrNo::MissingMemory);                        \
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

#define MEM_PTR_CHECK(OutPtr, MemInst, Type, Offset, Message,isRequired)                  \
      Type *OutPtr = MemInst->getPointerOrNull<Type *>(Offset);                                     \
      if (isRequired && unlikely(OutPtr == nullptr)) {                                           \
        spdlog::error("[WasmEdge-FFmpeg] "sv Message);                         \
        return static_cast<int32_t>(WasmEdgeFFmpegEnv::ErrNo::MissingMemory);                        \
      }

private:
  // Can have separate linear memory for each submodule i.e avformat, avcodec, avutil, swresample, swscale, avdevice
  std::vector<void *> ffmpegLinearMemory;
  static Plugin::PluginRegister Register;
  static std::weak_ptr<WasmEdgeFFmpegEnv> Instance;
};

}
}
}