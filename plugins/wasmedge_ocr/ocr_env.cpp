// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#include "ocr_env.h"
#include "ocr_module.h"

namespace WasmEdge {
namespace Host {
namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeOCRModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_ocr",
    .Description = "A WasmEdge Plugin for Optical Character Recognition (OCR) "
                   "powered by the Tesseract API.",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_ocr",
                .Description =
                    "A WasmEdge Plugin for Optical Character Recognition (OCR) "
                    "powered by the Tesseract API.",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
