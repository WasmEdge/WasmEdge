// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#pragma once

#include "ocr_base.h"

#include "runtime/callingframe.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeOCR {

class NumOfExtractions : public HostFunction<NumOfExtractions> {
public:
  NumOfExtractions(OCREnv &HostEnv) : HostFunction(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t ImagePathPtr,
                        uint32_t ImagePathLen);
};

class GetOutput : public HostFunction<GetOutput> {
public:
  GetOutput(OCREnv &HostEnv) : HostFunction(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t OutBufferPtr,
                        uint32_t OutBufferMaxSize);
};

} // namespace WasmEdgeOCR
} // namespace Host
} // namespace WasmEdge
