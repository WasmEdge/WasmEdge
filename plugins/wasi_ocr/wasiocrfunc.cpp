// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#include "wasiocrfunc.h"
#include "common/log.h"

#include <iostream>
#include <string>

namespace WasmEdge {
namespace Host {

Expect<uint32_t>
WasiOCRNumOfExtractions::body(const Runtime::CallingFrame &Frame,
                              uint32_t ImagePathPtr, uint32_t ImagePathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto ImagePtr = MemInst->getSpan<char>(ImagePathPtr, ImagePathLen);
  if (unlikely(ImagePtr.size() != ImagePathLen)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  Pix *image = pixRead(ImagePtr.data());

  Env.TesseractApi->SetImage(image);
  Env.TesseractApi->Recognize(0);

  tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
  const char *outText = Env.TesseractApi->GetTSVText(level);

  uint32_t length = strlen(outText);
  pixDestroy(&image);
  return static_cast<uint32_t>(length);
}

Expect<uint32_t> WasiOCRGetOutput::body(const Runtime::CallingFrame &Frame,
                                        uint32_t OutBufferPtr [[maybe_unused]],
                                        uint32_t OutBufferMaxSize
                                        [[maybe_unused]]) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check the return value: OutBufferPtr should be valid.
  auto Buf = MemInst->getSpan<char>(OutBufferPtr, OutBufferMaxSize);
  if (unlikely(Buf.empty())) {
    spdlog::error(
        "[WASI-OCR] Failed when accessing the return OutBufferPtr memory.");
    return static_cast<uint32_t>(WASIOCR::ErrNo::InvalidArgument);
  }

  tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
  const char *outText = Env.TesseractApi->GetTSVText(level);
  std::strcpy(Buf.data(), outText);

  // remaining free and deltee memory stuff
  Env.TesseractApi->End();
  delete[] outText; // USE WHEN USING TESS API

  return static_cast<uint32_t>(WASIOCR::ErrNo::Success);
  // return outText;
}

} // namespace Host
} // namespace WasmEdge
