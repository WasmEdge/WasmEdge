
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "wasmedge/wasmedge.h"

#include <stddef.h>

static WasmEdge_String NameString;
static const char NameCString[] = "name";
static const WasmEdge_String NameStringDefaultValue = {.Buf = NameCString,
                                                       .Length = 4};

WasmEdge_Result HostFuncAdd(void *Data __attribute__((unused)),
                            const WasmEdge_CallingFrameContext *CallFrameCxt
                            __attribute__((unused)),
                            const WasmEdge_Value *In, WasmEdge_Value *Out) {
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result HostFuncSub(void *Data __attribute__((unused)),
                            const WasmEdge_CallingFrameContext *CallFrameCxt
                            __attribute__((unused)),
                            const WasmEdge_Value *In, WasmEdge_Value *Out) {
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(Val1 - Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_ModuleInstanceContext *
CreateTestModule(const struct WasmEdge_ModuleDescriptor *Desc) {
  WasmEdge_String ModuleName =
      WasmEdge_StringCreateByCString(Desc->Name);
  WasmEdge_ModuleInstanceContext *Mod =
      WasmEdge_ModuleInstanceCreate(ModuleName);
  WasmEdge_StringDelete(ModuleName);

  WasmEdge_String FuncName;
  WasmEdge_FunctionTypeContext *FType;
  WasmEdge_FunctionInstanceContext *FuncCxt;
  enum WasmEdge_ValType ParamTypes[2], ReturnTypes[1];
  ParamTypes[0] = WasmEdge_ValType_I32;
  ParamTypes[1] = WasmEdge_ValType_I32;
  ReturnTypes[0] = WasmEdge_ValType_I32;

  FType = WasmEdge_FunctionTypeCreate(ParamTypes, 2, ReturnTypes, 1);
  FuncName = WasmEdge_StringCreateByCString("add");
  FuncCxt = WasmEdge_FunctionInstanceCreate(FType, HostFuncAdd, NULL, 0);
  WasmEdge_ModuleInstanceAddFunction(Mod, FuncName, FuncCxt);
  WasmEdge_StringDelete(FuncName);
  FuncName = WasmEdge_StringCreateByCString("sub");
  FuncCxt = WasmEdge_FunctionInstanceCreate(FType, HostFuncSub, NULL, 0);
  WasmEdge_ModuleInstanceAddFunction(Mod, FuncName, FuncCxt);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_FunctionTypeDelete(FType);

  return Mod;
}

static WasmEdge_ProgramOption PODesc[] = {{
    .Name = "name",
    .Description = "test name string",
    .Type = WasmEdge_ProgramOptionType_String,
    .Storage = &NameString,
    .DefaultValue = &NameStringDefaultValue,
}};
static WasmEdge_ModuleDescriptor ModuleDesc[] = {{
    .Name = "wasmedge_plugintest_c_module",
    .Description = "This is for the plugin tests in WasmEdge C API.",
    .Create = CreateTestModule,
}};
static WasmEdge_PluginDescriptor Desc = {
    .Name = "wasmedge_plugintest_c",
    .Description = "",
    .APIVersion = WasmEdge_Plugin_CurrentAPIVersion,
    .Version =
        {
            .Major = 0,
            .Minor = 10,
            .Patch = 0,
            .Build = 0,
        },
    .ModuleCount = 1,
    .ProgramOptionCount = 1,
    .ModuleDescriptions = ModuleDesc,
    .ProgramOptions = PODesc,
};

WASMEDGE_CAPI_PLUGIN_EXPORT const WasmEdge_PluginDescriptor *
WasmEdge_Plugin_GetDescriptor(void) {
  return &Desc;
}
