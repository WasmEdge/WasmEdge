
#include "avInputFormat.h"

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace AVFormat {

// Need to write logic to return string back to rust....
Expect<void> AVInputFormat_name::body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr) {
  auto* MemInst = Frame.getMemoryByIndex(0);
  uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avInputFormatPtr);
  if(avFormatCtxIdx == nullptr){
    // Error Handling...
  }
  auto ffmpegMemory = Env.get();

  struct AVInputFormat* avInputFormat = static_cast<struct AVInputFormat *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

  printf("%s",avInputFormat->mime_type);
  return {};
}

Expect<void> AVInputFormat_long_name::body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr) {

    auto* MemInst = Frame.getMemoryByIndex(0);
    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avInputFormatPtr);
    if(avFormatCtxIdx == nullptr){
        // Error Handling...
    }
    auto ffmpegMemory = Env.get();

    struct AVInputFormat* avInputFormat = static_cast<struct AVInputFormat *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    printf("%s",avInputFormat->mime_type);
    return {};
}

Expect<void> AVInputFormat_extensions::body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr) {

    auto* MemInst = Frame.getMemoryByIndex(0);
    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avInputFormatPtr);
    if(avFormatCtxIdx == nullptr){
        // Error Handling...
    }
    auto ffmpegMemory = Env.get();

    struct AVInputFormat* avInputFormat = static_cast<struct AVInputFormat *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    printf("%s",avInputFormat->mime_type);
    return {};
}

Expect<void> AVInputFormat_mime_type::body(const Runtime::CallingFrame &Frame, uint32_t avInputFormatPtr) {

    auto* MemInst = Frame.getMemoryByIndex(0);
    uint32_t* avFormatCtxIdx = MemInst->getPointerOrNull<uint32_t*>(avInputFormatPtr);
    if(avFormatCtxIdx == nullptr){
        // Error Handling...
    }
    auto ffmpegMemory = Env.get();

    struct AVInputFormat* avInputFormat = static_cast<struct AVInputFormat *>(ffmpegMemory->fetchData(*avFormatCtxIdx));

    printf("%s",avInputFormat->mime_type);
    return {};
}
}
}
}
}