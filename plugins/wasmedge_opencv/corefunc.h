// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "opencvbase.h"
#include <opencv2/core.hpp>

namespace WasmEdge {
namespace Host {
class WasmedgeOpenCVCoreAcos : public WasmEdgeOpenCV<WasmedgeOpenCVCoreAcos> {
public:
	WasmedgeOpenCVCoreAcos(WasmEdgeOpenCVEnvironment &HostEnv) : WasmEdgeOpenCV(HostEnv) {}	

};

} // namespace Host
} // namespace WasmEdge
