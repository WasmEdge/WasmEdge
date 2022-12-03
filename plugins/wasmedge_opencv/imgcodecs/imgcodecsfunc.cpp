// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "imgcodecsfunc.h"
#include <opencv2/imgcodecs.hpp>
namespace WasmEdge {
namespace Host {
using namespace cv;

Expect<uint32_t>
WasmEdgeOpenCvImgcodecsImread::body(const Runtime::CallingFrame &Frame,
				    uint32_t ImgPtr,
                                    uint32_t FilenamePtr) {

  auto *MemInst = Frame.getMemoryByIndex(0);

  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *Filename = MemInst->getPointer<const char *>(FilenamePtr);

  if (Filename == nullptr) {
    spdlog::error("[WasmEdge OpenCV Imgcodecs] Failed to get Filename");
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto MatPtr = MemInst->getPointer<Mat*>(ImgPtr);

  auto mat = imread(Filename);

  *MatPtr = mat.clone();
	
  return Env.ExitCode;
}

Expect<void>
WasmEdgeOpenCvImgcodecsImwrite::body(const Runtime::CallingFrame &Frame,
                                     uint32_t FilenamePtr, uint32_t ImgPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);

  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *Filename = MemInst->getPointer<const char *>(FilenamePtr);

  if (Filename == nullptr) {
    spdlog::error("[WasmEdge OpenCV Imgcodecs] Failed to get Filename");
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto TempPtr = MemInst->getPointer<Mat*>(ImgPtr);

  Mat* MatPtr = reinterpret_cast<Mat*>(TempPtr);

  cv::imwrite(Filename, *MatPtr);

  return {};
}

} // namespace Host
} // namespace WasmEdge
