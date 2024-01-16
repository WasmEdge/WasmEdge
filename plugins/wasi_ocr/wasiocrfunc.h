// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#pragma once

#include "runtime/callingframe.h"
#include "wasiocrbase.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasiOCRNumOfExtractions : public WasiOCR<WasiOCRNumOfExtractions> {
public:
  WasiOCRNumOfExtractions(WASIOCR::WasiOCREnvironment &HostEnv)
      : WasiOCR(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t ImagePathPtr,
                        uint32_t ImagePathLen);
};

class WasiOCRGetOutput : public WasiOCR<WasiOCRGetOutput> {
public:
  WasiOCRGetOutput(WASIOCR::WasiOCREnvironment &HostEnv) : WasiOCR(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t OutBufferPtr,
                        uint32_t OutBufferMaxSize);
};

} // namespace Host
} // namespace WasmEdge
