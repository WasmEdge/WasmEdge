#pragma once

#include "plugin/plugin.h"

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

    printf("Len of linear memory %zu\n",ffmpegLinearMemory.size());
    ffmpegLinearMemory.push_back(data);
    *dataPtr = ffmpegLinearMemory.size() - 1;
  }

  void *fetchData(size_t index) {
    if(index >= ffmpegLinearMemory.size()){
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
    ffmpegLinearMemory[index] = nullptr;
  }

  void test(){
    printf("Len of linear memory %zu\n",ffmpegLinearMemory.size());
  }

  WasmEdgeFFmpegEnv() noexcept {}
private:
  // Can have separate linear memory for each submodule i.e avformat, avcodec, avutil, swresample, swscale, avdevice
  std::vector<void *> ffmpegLinearMemory;
  static Plugin::PluginRegister Register;
  static std::weak_ptr<WasmEdgeFFmpegEnv> Instance;
};

}
}
}