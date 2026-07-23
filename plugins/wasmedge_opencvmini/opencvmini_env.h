// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

  uint32_t NextMatId = 0;
  std::map<uint32_t, cv::Mat> MatPool;

  Expect<cv::Mat> getMat(uint32_t MatKey) {
    if (auto V = this->MatPool.find(MatKey); V != this->MatPool.end()) {
      return V->second;
    } else {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
  }

  Expect<void> removeMat(uint32_t MatKey) {
    if (this->MatPool.erase(MatKey) > 0) {
      return {};
    } else {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
  }

  Expect<uint32_t> insertMat(const cv::Mat &Img) {
    uint32_t Id = NextMatId++;
    this->MatPool[Id] = Img;
    return Id;
  }
};

} // namespace Host
} // namespace WasmEdge
