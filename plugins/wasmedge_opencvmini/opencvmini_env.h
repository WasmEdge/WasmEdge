// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <limits>
#include <map>
#include <opencv2/core/mat.hpp>

namespace WasmEdge {
namespace Host {

class WasmEdgeOpenCVMiniEnvironment {
public:
  WasmEdgeOpenCVMiniEnvironment() noexcept;

  Expect<cv::Mat> getMat(uint32_t MatKey) {
    if (auto V = MatPool.find(MatKey); V != MatPool.end()) {
      return V->second;
    } else {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
  }

  Expect<uint32_t> insertMat(const cv::Mat &Img) {
    if (MatPool.size() > std::numeric_limits<uint32_t>::max()) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    while (true) {
      const uint32_t MatKey = NextMatKey++;
      if (MatPool.try_emplace(MatKey, Img).second) {
        return MatKey;
      }
    }
  }

  Expect<void> releaseMat(uint32_t MatKey) {
    if (MatPool.erase(MatKey) == 0) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    return {};
  }

private:
  std::map<uint32_t, cv::Mat> MatPool;
  uint32_t NextMatKey = 0;
};

} // namespace Host
} // namespace WasmEdge
