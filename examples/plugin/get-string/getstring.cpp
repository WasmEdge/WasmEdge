// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include <algorithm>

namespace {

using namespace std::literals::string_view_literals;
using namespace WasmEdge;

PO::Option<std::string> StringOpt(PO::Description("string to return"sv),
                                  PO::MetaVar("STRING"sv),
                                  PO::DefaultValue<std::string>("hello"));

PO::Option<PO::Toggle> UpperOpt(PO::Description("return in upper case"sv));

void addOptions(PO::ArgumentParser &Parser) noexcept {
  Parser.add_option("string"sv, StringOpt).add_option("upper"sv, UpperOpt);
}

class GetString : public Runtime::HostFunction<GetString> {
public:
  GetString(const std::string &String, bool Upper)
      : String(String), Upper(Upper) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                    uint32_t BufLen, uint32_t WrittenPtr) {
    // Check memory instance from module.
    auto *MemInst = Frame.getMemoryByIndex(0);
    if (MemInst == nullptr) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }

    // Validate range of the buffer.
    char *const Buf = MemInst->getPointer<char *>(BufPtr, BufLen);
    if (unlikely(Buf == nullptr)) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }

    // Validate pointer to written count.
    uint32_t *const Written = MemInst->getPointer<uint32_t *>(WrittenPtr);
    if (unlikely(Written == nullptr)) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }

    if (Upper) {
      char *const End =
          std::transform(String.begin(), String.end(), Buf,
                         [](unsigned char C) { return std::toupper(C); });
      *Written = End - Buf;
    } else {
      char *const End = std::copy(String.begin(), String.end(), Buf);
      *Written = End - Buf;
    }
    return {};
  }

private:
  std::string_view String;
  bool Upper;
};

class PluginModule : public Runtime::Instance::ModuleInstance {
public:
  PluginModule() : ModuleInstance("module_wasm_name") {
    addHostFunc("get_string", std::make_unique<GetString>(StringOpt.value(),
                                                          UpperOpt.value()));
  }
};

Runtime::Instance::ModuleInstance *create(void) noexcept {
  return new PluginModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "plugin_name",
    .Description = "Example plugin",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "module_name",
                .Description = "Example module",
                .Create = create,
            },
        },
    .AddOptions = addOptions,
};

Plugin::PluginRegister Register(&Descriptor);

} // namespace
