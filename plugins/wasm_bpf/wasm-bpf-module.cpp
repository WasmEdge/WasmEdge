// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasm-bpf-module.h"
#include "func-attach-bpf-program.h"
#include "func-bpf-buffer-poll.h"
#include "func-bpf-map-fd-by-name.h"
#include "func-bpf-map-operate.h"
#include "func-close-bpf-object.h"
#include "func-load-bpf-object.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "state.h"
#include <algorithm>

namespace WasmEdge {
namespace Host {

using namespace std::literals::string_view_literals;

WasmBpfModule::WasmBpfModule() : ModuleInstance("wasm_bpf") {
  state_t state = std::make_shared<WasmBpfState>();
  addHostFunc("wasm_load_bpf_object", std::make_unique<LoadBpfObject>(state));
  addHostFunc("wasm_close_bpf_object", std::make_unique<CloseBpfObject>(state));
  addHostFunc("wasm_attach_bpf_program",
              std::make_unique<AttachBpfProgram>(state));
  addHostFunc("wasm_bpf_buffer_poll", std::make_unique<BpfBufferPoll>(state));
  addHostFunc("wasm_bpf_map_fd_by_name",
              std::make_unique<BpfMapFdByName>(state));
  addHostFunc("wasm_bpf_map_operate", std::make_unique<BpfMapOperate>(state));
}

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmBpfModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasm_bpf",
    .Description = "A plugin provides API for eBPF",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasm_bpf",
                .Description = "Provide functions for eBPF",
                .Create = create,
            },
        },
    .AddOptions = nullptr};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace Host
} // namespace WasmEdge
