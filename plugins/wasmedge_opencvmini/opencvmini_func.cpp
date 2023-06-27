// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "opencvmini_func.h"
#include "common/defines.h"

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

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

  return Env.insertMat(Img);
}

Expect<void> WasmEdgeOpenCVMiniImshow::body(const Runtime::CallingFrame &Frame,
                                            uint32_t WindowNamePtr,
                                            uint32_t WindowNameLen,
                                            uint32_t MatKey) {
  std::string WindowName;

  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  char *Buf = MemInst->getPointer<char *>(WindowNamePtr);
  std::copy_n(Buf, WindowNameLen, std::back_inserter(WindowName));

  if (auto Img = Env.getMat(MatKey); Img) {
    cv::imshow(WindowName.c_str(), *Img);
  }

  return {};
}

Expect<void> WasmEdgeOpenCVMiniWaitKey::body(const Runtime::CallingFrame &,
                                             uint32_t Delay) {
  cv::waitKey(static_cast<int>(Delay));
  return {};
}

Expect<uint32_t> WasmEdgeOpenCVMiniBlur::body(const Runtime::CallingFrame &,
                                              uint32_t SrcMatKey) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::blur(*Src, Dst, cv::Size(5, 5));
  }
  return Env.insertMat(Dst);
}

Expect<void> WasmEdgeOpenCVMiniImwrite::body(const Runtime::CallingFrame &Frame,
                                             uint32_t TargetFileNamePtr,
                                             uint32_t TargetFileNameLen,
                                             uint32_t MatKey) {
  std::string TargetFileName;

  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  char *Buf = MemInst->getPointer<char *>(TargetFileNamePtr);
  std::copy_n(Buf, TargetFileNameLen, std::back_inserter(TargetFileName));

  if (auto Img = Env.getMat(MatKey); Img) {
    cv::imwrite(TargetFileName.c_str(), *Img);
  }

  return {};
}

} // namespace Host
} // namespace WasmEdge
