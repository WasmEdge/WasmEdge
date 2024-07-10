// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasillmfunc.h"
#include "common/spdlog.h"

#include <string>
#include <string_view>

namespace WasmEdge {
namespace Host {

Expect<WASILLM::ErrNo>
WasiLLMModelCreate::bodyImpl(const Runtime::CallingFrame &Frame,
                             uint32_t CheckPointPath,
                             uint32_t CheckPointPathLen) {
  (void)Frame;
  (void)CheckPointPath;
  (void)CheckPointPathLen;
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMModelFree::bodyImpl(const Runtime::CallingFrame &Frame,
                           uint32_t ModelPtr) {
  (void)Frame;
  (void)ModelPtr;
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMDataLoaderCreate::bodyImpl(const Runtime::CallingFrame &Frame,
                                  uint32_t DataPath, uint32_t DataPathLen) {
  (void)Frame;
  (void)DataPath;
  (void)DataPathLen;
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMDataLoaderFree::bodyImpl(const Runtime::CallingFrame &Frame,
                                uint32_t DataLoaderPtr) {
  (void)Frame;
  (void)DataLoaderPtr;
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMTokenizerCreate::bodyImpl(const Runtime::CallingFrame &Frame,
                                 uint32_t FilePath, uint32_t FilePathLen) {
  (void)Frame;
  (void)FilePath;
  (void)FilePathLen;
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMTokenizerFree::bodyImpl(const Runtime::CallingFrame &Frame,
                               uint32_t TokenizerPtr) {
  (void)Frame;
  (void)TokenizerPtr;
  return WASILLM::ErrNo::InvalidArgument;
}

Expect<WASILLM::ErrNo>
WasiLLMModelTrain::bodyImpl(const Runtime::CallingFrame &Frame,
                            uint32_t ModelPtr, uint32_t TrainDataLoaderPtr,
                            uint32_t ValDataLoaderPtr, uint32_t TokenizerPtr,
                            uint32_t Lr, uint32_t Epoch) {
  (void)Frame;
  (void)ModelPtr;
  (void)TrainDataLoaderPtr;
  (void)ValDataLoaderPtr;
  (void)TokenizerPtr;
  (void)Lr;
  (void)Epoch;
  return WASILLM::ErrNo::InvalidArgument;
}

} // namespace Host
} // namespace WasmEdge
