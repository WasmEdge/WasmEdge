// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "imgcodecsmodule.h"
#include "imgcodecsfunc.h"

namespace WasmEdge {
namespace Host {

/// Register your functions in module.
	WasmEdgeOpenCvImgcodecsModule::WasmEdgeOpenCvImgcodecsModule() : ModuleInstance("wasmedge_imgcodecs_ephemeral") {
	addHostFunc("wasmedge_imgcodecs_imread", std::make_unique<WasmEdgeOpenCvImgcodecsImread>(Env));

	addHostFunc("wasmedge_opencv_imgcodecs_imwrite", std::make_unique<WasmEdgeOpenCvImgcodecsImwrite>(Env));
	}

} // namespace Host
} // namespace WasmEdge
