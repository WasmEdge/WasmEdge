// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <map>
#include <opencv2/core/mat.hpp>

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
    uint32_t Key = NextMatKey++;
    this->MatPool[Key] = Img;
    return Key;
  }

private:
  uint32_t NextMatKey = 0;
};

} // namespace Host
} // namespace WasmEdge
