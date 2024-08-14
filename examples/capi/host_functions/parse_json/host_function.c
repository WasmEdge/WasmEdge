#include <json-c/json_tokener.h>
#include <stdio.h>
#include <wasmedge/wasmedge.h>

WasmEdge_Result parseJson(void *Data,
                          const WasmEdge_CallingFrameContext *CallingFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  FILE *Fp = fopen("test.json", "r");
  char Buffer[1024];
  fread(Buffer, 1024, 1, Fp);
  fclose(Fp);

  void *Input = WasmEdge_ValueGetExternRef(In[0]);
  char *Key = (char *)Input;
  struct json_object *Value;
  json_object *ParsedJson = json_tokener_parse(Buffer);
  json_object_object_get_ex(ParsedJson, Key, &Value);
  void *Output = (void *)json_object_get_string(Value);
  Out[0] = WasmEdge_ValueGenExternRef(Output);
  return WasmEdge_Result_Success;
}

int main() {
  /* Create the VM context. */
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

  /* Create the import object. */
  WasmEdge_String ExportName = WasmEdge_StringCreateByCString("extern");
  WasmEdge_ModuleInstanceContext *ImpObj =
      WasmEdge_ModuleInstanceCreate(ExportName);
  WasmEdge_ValType ParamList[1] = {WasmEdge_ValTypeGenExternRef()};
  WasmEdge_ValType ReturnList[1] = {WasmEdge_ValTypeGenExternRef()};
  WasmEdge_FunctionTypeContext *HostFType =
      WasmEdge_FunctionTypeCreate(ParamList, 1, ReturnList, 1);
  WasmEdge_FunctionInstanceContext *HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, parseJson, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  WasmEdge_String HostFuncName =
      WasmEdge_StringCreateByCString("func-parse-json");
  WasmEdge_ModuleInstanceAddFunction(ImpObj, HostFuncName, HostFunc);
  WasmEdge_StringDelete(HostFuncName);

  WasmEdge_VMRegisterModuleFromImport(VMCxt, ImpObj);

  /* The parameters and returns arrays. */
  char *Key = "testValue";
  WasmEdge_Value Params[1] = {WasmEdge_ValueGenExternRef(Key)};
  WasmEdge_Value Returns[1];
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("parseJson");
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(
      VMCxt, "parse-json.wasm", FuncName, Params, 1, Returns, 1);
  if (WasmEdge_ResultOK(Res)) {
    printf("Got the result: %s\n",
           (char *)WasmEdge_ValueGetExternRef(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }

  /* Resources deallocations. */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_ModuleInstanceDelete(ImpObj);
  return 0;
}
