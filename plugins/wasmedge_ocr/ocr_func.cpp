// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "ocr_func.h"

#include "common/span.h"
#include "common/spdlog.h"
#include "host/filesystem.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeOCR {

namespace {

constexpr __wasi_filesize_t MaxImageSize = UINT32_C(1) << 30;

// Read the image of the guest path with the filesystem of the caller.
Pix *readImage(const Host::FileSystem *FS, std::string_view Path) {
  if (FS == nullptr) {
    return nullptr;
  }
  auto File = FS->open(Path, static_cast<__wasi_oflags_t>(0),
                       __WASI_RIGHTS_FD_READ, static_cast<__wasi_fdflags_t>(0));
  if (!File) {
    return nullptr;
  }
  // The size of the file is the offset of its end.
  auto Size = (*File)->seek(0, __WASI_WHENCE_END);
  if (!Size || *Size > MaxImageSize || !(*File)->seek(0, __WASI_WHENCE_SET)) {
    return nullptr;
  }
  std::vector<uint8_t> Data;
  try {
    Data.resize(static_cast<std::size_t>(*Size));
  } catch (const std::bad_alloc &) {
    return nullptr;
  }
  auto ReadSize = (*File)->read(Data);
  if (!ReadSize) {
    return nullptr;
  }
  return pixReadMem(Data.data(), *ReadSize);
}

} // namespace

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
  Pix *image = readImage(Frame.getFileSystem(),
                         std::string_view(ImagePtr.data(), ImagePtr.size()));
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
