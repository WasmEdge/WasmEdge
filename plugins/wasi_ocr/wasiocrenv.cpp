// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#include "wasiocrenv.h"
#include "wasiocrmodule.h"

namespace WasmEdge {
namespace Host {

namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiOCRModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_ocr",
    .Description = "A WasmEdge Plugin for Optical Character Recognition (OCR) "
                   "powered by the Tesseract API.",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi_ocr",
                .Description =
                    "A WasmEdge Plugin for Optical Character Recognition (OCR) "
                    "powered by the Tesseract API.",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

} // namespace

Plugin::PluginRegister WASIOCR::WasiOCREnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
