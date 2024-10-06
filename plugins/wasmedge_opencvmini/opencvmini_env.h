// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <map>
#include <opencv2/opencv.hpp>

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCVMiniEnvironment {
public:
  WasmEdgeOpenCVMiniEnvironment() noexcept;

  std::map<uint32_t, cv::Mat> MatPool;

  Expect<cv::Mat> getMat(uint32_t MatKey) {
    if (auto V = this->MatPool.find(MatKey); V != this->MatPool.end()) {
      return V->second;
    } else {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
  }

  Expect<uint32_t> insertMat(const cv::Mat &Img) {
    // cv::Mat::flags contains magic signature & I believe it's a good enough
    // key for this purpose.
    this->MatPool[static_cast<uint32_t>(Img.flags)] = Img;
    return static_cast<uint32_t>(Img.flags);
  }
};

} // namespace Host
} // namespace WasmEdge
