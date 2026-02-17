// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge.h"

static WasmEdge_PluginDescriptor Desc = {
    .Name = "wasmedge_plugintest_version_mismatch",
    .Description = "Plugin with incompatible API version",
    .APIVersion = 0xFFFFFFFF, // Intentionally unsupported API version
    .Version =
        {
            .Major = 0,
            .Minor = 0,
            .Patch = 0,
            .Build = 0,
        },
    .ModuleCount = 0,
    .ProgramOptionCount = 0,
    .ModuleDescriptions = NULL,
    .ProgramOptions = NULL,
};

WASMEDGE_CAPI_PLUGIN_EXPORT const WasmEdge_PluginDescriptor *
WasmEdge_Plugin_GetDescriptor(void) {
  return &Desc;
}
