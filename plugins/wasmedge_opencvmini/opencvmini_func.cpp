// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "opencvmini_func.h"
#include "common/defines.h"
#include "common/errcode.h"

#include <cstdint>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
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
                                              uint32_t SrcMatKey,
                                              uint32_t KernelWidth,
                                              uint32_t KernelHeight) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::blur(*Src, Dst, cv::Size(KernelWidth, KernelHeight));
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t>
WasmEdgeOpenCVMiniBilateralFilter::body(const Runtime::CallingFrame &,
                                        uint32_t SrcMatKey, uint32_t D,
                                        double SigmaColor, double SigmaSpace) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::bilateralFilter(*Src, Dst, D, SigmaColor, SigmaSpace);
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t>
WasmEdgeOpenCVMiniBoxFilter::body(const Runtime::CallingFrame &,
                                  uint32_t SrcMatKey, uint32_t Ddepth,
                                  uint32_t KernelWidth, uint32_t KernelHeight) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::boxFilter(*Src, Dst, Ddepth, cv::Size(KernelWidth, KernelHeight));
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t>
WasmEdgeOpenCVMiniEmptyMat::body(const Runtime::CallingFrame &) {
  cv::Mat Kernel;
  return Env.insertMat(Kernel);
}

Expect<uint32_t> WasmEdgeOpenCVMiniDilate::body(const Runtime::CallingFrame &,
                                                uint32_t SrcMatKey,
                                                uint32_t KernelMatKey) {
  cv::Mat Dst;
  auto Kernel = Env.getMat(KernelMatKey);
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::dilate(*Src, Dst, *Kernel);
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t> WasmEdgeOpenCVMiniErode::body(const Runtime::CallingFrame &,
                                               uint32_t SrcMatKey,
                                               uint32_t KernelMatKey) {
  cv::Mat Dst;
  auto Kernel = Env.getMat(KernelMatKey);
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::erode(*Src, Dst, *Kernel);
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t>
WasmEdgeOpenCVMiniGaussianBlur::body(const Runtime::CallingFrame &,
                                     uint32_t SrcMatKey, uint32_t KernelWidth,
                                     uint32_t KernelHeight, double SigmaX) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::GaussianBlur(*Src, Dst, cv::Size(KernelWidth, KernelHeight), SigmaX);
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t>
WasmEdgeOpenCVMiniLaplacian::body(const Runtime::CallingFrame &,
                                  uint32_t SrcMatKey, uint32_t Ddepth) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::Laplacian(*Src, Dst, Ddepth);
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t>
WasmEdgeOpenCVMiniMedianBlur::body(const Runtime::CallingFrame &,
                                   uint32_t SrcMatKey, uint32_t Ksize) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::medianBlur(*Src, Dst, Ksize);
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t> WasmEdgeOpenCVMiniPyrDown::body(const Runtime::CallingFrame &,
                                                 uint32_t SrcMatKey,
                                                 uint32_t KernelWidth,
                                                 uint32_t KernelHeight) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::pyrDown(*Src, Dst, cv::Size(KernelWidth, KernelHeight));
  }
  return Env.insertMat(Dst);
}

Expect<uint32_t> WasmEdgeOpenCVMiniPyrUp::body(const Runtime::CallingFrame &,
                                               uint32_t SrcMatKey,
                                               uint32_t KernelWidth,
                                               uint32_t KernelHeight) {
  cv::Mat Dst;
  if (auto Src = Env.getMat(SrcMatKey); Src) {
    cv::pyrUp(*Src, Dst, cv::Size(KernelWidth, KernelHeight));
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

Expect<void> WasmEdgeOpenCVMiniImencode::body(
    const Runtime::CallingFrame &Frame, uint32_t ExtPtr, uint32_t ExtLen,
    uint32_t MatKey, uint32_t BufPtr, uint32_t BufLen) {
  std::string Ext;

  auto *MemInst = Frame.getMemoryByIndex(0);

  char *Buf = MemInst->getPointer<char *>(ExtPtr);
  std::copy_n(Buf, ExtLen, std::back_inserter(Ext));

  auto Img = Env.getMat(MatKey);
  if (!Img) {
    spdlog::error("[WasmEdge-OpenCVMini] "sv
                  "Failed to get matrix by key."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto OutSpan = MemInst->getSpan<uchar>(BufPtr, BufLen);
  if (unlikely(OutSpan.size() != BufLen)) {
    spdlog::error("[WasmEdge-OpenCVMini] "sv
                  "Failed when accessing the image target buffer memory."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  std::vector<uchar> WriteTo;
  cv::imencode(Ext, *Img, WriteTo);

  std::copy_n(WriteTo.begin(), WriteTo.size(), OutSpan.begin());

  return {};
}

Expect<uint32_t>
WasmEdgeOpenCVMiniNormalize::body(const Runtime::CallingFrame &,
                                  uint32_t SrcMatKey) {
  auto Src = Env.getMat(SrcMatKey);
  if (!Src) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  cv::Mat Dst;
  // convert each elements `v` of `Src` to `(1/255) * v + 0`
  Src->convertTo(Dst, CV_32F, 1. / 255., 0.);
  return Env.insertMat(Dst);
}

Expect<uint32_t>
WasmEdgeOpenCVMiniBilinearSampling::body(const Runtime::CallingFrame &,
                                         uint32_t SrcMatKey, uint32_t OutImgW,
                                         uint32_t OutImgH) {
  auto Src = Env.getMat(SrcMatKey);
  if (!Src) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  cv::Mat Dst;
  cv::resize(*Src, Dst, cv::Size(OutImgW, OutImgH), 0, 0, cv::INTER_LINEAR);
  return Env.insertMat(Dst);
}

Expect<void> WasmEdgeOpenCVMiniRectangle::body(
    const Runtime::CallingFrame &, uint32_t SrcMatKey, uint32_t Top,
    uint32_t Left, uint32_t Bot, uint32_t Right, double R, double G, double B,
    int32_t Thickness, int32_t LineType, int32_t Shift) {
  auto Src = Env.getMat(SrcMatKey);
  if (!Src) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  cv::Point TopLeft(Top, Left);
  cv::Point BottomRight(Bot, Right);

  cv::rectangle(*Src, TopLeft, BottomRight, cv::Scalar(B, G, R), Thickness,
                LineType, Shift);
  return {};
}

Expect<uint32_t> WasmEdgeOpenCVMiniCvtColor::body(const Runtime::CallingFrame &,
                                                  uint32_t SrcMatKey,
                                                  int32_t Code,
                                                  int32_t DestChannelN) {
  auto Src = Env.getMat(SrcMatKey);
  if (!Src) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto Img = *Src;

  cv::Mat Dst;
  cvtColor(Img, Dst, Code, DestChannelN);
  return Env.insertMat(Dst);
}

} // namespace Host
} // namespace WasmEdge
