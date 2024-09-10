// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#pragma once

#include "common/spdlog.h"
#include "plugin/plugin.h"

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeOCR {

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  MissingMemory = 2,   // Caller module is missing a memory export.
  Busy = 3             // Device or resource busy.
};

class OCREnv {
public:
  OCREnv() noexcept {
    // check Tesseract API by initializing tesseract-ocr with English, without
    // specifying tessdata path
    if (TesseractApi->Init(NULL, "eng")) {
      spdlog::error(
          "[WasmEdge-OCR] Error occurred when initializing tesseract.");
    }
  }
  ~OCREnv() noexcept {
    if (TesseractApi) {
      TesseractApi->End();
      ;
    }
  }
  tesseract::TessBaseAPI *TesseractApi = new tesseract::TessBaseAPI();

  static Plugin::PluginRegister Register;
};

} // namespace WasmEdgeOCR
} // namespace Host
} // namespace WasmEdge
