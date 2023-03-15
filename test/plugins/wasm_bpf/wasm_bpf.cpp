// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include <gtest/gtest.h>
#include "plugin/plugin.h"
#include "runtime/instance/module.h"
#include "wasm-bpf-module.h"
namespace {
WasmEdge::Runtime::Instance::ModuleInstance* createModule() {
    using namespace std::literals::string_view_literals;
    WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
        "../../../plugins/wasm_bpf/"
        "libwasmedgePluginWasmBpf" WASMEDGE_LIB_EXTENSION));
    if (const auto* Plugin = WasmEdge::Plugin::Plugin::find("wasm_bpf"sv)) {
        if (const auto* Module = Plugin->findModule("wasm_bpf"sv)) {
            return Module->create().release();
        }
    }
    return nullptr;
}
}  // namespace


TEST(WasmBpfTest, Module) {
    auto module = dynamic_cast<WasmEdge::Host::WasmBpfModule*>(createModule());
    EXPECT_NE(module, nullptr);
    // Test whether functions are exported
    EXPECT_EQ(module->getFuncExportNum(), 6U);
    EXPECT_NE(module->findFuncExports("wasm_load_bpf_object"), nullptr);
    EXPECT_NE(module->findFuncExports("wasm_close_bpf_object"), nullptr);
    EXPECT_NE(module->findFuncExports("wasm_attach_bpf_program"), nullptr);
    EXPECT_NE(module->findFuncExports("wasm_bpf_buffer_poll"), nullptr);
    EXPECT_NE(module->findFuncExports("wasm_bpf_map_fd_by_name"), nullptr);
    EXPECT_NE(module->findFuncExports("wasm_bpf_map_operate"), nullptr);

    delete module;
}


GTEST_API_ int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
