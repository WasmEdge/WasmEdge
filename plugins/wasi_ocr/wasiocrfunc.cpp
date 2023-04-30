// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#include "wasiocrfunc.h"
#include "common/log.h"

#include <string>
#include <iostream>

namespace WasmEdge {
namespace Host {
    
Expect<uint32_t> WasiOCRNumOfExtractions::body(const Runtime::CallingFrame &Frame,
                    uint32_t ImagePathPtr,
                    uint32_t ImagePathLen){
                    // uint32_t NumOfExtractionsPtr){
// Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  // // Check the return value: NumOfExtractionsPtr should be valid.
  // uint32_t *NumOfExtractions = MemInst->getPointer<uint32_t *>(NumOfExtractionsPtr, 1);
  // if (unlikely(NumOfExtractions == nullptr)) {
  //   spdlog::error("[WASI-OCR] Failed when accessing the return NumOfExtractions memory.");
  //   return static_cast<uint32_t>(WASIOCR::ErrNo::InvalidArgument);
  // }
  // dont know if necessary, mostly is since first actually accesses the memory, second just creates a string from it
  char *ImagePtr = MemInst->getPointer<char *>(ImagePathPtr, ImagePathLen);
  Pix *image = pixRead(ImagePtr);

  Env.TesseractApi->SetImage(image);
  Env.TesseractApi->Recognize(0);

  tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
  const char *outText = Env.TesseractApi->GetTSVText(level);

  uint32_t length = strlen(outText);
  return static_cast<uint32_t>(length);
}

Expect<uint32_t> WasiOCRGetOutput::body(const Runtime::CallingFrame &Frame,
                                        uint32_t OutBufferPtr [[maybe_unused]],
                                        uint32_t OutBufferMaxSize [[maybe_unused]]){
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check the return value: OutBufferPtr should be valid.
  char *Buf = MemInst->getPointer<char *>(OutBufferPtr, OutBufferMaxSize);
  if (unlikely(Buf == nullptr)) {
    spdlog::error("[WASI-OCR] Failed when accessing the return OutBufferPtr memory.");
    return static_cast<uint32_t>(WASIOCR::ErrNo::InvalidArgument);
  }

  tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
  const char* outText = Env.TesseractApi->GetTSVText(level);
  std::strcpy(Buf,outText);

  // remaining free and deltee memory stuff

  return static_cast<uint32_t>(WASIOCR::ErrNo::Success);
  // return outText;
}

}
}
