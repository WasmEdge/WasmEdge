// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "opencvmini_func.h"
#include "common/defines.h"

#include <opencv2/opencv.hpp>

namespace WasmEdge {
namespace Host {

Expect<void>
WasmEdgeOpenCVMiniImdecode::body(const Runtime::CallingFrame &Frame,
                                 uint32_t BufPtr, uint32_t BufLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  char *Buf = MemInst->getPointer<char *>(BufPtr);

  std::vector<char> Content(Buf, Buf + BufLen);
  cv::Mat img = cv::imdecode(cv::InputArray(Content), cv::IMREAD_COLOR);

  return {};
}

} // namespace Host
} // namespace WasmEdge
