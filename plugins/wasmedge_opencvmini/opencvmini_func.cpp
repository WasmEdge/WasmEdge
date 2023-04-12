// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "opencvmini_func.h"
#include "common/defines.h"

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

namespace WasmEdge {
namespace Host {

Expect<uint32_t>
WasmEdgeOpenCVMiniImdecode::body(const Runtime::CallingFrame &Frame,
                                 uint32_t BufPtr, uint32_t BufLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  char *Buf = MemInst->getPointer<char *>(BufPtr);

  std::vector<char> Content(Buf, Buf + BufLen);
  cv::Mat Img = cv::imdecode(cv::InputArray(Content), cv::IMREAD_COLOR);

  // cv::Mat::flags contains magic signature & I believe it's a good enough key for this purpose.
  Env.MatPool[static_cast<uint32_t>(Img.flags)] = Img;

  return static_cast<uint32_t>(Img.flags);
}

Expect<void>
WasmEdgeOpenCVMiniImshow::body(const Runtime::CallingFrame &Frame, uint32_t WindowNamePtr,
                               uint32_t WindowNameLen, uint32_t MatKey) {
  if (auto V = Env.MatPool.find(MatKey);V!= Env.MatPool.end()) {
    std::string WindowName;

    // Check memory instance from module.
    auto *MemInst = Frame.getMemoryByIndex(0);
    if (MemInst == nullptr) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }

    char *Buf = MemInst->getPointer<char *>(WindowNamePtr);
    std::copy_n(Buf, WindowNameLen, std::back_inserter(WindowName));

    cv::imshow(WindowName.c_str(), V->second);
  }

  return {};
}

Expect<void>
    WasmEdgeOpenCVMiniWaitKey::body(const Runtime::CallingFrame &, uint32_t Delay){
  cv::waitKey(static_cast<int>(Delay));
  return {};
}

} // namespace Host
} // namespace WasmEdge
