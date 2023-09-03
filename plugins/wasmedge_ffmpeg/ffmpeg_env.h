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


  // Can improvise by implementing a queue or by using a hashmap.
  // For test, using a vector and appending it to end.
  void alloc(void *data) {
    ffmpegLinearMemory.push_back(data);
  }

  void *fetchData(size_t index) { return ffmpegLinearMemory[index]; }

  void dealloc(size_t index) {
    size_t memory_size = ffmpegLinearMemory.size();
    if (memory_size == 0) {
      // Memory is Empty.
    }

    if (index >= memory_size) {
      // Throw error.
    }

    ffmpegLinearMemory[index] = nullptr;
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