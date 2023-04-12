// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

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

  static Plugin::PluginRegister Register;

  std::map<uint32_t, cv::Mat> MatPool;
};

} // namespace Host
} // namespace WasmEdge
