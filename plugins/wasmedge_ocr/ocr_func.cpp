// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "ocr_func.h"

#include "common/spdlog.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeOCR {

Expect<uint32_t> NumOfExtractions::body(const Runtime::CallingFrame &Frame,
                                        uint32_t ImagePathPtr,
                                        uint32_t ImagePathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto ImagePtr = MemInst->getSpan<char>(ImagePathPtr, ImagePathLen);
  if (unlikely(ImagePtr.size() != ImagePathLen)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  const std::string ImagePath(ImagePtr.data(), ImagePtr.size());
  Pix *image = pixRead(ImagePath.c_str());
  if (image == nullptr) {
    spdlog::error("[WasmEdge-OCR] Failed to read the image file."sv);
    return static_cast<uint32_t>(0);
  }

  Env.TesseractApi->SetImage(image);
  Env.TesseractApi->Recognize(0);

  tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
  const std::unique_ptr<const char[]> outText(
      Env.TesseractApi->GetTSVText(level));
  pixDestroy(&image);
  if (!outText) {
    spdlog::error("[WasmEdge-OCR] Failed to recognize the image."sv);
    return static_cast<uint32_t>(0);
  }
  return static_cast<uint32_t>(std::strlen(outText.get()));
}

Expect<uint32_t> GetOutput::body(const Runtime::CallingFrame &Frame,
                                 uint32_t OutBufferPtr [[maybe_unused]],
                                 uint32_t OutBufferMaxSize [[maybe_unused]]) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check the return value: OutBufferPtr should be valid.
  auto Buf = MemInst->getSpan<char>(OutBufferPtr, OutBufferMaxSize);
  if (unlikely(Buf.empty())) {
    spdlog::error(
        "[WasmEdge-OCR] Failed when accessing the return OutBufferPtr memory."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }

  tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
  std::unique_ptr<const char[]> outText(Env.TesseractApi->GetTSVText(level));
  if (!outText) {
    spdlog::error("[WasmEdge-OCR] There is no recognized text to return."sv);
    return static_cast<uint32_t>(ErrNo::InvalidArgument);
  }
  std::copy_n(outText.get(),
              std::min<size_t>(std::strlen(outText.get()), Buf.size()),
              Buf.begin());

  // remaining free and deltee memory stuff
  Env.TesseractApi->End();

  return static_cast<uint32_t>(ErrNo::Success);
  // return outText;
}

} // namespace WasmEdgeOCR
} // namespace Host
} // namespace WasmEdge
