#pragma once

#include "plugin/plugin.h"
#include "utils.h"

#include "vector"

namespace WasmEdge {
namespace Host {
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
  void alloc(void *data, uint32_t *dataPtr) {
    ffmpegPtrArr[ffmpegPtrArrKey++] = data;
    *dataPtr = ffmpegPtrArrKey - 1;
  }

  void *fetchData(const size_t index) {
    if (index >= ffmpegPtrArrKey) {
      // Error Handling...
    }
    // Check this condition.
    if (ffmpegPtrArr[index] == nullptr) {
      // Error Handling...
      return nullptr;
    }

    return ffmpegPtrArr[index];
  }

  void dealloc(size_t index) {
    if (ffmpegPtrArrKey == 0) {
      // Memory is Empty.
    }

    if (index >= ffmpegPtrArrKey) {
      // Error Handling...
    }

    // Do I need to clear??? Internally Eg: av_close_input(); will clear the
    // pointer value. Just the pointer exist. However, the rust side will face
    // problem as we have value to sm kind of index. Need to make that value
    // null(RUST).
    ffmpegPtrArr.erase(index);
  }

  WasmEdgeFFmpegEnv() noexcept {}

private:
  //  Using zero as NULL Value.
  uint32_t ffmpegPtrArrKey = 1;
  // Can update this to uint64_t to get more memory.
  std::map<uint32_t, void *> ffmpegPtrArr;
  static Plugin::PluginRegister Register;
  static std::weak_ptr<WasmEdgeFFmpegEnv> Instance;
};

// Utils functions.
#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-FFmpeg] Memory instance not found."sv);           \
    return static_cast<int32_t>(ErrNo::MissingMemory);                         \
  }

// If FFmpegStructID == 0, means Struct Pointer Doesn't exist in WasmEdge
// Plugin. Can Remove the If statement. Doesn't Matter I think.
#define FFMPEG_PTR_FETCH(StructPtr, FFmpegStructId, Type)                      \
  Type *StructPtr = NULL;                                                      \
  if (FFmpegStructId != 0)                                                     \
    StructPtr = static_cast<Type *>(Env.get()->fetchData(FFmpegStructId));

#define MEM_SPAN_CHECK(OutSpan, MemInst, Type, BufPtr, BufLen, Message)        \
  auto OutSpan = MemInst->getSpan<Type>(BufPtr, BufLen);                       \
  if (unlikely(OutSpan.size() != BufLen)) {                                    \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                             \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define FFMPEG_PTR_STORE(StructPtr, FFmpegStructId)                            \
  Env.get()->alloc(StructPtr, FFmpegStructId);

#define FFMPEG_PTR_DELETE(FFmpegStructId) Env.get()->dealloc(FFmpegStructId);

#define MEM_PTR_CHECK(OutPtr, MemInst, Type, Offset, Message)                  \
  Type *OutPtr = MemInst->getPointerOrNull<Type *>(Offset);                    \
  if (unlikely(OutPtr == nullptr)) {                                           \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                             \
    return static_cast<int32_t>(ErrNo::MissingMemory);                         \
  }

// Starting from 200 because, posix codes take values till 131.
// Hence using 200.
enum class ErrNo : int32_t {
  Success = 0,          // No error occurred.
  MissingMemory = -201, // Caller module is missing a memory export.
  NullStructId = -202,  // Rust Sdk Passes null id.
  InternalError = -203
};

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge