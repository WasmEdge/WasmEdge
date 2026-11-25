
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasmedge/wasmedge.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static WasmEdge_String NameString;
static const char NameCString[] = "name";
static const WasmEdge_String NameStringDefaultValue = {.Buf = NameCString,
                                                       .Length = 4};
static void Finalizer(void *Data) {
  printf("Deallocate host data\n");
  free((int32_t *)Data);
}

static WasmEdge_Result
HostFuncAdd(void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
            const WasmEdge_Value *In, WasmEdge_Value *Out) {
  (void)CallFrameCxt;
  /*
   * Host function to calculate A + B,
   * and accumulate (A + B) to the host data.
   */
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
  int32_t *Accum = (int32_t *)Data;
  *Accum += WasmEdge_ValueGetI32(Out[0]);
  printf("Current accumulate: %d\n", *Accum);
  return WasmEdge_Result_Success;
}

static WasmEdge_Result
HostFuncSub(void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
            const WasmEdge_Value *In, WasmEdge_Value *Out) {
  (void)CallFrameCxt;
  /*
   * Host function to calculate A - B,
   * and accumulate (A - B) to the host data.
   */
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(Val1 - Val2);
  int32_t *Accum = (int32_t *)Data;
  *Accum += WasmEdge_ValueGetI32(Out[0]);
  printf("Current accumulate: %d\n", *Accum);
  return WasmEdge_Result_Success;
}

static WasmEdge_ModuleInstanceContext *
CreateTestModule(const struct WasmEdge_ModuleDescriptor *Desc) {
  /* Allocate and initialize a host data. */
  printf("Allocate host data\n");
  int32_t *Accumulate = (int32_t *)malloc(sizeof(int32_t));
  *Accumulate = 0;

  /* Create the module instance. */
  WasmEdge_String ModuleName = WasmEdge_StringCreateByCString(Desc->Name);
  WasmEdge_ModuleInstanceContext *Mod =
      WasmEdge_ModuleInstanceCreateWithData(ModuleName, Accumulate, Finalizer);
  WasmEdge_StringDelete(ModuleName);

  WasmEdge_String FuncName;
  WasmEdge_FunctionTypeContext *FType;
  WasmEdge_FunctionInstanceContext *FuncCxt;
  WasmEdge_ValType ParamTypes[2], ReturnTypes[1];
  ParamTypes[0] = WasmEdge_ValTypeGenI32();
  ParamTypes[1] = WasmEdge_ValTypeGenI32();
  ReturnTypes[0] = WasmEdge_ValTypeGenI32();

  /* Create the "add" and "sub" functions and add them to the module
   * instance. If one of them cannot be made or added, give no module. */
  FType = WasmEdge_FunctionTypeCreate(ParamTypes, 2, ReturnTypes, 1);
  const char *Names[2] = {"add", "sub"};
  WasmEdge_HostFunc_t Funcs[2] = {HostFuncAdd, HostFuncSub};
  for (int I = 0; I < 2 && Mod; I++) {
    FuncName = WasmEdge_StringCreateByCString(Names[I]);
    FuncCxt = WasmEdge_FunctionInstanceCreate(FType, Funcs[I], Accumulate, 0);
    WasmEdge_Result Res =
        WasmEdge_ModuleInstanceAddFunction(Mod, FuncName, FuncCxt);
    WasmEdge_StringDelete(FuncName);
    if (!WasmEdge_ResultOK(Res)) {
      WasmEdge_FunctionInstanceDelete(FuncCxt);
      WasmEdge_ModuleInstanceDelete(Mod);
      Mod = NULL;
    }
  }
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
