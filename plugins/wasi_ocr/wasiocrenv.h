// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#pragma once

#include "common/log.h"
#include "plugin/plugin.h"
#include <cstdint>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

namespace WasmEdge {
namespace Host {
namespace WASIOCR {

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  MissingMemory = 2,   // Caller module is missing a memory export.
  Busy = 3             // Device or resource busy.
};

class WasiOCREnvironment {
public:
  WasiOCREnvironment() noexcept {
    // check Tesseract API by initializing tesseract-ocr with English, without
    // specifying tessdata path
    if (TesseractApi->Init(NULL, "eng")) {
      spdlog::error("[WASI-OCR] Error occurred when initializing tesseract.");
    }
  }
  ~WasiOCREnvironment() noexcept {
    if (TesseractApi) {
      TesseractApi->End();
      ;
    }
  }
  tesseract::TessBaseAPI *TesseractApi = new tesseract::TessBaseAPI();

  static Plugin::PluginRegister Register;
};

} // namespace WASIOCR
} // namespace Host
} // namespace WasmEdge
