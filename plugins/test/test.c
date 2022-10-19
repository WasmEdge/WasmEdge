// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "wasmedge/wasmedge.h"
#include <stddef.h>

static int32_t TestingOption;
static const int32_t TestingOptionDefaultValue = 42;

static WasmEdge_Result Test(void *Data __attribute__((unused)),
                            const WasmEdge_CallingFrameContext *CallFrameCxt
                            __attribute__((unused)),
                            const WasmEdge_Value *In __attribute__((unused)),
                            WasmEdge_Value *Out __attribute__((unused))) {
  return WasmEdge_Result_Success;
}

static WasmEdge_ModuleInstanceContext *
CreateTestModule(const struct WasmEdge_ModuleDescriptor *Desc
                 __attribute__((unused))) {
  WasmEdge_ModuleInstanceContext *Mod;

  {
    WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("test");
    Mod = WasmEdge_ModuleInstanceCreate(ModuleName);
    WasmEdge_StringDelete(ModuleName);
  }

  {
    WasmEdge_FunctionTypeContext *FType =
        WasmEdge_FunctionTypeCreate(NULL, 0, NULL, 0);
    WasmEdge_FunctionInstanceContext *Func =
        WasmEdge_FunctionInstanceCreate(FType, Test, NULL, 0);
    WasmEdge_FunctionTypeDelete(FType);
    WasmEdge_String FName = WasmEdge_StringCreateByCString("test");
    WasmEdge_ModuleInstanceAddFunction(Mod, FName, Func);
    WasmEdge_StringDelete(FName);
  }

  return Mod;
}

static WasmEdge_ProgramOption PODesc[] = {{
    .Name = "test",
    .Description = "testing option",
    .Type = WasmEdge_ProgramOptionType_Int32,
    .Storage = &TestingOption,
    .DefaultValue = &TestingOptionDefaultValue,
}};
static WasmEdge_ModuleDescriptor ModuleDesc[] = {{
    .Name = "test",
    .Description = "testing module",
    .Create = CreateTestModule,
}};
static WasmEdge_PluginDescriptor Desc[] = {{
    .Name = "test",
    .Description = "testing plugin",
    .APIVersion = WasmEdge_Plugin_CurrentAPIVersion,
    .Version =
        {
            .Major = 0,
            .Minor = 0,
            .Patch = 0,
            .Build = 0,
        },
    .ModuleCount = 1,
    .ProgramOptionCount = 1,
    .ModuleDescriptions = ModuleDesc,
    .ProgramOptions = PODesc,
}};

WASMEDGE_CAPI_PLUGIN_EXPORT const WasmEdge_PluginDescriptor *
WasmEdge_Plugin_GetDescriptor(void) {
  return Desc;
}
